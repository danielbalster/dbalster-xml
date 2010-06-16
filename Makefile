#!/usr/bin/make

CFLAGS = -O9 -x c -pipe -std=gnu99
LDFLAGS = -s
httpd: main.o xml.o
	$(CC) -o xml *.o	

clean:
	rm -f xml *.o
