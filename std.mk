# Standard included makefile instead of having
# more built in stuff

ifeq ($(OS),Windows)
    RM=del /r
else
    POSIX=1
    RM=rm -f
endif

ifdef POSIX
    ifneq ($(shell command -v cc),)
        CC=cc
    endif
endif
