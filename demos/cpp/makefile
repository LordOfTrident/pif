SRC  = $(wildcard *.cc)
DEPS = $(wildcard ../../*.h) $(wildcard ../../*.c) $(wildcard ../*.c) $(wildcard ../*.cc)
OUT  = $(basename $(SRC))

CXXSTD   = c++11
CXXLIBS  = -lSDL2
CXXFLAGS = -O3 -g -std=$(CXXSTD) -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations \
           -I../../ -I../

build: $(OUT)

%: %.cc $(DEPS)
	$(CXX) $< $(CXXFLAGS) $(CXXLIBS) -o $@

clean:
	-rm -f $(OUT)

all:
	@echo build, clean
