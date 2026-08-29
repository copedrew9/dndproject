/* dnd.h -- core types shared across the character creator.
 *
 * Implements the D&D 5th Edition character creation rules from the Player's
 * Handbook (chapters 1-6). Game statistics only; no descriptive prose from
 * the sourcebooks is reproduced here.
 */
#ifndef DND_H
#define DND_H

#include <stddef.h>

#define MAX_NAME 64
#define MAX_TEXT 256
/* The backstory is the one line of prose on a character that is not typed
   by hand. Xanathar's "This Is Your Life" walks thirteen tables and joins
   the answers, and the table names and separators alone cost 241
   characters before a single answer -- so the shortest complete life path
   is over 400 and MAX_TEXT could never hold one. It used to break out of
   the loop mid-word around the seventh table and store the fragment. */
#define MAX_BACKSTORY 1024
#define MAX_CLASSES 12      /* a character may not exceed 20 total levels */
#define MAX_SPELLS 128
#define MAX_ITEMS 96
#define MAX_FEATS 16
#define MAX_LANGS 12
#define MAX_PROFS 32
#define MAX_CHOICES 48
#define MAX_LEVEL 20

/* How many of one thing a prompt will take at once, how many one line of
   the inventory may hold, and how many of one coin. Buying twice adds up,
   so a stack may stand higher than a single purchase -- but not without a
   ceiling: ninety-six lines of an unbounded quantity of the heaviest thing
   in the book is not a heavy pack, it is an overflowed weight. */
/* Game mode: how many resources a character may track by hand, and how
   many purchases the ledger remembers. Both are generous for one session
   and small enough to sit in the file. */
#define MAX_RESOURCES 16
#define MAX_LEDGER 32
#define MAX_VALUABLES 32
#define MAX_EXHAUSTION 6

#define MAX_QUANTITY 99
#define MAX_STACK 999
#define MAX_COINS 999999

/* ---------------------------------------------------------------- abilities */

typedef enum {
    ABL_STR, ABL_DEX, ABL_CON, ABL_INT, ABL_WIS, ABL_CHA, ABL_COUNT
} Ability;

extern const char *const ABILITY_NAME[ABL_COUNT];
extern const char *const ABILITY_ABBREV[ABL_COUNT];

/* ------------------------------------------------------------------- skills */

typedef enum {
    SKL_ACROBATICS, SKL_ANIMAL_HANDLING, SKL_ARCANA, SKL_ATHLETICS,
    SKL_DECEPTION, SKL_HISTORY, SKL_INSIGHT, SKL_INTIMIDATION,
    SKL_INVESTIGATION, SKL_MEDICINE, SKL_NATURE, SKL_PERCEPTION,
    SKL_PERFORMANCE, SKL_PERSUASION, SKL_RELIGION, SKL_SLEIGHT_OF_HAND,
    SKL_STEALTH, SKL_SURVIVAL, SKL_COUNT
} Skill;

extern const char *const SKILL_NAME[SKL_COUNT];
int skill_by_name(const char *name);
extern const Ability SKILL_ABILITY[SKL_COUNT];

/* --------------------------------------------------------------- size, etc. */

typedef enum { SZ_SMALL, SZ_MEDIUM } CreatureSize;
extern const char *const SIZE_NAME[];

typedef enum {
    ALIGN_LG, ALIGN_NG, ALIGN_CG,
    ALIGN_LN, ALIGN_TN, ALIGN_CN,
    ALIGN_LE, ALIGN_NE, ALIGN_CE, ALIGN_COUNT
} Alignment;
extern const char *const ALIGNMENT_NAME[ALIGN_COUNT];

/* ---------------------------------------------------------------- the sheet */

typedef struct {
    int class_id;       /* index into CLASSES */
    int level;          /* levels taken in this class */
    int subclass_id;    /* index into SUBCLASSES, -1 if not yet chosen */
    int subclass_option;/* totem animal, land terrain, ...; -1 when N/A */
} ClassLevel;

/* A carried thing: either a line from the equipment catalogue or one of the
   Dungeon Master's Guide magic items. Which table item_id indexes depends on
   is_magic, so the two can sit in one list and be counted, weighed and
   printed together. */
typedef struct {
    int item_id;        /* index into ITEMS, or MAGIC_ITEMS when is_magic */
    int quantity;
    int equipped;       /* armour/shield actually worn */
    int is_magic;
    int attuned;        /* magic items only; at most three at a time */
    int plus;           /* for a +1/+2/+3 item, which one this copy is */
    /* For an item whose effect names something -- the damage a ring of
       resistance resists, the giant a belt draws its Strength from, the
       weapon a +1 weapon is. */
    char variant[MAX_NAME];
    /* What the sheet is allowed to say about a magic item. A DM handing
       one over often wants the player to have the thing without the
       entry: an unidentified rod is a rod. Both are set by the table and
       cleared again when the item is identified or the curse bites.
       Neither changes what the item does -- the numbers it brings still
       reach Armor Class and the attacks block, because the character is
       carrying it either way. */
    int concealed;      /* print the name and nothing else */
    int curse_hidden;   /* print the entry, but not what it costs you */
    /* How worn the thing is. The Player's Handbook has no general rule for
       equipment wearing out -- it is a house rule at most tables that use
       one at all -- so this records what the table decided rather than
       computing anything: 0 sound, 1 damaged, 2 broken. Nothing here
       changes a number on the sheet. */
    int wear;
} InventoryEntry;

#define MAX_ATTUNED 3
#define MAX_SIDEKICKS 4
#define MAX_NOTES 16
#define MAX_LORE 2048
#define MAX_SK_CHOICES 12


/* Something with a limited number of uses between rests: Bardic
   Inspiration, Ki, sorcery points, Channel Divinity, Rage, a wand's
   charges. The books spell out how many of each a character has, but the
   count depends on level, subclass and sometimes on a feat, so the table
   sets the maximum once and the program only counts down from it. */
typedef struct {
    char name[MAX_NAME];
    int used;
    int max;
    int per_long_rest;      /* 0 when a short rest brings it back */
} Resource;

/* Where the money went. Coins tell a player how much they have and never
   how they came to have that much, which is the question that starts an
   argument three sessions later. */
typedef struct {
    int copper;             /* negative when spent, positive when earned */
    char what[MAX_NAME];
} LedgerEntry;

/* Something carried that is not equipment: a gemstone off the Dungeon
   Master's Guide's treasure tables, or a thing with no entry anywhere --
   a letter of marque, a signet ring, the innkeeper's daughter's locket.
   Both are the same shape, because what a player wants from either is the
   same: a name, what it is worth if anything, and a line saying what it
   is or where it came from. */
typedef struct {
    char name[MAX_NAME];
    int value_cp;           /* 0 when it is worth nothing in particular */
    int quantity;
    char note[MAX_TEXT];
} Valuable;

/* Free-form in-class choices: fighting styles, pact boons, metamagic,
   eldritch invocations, battle master maneuvers, expertise notes. */
typedef struct {
    char label[MAX_NAME];
    char value[MAX_TEXT];
} ChoiceEntry;

/* A note the player keeps on a character: a contact, a debt, a patron's
   demands, the history behind a family sword. The body may run to
   paragraphs, so it is far larger than the other text on a character. */
typedef struct {
    char title[MAX_NAME];
    char body[MAX_LORE];
} Note;

/* A sidekick: a creature from the beast tables (or one typed in by hand)
   with levels in Expert, Spellcaster or Warrior. The creature supplies the
   ability scores, hit die, speed and attacks; the class supplies the
   proficiency bonus, features and any spellcasting. */
typedef struct {
    char name[MAX_NAME];
    char creature[MAX_NAME];    /* the stat block it is built on */
    int  cls;                   /* SidekickClass */
    int  level;
    int  role;                  /* SpellcasterRole, -1 when not a caster */
    int  abilities[6];          /* after any ability score improvements */
    int  hp;
    int  ac;
    char speed[MAX_NAME];
    ChoiceEntry choices[MAX_SK_CHOICES];
    int  choice_count;
    int  spells[MAX_SPELLS / 4];
    int  spell_count;
} Sidekick;

typedef struct {
    int spell_id;       /* index into SPELLS */
    int class_id;       /* the class this spell was learned as, or -1 when a
                           feat granted it and no class's limits apply */
    int prepared;       /* for preparation casters */
    int always_prepared;/* domain/oath/circle spells */
} SpellEntry;

typedef struct {
    char name[MAX_NAME];
    char player[MAX_NAME];

    int race_id;
    int subrace_id;         /* -1 when the race has no subraces */
    int background_id;
    Alignment alignment;

    /* Dragonborn draconic ancestry; -1 for every other race. */
    int ancestry_id;

    ClassLevel classes[MAX_CLASSES];
    int class_count;

    int base_score[ABL_COUNT];   /* as assigned, before any increases */
    int racial_bonus[ABL_COUNT]; /* from race and subrace */
    int asi_bonus[ABL_COUNT];    /* from Ability Score Improvements and feats */

    int skill_prof[SKL_COUNT];      /* 0 none, 1 proficient */
    int skill_expertise[SKL_COUNT]; /* rogue/bard expertise doubles the bonus */
    int save_prof[ABL_COUNT];

    char languages[MAX_LANGS][MAX_NAME];
    int language_count;

    char tool_profs[MAX_PROFS][MAX_NAME];
    int tool_prof_count;

    char other_profs[MAX_PROFS][MAX_NAME];  /* armour and weapon proficiencies */
    int other_prof_count;

    int feats[MAX_FEATS];
    int feat_count;

    InventoryEntry inventory[MAX_ITEMS];
    int item_count;
    int copper, silver, electrum, gold, platinum;

    /* Experience earned, when the table is using it. Kept as a total rather
       than derived from the level, because the two disagree on purpose: a
       character sits on their level until the DM says otherwise, and the
       gap between the total and the next threshold is the thing a player
       wants to see. */
    int xp;

    SpellEntry spells[MAX_SPELLS];
    int spell_count;

    /* One hit-die result per character level, excluding the Constitution
       modifier. Storing the rolls (rather than a total) keeps hit points
       correct when a later Ability Score Improvement changes Constitution. */
    int hp_rolls[MAX_LEVEL];
    int hp_roll_count;

    ChoiceEntry choices[MAX_CHOICES];
    int choice_count;

    /* Sidekicks (Tasha's chapter 4). A character may have more than one,
       though the book warns the table slows down past one each. */
    Sidekick sidekicks[MAX_SIDEKICKS];
    int sidekick_count;

    /* A background built with the PHB's customization rules rather than
       taken from the table. background_id is -1 in that case, and these
       carry what the player chose instead. */
    char background_name[MAX_NAME];
    char background_feature[MAX_NAME];
    char background_feature_text[MAX_TEXT];
    char background_equipment[MAX_TEXT];

    /* Anything the player wants to remember, from a one-line reminder to
       a character's history. Each has a title so a long one can be found
       again without reading it. */
    Note notes[MAX_NOTES];
    int  note_count;

    /* Personality (PHB chapter 4). */
    char trait[MAX_TEXT];
    char ideal[MAX_TEXT];
    char bond[MAX_TEXT];
    char flaw[MAX_TEXT];
    char appearance[MAX_TEXT];
    char backstory[MAX_BACKSTORY];
    int age, height_in, weight_lb;
    char eyes[MAX_NAME], skin[MAX_NAME], hair[MAX_NAME];

    /* ------------------------------------------------------- at the table
     *
     * Everything above is what the character IS, and is settled when they
     * are made or when they gain a level. Everything here is what has
     * happened to them since, and changes between one roll and the next:
     * how hurt they are, what they have spent, what is wrong with them.
     *
     * It is kept on the character rather than in a separate file because
     * it is the same character -- a sheet that says 42 hit points and a
     * note elsewhere saying 11 of them are gone is two sources for one
     * number, and they drift.
     */
    /* Spell slots spent, by level, and the pact slots a warlock has used.
       The maximum is worked out from the class table every time it is
       needed; what has to be remembered is only how many are gone. */
    int slots_used[10];
    int pact_used;

    int damage;                 /* hit points lost; current = max - damage */
    int temp_hp;                /* a separate pool, lost first, never healed */
    int hit_dice_used[MAX_CLASSES];   /* spent on a short rest, by class */
    int death_success, death_fail;    /* death saving throws, 0-3 each */
    int exhaustion;             /* 0 to MAX_EXHAUSTION */
    /* One bit per entry of CONDITIONS[], except exhaustion, which has
       levels rather than a yes or no and is counted above. */
    unsigned int conditions;

    Resource resources[MAX_RESOURCES];
    int resource_count;

    LedgerEntry ledger[MAX_LEDGER];
    int ledger_count;

    /* Gems and anything else carried that the equipment tables do not
       have. Kept apart from the inventory because they are not equipment:
       nothing is worn, wielded or weighed, and the only number that
       matters is what they would fetch. */
    Valuable valuables[MAX_VALUABLES];
    int valuable_count;
} Character;

/* ---------------------------------------------------------- derived numbers */

int  total_level(const Character *c);
int  ability_score(const Character *c, Ability a);
int  ability_mod_of(int score);
int  ability_mod(const Character *c, Ability a);
int  proficiency_bonus(const Character *c);
int  skill_bonus(const Character *c, Skill s);
int  save_bonus(const Character *c, Ability a);

/* Movement and defences a character's worn magic items grant. */
int  magic_fly_speed(const Character *c);
int  magic_swim_speed(const Character *c);
int  magic_climb_speed(const Character *c);
int  magic_defences(const Character *c, char *out, size_t n);
int  passive_perception(const Character *c);
int  hit_points_max(const Character *c);
int  caster_level(const Character *c);
int  spell_save_dc(const Character *c, int class_id);
int  spell_attack_bonus(const Character *c, int class_id);
int  find_class_slot(const Character *c, int class_id);
int  armour_class(const Character *c);

/* One line of the attacks block: what the character swings, what it hits
   at, and what it does when it lands. */
#define MAX_ATTACKS 24

typedef struct {
    char name[MAX_NAME];
    int  bonus;                 /* to hit */
    char damage[48];            /* "1d8 + 3 slashing" */
    char note[MAX_TEXT];        /* reach, range, versatile, thrown */
    int  proficient;
} Attack;

int  attacks_of(const Character *c, Attack *out, int max);
int  initiative_bonus(const Character *c);
int  speed_of(const Character *c);
int  carrying_capacity(const Character *c);
int  current_weight_tenths(const Character *c);
int  class_level_of(const Character *c, int class_id);
int  has_feat(const Character *c, int feat_id);

#endif /* DND_H */
