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

# Checks every name in data/ against the book text in TextFiles/.
audit:
	python3 tools/audit.py

# Checks the numbers, not just the names: every PHB equipment row against
# the book's own tables, and every deity against appendix B. verify_coverage
# runs the other way, looking for spells and magic items the books have and
# data/ does not -- the direction a gap hides in, since a missing entry is
# not a name that fails to resolve.
verify:
	python3 tools/verify_equipment.py
	python3 tools/verify_deities.py
	python3 tools/verify_races.py
	python3 tools/verify_coverage.py

# Assertions on the rules engine.
TESTBIN  = selftest

$(TESTBIN): tools/selftest.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/selftest.c $(TESTOBJS)

test: $(TESTBIN)
	./$(TESTBIN)

# Build many characters at random and check none of them crash, that a saved
# character reloads unchanged, and that it all holds under valgrind.
check: test dataverify verify $(BIN)
	python3 tools/drive.py --runs 30 --seed 1
	python3 tools/drive.py --runs 10 --seed 500 --levelup
	python3 tools/roundtrip.py
	python3 tools/drive.py --runs 8 --seed 900 --valgrind

# The same drive, built with the sanitizers. This is what caught the race
# menu writing past the end of its array; a plain build did not notice, and
# neither did valgrind, the array being on the stack.
asan:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c99 -O1 -g -fsanitize=address,undefined \
	  -fno-omit-frame-pointer"
	ASAN_OPTIONS=detect_leaks=0 python3 tools/drive.py --runs 25 --seed 1
	ASAN_OPTIONS=detect_leaks=0 python3 tools/drive.py --runs 10 --seed 500 \
	  --levelup
	$(MAKE) clean

clean:
	rm -rf $(OBJDIR) $(BIN) $(TESTBIN) $(TESTBIN).d $(DUMPBIN) $(DUMPBIN).d

.PHONY: all clean check test data dataverify audit verify asan
