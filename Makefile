
CFLAGS += -std=c99 -Wall -Wextra -O2 -march=native -DNDEBUG

all: monitor gibgraph

SRC = cpu_info.c mem_info.c gibgraph.c graph.c monitor.c queue.c
OBJ = $(SRC:.c=.o)

gibgraph: gibgraph.o graph.o

monitor: monitor.o cpu_info.o mem_info.o graph.o queue.o

clean:
	$(RM) $(OBJ)

distclean: clean
	$(RM) monitor gibgraph

.PHONY: all clean distclean
