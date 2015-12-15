CFLAGS = -std=gnu99 -m32 -g -W -Wall -Wshadow
LDFLAGS = -lpthread -lm

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) > /dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

O ?= 2
TARGET = dispatch_test
SRCS = queue.c drive.c dispatch.c dispatch_test.c
OBJS = $(SRCS:.c=.o)

COMPILEC = $(CC) $(DEPFLAGS) $(CFLAGS) -c -O$(O)
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo LINK $@
	@$(CC) $(CFLAGS) -O$(O) -o $@ $^ $(LDFLAGS)

%.o: %.c $(DEPDIR)/%.d
	@echo COMPILE $@
	@$(COMPILEC) -o $@ $<
	@$(POSTCOMPILE)

$(DEPDIR)/%.d: ;

check: all
	@echo CHECKING
	@python3 check.py ./$(TARGET) ./dispatch_tests

clean:
	@echo CLEANING
	@rm -vf $(TARGET) $(OBJS)

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

.PHONY: all clean
