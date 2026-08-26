/* data.h -- PHB game data tables (races, classes, backgrounds, feats, gear). */
#ifndef DATA_H
#define DATA_H

#include "dnd.h"

/* ------------------------------------------------------------------- races */

typedef struct {
    const char *name;
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
} RaceData;

typedef struct {
    const char *name;
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

extern const ItemData ITEMS[];
extern const int ITEM_COUNT;

int find_item(const char *name);

/* ------------------------------------------------------------------ lookups */

extern const char *const LANGUAGES[];
extern const int LANGUAGE_COUNT;

#endif /* DATA_H */
