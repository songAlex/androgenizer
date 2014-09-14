LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=androgenizer

LOCAL_MODULE_TAGS:=optional 

LOCAL_SRC_FILES := \
	main.c \
	options.c \
	emit.c \
	library.c

LOCAL_CFLAGS := \
	-Wall \
	-g3

LOCAL_PRELINK_MODULE := false
include $(BUILD_HOST_EXECUTABLE)
