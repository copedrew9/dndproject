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

DUMPBIN  = dumpdata
TESTOBJS = $(filter-out $(OBJDIR)/main.o,$(OBJECTS))

$(DUMPBIN): tools/dump_data.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/dump_data.c $(TESTOBJS)

# The game data lives in data/*.txt, hand-written. This turns it into the
# src/gen_*.c tables the program compiles. Those are checked in, so building
# the program needs no Python; run this after editing a data file.
data:
	python3 tools/build_data.py

# The data files and the compiled tables have to say the same thing. This
# dumps what was built from data/ into a scratch directory and compares:
# any difference means a row was dropped or changed in translation.
dataverify: $(DUMPBIN)
	@tmp=`mktemp -d` && ./$(DUMPBIN) $$tmp && \
	  if diff -r $$tmp data; then echo "data/ and the compiled tables agree"; \
	  else rm -rf $$tmp; exit 1; fi; rm -rf $$tmp

# Checks every name in data/ against the book dumps in TextFiles/.
audit:
	python3 tools/audit.py

# Assertions on the rules engine.
TESTBIN  = selftest

$(TESTBIN): tools/selftest.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/selftest.c $(TESTOBJS)

test: $(TESTBIN)
	./$(TESTBIN)

# Build many characters at random and check none of them crash, that a saved
# character reloads unchanged, and that it all holds under valgrind.
check: test dataverify $(BIN)
	python3 tools/drive.py --runs 30 --seed 1
	python3 tools/drive.py --runs 10 --seed 500 --levelup
	python3 tools/roundtrip.py
	python3 tools/drive.py --runs 8 --seed 900 --valgrind

clean:
	rm -rf $(OBJDIR) $(BIN) $(TESTBIN) $(TESTBIN).d $(DUMPBIN) $(DUMPBIN).d

.PHONY: all clean check test data dataverify audit
