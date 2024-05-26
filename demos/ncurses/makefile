SRC  = $(wildcard *.c) $(wildcard *.cc)
DEPS = $(wildcard *.inc) $(wildcard ../*.h) $(wildcard ../*.c)
OUT  = $(basename $(SRC))

CSTD   = c99
CXXSTD = c++11
LIBS   = -lm -lncurses
FLAGS  = -O3 -g -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations -I../../

build: $(OUT)

%: %.c $(DEPS)
	$(CC) $< $(FLAGS) -std=$(CSTD) $(LIBS) -o $@

%: %.cc $(DEPS)
	$(CXX) $< $(FLAGS) -std=$(CXXSTD) $(LIBS) -o $@

clean:
	-rm -f $(OUT)

all:
	@echo build, clean
