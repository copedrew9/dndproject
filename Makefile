CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
SRCDIR   = src
OBJDIR   = build
BIN      = dndcreator

SOURCES  = $(wildcard $(SRCDIR)/*.c)
OBJECTS  = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPS     = $(OBJECTS:.o=.d)

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# -MMD -MP emits a .d file per object listing the headers it includes, so a
# header change rebuilds everything that uses it. Without this, editing a
# struct in a header leaves stale objects that disagree about its layout.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

-include $(DEPS)

# Regenerate the spell tables from the sourcebook text dump.
spells:
	python3 tools/extract_spells.py

# Assertions on the rules engine.
TESTBIN  = selftest
TESTOBJS = $(filter-out $(OBJDIR)/main.o,$(OBJECTS))

$(TESTBIN): tools/selftest.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/selftest.c $(TESTOBJS)

test: $(TESTBIN)
	./$(TESTBIN)

# Build many characters at random and check none of them crash, that a saved
# character reloads unchanged, and that it all holds under valgrind.
check: test $(BIN)
	python3 tools/drive.py --runs 30 --seed 1
	python3 tools/drive.py --runs 10 --seed 500 --levelup
	python3 tools/roundtrip.py
	python3 tools/drive.py --runs 8 --seed 900 --valgrind

clean:
	rm -rf $(OBJDIR) $(BIN) $(TESTBIN) $(TESTBIN).d

.PHONY: all clean spells check test
