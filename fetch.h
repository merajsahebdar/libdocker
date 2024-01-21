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

#ifndef LIBDOCKER_FETCH_H
#define LIBDOCKER_FETCH_H

#include <glib-object.h>
#include <glib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

typedef enum
{
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE
} HttpMethod;

#define TYPE_HTTP_METHOD (http_method_get_type ())
GType http_method_get_type (void);

#define FETCH_TYPE (fetch_get_type ())
G_DECLARE_FINAL_TYPE (Fetch, fetch, , FETCH, GObject)

Fetch *
fetch_new (HttpMethod method, const gchar *url);

JsonNode *
fetch_send (Fetch *self, GError **error, JsonNode *body);

G_END_DECLS

#endif // LIBDOCKER_FETCH_H
