CFLAGS=-g -Wall

all: build

build:	invert resize feed dummy

install:
	/bin/cp invert /usr/sbin/invert_vloopback
	/bin/cp resize /usr/sbin/resize_vloopback
	/bin/cp feed /usr/sbin/feed_vloopback
	/bin/cp dummy /usr/sbin/dummy_vloopback

clean:
	rm -f dummy resize invert feed *o *~

invert:	invert.c

resize:	resize.c

feed: feed.c

dummy:	dummy.c
