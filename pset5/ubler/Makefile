CFLAGS = -std=gnu99 -m32 -g -W -Wall -Wshadow

O ?= 3

TARGET = ubler_test
OBJS = ubler_helpers.o ubler.o ubler_test.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -O$(O) -o $@ $^ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c -O$(O) -o $@ $<

extra: $(TARGET)
	@python3 check.py -e

extra-%: $(TARGET)
	@python3 check.py $(subst extra-,,$@)

check: $(TARGET)
	@python3 check.py

check-%: $(TARGET)
	@python3 check.py $(subst check-,,$@)

clean:
	rm -vf $(TARGET) $(OBJS)

.PHONY: all clean
