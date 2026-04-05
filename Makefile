CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2 -I include
LDFLAGS =

SRC     = $(wildcard src/*.c) platform/posix.c
OBJ     = $(SRC:.c=.o)
LIB     = libeztui.a

EXAMPLES = $(patsubst examples/%.c, build/%, $(wildcard examples/*.c))

.PHONY: all clean examples

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $^

build/imgtest: examples/imgtest.c $(LIB)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -L. -leztui $(LDFLAGS) -lm -o $@

build/%: examples/%.c $(LIB)
	@mkdir -p build
	$(CC) $(CFLAGS) $< -L. -leztui $(LDFLAGS) -o $@

examples: $(EXAMPLES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
	rm -rf build
