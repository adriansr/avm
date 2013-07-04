OBJECTS=avm.o \
        hash.o \
        dict.o \
        objects.o \
        stack.o \
        run.o

GHEADERS=generated/parser-table.h \
         generated/parsers-decl.h \
         generated/opcode-name-table.h \
         generated/opcodes.h

TARGET=libavm.a
CFLAGS=-g -Wall -pedantic -I..

default: generated $(TARGET) test

$(TARGET): $(OBJECTS)
	ar -rs $@ $^

test: test.o
	$(CC) $(CFLAGS) $< $(TARGET) -o $@

generated: opcodes.list
	mkdir -p generated
	python opcodes-gen.py

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET) test.o test
	rm -rf generated

