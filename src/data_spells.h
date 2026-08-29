/* data_spells.h -- the spell record and its enums.
 *
 * The table itself is generated from data/spells.txt into src/gen_spells.c;
 * this is the shape it is generated into.
 */
#ifndef DATA_SPELLS_H
#define DATA_SPELLS_H

typedef enum {
    SCHOOL_ABJURATION,
    SCHOOL_CONJURATION,
    SCHOOL_DIVINATION,
    SCHOOL_ENCHANTMENT,
    SCHOOL_EVOCATION,
    SCHOOL_ILLUSION,
    SCHOOL_NECROMANCY,
    SCHOOL_TRANSMUTATION,
    SCHOOL_COUNT
} School;

typedef enum {
    SPL_BARD = 1u << 0,
    SPL_CLERIC = 1u << 1,
    SPL_DRUID = 1u << 2,
    SPL_PALADIN = 1u << 3,
    SPL_RANGER = 1u << 4,
    SPL_SORCERER = 1u << 5,
    SPL_WARLOCK = 1u << 6,
    SPL_WIZARD = 1u << 7,
    SPL_ARTIFICER = 1u << 8,
} SpellClassBit;

typedef struct {
    const char *name;
    unsigned char book;         /* matches SourceBook in data.h:
                                   0 PHB, 1 XGE, 2 TCE */
    unsigned char level;        /* 0 = cantrip */
    unsigned char school;       /* School */
    unsigned char ritual;
    unsigned char concentration;
    const char *casting_time;
    const char *range;
    const char *components;
    const char *duration;
    unsigned short classes;     /* SpellClassBit mask */
} SpellData;

/* The bank is a pointer, not an array, so homebrew.c can
   replace it with a larger one holding the book spells plus
   whatever the DM has added. SPELLS[i] reads the same either
   way, and BOOK_SPELLS is what the book itself provides. */
extern const SpellData *SPELLS;
extern int SPELL_COUNT;
extern const SpellData BOOK_SPELLS[];
extern const int BOOK_SPELL_COUNT;
extern const char *const SCHOOL_NAMES[SCHOOL_COUNT];

/* What a spell actually does, in a sentence or two.
 *
 * The spell row carries the numbers a sheet needs -- level, school, range,
 * duration -- and none of what a player choosing between two of them wants
 * to know, which is what happens when it is cast. So the prose is held
 * beside the row rather than in it, the way an item's note is: the row
 * stays a fixed set of fields, and the description is looked up by name so
 * that a spell the DM adds simply has none.
 *
 * The words are the project's own, written from the rules, not the book's. */
typedef struct {
    const char *spell;
    const char *text;
} SpellNote;

extern const SpellNote SPELL_NOTES[];
extern const int SPELL_NOTE_COUNT;

/* The description for a spell, or NULL when there is none. */
const char *spell_notes(const char *name);

#endif
