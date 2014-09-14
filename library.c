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
#include <string.h>
#include "common.h"
#include "library.h"

/*
	these libraries are supplied in the NDK
*/
static char *libs[] = {
	"c",
	"m",
	"dl",
	"jnigraphics",
	"log",
	"stdc++",
	"thread_db",
	"z",
	NULL
};

enum library_type library_scope(char *name)
{
	int i;

	for (i = 0; libs[i] != NULL; i++) {
		if (strcmp(name, libs[i]) == 0)
			return LIBRARY_NDK;
	}
	return LIBRARY_EXTERNAL;
}

enum flag_arg {
	FLAG_ARG_NO,
	FLAG_ARG_YES,
	FLAG_ARG_JOINED,
};

struct flag_exclusion {
	const char *name;
	enum flag_arg arg;
};

static const struct flag_exclusion dropped_ldflags[] = {
	{ "-pthread",		FLAG_ARG_NO },
	{ "-lpthread",		FLAG_ARG_NO },
	{ "-lrt",		FLAG_ARG_NO },
	{ "-no-undefined",	FLAG_ARG_NO },
	{ "-avoid-version", 	FLAG_ARG_NO },
	{ "-module", 		FLAG_ARG_NO },
	{ "-dlopen",		FLAG_ARG_YES },
	{ "-version-info",	FLAG_ARG_YES },
	{ "-L", 		FLAG_ARG_JOINED },
	{ "-R", 		FLAG_ARG_JOINED },
	{ NULL, 0 }
};

enum flag_action ldflag_action(const char *flag)
{
	const struct flag_exclusion *fe;

	for (fe = &dropped_ldflags[0]; fe->name; fe++) {
		if (fe->arg == FLAG_ARG_JOINED) {
			if (strncmp(flag, fe->name, strlen(fe->name)) != 0)
				continue;
		} else {
			if (strcmp(flag, fe->name) != 0)
				continue;
		}

		switch (fe->arg) {
		case FLAG_ARG_NO:
			return FLAG_SKIP;
		case FLAG_ARG_YES:
			return FLAG_SKIP_WITH_ARG;
		case FLAG_ARG_JOINED:
			if (strlen(flag) == strlen(fe->name))
				return FLAG_SKIP_WITH_ARG;
			else
				return FLAG_SKIP;
		}
	}

	return FLAG_USE;
}
