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
#define MAX_CLASSES 12      /* a character may not exceed 20 total levels */
#define MAX_SPELLS 128
#define MAX_ITEMS 96
#define MAX_FEATS 16
#define MAX_LANGS 12
#define MAX_PROFS 32
#define MAX_CHOICES 48
#define MAX_LEVEL 20

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
       resistance resists, the giant a belt draws its Strength from. */
    char variant[24];
} InventoryEntry;

#define MAX_ATTUNED 3
#define MAX_SIDEKICKS 4
#define MAX_NOTES 16
#define MAX_LORE 2048
#define MAX_SK_CHOICES 12


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
    int  beast_id;              /* index into BEASTS, or -1 when typed in */
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
    int class_id;       /* the class this spell was learned as */
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
    char backstory[MAX_TEXT];
    int age, height_in, weight_lb;
    char eyes[MAX_NAME], skin[MAX_NAME], hair[MAX_NAME];
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
int  initiative_bonus(const Character *c);
int  speed_of(const Character *c);
int  carrying_capacity(const Character *c);
int  current_weight_tenths(const Character *c);
int  class_level_of(const Character *c, int class_id);
int  has_feat(const Character *c, int feat_id);

#endif /* DND_H */
