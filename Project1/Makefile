CC = gcc
TARGET = testlib
SRCS = main.c bitmap.c debug.c hash.c hex_dump.c list.c
OBJS = $(SRCS:.c=.o)
HEADER = bitmap.h debug.h hash.h hex_dump.h limits.h list.h round.h
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADER)
	$(CC) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)