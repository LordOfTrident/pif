SRC  = $(wildcard *.c)
DEPS = $(wildcard ../../*.h) $(wildcard ../../*.c) $(wildcard *.inc)
OUT  = $(basename $(SRC))

build: $(OUT)

CSTD   = c11
CLIBS  = -lm -lSDL2
CFLAGS = -O3 -std=$(CSTD) -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations -g -I../../

build: $(OUT)

%: %.c $(DEPS)
	$(CC) $< $(CFLAGS) $(CLIBS) -o $@

clean:
	-rm -f $(OUT)

all:
	@echo build, clean
