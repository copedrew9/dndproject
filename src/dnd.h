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

typedef struct {
    int item_id;        /* index into ITEMS */
    int quantity;
    int equipped;       /* armour/shield actually worn */
} InventoryEntry;

/* Free-form in-class choices: fighting styles, pact boons, metamagic,
   eldritch invocations, battle master maneuvers, expertise notes. */
typedef struct {
    char label[MAX_NAME];
    char value[MAX_TEXT];
} ChoiceEntry;

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
