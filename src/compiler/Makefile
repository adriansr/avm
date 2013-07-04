OBJECTS=args.o \
        main.o \
        parser.o \
        compiler.o \
        buffer.o

TARGET=avmcc
CFLAGS=-g -Wall -pedantic -I.. 
LFLAGS=-L../avm -lavm

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET)

