OBJECTS=args.o \
        main.o \
        parser.o \
        compiler.o \
        buffer.o

TARGET=avmcc
CFLAGS=-g -Wall -pedantic -I..

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET)

