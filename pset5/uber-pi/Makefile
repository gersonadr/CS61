CFLAGS = -std=gnu99 -m32 -g -W -Wall -Wshadow

O ?= 3
TARGET = uber-pi
OBJS = uber-pi.o

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo LINK $@
	@$(CC) $(CFLAGS) -O$(O) -o $@ $^ -lpthread

%.o: %.c
	@echo COMPILE $@
	@$(CC) $(CFLAGS) -c -O$(O) -o $@ $<

clean:
	@echo CLEANNING
	@rm -vf $(TARGET) $(OBJS)

check: $(TARGET)
	@echo CHECKING
	@python3 check.py

.PHONY: all clean
