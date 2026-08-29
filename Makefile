CC      ?= gcc
# An implicit declaration is a build error rather than a warning. It is the
# one warning here that is silently fatal at runtime: a function declared by
# accident returns int, so a pointer coming back from it is truncated to 32
# bits. tools/selftest.c had been compiling that way for want of ui.h, and
# the first use of a pointer-returning helper from it segfaulted.
CFLAGS  ?= -std=c99 -Wall -Wextra -Werror=implicit-function-declaration -O2
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
# and verify_equipment_coverage run the other way, looking for spells, magic
# items and priced equipment rows the books have and data/ does not -- the
# direction a gap hides in, since a missing entry is not a name that fails to
# resolve. That direction found five PHB items nobody had entered: the glass
# bottle, two cases, and two of the four gaming sets.
verify:
	python3 tools/verify_equipment.py
	python3 tools/verify_equipment_coverage.py
	python3 tools/verify_packs.py
	python3 tools/verify_gems.py
	python3 tools/verify_deities.py
	python3 tools/verify_races.py
	python3 tools/verify_spells.py
	python3 tools/verify_magic_items.py
	python3 tools/verify_magic_rules.py
	python3 tools/verify_beasts.py
	python3 tools/verify_classes.py
	python3 tools/verify_backgrounds.py
	python3 tools/verify_feats.py
	python3 tools/verify_races_phb.py
	python3 tools/verify_tables.py
	python3 tools/verify_options.py
	python3 tools/verify_prereq_coverage.py
	python3 tools/verify_spell_lists.py
	python3 tools/verify_reference.py
	python3 tools/verify_sidekicks.py
	python3 tools/verify_small_tables.py
	python3 tools/verify_coverage.py

# Every combination of race, class, subclass, level, background, feat, spell,
# item and setting the tables allow, measured for a number that cannot be
# right. Worth the most under the sanitizers, which is what `make asan` does
# with it.
COMBOBIN = combosweep

$(COMBOBIN): tools/combos.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/combos.c $(TESTOBJS)

combos: $(COMBOBIN)
	./$(COMBOBIN)

# Assertions on the rules engine.
TESTBIN  = selftest

$(TESTBIN): tools/selftest.c $(TESTOBJS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -o $@ tools/selftest.c $(TESTOBJS)

test: $(TESTBIN)
	./$(TESTBIN)

# Build many characters at random and check none of them crash, that a saved
# character reloads unchanged, and that it all holds under valgrind.
#
# drive.py answers the creation wizard; stress.py wanders the rest of the
# main menu -- settings, reference, inventory, sidekicks, homebrew, notes --
# in one session, and fuzz_files.py and fuzz_shop.py corrupt the files the
# program reads rather than the answers it is given: a character sheet and
# homebrew.txt for the first, a shop for the second.
check: test combos dataverify verify $(BIN)
	python3 tools/drive.py --runs 30 --seed 1
	python3 tools/drive.py --runs 10 --seed 500 --levelup
	python3 tools/drive.py --runs 10 --seed 700 --magic 3
	python3 tools/drive.py --runs 8 --seed 810 --back-at 2
	python3 tools/drive.py --runs 8 --seed 820 --quit-at 3
	python3 tools/roundtrip.py
	python3 tools/stress.py --runs 1 --seed 11 --tour --ops 9 --grace 0.02
	python3 tools/stress.py --runs 4 --seed 20 --ops 4 --grace 0.02
	python3 tools/fuzz_files.py --runs 150 --seed 1
	python3 tools/fuzz_shop.py --runs 120 --seed 1
	python3 tools/drive.py --runs 8 --seed 900 --valgrind

# The same drive, built with the sanitizers. This is what caught the race
# menu writing past the end of its array; a plain build did not notice, and
# neither did valgrind, the array being on the stack.
#
# -fno-sanitize-recover matters as much as the sanitizers do. Without it the
# undefined-behaviour checker prints its complaint and carries on, the
# program exits zero, and every harness below reports success: five signed
# overflows on numbers read out of a character file went that way for a
# whole round of testing. With it, the first one aborts and is seen.
SANFLAGS = -std=c99 -O1 -g -fsanitize=address,undefined \
	   -fno-sanitize-recover=all -fno-omit-frame-pointer
SANENV = ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1

asan:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(SANFLAGS)" all $(COMBOBIN) $(TESTBIN)
	$(SANENV) ./$(TESTBIN)
	$(SANENV) ./$(COMBOBIN)
	$(SANENV) python3 tools/drive.py --runs 25 --seed 1
	$(SANENV) python3 tools/drive.py --runs 10 --seed 500 \
	  --levelup
	$(SANENV) python3 tools/stress.py --runs 1 --seed 11 \
	  --tour --ops 9 --nasty 0.3 --grace 0.02 --seconds 1800
	$(SANENV) python3 tools/stress.py --runs 3 --seed 20 \
	  --ops 5 --nasty 0.3 --grace 0.02 --seconds 1800
	$(SANENV) python3 tools/fuzz_files.py --runs 200 --seed 1
	$(SANENV) python3 tools/fuzz_shop.py --runs 200 --seed 1
	$(MAKE) clean

clean:
	rm -rf $(OBJDIR) $(BIN) $(TESTBIN) $(TESTBIN).d $(DUMPBIN) \
	  $(DUMPBIN).d $(COMBOBIN) $(COMBOBIN).d

.PHONY: all clean check test combos data dataverify audit verify asan
