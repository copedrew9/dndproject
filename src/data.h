/* data.h -- PHB game data tables (races, classes, backgrounds, feats, gear). */
#ifndef DATA_H
#define DATA_H

#include "dnd.h"

/* ------------------------------------------------------------- source books */

typedef enum {
    BOOK_PHB,       /* Player's Handbook */
    BOOK_XGE,       /* Xanathar's Guide to Everything */
    BOOK_TCE,       /* Tasha's Cauldron of Everything */
    BOOK_DMG,       /* Dungeon Master's Guide */
    BOOK_MPMM,      /* Mordenkainen Presents: Monsters of the Multiverse */
    BOOK_MM,        /* Monster Manual */
    BOOK_HOMEBREW,  /* whatever the DM has added */
    BOOK_COUNT
} SourceBook;

extern const char *const BOOK_NAME[BOOK_COUNT];
extern const char *const BOOK_ABBREV[BOOK_COUNT];

/* Which books and optional rules a build may draw on. Chosen from the
   settings menu and stored with the character so a reload offers the same
   content. */
typedef struct {
    int book[BOOK_COUNT];       /* 1 = enabled */
    int custom_origins;         /* Tasha's Customizing Your Origin */
    int optional_features;      /* Tasha's optional class features */
    int multiclassing;          /* PHB chapter 6 variant rule */
    int feats;                  /* PHB feats variant rule */
} Settings;

extern Settings SETTINGS;

void settings_defaults(Settings *s);
int  book_enabled(SourceBook b);
void settings_menu(Settings *s);
void settings_summary(const Settings *s, char *out, size_t n);

/* ------------------------------------------------------------------- races */

typedef struct {
    const char *name;
    SourceBook book;
    int ability[ABL_COUNT];
    int speed;
    CreatureSize size;
    int darkvision;             /* feet; 0 = none */
    const char *languages;      /* comma separated, always known */
    int extra_languages;        /* free language choices */
    int extra_skills;           /* free skill choices (half-elf) */
    int choice_asi_count;       /* +1 to N abilities of choice (half-elf, variant human) */
    int choice_asi_amount;
    int bonus_feats;            /* variant human */
    int has_ancestry;           /* dragonborn */
    int first_subrace, subrace_count;
    const char *traits;         /* '|' separated trait summaries */
    /* Races from Monsters of the Multiverse have no fixed ability
       increases at all: the spread is always chosen, whatever the custom
       origins setting says. Appended last so the older rows, which leave it
       out, are zero-filled. */
    int origin_choice;
} RaceData;

typedef struct {
    const char *name;
    SourceBook book;
    int ability[ABL_COUNT];
    int speed_override;         /* 0 = inherit from race (wood elf: 35) */
    int darkvision_override;    /* 0 = inherit (drow: 120) */
    int extra_languages;
    int choice_asi_count;       /* variant human: +1 to two chosen abilities */
    int extra_skills;           /* variant human: one skill of choice */
    int bonus_feats;            /* variant human: one feat */
    int replaces_race_asi;      /* variant human replaces the +1-to-all */
    const char *traits;
} SubraceData;

extern const RaceData RACES[];
extern const int RACE_COUNT;
extern const SubraceData SUBRACES[];
extern const int SUBRACE_COUNT;

typedef struct {
    const char *dragon;
    const char *damage;
    const char *breath;
} AncestryData;
extern const AncestryData ANCESTRIES[];
extern const int ANCESTRY_COUNT;

/* ----------------------------------------------------------------- classes */

typedef enum {
    CAST_NONE,      /* no spellcasting */
    CAST_FULL,      /* bard, cleric, druid, sorcerer, wizard */
    CAST_HALF,      /* paladin, ranger */
    CAST_THIRD,     /* eldritch knight, arcane trickster */
    CAST_PACT       /* warlock -- pact magic, its own slot table */
} CasterType;

typedef enum {
    PREP_NONE,      /* not a caster */
    PREP_KNOWN,     /* bard, ranger, sorcerer, warlock: fixed spells known */
    PREP_PREPARED,  /* cleric, druid, paladin: prepare mod + level each day */
    PREP_SPELLBOOK  /* wizard */
} PrepStyle;

typedef struct {
    const char *name;
    SourceBook book;
    int hit_die;
    Ability save_prof[2];
    const char *armour_profs;
    const char *weapon_profs;
    const char *tool_profs;         /* "" when none */
    const Skill *skill_options;
    int skill_option_count;
    int skill_picks;
    CasterType caster;
    PrepStyle prep;
    Ability spell_ability;
    int subclass_level;
    const char *subclass_label;
    int first_subclass, subclass_count;   /* unused; see subclasses_of() */
    /* Multiclassing prerequisites (PHB chapter 6). */
    Ability mc_req[2];
    int mc_req_score[2];
    int mc_req_count;
    int mc_req_either;              /* fighter/monk: either of the two suffices */
    const char *mc_profs;
    int gold_dice;                  /* starting wealth: gold_dice d4 x gold_mult gp */
    int gold_mult;                  /* 10 for every class except the monk */
    const unsigned char *cantrips_known;  /* [MAX_LEVEL+1] or NULL */
    const unsigned char *spells_known;    /* [MAX_LEVEL+1] or NULL */
    const char *quick_build;
    const char *equipment;          /* '|' separated starting equipment choices */
    /* The class level at which Spellcasting arrives: 1 for full casters and
       the artificer, 2 for paladins and rangers, 3 for the third-casters,
       0 for classes that never cast. */
    int caster_start_level;
    /* The artificer alone counts half its levels rounded UP towards the
       multiclass spellcaster table (Tasha's, p.13). */
    int mc_round_up;
} ClassData;

typedef struct {
    int class_id;
    SourceBook book;
    const char *name;
    const char *summary;
    /* Domain/oath/circle spells always prepared, "" when none. Groups are
       '|' separated in ascending order of the level they are gained. */
    const char *bonus_spells;
    /* Some subclasses carry a further choice (totem animal, land terrain). */
    const char *option_label;
    const char *options;        /* '|' separated, "" when none */
} SubclassData;

typedef struct {
    int class_id;
    int subclass_id;    /* -1 = base class feature */
    int level;
    const char *name;
    const char *summary;
} FeatureData;

/* Subclasses are found by scanning for their owning class rather than by a
   contiguous index window, so new ones can be appended to the table without
   renumbering anything. */
int subclass_by_name(const char *name);
int subclass_is(int subclass_id, const char *name);
int subclasses_of(int class_id, int *out, int max);
int class_by_name(const char *name);

extern const ClassData CLASSES[];
extern const int CLASS_COUNT;
extern const SubclassData SUBCLASSES[];
extern const int SUBCLASS_COUNT;
extern const FeatureData FEATURES[];
extern const int FEATURE_COUNT;

/* Spell slot progressions. */
extern const unsigned char FULL_SLOTS[MAX_LEVEL + 1][10];
extern const unsigned char PACT_SLOTS[MAX_LEVEL + 1][2];  /* {count, level} */
/* Tasha's optional class features. */
typedef struct {
    int class_id;
    SourceBook book;
    int level;                  /* class level at which it becomes available */
    const char *name;
    const char *replaces;       /* "" when it adds rather than replaces */
    const char *summary;
} OptionalFeature;

extern const OptionalFeature OPTIONAL_FEATURES[];
extern const int OPTIONAL_FEATURE_COUNT;

/* Spells the "Additional <Class> Spells" features add to a class list. */
typedef struct {
    int class_id;
    const char *spells;         /* comma separated, lowercase */
} AdditionalSpells;

extern const AdditionalSpells ADDITIONAL_SPELLS[];
extern const int ADDITIONAL_SPELLS_COUNT;

extern const char *const TASHA_FIGHTER_STYLES;
extern const char *const TASHA_PALADIN_STYLES;
extern const char *const TASHA_RANGER_STYLES;

/* Artificer infusions (Tasha's). */
typedef struct {
    const char *name;
    const char *prereq;         /* "" when none; otherwise a level note */
    int min_level;              /* artificer level required */
    const char *item;           /* the kind of object infused */
    const char *summary;
} InfusionData;

extern const InfusionData INFUSIONS[];
extern const int INFUSION_COUNT;

/* Infusions known and items that can bear one, by artificer level. */
extern const unsigned char INFUSIONS_KNOWN[MAX_LEVEL + 1];
extern const unsigned char INFUSED_ITEMS[MAX_LEVEL + 1];

/* Eldritch Knight / Arcane Trickster progressions. */
extern const unsigned char *const THIRD_CANTRIPS;
extern const unsigned char *const THIRD_SPELLS_KNOWN;

/* Levels at which a class grants an Ability Score Improvement. */
int asi_levels_for(int class_id, int out[], int max);

/* ------------------------------------------------------------- backgrounds */

typedef struct {
    const char *name;
    SourceBook book;
    Skill skills[2];
    const char *tool_profs;
    int extra_languages;
    const char *equipment;
    int gold;
    const char *feature_name;
    const char *feature_summary;
    const char *traits[8];      /* suggested personality traits */
    const char *ideals[6];
    const char *bonds[6];
    const char *flaws[6];
} BackgroundData;

extern const BackgroundData BACKGROUNDS[];
extern const int BACKGROUND_COUNT;

/* -------------------------------------------------------------------- feats */

typedef struct {
    const char *name;
    SourceBook book;
    const char *prereq;         /* human readable, "" when none */
    /* Machine-checkable prerequisites. */
    Ability req_ability;        /* ABL_COUNT = none */
    Ability req_ability2;       /* ABL_COUNT = none; either one satisfies it */
    int req_score;
    const char *req_prof;       /* required armour proficiency, "" = none */
    int req_spellcasting;
    int asi[ABL_COUNT];         /* half-feats that raise a fixed score */
    int asi_choice_count;       /* half-feats that raise a chosen score */
    const char *asi_choices;    /* "STR,DEX"; "" means any ability */
    const char *summary;
    /* Race the feat is limited to, matched against the character's race and
       subrace names. "" means anyone may take it. Several of Xanathar's
       racial feats accept more than one, separated by '|'. */
    const char *req_race;
} FeatData;

extern const FeatData FEATS[];
extern const int FEAT_COUNT;

/* ---------------------------------------------------------------- equipment */

typedef enum {
    ITEM_LIGHT_ARMOR, ITEM_MEDIUM_ARMOR, ITEM_HEAVY_ARMOR, ITEM_SHIELD,
    ITEM_SIMPLE_MELEE, ITEM_SIMPLE_RANGED, ITEM_MARTIAL_MELEE,
    ITEM_MARTIAL_RANGED, ITEM_GEAR, ITEM_TOOL, ITEM_PACK, ITEM_MOUNT
} ItemCategory;

typedef struct {
    const char *name;
    SourceBook book;
    ItemCategory category;
    int cost_cp;                /* in copper pieces */
    int weight_tenths;          /* pounds x 10, so 0.5 lb is 5 */
    /* Armour */
    int base_ac;
    int dex_cap;                /* -1 = uncapped, 0 = none added */
    int str_req;
    int stealth_disadvantage;
    /* Weapons */
    const char *damage;
    const char *damage_type;
    const char *properties;
    const char *contents;       /* for packs */
} ItemData;

/* The banks are pointers, not arrays, so homebrew.c can replace them with
   larger ones holding the book's entries plus whatever the DM has added.
   ITEMS[i] reads the same either way; BOOK_ITEMS is what the books give. */
extern const ItemData *ITEMS;
extern int ITEM_COUNT;
extern const ItemData BOOK_ITEMS[];
extern const int BOOK_ITEM_COUNT;

int find_item(const char *name);

/* Item detail: descriptions, and what an item does when it has no stat line.
 * data_itemtext.c carries the prose; data_equipment.c carries the numbers. */
typedef struct {
    const char *item;
    const char *text;
} ItemNote;

extern const ItemNote ITEM_NOTES[];
extern const int ITEM_NOTE_COUNT;
const char *item_notes(const char *name);

/* Class indexes into CLASSES; they must match the order in
 * data_classes.c. */
enum { CLS_BARBARIAN = 0, CLS_BARD = 1, CLS_CLERIC = 2, CLS_DRUID = 3,
       CLS_FIGHTER = 4, CLS_MONK = 5, CLS_PALADIN = 6, CLS_RANGER = 7,
       CLS_ROGUE = 8, CLS_SORCERER = 9, CLS_WARLOCK = 10, CLS_WIZARD = 11,
       CLS_ARTIFICER = 12 };

/* Xanathar's "This Is Your Life" tables. Each row carries the range of
 * results that produce it, so a table can be rolled on or read down and
 * picked from. */
typedef struct {
    int lo, hi;
    const char *text;
} LifeEntry;

typedef struct {
    const char *name;
    const char *die;
    const LifeEntry *rows;
    int count;
} LifeTable;

extern const LifeTable LIFE_TABLES[];
extern const int LIFE_TABLE_COUNT;

/* The gods of appendix B. A cleric or paladin names one, and the suggested
 * domains connect that choice to the Divine Domain menu. */
typedef struct {
    const char *name;
    const char *title;          /* "goddess of winter" */
    const char *pantheon;
    const char *alignment;
    const char *domains;        /* comma separated, "None" when none */
    const char *symbol;
} Deity;

extern const Deity DEITIES[];
extern const int DEITY_COUNT;
int find_deity(const char *name);

/* Tasha's sidekicks: a creature of challenge 1/2 or lower given levels in
 * one of three simple classes. */
typedef enum { SK_EXPERT, SK_SPELLCASTER, SK_WARRIOR, SK_CLASS_COUNT }
    SidekickClass;
typedef enum { SK_MAGE, SK_HEALER, SK_PRODIGY, SK_ROLE_COUNT }
    SpellcasterRole;

extern const char *const SIDEKICK_CLASS_NAME[SK_CLASS_COUNT];
extern const char *const SIDEKICK_CLASS_BLURB[SK_CLASS_COUNT];
extern const char *const SPELLCASTER_ROLE_NAME[SK_ROLE_COUNT];
extern const char *const SPELLCASTER_ROLE_DESC[SK_ROLE_COUNT];

typedef struct {
    SidekickClass cls;
    int level;
    const char *name;
    const char *summary;
} SidekickFeature;

extern const SidekickFeature SIDEKICK_FEATURES[];
extern const int SIDEKICK_FEATURE_COUNT;

extern const unsigned char SPELLCASTER_CANTRIPS[MAX_LEVEL + 1];
extern const unsigned char SPELLCASTER_SPELLS_KNOWN[MAX_LEVEL + 1];

/* Beasts, from the Monster Manual: what Wild Shape, a Beast Master
 * companion and find familiar draw on. Challenge is stored in eighths, so
 * 1/8 is 1, 1/4 is 2, 1/2 is 4 and 1 is 8; the druid's limits are then
 * integer comparisons. */
typedef enum {
    BSIZE_TINY, BSIZE_SMALL, BSIZE_MEDIUM, BSIZE_LARGE, BSIZE_HUGE,
    BSIZE_GARGANTUAN
} BeastSize;

extern const char *const BEAST_SIZE_NAME[];

typedef struct {
    const char *name;
    BeastSize size;
    int ac;
    int hp;
    int cr_eighths;
    const char *speed;
    int abilities[6];           /* STR DEX CON INT WIS CHA */
    const char *cr_text;
    const char *senses;
} BeastData;

extern const BeastData BEASTS[];
extern const int BEAST_COUNT_ACTUAL;
int find_beast(const char *name);

/* Does the beast's speed line mention a swimming or flying speed? Wild
 * Shape withholds those until 4th and 8th level. */
int beast_swims(const BeastData *b);
int beast_flies(const BeastData *b);

/* The lists a class picks from as it levels: eldritch invocations,
 * metamagic, maneuvers, pact boons, arcane shots, elemental disciplines,
 * runes, and the ranger's favoured enemies and terrains. */
typedef struct {
    const char *name;
    SourceBook book;
    int min_level;              /* class level required; 0 = none */
    const char *prereq;         /* other requirement, "" when none */
    const char *summary;
} ClassOption;

typedef struct {
    const char *class_name;
    const char *subclass_name;  /* "" means the whole class */
    const char *label;          /* singular, e.g. "Eldritch Invocation" */
    const char *plural;
    const ClassOption *options;
    int count;
    const unsigned char *known; /* known[level], indexed by class level */
    int repeatable;             /* the same entry may be chosen twice */
} OptionList;

/* Extra spells that depend on a subclass option rather than the subclass:
 * the Circle of the Land's terrain and the Genie warlock's patron kind.
 * "levels" is a comma-separated list matching the '|' separated groups. */
typedef struct {
    const char *subclass;
    const char *option;
    const char *levels;
    const char *spells;
} OptionSpells;

extern const OptionSpells OPTION_SPELLS[];
extern const int OPTION_SPELLS_COUNT;

extern const OptionList OPTION_LISTS[];
extern const int OPTION_LIST_COUNT;

/* Trinkets, lifestyles and the price of things that are not equipment. */
extern const char *const TRINKETS[];
extern const int TRINKET_COUNT;

typedef struct {
    const char *name;
    int cost_cp_per_day;        /* 0 for wretched, which costs nothing */
    const char *text;
} Lifestyle;

extern const Lifestyle LIFESTYLES[];
extern const int LIFESTYLE_COUNT;

typedef struct {
    const char *name;
    int cost_cp;
} PriceEntry;

extern const PriceEntry SERVICES[];
extern const int SERVICE_COUNT;
extern const PriceEntry SPELLCASTING_SERVICES[];
extern const int SPELLCASTING_SERVICE_COUNT;

/* Magic items, from the Dungeon Master's Guide. Attunement is separate from
 * the rarity line because a character may attune to only three at once. */
typedef struct {
    const char *name;
    SourceBook book;
    const char *type;
    const char *rarity;
    const char *attunement;     /* NULL when no attunement is required */
    const char *text;
} MagicItem;

extern const MagicItem *MAGIC_ITEMS;
extern int MAGIC_ITEM_COUNT;
extern const MagicItem BOOK_MAGIC_ITEMS[];
extern const int BOOK_MAGIC_ITEM_COUNT;
int find_magic_item(const char *name);

/* The handful of magic items that change a number the sheet computes.
 * Kept beside MAGIC_ITEMS rather than inside it, because it describes about
 * twenty of the two hundred and seventy entries. Only unconditional effects
 * live here; anything situational stays prose. */
typedef struct {
    const char *item;           /* names a MAGIC_ITEMS entry */
    int ac_bonus;               /* flat, added to Armor Class */
    int save_bonus;             /* flat, added to every saving throw */
    int armor_base;             /* >0 when the item is itself armour */
    int armor_dex;              /* -1 full modifier, 0 none, N a cap */
    int armor_str;              /* Strength needed to avoid being slowed */
    int armor_stealth;          /* disadvantage on Stealth */
    int shield;                 /* the whole shield bonus, its own +2 too */
    int only_unarmored;         /* applies only with no armour or shield */
    int unarmored_base;         /* sets the unarmoured base AC instead */
    int variable;               /* the bonus is the copy's own +N */

    /* Scores an item sets outright, rather than adding to. sets_ability is
       the ability plus one, so zero means none; sets_to of 0 means the copy
       carries the score, as a belt of giant strength does. */
    int sets_ability;
    int sets_to;

    int sets_speed;             /* walking speed becomes this, if higher */
    int fly_speed;              /* -1 means "equal to your walking speed" */
    int swim_speed;
    int climb_speed;

    /* Damage the wearer resists or ignores. "*" means the copy carries the
       type, as armour and a ring of resistance do. */
    const char *resist;
    const char *immune;
} MagicRule;

extern const MagicRule MAGIC_RULES[];
extern const int MAGIC_RULE_COUNT;
const MagicRule *magic_rule_for(const char *name);

extern const ItemNote WEAPON_PROPERTIES[];
extern const int WEAPON_PROPERTY_COUNT;

/* ------------------------------------------------------------------ lookups */

extern const char *const LANGUAGES[];
extern const int LANGUAGE_COUNT;

/* The names the data files use for an item's category, a spell's school and
 * the classes a spell belongs to. homebrew.txt is written in the same terms,
 * so a DM editing it by hand writes "martial-melee" rather than a 6. */
extern const char *const ITEM_CATEGORY_NAME[];
extern const int ITEM_CATEGORY_COUNT;
extern const char *const SPELL_CLASS_NAME[];
extern const int SPELL_CLASS_NAME_COUNT;

int item_category_by_name(const char *s);       /* -1 when unknown */
int school_by_name(const char *s);              /* -1 when unknown */
unsigned spell_classes_by_name(const char *csv);
void spell_classes_text(unsigned mask, char *out, size_t n);

#endif /* DATA_H */
