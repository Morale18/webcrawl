
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDLIBS = -lcurl

SRCS = crawler.c queue.c visited.c fetch.c
OBJS = $(SRCS:.c=.o)

crawler: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f crawler $(OBJS)

.PHONY: clean
