include ../include.mk

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
SOBJS = $(SRCS:%.c=%.so.o)

static: libsupernova.a
	@echo $(CFLAGS)

libsupernova.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.so.o: %.c
	$(CC) -c $(CFLAGS) -fPIC $^ -o $@

shared: libsupernova.so

libsupernova.so: $(SOBJS)
	$(CC) -shared -o $@ $(SOBJS)

group%.c: group%.h

.PHONY: clean

clean:
	$(RM) $(OBJS) $(SOBJS) libsupernova.*
