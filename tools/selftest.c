/* selftest.c -- assertions on the derived-statistics engine.
 *
 * Builds characters in memory and checks the numbers the PHB says they
 * should have. Build and run with "make test".
 */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>

enum { CLS_BARBARIAN = 0, CLS_BARD = 1, CLS_CLERIC = 2, CLS_DRUID = 3,
       CLS_FIGHTER = 4, CLS_MONK = 5, CLS_PALADIN = 6, CLS_RANGER = 7,
       CLS_ROGUE = 8, CLS_SORCERER = 9, CLS_WARLOCK = 10, CLS_WIZARD = 11 };

enum { SUB_ELDRITCH_KNIGHT = 15, SUB_DRACONIC = 27 };

static int failures;

static void check(int ok, const char *what, long got, long want)
{
    if (ok) return;
    printf("  FAIL %-52s got %ld, want %ld\n", what, got, want);
    failures++;
}

#define EQ(expr, want, what) do {                                           \
        long g_ = (long)(expr), w_ = (long)(want);                          \
        check(g_ == w_, what, g_, w_);                                      \
    } while (0)

static void reset(Character *c)
{
    memset(c, 0, sizeof *c);
    c->race_id = c->subrace_id = c->background_id = c->ancestry_id = -1;
    c->classes[0].subclass_id = -1;
}

static void add_class(Character *c, int id, int level, int subclass)
{
    int k = c->class_count++;
    c->classes[k].class_id = id;
    c->classes[k].level = level;
    c->classes[k].subclass_id = subclass;
    c->classes[k].subclass_option = -1;
}

static int find_race(const char *n)
{
    int i;
    for (i = 0; i < RACE_COUNT; i++) if (!strcmp(RACES[i].name, n)) return i;
    return -1;
}
static int find_subrace(const char *n)
{
    int i;
    for (i = 0; i < SUBRACE_COUNT; i++) if (!strcmp(SUBRACES[i].name, n)) return i;
    return -1;
}

/* ------------------------------------------------------------------ tests */

static void test_modifiers(void)
{
    printf("ability modifiers\n");
    EQ(ability_mod_of(1), -5, "score 1");
    EQ(ability_mod_of(3), -4, "score 3");
    EQ(ability_mod_of(8), -1, "score 8");
    EQ(ability_mod_of(9), -1, "score 9");
    EQ(ability_mod_of(10), 0, "score 10");
    EQ(ability_mod_of(11), 0, "score 11");
    EQ(ability_mod_of(15), 2, "score 15");
    EQ(ability_mod_of(20), 5, "score 20");
    EQ(ability_mod_of(30), 10, "score 30");
}

static void test_proficiency(void)
{
    Character c;
    int lvl;
    static const int want[21] = {0, 2,2,2,2, 3,3,3,3, 4,4,4,4,
                                 5,5,5,5, 6,6,6,6};
    printf("proficiency bonus\n");
    for (lvl = 1; lvl <= 20; lvl++) {
        char buf[48];
        reset(&c);
        add_class(&c, CLS_FIGHTER, lvl, -1);
        snprintf(buf, sizeof buf, "level %d", lvl);
        EQ(proficiency_bonus(&c), want[lvl], buf);
    }
}

/* The PHB's own worked example: Bruenor, a 1st-level mountain dwarf fighter
 * with the standard array, ends on 17/10/16/8/13/12 and 13 hit points. */
static void test_bruenor(void)
{
    Character c;

    printf("Bruenor (PHB chapter 1 example)\n");
    reset(&c);
    c.race_id = find_race("Dwarf");
    c.subrace_id = find_subrace("Mountain Dwarf");
    add_class(&c, CLS_FIGHTER, 1, -1);

    c.base_score[ABL_STR] = 15; c.base_score[ABL_DEX] = 10;
    c.base_score[ABL_CON] = 14; c.base_score[ABL_INT] = 8;
    c.base_score[ABL_WIS] = 13; c.base_score[ABL_CHA] = 12;
    c.racial_bonus[ABL_CON] = 2;    /* dwarf */
    c.racial_bonus[ABL_STR] = 2;    /* mountain dwarf */
    c.hp_rolls[0] = 10; c.hp_roll_count = 1;

    EQ(ability_score(&c, ABL_STR), 17, "Strength");
    EQ(ability_score(&c, ABL_CON), 16, "Constitution");
    EQ(ability_mod(&c, ABL_STR), 3, "Strength modifier");
    EQ(ability_mod(&c, ABL_CON), 3, "Constitution modifier");
    EQ(hit_points_max(&c), 13, "hit points (10 + Con 3)");
    EQ(speed_of(&c), 25, "dwarf speed");
    EQ(carrying_capacity(&c), 255, "carrying capacity (Str 17 x 15)");
}

static void test_hill_dwarf_and_tough(void)
{
    Character c;
    int tough = -1, i;

    printf("hit point bonuses\n");
    reset(&c);
    c.race_id = find_race("Dwarf");
    c.subrace_id = find_subrace("Hill Dwarf");
    add_class(&c, CLS_FIGHTER, 5, -1);
    for (i = 0; i < 5; i++) c.hp_rolls[i] = 6;
    c.hp_roll_count = 5;
    c.base_score[ABL_CON] = 14;     /* +2 */

    /* 30 rolled + 5 x Con 2 + 5 from Dwarven Toughness. */
    EQ(hit_points_max(&c), 45, "hill dwarf 5th level");

    for (i = 0; i < FEAT_COUNT; i++) if (!strcmp(FEATS[i].name, "Tough")) tough = i;
    c.feats[c.feat_count++] = tough;
    EQ(hit_points_max(&c), 55, "with the Tough feat (+2 per level)");
}

static void test_armour_class(void)
{
    Character c;

    printf("armor class\n");

    /* Unarmoured, Dex 14. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 14;
    EQ(armour_class(&c), 12, "unarmoured, Dex 14");

    /* Chain mail: fixed 16, Dexterity ignored. */
    add_item_by_name(&c, "Chain mail", 1, 1);
    EQ(armour_class(&c), 16, "chain mail");

    /* Plus a shield. */
    add_item_by_name(&c, "Shield", 1, 1);
    EQ(armour_class(&c), 18, "chain mail and shield");

    /* Leather armour caps nothing: 11 + full Dex. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 18;
    add_item_by_name(&c, "Leather armor", 1, 1);
    EQ(armour_class(&c), 15, "leather armor, Dex 18");

    /* Half plate caps the Dexterity bonus at +2. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 18;
    add_item_by_name(&c, "Half plate", 1, 1);
    EQ(armour_class(&c), 17, "half plate caps Dex at +2");

    /* Barbarian Unarmored Defense: 10 + Dex + Con. */
    reset(&c);
    add_class(&c, CLS_BARBARIAN, 1, -1);
    c.base_score[ABL_DEX] = 14; c.base_score[ABL_CON] = 16;
    EQ(armour_class(&c), 15, "barbarian unarmored defense");

    /* Monk Unarmored Defense: 10 + Dex + Wis. */
    reset(&c);
    add_class(&c, CLS_MONK, 1, -1);
    c.base_score[ABL_DEX] = 16; c.base_score[ABL_WIS] = 14;
    EQ(armour_class(&c), 15, "monk unarmored defense");

    /* Draconic Resilience: 13 + Dex. */
    reset(&c);
    add_class(&c, CLS_SORCERER, 1, SUB_DRACONIC);
    c.base_score[ABL_DEX] = 12;
    EQ(armour_class(&c), 14, "draconic sorcerer unarmoured");
}

static void test_spell_slots(void)
{
    Character c;
    int slots[10];

    printf("spell slots\n");

    /* A 5th-level wizard: 4/3/2. */
    reset(&c);
    add_class(&c, CLS_WIZARD, 5, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "wizard 5, 1st level");
    EQ(slots[2], 3, "wizard 5, 2nd level");
    EQ(slots[3], 2, "wizard 5, 3rd level");

    /* A 5th-level paladin uses the half-caster table: 4/2. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5, 1st level");
    EQ(slots[2], 2, "paladin 5, 2nd level");

    /* Paladin 5 with a non-casting class still uses the paladin table:
     * the PHB's multiclass table applies only when Spellcasting comes from
     * more than one class. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    add_class(&c, CLS_ROGUE, 1, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5 / rogue 1, 1st level");
    EQ(slots[2], 2, "paladin 5 / rogue 1, 2nd level");

    /* Two casting classes: half levels round down, so paladin 5 counts 2,
     * plus sorcerer 1, giving caster level 3 -> 4/2. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    add_class(&c, CLS_SORCERER, 1, -1);
    EQ(caster_level(&c), 3, "paladin 5 / sorcerer 1 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5 / sorcerer 1, 1st level");
    EQ(slots[2], 2, "paladin 5 / sorcerer 1, 2nd level");

    /* Wizard 3 / cleric 3 -> caster level 6 -> 4/3/3. */
    reset(&c);
    add_class(&c, CLS_WIZARD, 3, -1);
    add_class(&c, CLS_CLERIC, 3, -1);
    EQ(caster_level(&c), 6, "wizard 3 / cleric 3 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "wizard 3 / cleric 3, 1st level");
    EQ(slots[2], 3, "wizard 3 / cleric 3, 2nd level");
    EQ(slots[3], 3, "wizard 3 / cleric 3, 3rd level");

    /* Eldritch Knight is a third-caster: fighter 3 -> caster level 1. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 3, SUB_ELDRITCH_KNIGHT);
    EQ(caster_level(&c), 1, "eldritch knight 3 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 2, "eldritch knight 3, 1st level");

    /* A plain fighter has none. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 10, -1);
    EQ(spell_slots_for(&c, slots), 0, "fighter has no slots");
}

static void test_pact_magic(void)
{
    Character c;
    int n, lvl;

    printf("pact magic\n");
    reset(&c);
    add_class(&c, CLS_WARLOCK, 1, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 1, "warlock 1 slot count");
    EQ(lvl, 1, "warlock 1 slot level");

    reset(&c);
    add_class(&c, CLS_WARLOCK, 11, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 3, "warlock 11 slot count");
    EQ(lvl, 5, "warlock 11 slot level");

    reset(&c);
    add_class(&c, CLS_WARLOCK, 20, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 4, "warlock 20 slot count");
    EQ(lvl, 5, "warlock 20 slot level");
}

static void test_skills_and_saves(void)
{
    Character c;

    printf("skills and saving throws\n");
    reset(&c);
    add_class(&c, CLS_ROGUE, 5, -1);
    c.base_score[ABL_DEX] = 16;         /* +3 */
    c.skill_prof[SKL_STEALTH] = 1;
    EQ(proficiency_bonus(&c), 3, "level 5 proficiency");
    EQ(skill_bonus(&c, SKL_STEALTH), 6, "Stealth, proficient");
    c.skill_expertise[SKL_STEALTH] = 1;
    EQ(skill_bonus(&c, SKL_STEALTH), 9, "Stealth, expertise");
    EQ(skill_bonus(&c, SKL_ACROBATICS), 3, "Acrobatics, not proficient");

    /* A bard of 2nd level adds half proficiency to everything else. */
    reset(&c);
    add_class(&c, CLS_BARD, 2, -1);
    c.base_score[ABL_DEX] = 14;         /* +2 */
    EQ(skill_bonus(&c, SKL_ACROBATICS), 3, "Jack of All Trades");

    /* A monk of 14th level is proficient in every save. */
    reset(&c);
    add_class(&c, CLS_MONK, 14, -1);
    c.base_score[ABL_INT] = 10;
    EQ(save_bonus(&c, ABL_INT), 5, "monk 14 Diamond Soul");
}

static void test_spell_data(void)
{
    int i, cantrips = 0, rituals = 0, conc = 0, noclass = 0;

    printf("spell data\n");
    EQ(SPELL_COUNT, 361, "PHB spell count");
    for (i = 0; i < SPELL_COUNT; i++) {
        if (SPELLS[i].level == 0) cantrips++;
        if (SPELLS[i].ritual) rituals++;
        if (SPELLS[i].concentration) conc++;
        if (SPELLS[i].classes == 0) noclass++;
        if (!SPELLS[i].name[0] || !SPELLS[i].casting_time[0]
            || !SPELLS[i].range[0] || !SPELLS[i].duration[0]) {
            printf("  FAIL spell %d has an empty field\n", i);
            failures++;
        }
    }
    EQ(cantrips, 27, "cantrip count");
    EQ(noclass, 0, "spells on no class list");
    check(rituals > 25 && rituals < 40, "ritual spell count in range",
          rituals, 31);
    check(conc > 140 && conc < 170, "concentration spell count in range",
          conc, 154);
}

static void test_data_integrity(void)
{
    int i;

    printf("data table integrity\n");
    EQ(RACE_COUNT, 9, "PHB races");
    EQ(CLASS_COUNT, 12, "PHB classes");
    EQ(SUBCLASS_COUNT, 40, "PHB subclasses");
    EQ(BACKGROUND_COUNT, 13, "PHB backgrounds");
    EQ(FEAT_COUNT, 42, "PHB feats");

    /* Every subclass must point back at a real class, and every class's
     * subclass window must land on subclasses belonging to it. */
    for (i = 0; i < CLASS_COUNT; i++) {
        int k;
        for (k = 0; k < CLASSES[i].subclass_count; k++) {
            int s = CLASSES[i].first_subclass + k;
            char buf[64];
            snprintf(buf, sizeof buf, "%s subclass %d owner", CLASSES[i].name, k);
            check(s >= 0 && s < SUBCLASS_COUNT && SUBCLASSES[s].class_id == i,
                  buf, s < SUBCLASS_COUNT ? SUBCLASSES[s].class_id : -1, i);
        }
    }
    /* Every feature must name a class that exists and a level in range. */
    for (i = 0; i < FEATURE_COUNT; i++) {
        if (FEATURES[i].class_id < 0 || FEATURES[i].class_id >= CLASS_COUNT
            || FEATURES[i].level < 1 || FEATURES[i].level > MAX_LEVEL) {
            printf("  FAIL feature %d (%s) is out of range\n", i,
                   FEATURES[i].name);
            failures++;
        }
        if (FEATURES[i].subclass_id != -1
            && SUBCLASSES[FEATURES[i].subclass_id].class_id
               != FEATURES[i].class_id) {
            printf("  FAIL feature %d (%s) names another class's subclass\n",
                   i, FEATURES[i].name);
            failures++;
        }
    }
    /* Armour must be findable by name, since the wizard looks it up that way. */
    check(find_item("Chain mail") >= 0, "find Chain mail",
          find_item("Chain mail"), 0);
    check(find_item("chain mail") >= 0, "find chain mail (case-insensitive)",
          find_item("chain mail"), 0);
}

static void test_carrying_and_coins(void)
{
    Character c;

    printf("weight and coins\n");
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_STR] = 15;
    EQ(carrying_capacity(&c), 225, "Str 15 x 15");

    add_item_by_name(&c, "Chain mail", 1, 1);     /* 55 lb */
    EQ(current_weight_tenths(&c), 550, "chain mail weighs 55 lb");

    c.gold = 100;                                  /* 50 coins to the pound */
    EQ(current_weight_tenths(&c), 570, "100 gp adds 2 lb");
}

int main(void)
{
    printf("dndcreator self-test\n\n");

    test_modifiers();
    test_proficiency();
    test_bruenor();
    test_hill_dwarf_and_tough();
    test_armour_class();
    test_spell_slots();
    test_pact_magic();
    test_skills_and_saves();
    test_spell_data();
    test_data_integrity();
    test_carrying_and_coins();

    printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
    return failures ? 1 : 0;
}
