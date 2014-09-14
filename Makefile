CFLAGS := -Wall -g3
SOURCES := main.c options.c emit.c common.h emit.h options.h library.h library.c option_entries.h
C_FILES := $(filter %.c,$(SOURCES))

all: androgenizer

androgenizer: $(SOURCES)
	$(CC) $(CFLAGS) $(C_FILES) -o androgenizer

clean:
	rm -f *.o androgenizer

Android.mk: androgenizer
	./androgenizer -:PROJECT androgenizer \
		-:HOST_EXECUTABLE androgenizer \
		-:TAGS optional \
		-:SOURCES $(SOURCES) \
		-:CFLAGS $(CFLAGS) \
		> $@

test: androgenizer
	./test.bash 2> /dev/null | diff -u test-reference.txt -
	@echo " *** Test: success ***"

.PHONY: all clean test
