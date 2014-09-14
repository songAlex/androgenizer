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
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <errno.h>
#include "common.h"
#include "library.h"

#define OPTION_ENTRY(x) MODE_##x,
enum mode {
#include "option_entries.h"
};
#undef OPTION_ENTRY

struct opstrings {
	char *str;
	enum mode mode;
};

#define OPTION_ENTRY(x) {"-:"#x, MODE_##x},
static struct opstrings opstrings[]={
#include "option_entries.h"
	{NULL, 0}
};
#undef OPTION_ENTRY

/* need to maintain state for parsing -I<space>path etc. */
static const char *cflag_space;

void die(char *error)
{
	fprintf(stderr, "Error in command line: %s\n", error);
	exit(1);
}

static char *add_slashes(char *in)
{
	int incmode = 0;
	int newlen = 0;
	char *ptr, *out, *outptr;

	if (strncasecmp(in, "-i", 2) == 0) {
		incmode = 1;
	}

	ptr = in;
	while (*ptr) {
		if (*ptr == '"' || *ptr == ' ')
			newlen++;
		if (!incmode && (*ptr == '(' || *ptr == ')'))
			newlen++;

		newlen++; ptr++;
	}
	out = malloc(sizeof(char) * newlen + 1);
	ptr = in;
	outptr = out;
	while (*ptr) {
		if (*ptr == '"' || *ptr == ' ')
			*(outptr++) = '\\';

		if (!incmode && (*ptr == '(' || *ptr ==')'))
			*(outptr++) = '\\';

		if (!incmode && (*ptr == '<' || *ptr =='>'))
			*(outptr++) = '\\';

		*(outptr++) = *(ptr++);
	}
	*outptr = 0;
	return out;
}

static const char *skip_root_path(struct project *p, const char *path)
{
	int len;

	if (!p->root_path)
		return path;

	len = strlen(p->root_path);

	if (strncmp(path, p->root_path, len) == 0) {
		path += len;

		if (path[0] == '/')
			path++;
	}

	return path;
}

static void set_abs_top(struct project *p, const char *str)
{
	const char *path;

	free(p->abs_top);

	path = skip_root_path(p, str);
	if (p->root_path && path == str) {
		fprintf(stderr,
			"androgenizer: Warning: The build root path '%s' is not part of ABS_TOP='%s'.\n",
			p->root_path, str);
	}

	p->abs_top = strdup(path);
}

static void set_rel_top(struct project *p, const char *str)
{
	free(p->rel_top);
	p->rel_top = strdup(str);
}

static struct project *new_project(char *name, enum script_type stype, enum build_type btype)
{
	const char *varname;
	struct project *p = calloc(1, sizeof(struct project));

	p->name = name;
	p->stype = stype;
	p->btype = btype;

	switch (btype) {
	case BUILD_NDK:
		varname = "NDK_ROOT";
		break;
	case BUILD_EXTERNAL:
		varname = "ANDROID_BUILD_TOP";
		break;
	}

	p->root_path = getenv(varname);

	return p;
}

static struct module *new_module(char *name, enum module_type mtype)
{
	struct module *out = calloc(1, sizeof(struct module));
	out->name = name;
	out->mtype = mtype;
	return out;
}

void add_tag(struct module *m, char *name)
{
	enum tags tag = TAG_NONE;

	if (strcmp("user", name) == 0)
		tag = TAG_USER;

	if (strcmp("eng", name) == 0)
		tag = TAG_ENG;

	if (strcmp("tests", name) == 0)
		tag = TAG_TESTS;

	if (strcmp("optional", name) == 0)
		tag = TAG_OPTIONAL;

        if (strcmp("debug", name) == 0)
                tag = TAG_DEBUG;

	m->tags |= tag;
	free(name);
}

static int begins_with(const char *str, const char *with)
{
	return strncmp(str, with, strlen(with)) == 0;
}

/*
 * At the beginning of a path:
 * - remove whitespace
 * - replace // with a / (thank you, pkg-config)
 * - replace $rel_top with $abs_top
 * - replace ./ with $(LOCAL_PATH)
 * - chop off the root path
 * prepend prefix, and return a newly malloc'd string.
 */
static char *flag_path_subst(struct project *p, const char *prefix,
			     const char *path)
{
	char *buf;
	int path_len;
	int abstop_len;
	const char *abstop = "";
	int prefix_len = strlen(prefix);

	/* skip leading whitespace */
	while (*path && isblank(*path))
		path++;

	if (path[0] == '/' && path[1] == '/')
		path++;

	if (p->abs_top && p->rel_top && begins_with(path, p->rel_top)) {
		path += strlen(p->rel_top);
		abstop = p->abs_top;
	} else if (path[0] == '.') {
		if (path[1] == 0 || path[1] == '/') {
			path++;
			abstop = "$(LOCAL_PATH)";
		}
	} else
		path = skip_root_path(p, path);

	path_len = strlen(path);
	abstop_len = strlen(abstop);

	buf = malloc(prefix_len + abstop_len + path_len + 1);

	strcpy(buf, prefix);
	strcpy(buf + prefix_len, abstop);
	strcpy(buf + prefix_len + abstop_len, path);

	return buf;
}

static void add_compiler_flag(struct project *p, struct module *m,
			      struct flag_array *arr, char *flag)
{
	char *new_flag;
	int i;

	if (strcmp("-I", flag) == 0) {
		cflag_space = "-I";
		goto out;
	}

	if (strcmp("-include", flag) == 0) {
		cflag_space = "-include ";
		goto out;
	}

	if (strcmp("-Werror", flag) == 0)
		goto out;

	if (strcmp("-pthread", flag) == 0)
		goto out;

	/* All -I flags are put in a separate array, without the -I */
	if (begins_with(flag, "-I")) {
		new_flag = flag_path_subst(p, "", flag + 2);
		arr = &m->include;
	} else if (cflag_space) {
		if (strcmp(cflag_space, "-I") == 0) {
			new_flag = flag_path_subst(p, "", flag);
			arr = &m->include;
		} else {
			new_flag = flag_path_subst(p, cflag_space, flag);
		}
		cflag_space = NULL;
	} else {
		new_flag = flag;
		flag = NULL;
	}

	for (i = 0; i < arr->nr_flags; i++) {
		if (strcmp(new_flag, arr->flags[i].flag) == 0) {
			free(new_flag);
			goto out;
		}
	}

	arr->nr_flags++;
	arr->flags = realloc(arr->flags, arr->nr_flags * sizeof(*arr->flags));
	arr->flags[arr->nr_flags - 1].flag = new_flag;

out:
	free(flag);
}

static void add_cflag(struct project *p, struct module *m, char *flag)
{
	add_compiler_flag(p, m, &m->c, flag);
}

static void add_cppflag(struct project *p, struct module *m, char *flag)
{
	add_compiler_flag(p, m, &m->cpp, flag);
}

static void add_cxxflag(struct project *p, struct module *m, char *flag)
{
	add_compiler_flag(p, m, &m->cxx, flag);
}

static int sources_filter(char *name)
{
	int len = strlen(name);
	if (len > 2) {
		if ((strcmp(".h", name + len - 2) == 0)
		 || (strcmp(".d", name + len - 2) == 0)) {
			return 1;
		}
	}
	if (len > 4) {
		if ((strcmp(".asn", name + len - 4) == 0)
		 || (strcmp(".map", name + len - 4) == 0)) {
			return 1;
		}
	}
	if (len > 5) {
		if (strcmp(".list", name + len - 5) == 0)
			return 1;
	}
	return 0;
}

static void add_source(struct module *m, char *name, struct generator *g)
{
	if (sources_filter(name)) {
		free(name);
		return;
	}
	m->sources++;
	m->source = realloc(m->source, m->sources * sizeof(struct source));
	m->source[m->sources - 1].name = name;
	m->source[m->sources - 1].gen = g;
}

static void add_header(struct module *m, char *name)
{
	m->headers++;
	m->header = realloc(m->header, m->headers * sizeof(struct header));
	m->header[m->headers - 1].name = name;
}

static void add_passthrough(struct module *m, char *name)
{
	m->passthroughs++;
	m->passthrough = realloc(m->passthrough, m->passthroughs * sizeof(struct passthrough));
	m->passthrough[m->passthroughs - 1].name = name;
}

static void add_libfilter(struct module *m, char *name, enum library_type ltype)
{
	m->libfilters++;
	m->libfilter = realloc(m->libfilter, m->libfilters * sizeof(struct library));
	m->libfilter[m->libfilters - 1].name = name;
	m->libfilter[m->libfilters - 1].ltype = ltype;
}

static void add_library(struct module *m, char *name, enum library_type ltype)
{
	m->libraries++;
	m->library = realloc(m->library, m->libraries * sizeof(struct library));
	m->library[m->libraries - 1].name = name;
	m->library[m->libraries - 1].ltype = ltype;
}

static int add_ldflag(struct module *m, char *flag, enum build_type btype)
{
	enum library_type ltype;
	enum flag_action action;
	int len = strlen(flag);

	if (len < 2)  {/* this is probably a WTF condition... */
		free(flag);
		return 0;
	}

	if (flag[0] == '-') {
		action = ldflag_action(flag);
		if (action == FLAG_SKIP) {
			free(flag);
			return 0;
		}
		if (action == FLAG_SKIP_WITH_ARG) {
			free(flag);
			return 1;
		}
		/* otherwise we have FLAG_USE */
		if (flag[1] == 'l') {/* actually figure out what libtype... */
			ltype = library_scope(flag + 2);
			add_library(m, strdup(flag+2), ltype);
			free(flag);
			return 0;
		}
		add_library(m, flag, LIBRARY_FLAG);
	} else {
		char *dot = rindex(flag, '.');

		if (dot && (strcmp(dot, ".lo") == 0)) {
			free(flag);
			return 0;
		}

		if (dot && (strcmp(dot, ".la") == 0 ||
			    strcmp(dot, ".a") == 0)) {
			char *slash = rindex(flag, '/');
			char *temp, *lname;

			if (slash)
				lname = strstr(slash, "lib");
			else
				lname = strstr(flag, "lib");
			*dot = 0;
			if (lname) {
				temp = flag;
				flag = strdup(lname + 3);
				free(temp);
				add_library(m, flag, LIBRARY_EXTERNAL);
				return 0;
			}
			free(flag);
			return 0;
		}
		free(flag);
/*		add_library(m, flag, LIBRARY_FLAG); */
	}

	return 0;
}

static void add_module(struct project *p, struct module *m)
{
	p->modules++;
	p->module = realloc(p->module, p->modules * sizeof(struct module));
	p->module[p->modules - 1] = *m;
	free(m);
}

static void add_subdir(struct project *p, char *name)
{
	p->subdirs++;
	p->subdir = realloc(p->subdir, p->subdirs * sizeof(struct subdir));
	p->subdir[p->subdirs - 1].name = name;
}

static enum mode get_mode(char *arg)
{
	int i;
	int outval = MODE_UNDEFINED;
	int len = strlen(arg);

	if (len < 3)
		return MODE_UNDEFINED;
	if ((arg[0] != '-') || (arg[1] != ':'))
		return MODE_UNDEFINED;

	for  (i = 0; opstrings[i].str != NULL; i++)
		if (strcmp(opstrings[i].str, arg) == 0)
			outval = opstrings[i].mode;

	return outval;
}

static enum module_type module_type_from_mode(enum mode mode)
{
	switch (mode) {
	case MODE_SHARED:
		return MODULE_SHARED_LIBRARY;
	case MODE_STATIC:
		return MODULE_STATIC_LIBRARY;
	case MODE_EXECUTABLE:
		return MODULE_EXECUTABLE;
	case MODE_HOST_SHARED:
		return MODULE_HOST_SHARED_LIBRARY;
	case MODE_HOST_STATIC:
		return MODULE_HOST_STATIC_LIBRARY;
	case MODE_HOST_EXECUTABLE:
		return MODULE_HOST_EXECUTABLE;
	default:
		fprintf(stderr, "Unknown mode %d passed to '%s'.\n", mode, __func__);
		abort();
	}
}

static enum build_type guess_build_type(void)
{
	const char *android_build_top = getenv("ANDROID_BUILD_TOP");

	if (android_build_top && strlen(android_build_top) > 0)
		return BUILD_EXTERNAL;

	return BUILD_NDK;
}

struct project *options_parse(int argc, char **args)
{
	enum mode mode = MODE_UNDEFINED;
	char *arg;
	int i, skip = 0;
	enum build_type bt;
	struct project *p = NULL;
	struct module *m = NULL;

	bt = guess_build_type();

	if (argc < 2) {
/* print help! */
		return NULL;
	}
	for (i = 1; i < argc; i++) {
		enum mode nm;
		nm = get_mode(args[i]);
		if (mode != MODE_PASSTHROUGH)
			arg = add_slashes(args[i]);
		else
			arg = strdup(args[i]);

		if (nm != MODE_UNDEFINED) {
			free(arg);
			skip = 0;
			mode = nm;
			continue;
		}

		if (skip) {
			skip = 0;
			free(arg);
			continue;
		}

		switch (mode) {
		case MODE_UNDEFINED:
			die("Androgenizer arguments must start with a valid -: switch, like -:PROJECT.");
			break;
		case MODE_PROJECT:
			p = new_project(arg, SCRIPT_SUBDIRECTORY, bt);
			break;
		case MODE_SUBDIR:
			if (!p)
				die("-:PROJECT must come before -:SUBDIR");
			add_subdir(p, arg);
			break;
		case MODE_SHARED:
		case MODE_STATIC:
		case MODE_EXECUTABLE:
		case MODE_HOST_SHARED:
		case MODE_HOST_STATIC:
		case MODE_HOST_EXECUTABLE:
			if (!p)
				die("-:PROJECT must come before a module type");
			if (m)
				add_module(p, m);
			m = new_module(arg, module_type_from_mode(mode));
			break;
		case MODE_SOURCES:
			if (!m)
				die("a module type must be declared before adding -:SOURCES");
			add_source(m, arg, NULL);
			break;
		case MODE_LDFLAGS:
			if (!m)
				die("a module type must be declared before adding -:LDFLAGS");
			skip = add_ldflag(m, arg, p->btype);
			break;
		case MODE_CFLAGS:
			if (!p || !m)
				die("a module type must be declared before adding -:CFLAGS");
			add_cflag(p, m, arg);
			break;
		case MODE_CPPFLAGS:
			if (!p || !m)
				die("a module type must be declared before adding -:CPPFLAGS");
			add_cppflag(p, m, arg);
			break;
		case MODE_CXXFLAGS:
			if (!p || !m)
				die("a module type must be declared before adding -:CXXFLAGS");
			add_cxxflag(p, m, arg);
			break;
		case MODE_TAGS:
			if (!m)
				die("a module type must be declared before setting -:TAGS");
			add_tag(m, arg);
			break;
		case MODE_HEADER_TARGET:
			if (!m)
				die("a module type must be declared before setting a -:HEADER_TARGET");
			if (m->header_target)
				free(m->header_target);
			m->header_target = arg;
			break;
		case MODE_HEADERS:
			if (!m)
				die("a module type must be declared before adding -:HEADERS");
			add_header(m, arg);
			break;
		case MODE_PASSTHROUGH:
			if (!m)
				die("a module type must be declared before a -:PASSTHROUGH");
			add_passthrough(m, arg);
			break;
		case MODE_REL_TOP:
			if (!p)
				die("a -:PROJECT must be declared before -:REL_TOP");
			set_rel_top(p, arg);
			free(arg);
			break;
		case MODE_ABS_TOP:
			if (!p)
				die("a -:PROJECT must be declared before -:ABS_TOP");
			set_abs_top(p, arg);
			free(arg);
			break;
		case MODE_LIBFILTER_STATIC:
			if (!m)
				die("a module type must be declared before adding libfilters");
			add_libfilter(m, arg, LIBRARY_STATIC);
			break;
		case MODE_LIBFILTER_WHOLE:
			if (!m)
				die("a module type must be declared before adding libfilters");
			add_libfilter(m, arg, LIBRARY_WHOLE_STATIC);
			break;
		case MODE_END:
			break;
		}
	}
	if (p && m)
		add_module(p, m);
	return p;
}
