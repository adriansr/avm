OBJECTS=avm.o \
        hash.o \
        dict.o \
        objects.o \
        stack.o \
        run.o

TARGET=libavm.a
SDKINCLUDE=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/usr/include/
CFLAGS=-g -Wall -pedantic -I.. -I$(SDKINCLUDE)

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

