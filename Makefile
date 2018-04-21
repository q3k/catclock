PLAN9 := /usr/local/plan9
CFLAGS := -I$(PLAN9)/include
LDFLAGS := -L$(PLAN9)/lib
LDLIBS := -ldraw -l9 -lmux -lm

default: all

catclock: catclock.c

catclock.c: eyes.p catback.p

all: catclock

clean:
	rm -rf catclock
