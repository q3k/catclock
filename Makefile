PLAN9 := /usr/local/plan9
CC := $(PLAN9)/bin/9c
LD := $(PLAN9)/bin/9l

default: all

catclock.o: catclock.c
	$(CC) "$<"

catclock: catclock.o
	$(LD) -o "$@" "$^"

catclock.c: eyes.p catback.p

all: catclock

clean:
	rm -rf catclock
