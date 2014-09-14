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
#include <stdio.h>
#include <stdlib.h>
#include "emit.h"
#include "options.h"
#include "common.h"

static void cleanup_flag(struct flag *c)
{
	free(c->flag);
}

static void cleanup_flag_array(struct flag_array *arr)
{
	int i;

	for (i = 0; i < arr->nr_flags; ++i)
		cleanup_flag(&arr->flags[i]);
	free(arr->flags);
}

static void cleanup_header(struct header *h)
{
	free(h->name);
}

static void cleanup_sources(struct source *s)
{
	free(s->name);
}

static void cleanup_library(struct library *l)
{
	free(l->name);
}

static void cleanup_passthrough(struct passthrough *p)
{
	free(p->name);
}

static void cleanup_module(struct module *m)
{
	int i;

	free(m->name);

	if (m->header_target)
		free(m->header_target);

	if (m->headers) {
		for (i = 0; i < m->headers; i++)
			cleanup_header(&m->header[i]);
		free(m->header);
	}

	if (m->sources) {
		for (i = 0; i < m->sources; i++)
			cleanup_sources(&m->source[i]);
		free(m->source);
	}

	cleanup_flag_array(&m->c);
	cleanup_flag_array(&m->cpp);
	cleanup_flag_array(&m->cxx);
	cleanup_flag_array(&m->include);

	if (m->libraries) {
		for (i = 0; i < m->libraries; i++)
			cleanup_library(&m->library[i]);
		free(m->library);
	}

	if (m->passthroughs) {
		for (i = 0; i < m->passthroughs; i++)
			cleanup_passthrough(&m->passthrough[i]);
		free(m->passthrough);
	}
}

static void cleanup_subdir(struct subdir *s)
{
	free(s->name);
}

static void cleanup(struct project *p)
{
	int i;
	if (p->modules) {
		for (i = 0; i < p->modules; i++)
			cleanup_module(&p->module[i]);
		free(p->module);
	}
	if (p->subdirs) {
		for (i = 0; i < p->subdirs; i++)
			cleanup_subdir(&p->subdir[i]);
		free(p->subdir);
	}
	free(p->name);
	free(p->abs_top);
	free(p->rel_top);
	free(p);
}

int main(int argc, char **argv)
{
	struct project *p;
	int err = 0;

	p = options_parse(argc, argv);
	if (p) {
		err = emit_file(p);
		cleanup(p);
	}
	return err;
}
