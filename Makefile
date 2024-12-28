#!/usr/bin/make
.SUFFIXES:
.PHONY: all run docs pack clean
.SILENT: run

TAR = calc
PCK = abgabe.zip

# set compiler flags, instructing the preprocessor to generate dependencies
CFLAGS = -std=c17 -c -g -O0 -Wall -MMD -MP

# collect the source files
TAR_SRC = $(wildcard src/*.c)
TAR_OBJ = $(TAR_SRC:%.c=%.o)
TAR_DEP = $(TAR_SRC:%.c=%.d)

# include the generated dependencies here
-include $(TAR_DEP)

# use the C compiler to create object files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# create the main binary from the source files in the 'src' folder
$(TAR): $(TAR_OBJ)
	$(CC) -o $@ $^

# standard targets
all: $(TAR)

run: all
	./$(TAR) traces/gcc.trace

pack: clean
	zip $(PCK) src/* DESIGN.md Makefile

docs:
	doxygen Doxyfile

clean:
	$(RM) $(RMFILES) $(PCK) $(TAR) $(TAR_OBJ) $(TAR_DEP)
