cmake_minimum_required(VERSION 3.27)
project(libdocker C)

set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GLIB_JSON REQUIRED json-glib-1.0)

include_directories(
        ${CURL_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${GLIB_JSON_INCLUDE_DIRS})

add_library(
        docker
        SHARED
        fetch.c)

target_link_libraries(
        docker
        ${CURL_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${GLIB_JSON_LIBRARIES})
