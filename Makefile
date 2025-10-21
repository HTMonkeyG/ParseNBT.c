CC = gcc
AR = ar

CFLAGS = -O3 -std=c11 -g -Wall -Wformat -Wno-strict-aliasing -ffunction-sections -fdata-sections -I./src

all: libcnbt.a

libcnbt.a: ./nbt.o
	$(AR) rcs $@ $^

./nbt.o: ./nbt.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-@del *.o
	-@del *.a
