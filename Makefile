
CFLAGS += -std=c99 -Wall -Wextra -O2 -march=native -DNDEBUG

all: gibgraph

gibgraph: graph.o

clean:
	$(RM) graph.o

distclean: clean
	$(RM) gibgraph

.PHONY: all clean distclean
