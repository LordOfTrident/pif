SRC  = $(wildcard *.c)
DEPS = $(wildcard ../../*.h) $(wildcard ../../*.c) $(wildcard ../*.c)
OUT  = $(basename $(SRC))

CSTD   = c11
CLIBS  = -lm -lSDL2
CFLAGS = -O3 -g -std=$(CSTD) -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations \
         -I../../ -I../

build: $(OUT)

%: %.c $(DEPS)
	$(CC) $< $(CFLAGS) $(CLIBS) -o $@

clean:
	-rm -f $(OUT)

all:
	@echo build, clean
