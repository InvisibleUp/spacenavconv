CC = gcc
CFLAGS = -pedantic -Wall -g -I../.. -I/usr/local/include
LDFLAGS = -L../.. -L/usr/local/lib -lspnav -lX11

$(phony all): spacenavconv_x11 spacenavconv_af_unix

spacenavconv_x11: spacenavconv.c
	$(CC) $(CFLAGS) -DBUILD_X11 -o $@ $< $(LDFLAGS)
spacenavconv_af_unix: spacenavconv.c
	$(CC) $(CFLAGS) -DBUILD_AF_UNIX -o $@ $< $(LDFLAGS)

$(phony clean):
	rm -f spacenavconv_x11 spacenavconv_af_unix
