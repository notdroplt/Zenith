
include include.mk

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
all: zenith supernova
supernova:
	cd supernova/ && $(MAKE)

zenith: $(OBJS) 

.PHONY: clean

clean:
	cd supernova && $(MAKE) clean
	$(RM) $(OBJS) $(TARGET)
