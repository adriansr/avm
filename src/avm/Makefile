OBJECTS=avm.o \
        hash.o \
        objects.o \
        stack.o \
        run.o

TARGET=libavm.a
CFLAGS=-g -Wall -pedantic -I..

default: $(TARGET) test

$(TARGET): $(OBJECTS)
	ar -rs $@ $^

test: test.o
	$(CC) $(CFLAGS) $< $(TARGET) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET) test.o test

