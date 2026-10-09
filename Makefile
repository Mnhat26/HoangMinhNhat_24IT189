CC = cc
CFLAGS = -Wall -Wextra -std=c99 -D_NETBSD_SOURCE

TARGET = myls
OBJS = main.o ls.o options.o display.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c ls.h options.h
	$(CC) $(CFLAGS) -c main.c

ls.o: ls.c ls.h options.h display.h
	$(CC) $(CFLAGS) -c ls.c

options.o: options.c options.h
	$(CC) $(CFLAGS) -c options.c

display.o: display.c display.h options.h
	$(CC) $(CFLAGS) -c display.c

clean:
	rm -f $(OBJS) $(TARGET)