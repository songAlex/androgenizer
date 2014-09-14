#!/bin/bash

set -x

export ANDROID_BUILD_TOP=/android/build/top

"$@" ./androgenizer \
	-:PROJECT libxkbcommon \
	-:REL_TOP .. -:ABS_TOP /android/build/top/libxkbcommon/absolute_top \
	-:STATIC libxkbcommon \
	-:TAGS eng debug \
	-:SOURCES xkbcomp/action.c xkbcomp/action.h xkbcomp/alias.c xkbcomp/alias.h xkbcomp/compat.c \
	-:CFLAGS -DHAVE_CONFIG_H -DDFLT_XKB_CONFIG_ROOT='""' \
	-I. -I.. -I . -I .. \
	-I./this_no/space -I ./this_with/space \
	-I../parent/no_space -I ../parent/with-space \
	-I.hidden_dir -I .hidden_dir/space \
	-I/android/build/top/rootpath \
	-I//android/build/top/doubleslash \
	-include ../parent/config.h \
	-include ./src/config.h  -DMALLOC_0_RETURNS_NULL \
	-:LDFLAGS -no-undefined -lmust_keep_lib -pthread -version-info 1:2:3 \
	-lmust_keep_lib_2 -Lkikkare -Rfuppare -lmust_keep_lib_3

