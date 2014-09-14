/*
    Copyright (C) 2011 Collabora Ltd. <http://www.collabora.com/>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __COMMON_H__
#define __COMMON_H__

enum module_type {
	MODULE_SHARED_LIBRARY,
	MODULE_STATIC_LIBRARY,
	MODULE_EXECUTABLE,
	MODULE_HOST_SHARED_LIBRARY,
	MODULE_HOST_STATIC_LIBRARY,
	MODULE_HOST_EXECUTABLE,
};

enum library_type {
	LIBRARY_NDK,
	LIBRARY_UNSUPPORTED,
	LIBRARY_EXTERNAL,
	LIBRARY_STATIC,
	LIBRARY_WHOLE_STATIC,
	LIBRARY_FLAG
};

enum script_type {
	SCRIPT_SUBDIRECTORY,
	SCRIPT_TOP,
};

enum build_type {
	BUILD_NDK,
	BUILD_EXTERNAL
};

enum tags {
	TAG_NONE = 0,
	TAG_USER = 1,
	TAG_ENG = 2,
	TAG_TESTS = 4,
	TAG_OPTIONAL = 8,
        TAG_DEBUG = 16
};

enum flag_action {
	FLAG_USE,
	FLAG_SKIP,
	FLAG_SKIP_WITH_ARG
};

struct generator {

};

struct passthrough {
	char *name;
};

struct source {
	char *name;
	struct generator *gen;
};

struct flag {
	char *flag;
};

struct flag_array {
	struct flag *flags;
	int nr_flags;
};

struct library {
	char *name;
	enum library_type ltype;
};

struct subdir {
	char *name;
};

struct header {
	char *name;
};

struct module {
	char *name; /* local_module; */
	enum module_type mtype;
	char *header_target;
	struct header *header;
	int headers;
	struct source *source;
	int sources;

	struct flag_array c;
	struct flag_array cpp;
	struct flag_array cxx;
	struct flag_array include;

	struct library *library;
	int libraries;
	struct library *libfilter;
	int libfilters;
	struct passthrough *passthrough;
	int passthroughs;
	int tags;
};

struct project {
	char *name;
	struct module *module;
	int modules;
	struct subdir *subdir;
	int subdirs;
	enum build_type btype;
	enum script_type stype;
	char *abs_top;
	char *rel_top;
	const char *root_path;
};

#endif /*__COMMON_H__*/
