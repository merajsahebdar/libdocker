/*
 * Copyright (C) 2024 Meraj Sahebdar
 *
 * This file is part of the libdocker.
 *
 * libdocker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libdocker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libdocker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fetch.h"
#include <curl/curl.h>
#include <glib-object.h>
#include <stdio.h>

static size_t
response_write_callback (void *contents, size_t size, size_t mem_blocks_count, void *user_data);

G_DEFINE_ENUM_TYPE (
    HttpMethod,
    http_method,
    { HTTP_GET, "GET", "get" },
    { HTTP_POST, "POST", "post" },
    { HTTP_PUT, "PUT", "put" },
    { HTTP_DELETE, "DELETE", "delete" })

struct _FetchPrivate
{
  HttpMethod method;
  gchar *url;
};
typedef struct _FetchPrivate FetchPrivate;

struct _Fetch
{
  GObject parent_instance;
  FetchPrivate *private;
};
typedef struct _Fetch Fetch;

G_DEFINE_FINAL_TYPE_WITH_PRIVATE (Fetch, fetch, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_METHOD,
  PROP_URL,
  N_PROPERTIES
};

static GParamSpec *object_properties[N_PROPERTIES] = {
  NULL,
};

static void
fetch_class_init (FetchClass *klass);

static void
fetch_init (Fetch *self);

static void
fetch_dispose (GObject *object);

static void
fetch_set_property (
    GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *param_spec);

static void
fetch_class_init (FetchClass *klass)
{
  GObjectClass *object_klass = G_OBJECT_CLASS (klass);
  object_klass->dispose = fetch_dispose;
  object_klass->set_property = fetch_set_property;

  object_properties[PROP_METHOD] = g_param_spec_enum (
      "method",
      "HTTP Method",
      "The request's method",
      TYPE_HTTP_METHOD,
      HTTP_GET,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  object_properties[PROP_URL] = g_param_spec_string (
      "url",
      "HTTP URL",
      "The request's url",
      NULL,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_klass, N_PROPERTIES, object_properties);
}

static void
fetch_init (Fetch *self)
{
  FetchPrivate *private = fetch_get_instance_private (self);
  private->method = HTTP_GET;
  private->url = NULL;
}

static void
fetch_dispose (GObject *object)
{
  Fetch *self = _FETCH (object);
  FetchPrivate *private = fetch_get_instance_private (self);

  if (private->url)
    {
      g_free (private->url);
      private->url = NULL;
    }

  G_OBJECT_CLASS (fetch_parent_class)->dispose (object);
}

static void
fetch_set_property (GObject *object,
                    guint property_id,
                    const GValue *value,
                    GParamSpec *)
{
  Fetch *self = _FETCH (object);
  FetchPrivate *private = fetch_get_instance_private (self);

  switch (property_id)
    {
    case PROP_METHOD:
      {
        private->method = g_value_get_enum (value);

        break;
      }
    case PROP_URL:
      {
        private->url = g_value_dup_string (value);

        break;
      }
    }
}

Fetch *
fetch_new (HttpMethod method, const gchar *url)
{
  Fetch *self = g_object_new (
      FETCH_TYPE,
      "method", method,
      "url", url,
      NULL);

  return self;
}

JsonNode *
fetch_send (Fetch *self, GError **error, JsonNode *body)
{
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (body == NULL, NULL);

  FetchPrivate *private = fetch_get_instance_private (self);

  JsonNode *response = NULL;
  CURL *curl = curl_easy_init ();
  if (!curl)
    {
      g_set_error (error, G_IO_ERR, G_IO_ERROR_FAILED, "Cannot initialize the request.");
      goto cleanup_curl;
    }

  curl_easy_setopt (curl, CURLOPT_URL, (char *) private->url);

  curl_easy_setopt (curl, CURLOPT_HTTPGET, 1L);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append (headers, "Content-Type: application/json");
  headers = curl_slist_append (headers, "Accept: application/json");
  curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);

  GString *response_raw = g_string_new ("");
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, response_write_callback);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &response_raw);

  CURLcode code = curl_easy_perform (curl);

  if (code != CURLE_OK)
    {
      g_set_error (error, G_IO_ERR, G_IO_ERROR_FAILED, "Cannot perform the request.");
      goto cleanup_curl_request;
    }

  JsonParser *response_parser = json_parser_new ();
  GError *parse_error = NULL;
  json_parser_load_from_data (response_parser, response_raw->str, response_raw->len, &parse_error);
  if (parse_error)
    {
      printf ("%s", parse_error->message);
      g_set_error (error, G_IO_ERR, G_IO_ERROR_FAILED, "Cannot parse the response.");
      goto cleanup_curl_response;
    }

  response = json_node_copy (json_parser_get_root (response_parser));

cleanup_curl_response:
  g_object_unref (response_parser);

cleanup_curl_request:
  if (response_raw)
    {
      g_string_free (response_raw, TRUE);
    }
  curl_slist_free_all (headers);

cleanup_curl:
  if (curl)
    {
      curl_easy_cleanup (curl);
      curl_global_cleanup ();
    }

  return response;
}

static size_t
response_write_callback (void *contents, size_t size, size_t mem_blocks_count, void *user_data)
{
  size_t real_size = size * mem_blocks_count;
  GString **response = (GString **) user_data;

  g_string_append_len (*response, contents, real_size);

  return real_size;
}
