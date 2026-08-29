/* combos.c -- every combination the tables allow, put through the engine.
 *
 * tools/selftest.c sweeps each table on its own: every race, every class at
 * every level, every subclass, every spell, every item, every beast. What it
 * does not do is cross them. A bug that needs a particular race in a
 * particular class at a particular level -- a dwarf cleric of the Forge
 * Domain at 17th, a variant human artificer with an infusion -- lives in the
 * space between those sweeps, and nothing was looking there.
 *
 * This walks that space. It builds characters in memory rather than through
 * the wizard, which is what makes the numbers affordable: a character built,
 * measured and thrown away costs microseconds, so the whole cross product of
 * race, class, subclass and level fits in a few seconds.
 *
 *   every race and subrace     x every class x every level        (1..20)
 *   every subclass             x every level it can be held at
 *   every subclass option      (totem, land, rune, genie and the rest)
 *   every pair of classes      x the level splits that matter
 *   every triple of classes    at 20 levels between them
 *   every background           x every class
 *   every feat                 on a character of every class that qualifies
 *   every weapon               x every class x three levels
 *   every armour               x seven Dexterities x two Strengths
 *                              x shield or none
 *   every combination of books and optional rules             (8,192 of them)
 *
 * What is checked at each point is not that the numbers are any particular
 * value -- selftest.c does that against the book -- but that they are
 * possible at all: hit points above zero, an armour class in the range armour
 * can produce, a proficiency bonus that matches the level, spell slots that
 * exist in the table they came from, an attacks block that fits its array,
 * and no read of a table outside its bounds. Run under the sanitizers, that
 * last one is the point of the whole exercise.
 *
 * A share of the combinations are also written to a file, read back and
 * written again, and the two files compared. Every combination cannot be:
 * there are two million of them and a file each would take an hour. The
 * subset is chosen to cover each axis completely rather than at random --
 * every race with a class, every subclass, every pair of classes -- which is
 * what makes it a check rather than a sample.
 *
 * Build and run with `make combos`.
 */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "saveload.h"
#include "data_spells.h"
#include "ui.h"
#include "sidekick.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long checked;            /* combinations measured */
static long roundtripped;       /* of those, written and read back */
static int  failures;
static int  quiet = 1;          /* the sheet goes to /dev/null unless asked */

static void fail(const char *what, const char *why)
{
    fprintf(stderr, "  FAIL %-58s %s\n", what, why);
    if (++failures > 40) {
        fprintf(stderr, "\n  stopping after 40 failures\n");
        exit(1);
    }
}

/* ------------------------------------------------------------- characters */

static void reset(Character *c)
{
    memset(c, 0, sizeof *c);
    c->race_id = c->subrace_id = c->background_id = c->ancestry_id = -1;
}

/* A character with scores high enough to meet any multiclass prerequisite,
   so that a combination is never skipped for a reason the sweep invented. */
static void base(Character *c, const char *name)
{
    int a;

    reset(c);
    snprintf(c->name, sizeof c->name, "%s", name);
    for (a = 0; a < ABL_COUNT; a++) c->base_score[a] = 15;
}

static void add_class_at(Character *c, int class_id, int level, int subclass)
{
    int k, h;

    if (c->class_count >= MAX_CLASSES) return;
    k = c->class_count++;
    c->classes[k].class_id = class_id;
    c->classes[k].level = level;
    c->classes[k].subclass_id = subclass;
    c->classes[k].subclass_option = -1;
    for (h = 0; h < level && c->hp_roll_count < MAX_LEVEL; h++) {
        c->hp_rolls[c->hp_roll_count++] = 5;
    }
}

/* ------------------------------------------------------------- invariants */

/* True when the character has the named feat, which two of the derived
   numbers below add to. */
static int has_feat_named(const Character *c, const char *name)
{
    int i;
    for (i = 0; i < c->feat_count; i++) {
        if (!strcmp(FEATS[c->feats[i]].name, name)) return 1;
    }
    return 0;
}

/* The proficiency bonus the PHB gives a character of this many levels. */
static int expected_prof(int level)
{
    return 2 + (level - 1) / 4;
}

static void measure(const Character *c, const char *what)
{
    Attack atk[MAX_ATTACKS];
    int slots[10];
    int i, n, hp, ac, eff, pact_n, pact_l;

    checked++;

    hp = hit_points_max(c);
    if (hp < 1) fail(what, "hit points below one");

    ac = armour_class(c);
    if (ac < 1 || ac > 40) fail(what, "an armour class outside 1-40");

    if (proficiency_bonus(c) != expected_prof(total_level(c))) {
        fail(what, "a proficiency bonus that does not match the level");
    }
    if (total_level(c) < 1 || total_level(c) > MAX_LEVEL) {
        fail(what, "a total level outside 1-20");
    }

    if (speed_of(c) < 0 || speed_of(c) > 200) fail(what, "an odd speed");

    /* A race's natural armour is a floor, not a replacement: whatever else
       is worn, the Armor Class cannot come out below the shell. */
    if (c->race_id >= 0 && RACES[c->race_id].natural_ac) {
        int floor_ac = RACES[c->race_id].natural_ac;
        if (RACES[c->race_id].natural_ac_dex) {
            floor_ac += ability_mod(c, ABL_DEX);
        }
        if (ac < floor_ac) fail(what, "an armour class below the race's own hide");
    }

    /* Three of the derived numbers are a formula the PHB states outright,
       so they are checked against the formula rather than against a range.
       Whatever a race, a class, a feat or a magic item does to the score
       underneath, the number on the sheet has to follow it. */
    {
        /* Strength x 15, doubled for a race that counts as one size larger
           -- Powerful Build, Little Giant, Equine Build. */
        int want = ability_score(c, ABL_STR) * 15;
        if (c->race_id >= 0 && RACES[c->race_id].powerful_build) want *= 2;
        if (carrying_capacity(c) != want) {
            fail(what, "a carrying capacity that is not Strength x 15, "
                       "doubled for Powerful Build");
        }
    }
    {
        int want = 10 + skill_bonus(c, SKL_PERCEPTION)
                 + (has_feat_named(c, "Observant") ? 5 : 0);
        if (passive_perception(c) != want) {
            fail(what, "a passive Perception that is not 10 + Perception");
        }
    }
    {
        int want = ability_mod(c, ABL_DEX) + (has_feat_named(c, "Alert") ? 5 : 0);
        if (initiative_bonus(c) != want) {
            fail(what, "an initiative that is not the Dexterity modifier");
        }
    }

    for (i = 0; i < SKL_COUNT; i++) {
        int b = skill_bonus(c, (Skill)i);
        if (b < -10 || b > 30) fail(what, "an impossible skill bonus");
    }
    for (i = 0; i < ABL_COUNT; i++) {
        int b = save_bonus(c, (Ability)i);
        if (b < -10 || b > 30) fail(what, "an impossible saving throw");
        if (ability_score(c, (Ability)i) < 1
            || ability_score(c, (Ability)i) > 30) {
            fail(what, "an ability score outside 1-30");
        }
    }

    eff = spell_slots_for(c, slots);
    if (eff < 0 || eff > MAX_LEVEL) fail(what, "an impossible caster level");
    for (i = 1; i <= 9; i++) {
        if (slots[i] < 0 || slots[i] > 4) fail(what, "an impossible slot count");
    }
    if (pact_slots_for(c, &pact_n, &pact_l)) {
        if (pact_n < 1 || pact_n > 4 || pact_l < 1 || pact_l > 5) {
            fail(what, "impossible pact magic");
        }
    }

    /* Unarmoured, with no shield, a monk's Armor Class is 10 + Dexterity +
       Wisdom and a barbarian's 10 + Dexterity + Constitution -- and the
       sheet takes the best of them, so the number can only ever be at least
       what the character's own class allows. */
    if (c->item_count == 0) {
        int dex = ability_mod(c, ABL_DEX);
        int floor_ac = 10 + dex;
        if (class_level_of(c, CLS_MONK) >= 1) {
            int monk = 10 + dex + ability_mod(c, ABL_WIS);
            if (monk > floor_ac) floor_ac = monk;
        }
        if (class_level_of(c, CLS_BARBARIAN) >= 1) {
            int rage = 10 + dex + ability_mod(c, ABL_CON);
            if (rage > floor_ac) floor_ac = rage;
        }
        if (ac < floor_ac) {
            fail(what, "an armour class below what its class grants unarmoured");
        }
    }

    /* A character with levels in more than one casting class uses the
       multiclass table, which never gives more of a slot level than the
       full caster's table gives at the same effective level. */
    if (eff > 0) {
        for (i = 1; i <= 9; i++) {
            if (slots[i] > FULL_SLOTS[eff][i]) {
                fail(what, "more slots than the caster level allows");
            }
        }
    }

    /* Unarmored Movement and Fast Movement are conditional in the PHB, and
       both conditions used to go unread: a monk in plate with a shield kept
       the whole of its bonus. Worked out here from the inventory rather than
       asked of the engine, so that the engine has to agree with the book
       rather than with itself. */
    {
        int worn_armour = 0, worn_shield = 0, worn_heavy = 0, i2;
        int too_heavy = 0;
        int want = 30, monk, barb;

        for (i2 = 0; i2 < c->item_count; i2++) {
            const InventoryEntry *e = &c->inventory[i2];
            if (!e->equipped) continue;
            if (e->is_magic) {
                const MagicRule *r =
                    magic_rule_for(MAGIC_ITEMS[e->item_id].name);
                if (!magic_rule_is_worn(r)) continue;
                if (MAGIC_ITEMS[e->item_id].attunement && !e->attuned) continue;
                if (r->shield) worn_shield = 1;
                else {
                    worn_armour = 1;
                    if (r->armor_base && r->armor_dex == 0) worn_heavy = 1;
                    if (r->armor_str
                        && ability_score(c, ABL_STR) < r->armor_str) {
                        too_heavy = 1;
                    }
                }
            } else {
                ItemCategory cat = ITEMS[e->item_id].category;
                if (cat == ITEM_SHIELD) worn_shield = 1;
                else if (cat <= ITEM_HEAVY_ARMOR) {
                    worn_armour = 1;
                    if (cat == ITEM_HEAVY_ARMOR) worn_heavy = 1;
                    if (ITEMS[e->item_id].str_req
                        && ability_score(c, ABL_STR)
                           < ITEMS[e->item_id].str_req) {
                        too_heavy = 1;
                    }
                }
            }
        }

        if (c->race_id >= 0 && c->race_id < RACE_COUNT) {
            want = RACES[c->race_id].speed;
            if (c->subrace_id >= 0 && c->subrace_id < SUBRACE_COUNT
                && SUBRACES[c->subrace_id].speed_override > 0) {
                want = SUBRACES[c->subrace_id].speed_override;
            }
        }
        monk = (worn_armour || worn_shield) ? 0 : class_level_of(c, CLS_MONK);
        if (monk >= 18)      want += 30;
        else if (monk >= 14) want += 25;
        else if (monk >= 10) want += 20;
        else if (monk >= 6)  want += 15;
        else if (monk >= 2)  want += 10;
        barb = class_level_of(c, CLS_BARBARIAN);
        if (barb >= 5 && !worn_heavy) want += 10;
        if (has_feat_named(c, "Mobile")) want += 10;
        /* Armour the wearer is not strong enough for costs ten feet. */
        if (too_heavy) want -= 10;
        if (want < 0) want = 0;
        /* An item that sets a floor under the speed -- boots of striding and
           springing -- may raise it, so where one is worn the check is only
           that the number is no lower. Where none is, it is exact. */
        {
            int floored = 0;
            for (i2 = 0; i2 < c->item_count; i2++) {
                const InventoryEntry *e = &c->inventory[i2];
                const MagicRule *r;
                if (!e->is_magic) continue;
                r = magic_rule_for(MAGIC_ITEMS[e->item_id].name);
                if (r && r->sets_speed) floored = 1;
            }
            if (floored ? (speed_of(c) < want) : (speed_of(c) != want)) {
                fail(what, "a speed the race and class do not add up to");
            }
        }
    }

    n = attacks_of(c, atk, MAX_ATTACKS);
    if (n < 0 || n > MAX_ATTACKS) fail(what, "an attacks block out of bounds");
    for (i = 0; i < n; i++) {
        if (!atk[i].name[0]) fail(what, "an attack with no name");
        if (atk[i].bonus < -10 || atk[i].bonus > 30) {
            fail(what, "an impossible attack bonus");
        }
    }

    for (i = 0; i < c->class_count; i++) {
        int id = c->classes[i].class_id;
        if (spell_save_dc(c, id) < 8 || spell_save_dc(c, id) > 30) {
            fail(what, "an impossible spell save DC");
        }
    }

    if (!quiet) print_sheet(c);
}

/* Writes the character, reads it back, writes it again, and compares. */
static char sheet_a[600000], sheet_b[600000];

static size_t slurp(const char *path, char *out, size_t max)
{
    FILE *f = fopen(path, "r");
    size_t n;

    if (!f) return 0;
    n = fread(out, 1, max - 1, f);
    out[n] = '\0';
    fclose(f);
    return n;
}

static void round_trip(const Character *c, const char *what)
{
    char path[MAX_NAME + 8];
    Character back;
    size_t na, nb;

    roundtripped++;
    if (save_character(c, path, sizeof path) != 0) {
        fail(what, "could not be written");
        return;
    }
    na = slurp(path, sheet_a, sizeof sheet_a);
    if (load_character(path, &back) != 0) {
        fail(what, "could not be read back");
        remove(path);
        return;
    }
    if (save_character(&back, path, sizeof path) != 0) {
        fail(what, "could not be rewritten");
        remove(path);
        return;
    }
    nb = slurp(path, sheet_b, sizeof sheet_b);
    remove(path);
    if (na == 0 || na != nb || memcmp(sheet_a, sheet_b, na) != 0) {
        fail(what, "does not survive the file");
    }
}

/* ---------------------------------------------------------------- sweeps */

/* Every race and subrace, in every class, at every level. */
static void sweep_race_class_level(void)
{
    Character c;
    char what[160];
    int r, k, cls, lvl;

    printf("every race and subrace in every class at every level\n");
    for (r = 0; r < RACE_COUNT; r++) {
        int subs = RACES[r].subrace_count;
        for (k = -1; k < subs; k++) {
            int sub = (k < 0) ? -1 : RACES[r].first_subrace + k;
            for (cls = 0; cls < CLASS_COUNT; cls++) {
                for (lvl = 1; lvl <= MAX_LEVEL; lvl++) {
                    base(&c, "Combo");
                    c.race_id = r;
                    c.subrace_id = sub;
                    if (RACES[r].has_ancestry) c.ancestry_id = 0;
                    add_class_at(&c, cls, lvl, -1);
                    snprintf(what, sizeof what, "%s%s%s %s %d",
                             RACES[r].name, sub >= 0 ? " / " : "",
                             sub >= 0 ? SUBRACES[sub].name : "",
                             CLASSES[cls].name, lvl);
                    measure(&c, what);
                    /* One level of each pairing goes through the file, so
                       every race meets every class there too. */
                    if (lvl == MAX_LEVEL) round_trip(&c, what);
                }
            }
        }
    }
}

/* Every subclass, at every level from the one it is chosen at to 20th, and
   with every option it offers. */
static void sweep_subclasses(void)
{
    Character c;
    char what[160];
    int s, lvl;

    printf("every subclass at every level it can be held at\n");
    for (s = 0; s < SUBCLASS_COUNT; s++) {
        int cls = SUBCLASSES[s].class_id;
        int from = CLASSES[cls].subclass_level;
        const char *opts = SUBCLASSES[s].options;

        for (lvl = (from > 0 ? from : 1); lvl <= MAX_LEVEL; lvl++) {
            base(&c, "Combo");
            c.race_id = 0;
            add_class_at(&c, cls, lvl, s);
            snprintf(what, sizeof what, "%s %s %d",
                     CLASSES[cls].name, SUBCLASSES[s].name, lvl);
            measure(&c, what);
        }
        base(&c, "Combo");
        c.race_id = 0;
        add_class_at(&c, cls, MAX_LEVEL, s);
        snprintf(what, sizeof what, "%s %s 20",
                 CLASSES[cls].name, SUBCLASSES[s].name);
        round_trip(&c, what);

        /* Each option the subclass offers -- totem animal, land terrain,
           rune, genie kind, draconic bloodline -- indexes its own list. */
        if (opts && *opts) {
            char buf[512];
            const char *parts[24];
            int n = split_pipe(opts, buf, sizeof buf, parts, 24), o;
            for (o = 0; o < n; o++) {
                base(&c, "Combo");
                c.race_id = 0;
                add_class_at(&c, cls, MAX_LEVEL, s);
                c.classes[0].subclass_option = o;
                snprintf(what, sizeof what, "%s %s (%s)",
                         CLASSES[cls].name, SUBCLASSES[s].name, parts[o]);
                measure(&c, what);
                round_trip(&c, what);
            }
        }
    }
}

/* Every race and subrace, in every subclass, at every level -- the three-way
   cross the two sweeps above only meet the edges of. A racial trait and a
   subclass feature that disagree (a dwarf's speed against a barbarian's
   Fast Movement, a drow's darkvision against a Gloom Stalker's) meet here
   and nowhere else. */
static void sweep_race_subclass_level(void)
{
    Character c;
    char what[160];
    int r, k, s, lvl;

    printf("every race in every subclass at every level\n");
    for (r = 0; r < RACE_COUNT; r++) {
        int subs = RACES[r].subrace_count;
        for (k = -1; k < subs; k++) {
            int sub = (k < 0) ? -1 : RACES[r].first_subrace + k;
            for (s = 0; s < SUBCLASS_COUNT; s++) {
                int cls = SUBCLASSES[s].class_id;
                int from = CLASSES[cls].subclass_level;
                for (lvl = (from > 0 ? from : 1); lvl <= MAX_LEVEL; lvl++) {
                    base(&c, "Combo");
                    c.race_id = r;
                    c.subrace_id = sub;
                    if (RACES[r].has_ancestry) c.ancestry_id = 0;
                    add_class_at(&c, cls, lvl, s);
                    snprintf(what, sizeof what, "%s%s%s %s %d",
                             RACES[r].name, sub >= 0 ? " / " : "",
                             sub >= 0 ? SUBRACES[sub].name : "",
                             SUBCLASSES[s].name, lvl);
                    measure(&c, what);
                }
            }
        }
    }
}

/* Every race in every background, every draconic ancestry in every class,
   and every feat on every race -- which is where the racial feats live. */
static void sweep_race_pairings(void)
{
    Character c;
    char what[160];
    int r, k, bg, ft, cls;

    printf("every race in every background\n");
    for (r = 0; r < RACE_COUNT; r++) {
        int subs = RACES[r].subrace_count;
        for (k = -1; k < subs; k++) {
            int sub = (k < 0) ? -1 : RACES[r].first_subrace + k;
            for (bg = 0; bg < BACKGROUND_COUNT; bg++) {
                base(&c, "Combo");
                c.race_id = r;
                c.subrace_id = sub;
                if (RACES[r].has_ancestry) c.ancestry_id = 0;
                c.background_id = bg;
                add_class_at(&c, CLS_FIGHTER, 8, -1);
                snprintf(what, sizeof what, "%s the %s",
                         RACES[r].name, BACKGROUNDS[bg].name);
                measure(&c, what);
                if (bg == 0) round_trip(&c, what);
            }
        }
    }

    printf("every feat on every race\n");
    for (ft = 0; ft < FEAT_COUNT; ft++) {
        for (r = 0; r < RACE_COUNT; r++) {
            int subs = RACES[r].subrace_count;
            for (k = -1; k < subs; k++) {
                int sub = (k < 0) ? -1 : RACES[r].first_subrace + k;
                int a;
                base(&c, "Combo");
                c.race_id = r;
                c.subrace_id = sub;
                if (RACES[r].has_ancestry) c.ancestry_id = 0;
                add_class_at(&c, CLS_FIGHTER, MAX_LEVEL, -1);
                c.feats[c.feat_count++] = ft;
                for (a = 0; a < ABL_COUNT; a++) {
                    c.asi_bonus[a] += FEATS[ft].asi[a];
                }
                snprintf(what, sizeof what, "%s on a %s",
                         FEATS[ft].name, RACES[r].name);
                measure(&c, what);
            }
        }
    }

    printf("every draconic ancestry in every class\n");
    {
        int dragonborn = -1;
        for (r = 0; r < RACE_COUNT; r++) {
            if (RACES[r].has_ancestry) dragonborn = r;
        }
        for (r = 0; dragonborn >= 0 && r < ANCESTRY_COUNT; r++) {
            for (cls = 0; cls < CLASS_COUNT; cls++) {
                base(&c, "Combo");
                c.race_id = dragonborn;
                c.ancestry_id = r;
                add_class_at(&c, cls, 12, -1);
                snprintf(what, sizeof what, "%s dragonborn %s",
                         ANCESTRIES[r].dragon, CLASSES[cls].name);
                measure(&c, what);
                round_trip(&c, what);
            }
        }
    }
}

/* A character carrying as much magic as the rules allow at once: the
   attunement limit, the armour and the shield, and every rule that adds to
   a number all applying together. */
static void sweep_loaded_character(void)
{
    Character c;
    char what[160];
    int i, first;

    printf("characters carrying magic items by the armful\n");
    for (first = 0; first < MAGIC_ITEM_COUNT; first += 8) {
        int attuned = 0;
        base(&c, "Combo");
        c.race_id = 0;
        add_class_at(&c, CLS_FIGHTER, MAX_LEVEL, -1);
        for (i = first; i < MAGIC_ITEM_COUNT && c.item_count < MAX_ITEMS - 1;
             i++) {
            int want = MAGIC_ITEMS[i].attunement && attuned < MAX_ATTUNED;
            add_magic_item(&c, i, 1, want, 0);
            if (want) attuned++;
            if (c.item_count) c.inventory[c.item_count - 1].equipped = 1;
        }
        snprintf(what, sizeof what, "%d magic items at once from %s",
                 c.item_count, MAGIC_ITEMS[first].name);
        measure(&c, what);
        round_trip(&c, what);
    }

    printf("characters carrying the whole catalogue\n");
    for (first = 0; first < ITEM_COUNT; first += 8) {
        base(&c, "Combo");
        c.race_id = 0;
        add_class_at(&c, CLS_FIGHTER, MAX_LEVEL, -1);
        for (i = first; i < ITEM_COUNT && c.item_count < MAX_ITEMS - 1; i++) {
            add_item(&c, i, 1, 1);
        }
        snprintf(what, sizeof what, "%d items at once from %s",
                 c.item_count, ITEMS[first].name);
        measure(&c, what);
        round_trip(&c, what);
    }
}

/* Every pair of classes, and every triple, at the splits that matter. */
static void sweep_multiclass(void)
{
    static const int SPLITS[][2] = { {1, 19}, {5, 15}, {10, 10}, {19, 1} };
    const int nsplits = (int)(sizeof SPLITS / sizeof SPLITS[0]);
    Character c;
    char what[160];
    int a, b, s;

    printf("every pair of classes at four level splits\n");
    for (a = 0; a < CLASS_COUNT; a++) {
        for (b = 0; b < CLASS_COUNT; b++) {
            if (a == b) continue;
            for (s = 0; s < nsplits; s++) {
                base(&c, "Combo");
                c.race_id = 0;
                add_class_at(&c, a, SPLITS[s][0], -1);
                add_class_at(&c, b, SPLITS[s][1], -1);
                snprintf(what, sizeof what, "%s %d / %s %d",
                         CLASSES[a].name, SPLITS[s][0],
                         CLASSES[b].name, SPLITS[s][1]);
                measure(&c, what);
                if (s == 2) round_trip(&c, what);
            }
        }
    }

    printf("every three classes together\n");
    {
        int x, y, z;
        for (x = 0; x < CLASS_COUNT; x++) {
            for (y = x + 1; y < CLASS_COUNT; y++) {
                for (z = y + 1; z < CLASS_COUNT; z++) {
                    base(&c, "Combo");
                    c.race_id = 0;
                    add_class_at(&c, x, 7, -1);
                    add_class_at(&c, y, 7, -1);
                    add_class_at(&c, z, 6, -1);
                    snprintf(what, sizeof what, "%s 7 / %s 7 / %s 6",
                             CLASSES[x].name, CLASSES[y].name,
                             CLASSES[z].name);
                    measure(&c, what);
                }
            }
        }
    }
}

/* Every background in every class, and every feat on every class that can
   hold it. */
static void sweep_backgrounds_and_feats(void)
{
    Character c;
    char what[160];
    int bg, cls, ft;

    printf("every background in every class\n");
    for (bg = 0; bg < BACKGROUND_COUNT; bg++) {
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            base(&c, "Combo");
            c.race_id = 0;
            c.background_id = bg;
            add_class_at(&c, cls, 10, -1);
            snprintf(what, sizeof what, "%s %s",
                     BACKGROUNDS[bg].name, CLASSES[cls].name);
            measure(&c, what);
            if (cls == 0) round_trip(&c, what);
        }
    }

    printf("every feat on every class\n");
    for (ft = 0; ft < FEAT_COUNT; ft++) {
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            int a;
            base(&c, "Combo");
            c.race_id = 0;
            add_class_at(&c, cls, MAX_LEVEL, -1);
            c.feats[c.feat_count++] = ft;
            /* A half-feat's increases land where the wizard would put them. */
            for (a = 0; a < ABL_COUNT; a++) c.asi_bonus[a] += FEATS[ft].asi[a];
            snprintf(what, sizeof what, "%s on a %s",
                     FEATS[ft].name, CLASSES[cls].name);
            measure(&c, what);
            if (cls == 0) round_trip(&c, what);
        }
    }
}

/* Every spell on every class that can learn it, and every magic item worn
   by a character who can wear it. */
static void sweep_spells_and_items(void)
{
    Character c;
    char what[160];
    int i, cls;

    printf("every spell under every class that has it\n");
    for (i = 0; i < SPELL_COUNT; i++) {
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            base(&c, "Combo");
            c.race_id = 0;
            add_class_at(&c, cls, MAX_LEVEL, -1);
            c.spells[0].spell_id = i;
            c.spells[0].class_id = cls;
            c.spells[0].prepared = 1;
            c.spell_count = 1;
            snprintf(what, sizeof what, "%s as a %s",
                     SPELLS[i].name, CLASSES[cls].name);
            measure(&c, what);
        }
    }

    printf("every magic item, worn and attuned, on every class\n");
    for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            base(&c, "Combo");
            c.race_id = 0;
            add_class_at(&c, cls, MAX_LEVEL, -1);
            add_magic_item(&c, i, 1, MAGIC_ITEMS[i].attunement ? 1 : 0, 1);
            if (c.item_count) c.inventory[0].equipped = 1;
            snprintf(what, sizeof what, "%s on a %s",
                     MAGIC_ITEMS[i].name, CLASSES[cls].name);
            measure(&c, what);
            if (cls == 0) round_trip(&c, what);
        }
    }

    printf("every item carried, on a fighter and a wizard\n");
    for (i = 0; i < ITEM_COUNT; i++) {
        for (cls = 0; cls < 2; cls++) {
            base(&c, "Combo");
            c.race_id = 0;
            add_class_at(&c, cls ? CLS_WIZARD : CLS_FIGHTER, MAX_LEVEL, -1);
            add_item(&c, i, 1, 1);
            snprintf(what, sizeof what, "%s carried", ITEMS[i].name);
            measure(&c, what);
        }
    }
}

/* Every beast, as every kind of sidekick, at every level. A sidekick's
   spells known and cantrips are read out of tables indexed by its level, and
   its stat block out of the beast tables, so this is two indexes crossed. */
static void sweep_sidekicks(void)
{
    Character c;
    char what[160];
    int b, cls, lvl;

    printf("every beast as every kind of sidekick at every level\n");
    for (b = 0; b < BEAST_COUNT_ACTUAL; b++) {
        for (cls = 0; cls < SK_CLASS_COUNT; cls++) {
            for (lvl = 1; lvl <= MAX_LEVEL; lvl++) {
                Sidekick *sk;
                base(&c, "Combo");
                c.race_id = 0;
                add_class_at(&c, CLS_FIGHTER, MAX_LEVEL, -1);
                sk = &c.sidekicks[c.sidekick_count++];
                memset(sk, 0, sizeof *sk);
                snprintf(sk->name, sizeof sk->name, "Companion");
                snprintf(sk->creature, sizeof sk->creature, "%s",
                         BEASTS[b].name);
                snprintf(sk->speed, sizeof sk->speed, "%s", BEASTS[b].speed);
                sk->cls = cls;
                sk->level = lvl;
                sk->role = (cls == SK_SPELLCASTER) ? lvl % SK_ROLE_COUNT : -1;
                sk->hp = BEASTS[b].hp;
                sk->ac = BEASTS[b].ac;
                snprintf(what, sizeof what, "%s as a %s %d",
                         BEASTS[b].name, SIDEKICK_CLASS_NAME[cls], lvl);
                measure(&c, what);
                if (lvl == MAX_LEVEL) round_trip(&c, what);
            }
        }
    }

    printf("as many sidekicks at once as a character may have\n");
    {
        int i;
        base(&c, "Combo");
        c.race_id = 0;
        add_class_at(&c, CLS_FIGHTER, MAX_LEVEL, -1);
        for (i = 0; i < MAX_SIDEKICKS && i < BEAST_COUNT_ACTUAL; i++) {
            Sidekick *sk = &c.sidekicks[c.sidekick_count++];
            memset(sk, 0, sizeof *sk);
            snprintf(sk->name, sizeof sk->name, "Companion %d", i);
            snprintf(sk->creature, sizeof sk->creature, "%s", BEASTS[i].name);
            snprintf(sk->speed, sizeof sk->speed, "%s", BEASTS[i].speed);
            sk->cls = i % SK_CLASS_COUNT;
            sk->level = MAX_LEVEL;
            sk->role = i % SK_ROLE_COUNT;
            sk->hp = BEASTS[i].hp;
            sk->ac = BEASTS[i].ac;
        }
        measure(&c, "four sidekicks at once");
        round_trip(&c, "four sidekicks at once");
    }
}

/* Every array on the character filled to its limit, all at once. The bounds
   are checked one at a time all over the program; this is the character that
   sits on every one of them together. */
static void sweep_at_the_limits(void)
{
    Character c;
    int i;

    printf("a character sitting on every limit at once\n");
    base(&c, "Limits");
    c.race_id = 0;
    c.background_id = 0;
    /* MAX_CLASSES classes, adding to twenty levels. */
    for (i = 0; i < MAX_CLASSES && i < CLASS_COUNT; i++) {
        add_class_at(&c, i, (i == 0) ? MAX_LEVEL - MAX_CLASSES + 1 : 1, -1);
    }
    for (i = 0; i < MAX_SPELLS && i < SPELL_COUNT; i++) {
        c.spells[c.spell_count].spell_id = i;
        c.spells[c.spell_count].class_id = 0;
        c.spells[c.spell_count].prepared = 1;
        c.spell_count++;
    }
    for (i = 0; i < ITEM_COUNT && c.item_count < MAX_ITEMS - 1; i++) {
        add_item(&c, i, 99, 0);
    }
    for (i = 0; i < MAX_FEATS && i < FEAT_COUNT; i++) {
        c.feats[c.feat_count++] = i;
    }
    for (i = 0; i < MAX_LANGS + 4; i++) {
        char lang[MAX_NAME];
        snprintf(lang, sizeof lang, "Language %d", i);
        add_language(&c, lang);
    }
    for (i = 0; i < MAX_PROFS + 4; i++) {
        char tool[MAX_NAME];
        snprintf(tool, sizeof tool, "Tool %d", i);
        add_tool(&c, tool);
        add_prof(&c, tool);
    }
    for (i = 0; i < MAX_CHOICES + 4; i++) {
        char v[MAX_TEXT];
        snprintf(v, sizeof v, "Choice %d", i);
        add_choice(&c, "Eldritch Invocation", v);
    }
    for (i = 0; i < MAX_NOTES + 2 && c.note_count < MAX_NOTES; i++) {
        size_t k;
        snprintf(c.notes[c.note_count].title,
                 sizeof c.notes[0].title, "Note %d", i);
        for (k = 0; k + 8 < sizeof c.notes[0].body - 1; k += 8) {
            memcpy(c.notes[c.note_count].body + k, "abcdefg\n", 8);
        }
        c.notes[c.note_count].body[k] = '\0';
        c.note_count++;
    }
    for (i = 0; i < ABL_COUNT; i++) {
        c.base_score[i] = 20;
        c.asi_bonus[i] = 0;
    }
    for (i = 0; i < SKL_COUNT; i++) {
        c.skill_prof[i] = c.skill_expertise[i] = 1;
    }
    for (i = 0; i < ABL_COUNT; i++) c.save_prof[i] = 1;
    c.copper = c.silver = c.electrum = c.gold = c.platinum = 999999;
    measure(&c, "every limit at once");
    round_trip(&c, "every limit at once");
}

/* An item in the pack is not an item in use. Carrying a magic item without
   wearing or attuning it must leave every number where it was -- which a
   "+1, +2, or +3" suit of armour and, worse, a "+1, +2, or +3" weapon did
   not: both added their plus to Armor Class from inside the pack, and the
   weapon added it even when wielded, where a plus belongs to the attack. */
static void sweep_carried_but_unused(void)
{
    Character bare, c;
    char what[160];
    int i, p, base_ac;

    printf("every magic item carried but not worn, at every plus\n");
    base(&bare, "Combo");
    bare.race_id = 0;
    add_class_at(&bare, CLS_FIGHTER, MAX_LEVEL, -1);
    base_ac = armour_class(&bare);

    for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
        for (p = 0; p <= 3; p++) {
            c = bare;
            add_magic_item(&c, i, 1, 0, p);
            if (!c.item_count) continue;
            c.inventory[c.item_count - 1].equipped = 0;
            c.inventory[c.item_count - 1].attuned = 0;
            snprintf(what, sizeof what, "%s +%d in the pack",
                     MAGIC_ITEMS[i].name, p);
            if (armour_class(&c) != base_ac) {
                fail(what, "changes Armor Class from inside the pack");
            }
            measure(&c, what);
        }
    }

    printf("every magic weapon wielded, at every plus\n");
    for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
        const MagicRule *r = magic_rule_for(MAGIC_ITEMS[i].name);
        if (!r || !r->weapon) continue;
        for (p = 0; p <= 3; p++) {
            c = bare;
            add_magic_item(&c, i, 1, MAGIC_ITEMS[i].attunement ? 1 : 0, p);
            if (!c.item_count) continue;
            c.inventory[c.item_count - 1].equipped = 1;
            snprintf(what, sizeof what, "%s +%d in hand",
                     MAGIC_ITEMS[i].name, p);
            if (armour_class(&c) != base_ac) {
                fail(what, "a weapon that adds to Armor Class");
            }
            measure(&c, what);
        }
    }
}

/* Every combination of the seven books and the six optional rules. The
   banks are rebuilt as the books change, and every menu the wizard shows is
   filtered by them, so a combination that leaves a bank empty is exactly
   where an off-by-one goes unnoticed. */
/* Every suit of armour, at every Dexterity, on the classes that answer to it.
 *
 * The Armor Class is the one number on the sheet that four rules argue
 * over: what the armour sets, how much Dexterity it lets through, whether
 * a shield is up, and the unarmoured formulas a monk and a barbarian bring
 * that only apply while nothing is worn. Each of those is tested on its own
 * elsewhere; what they do to each other is tested here, by working the
 * number out a second way from the equipment table and comparing.
 *
 * The Dexterity range is what makes it worth crossing: a medium suit's cap
 * only shows up above +2, and heavy armour's refusal of Dexterity only
 * above +0.
 */
static int dex_through(int cap, int dex)
{
    if (cap < 0) return dex;                    /* light armour: all of it */
    if (cap == 0) return 0;                     /* heavy armour: none */
    return dex < cap ? dex : cap;               /* medium armour: up to cap */
}

static void sweep_armour_and_dex(void)
{
    static const int SCORES[] = { 1, 8, 10, 14, 18, 20, 30 };
    /* Both sides of every armour's Strength requirement: 10 is under the
       lightest of them, 15 meets the heaviest. */
    static const int STRENGTHS[] = { 10, 15 };
    static const int CLASSES_TRIED[] = {
        CLS_FIGHTER, CLS_MONK, CLS_BARBARIAN, CLS_WIZARD
    };
    Character c;
    char what[160];
    int a, s, k, st, shield_on, shield_id = find_item("Shield");

    if (shield_id < 0) {
        fail("armour", "the equipment table has no shield");
        return;
    }

    printf("every armour x every Dexterity x shield or none\n");
    for (a = -1; a < ITEM_COUNT; a++) {
        if (a >= 0 && ITEMS[a].category > ITEM_HEAVY_ARMOR) continue;
        if (a >= 0 && ITEMS[a].category == ITEM_SHIELD) continue;

        for (s = 0; s < (int)(sizeof SCORES / sizeof *SCORES); s++) {
            for (shield_on = 0; shield_on < 2; shield_on++) {
              for (st = 0; st < (int)(sizeof STRENGTHS / sizeof *STRENGTHS);
                   st++) {
                for (k = 0; k < (int)(sizeof CLASSES_TRIED
                                      / sizeof *CLASSES_TRIED); k++) {
                    int dex, want, got;

                    base(&c, "Combo");
                    c.race_id = 0;
                    c.base_score[ABL_DEX] = SCORES[s];
                    c.base_score[ABL_STR] = STRENGTHS[st];
                    add_class_at(&c, CLASSES_TRIED[k], 5, -1);
                    if (a >= 0) {
                        add_item(&c, a, 1, 1);
                        c.inventory[c.item_count - 1].equipped = 1;
                    }
                    if (shield_on) {
                        add_item(&c, shield_id, 1, 1);
                        c.inventory[c.item_count - 1].equipped = 1;
                    }

                    dex = ability_mod(&c, ABL_DEX);
                    if (a >= 0) {
                        want = ITEMS[a].base_ac
                             + dex_through(ITEMS[a].dex_cap, dex);
                    } else {
                        want = 10 + dex;
                        /* Unarmoured, the class formulas apply -- and the
                           monk's is refused by a shield, the barbarian's
                           is not. */
                        if (CLASSES_TRIED[k] == CLS_BARBARIAN) {
                            int alt = 10 + dex + ability_mod(&c, ABL_CON);
                            if (alt > want) want = alt;
                        }
                        if (CLASSES_TRIED[k] == CLS_MONK && !shield_on) {
                            int alt = 10 + dex + ability_mod(&c, ABL_WIS);
                            if (alt > want) want = alt;
                        }
                    }
                    if (shield_on) want += ITEMS[shield_id].base_ac;

                    snprintf(what, sizeof what,
                             "a %s with %s%s at DEX %d, STR %d",
                             CLASSES[CLASSES_TRIED[k]].name,
                             a >= 0 ? ITEMS[a].name : "no armour",
                             shield_on ? " and a shield" : "", SCORES[s],
                             STRENGTHS[st]);
                    got = armour_class(&c);
                    if (got != want) {
                        fprintf(stderr, "  FAIL %-58s AC %d, worked out "
                                        "a second way as %d\n",
                                what, got, want);
                        fail(what, "an armour class the equipment table "
                                   "does not give");
                    }
                    measure(&c, what);
                }
              }
            }
        }
    }

    /* Magic armour carries a Strength requirement of its own -- dwarven
       plate is plate -- and it is read from the rule rather than from the
       equipment table, so it is walked separately. The Armor Class of a
       magic suit is checked by selftest.c against the DMG; what is added
       here is the speed, which measure() works out from the inventory. */
    printf("every magic armour x two Strengths\n");
    for (a = 0; a < MAGIC_ITEM_COUNT; a++) {
        const MagicRule *r = magic_rule_for(MAGIC_ITEMS[a].name);
        if (!magic_rule_is_worn(r) || r->shield) continue;

        for (st = 0; st < (int)(sizeof STRENGTHS / sizeof *STRENGTHS); st++) {
            base(&c, "Combo");
            c.race_id = 0;
            c.base_score[ABL_STR] = STRENGTHS[st];
            add_class_at(&c, CLS_FIGHTER, 5, -1);
            add_magic_item(&c, a, 1, MAGIC_ITEMS[a].attunement ? 1 : 0, 1);
            if (c.item_count) c.inventory[0].equipped = 1;
            snprintf(what, sizeof what, "a Fighter in %s at STR %d",
                     MAGIC_ITEMS[a].name, STRENGTHS[st]);
            measure(&c, what);
        }
    }
}

/* Every weapon in the book, in the hands of every class.
 *
 * A class's weapon proficiencies are prose, and no two classes word them
 * alike: the fighter's line reads "Simple weapons, martial weapons", the
 * cleric's "All simple weapons", the druid's a list of ten names. The sheet
 * has to reach the same answer from all three spellings.
 *
 * Two things are checked. Weapon by weapon, whether the sheet calls it
 * proficient agrees with what the class's own line says, and the attack
 * bonus is the ability modifier plus the proficiency bonus exactly when it
 * does. Class by class, the number of weapons it ends up proficient with is
 * the number the PHB fixes: fourteen simple weapons, twenty-three martial
 * ones, or the names the class lists.
 *
 * The bug this was written for: the category phrases were matched against
 * the whole line, so "All simple weapons" matched nothing and a 20th-level
 * cleric's mace showed +5 rather than +11, under a proficiency list that
 * said "All simple weapons".
 */
static const struct { const char *name; int weapons; } WEAPONS_KNOWN[] = {
    { "Barbarian", 37 },        /* simple and martial: all of them */
    { "Bard",      18 },        /* simple, + hand crossbow, longsword,
                                   rapier, shortsword */
    { "Cleric",    14 },        /* "All simple weapons" */
    { "Druid",     10 },        /* club, dagger, dart, javelin, mace,
                                   quarterstaff, scimitar, sickle, sling,
                                   spear */
    { "Fighter",   37 }, { "Monk",     15 },   /* simple, + shortsword */
    { "Paladin",   37 }, { "Ranger",   37 },
    { "Rogue",     18 }, { "Sorcerer",  5 },   /* dagger, dart, sling,
                                                  quarterstaff, light
                                                  crossbow */
    { "Warlock",   14 },        /* "Simple weapons" */
    { "Wizard",     5 }, { "Artificer", 14 }
};

/* What the class's own line says about this weapon, read here rather than
   asked of the engine, so that the two have to agree. */
static int line_covers(const char *line, const ItemData *it)
{
    int simple = (it->category == ITEM_SIMPLE_MELEE
                  || it->category == ITEM_SIMPLE_RANGED);

    if (contains_ci(line, "all weapons")) return 1;
    if (contains_ci(line, simple ? "simple weapons" : "martial weapons")) {
        return 1;
    }
    /* Named, in the plural the books write them in: "longswords" covers the
       longsword, and does not cover the sword that is not one. */
    return contains_ci(line, it->name);
}

/* Every race against every suit of armour and every Dexterity.
 *
 * Four of the things a race's traits promise are numbers somebody has to
 * work out, and until recently nobody did: a tortle's shell, a goliath's
 * carrying capacity, a lizardfolk's bite, an elf's Perception. Each is
 * checked on its own in the selftest against the book. What is checked
 * here is that they survive being crossed with everything else -- that the
 * shell still wins over leather armour and still loses to plate, that the
 * bite is still the bite when a weapon is also carried, and that no
 * combination produces a number outside what the rules can make.
 */
static void sweep_race_traits(void)
{
    static const int DEXES[] = { 1, 8, 10, 14, 18, 20, 30 };
    int r, a, d;

    for (r = 0; r < RACE_COUNT; r++) {
        for (d = 0; d < (int)(sizeof DEXES / sizeof DEXES[0]); d++) {
            for (a = -1; a < ITEM_COUNT; a++) {
                Character c;
                char what[160];
                int worn = 0, want, got;

                if (a >= 0) {
                    ItemCategory cat = ITEMS[a].category;
                    if (cat != ITEM_LIGHT_ARMOR && cat != ITEM_MEDIUM_ARMOR
                        && cat != ITEM_HEAVY_ARMOR) {
                        continue;
                    }
                    worn = 1;
                }

                base(&c, "Traits");
                c.race_id = r;
                add_class_at(&c, CLS_FIGHTER, 1, -1);
                c.base_score[ABL_DEX] = DEXES[d];
                c.base_score[ABL_STR] = 15;
                c.item_count = 0;
                if (worn) add_item(&c, a, 1, 1);

                snprintf(what, sizeof what, "%s in %s at Dex %d",
                         RACES[r].name, worn ? ITEMS[a].name : "nothing",
                         DEXES[d]);

                /* The Armor Class, worked out a second way. */
                if (worn) {
                    want = ITEMS[a].base_ac
                         + dex_through(ITEMS[a].dex_cap,
                                       ability_mod(&c, ABL_DEX));
                } else {
                    want = 10 + ability_mod(&c, ABL_DEX);
                }
                if (RACES[r].natural_ac) {
                    int nat = RACES[r].natural_ac;
                    if (RACES[r].natural_ac_dex) {
                        nat += ability_mod(&c, ABL_DEX);
                    }
                    if (nat > want) want = nat;
                }
                got = armour_class(&c);
                if (got != want) {
                    char why[128];
                    snprintf(why, sizeof why,
                             "armour class %d where the tables give %d",
                             got, want);
                    fail(what, why);
                }

                /* The carrying capacity, likewise. */
                want = ability_score(&c, ABL_STR) * 15;
                if (RACES[r].powerful_build) want *= 2;
                if (carrying_capacity(&c) != want) {
                    fail(what, "the wrong carrying capacity");
                }

                /* And the unarmed strike says what the race's own weapon
                   says, whatever else is being carried. */
                {
                    Attack at[24];
                    int n = attacks_of(&c, at, 24), k, seen = 0;
                    for (k = 0; k < n; k++) {
                        if (strcmp(at[k].name, "Unarmed strike")) continue;
                        seen = 1;
                        if (RACES[r].natural_weapon[0]) {
                            char die[16];
                            const char *sp =
                                strchr(RACES[r].natural_weapon, ' ');
                            size_t dn = sp ? (size_t)(sp
                                       - RACES[r].natural_weapon) : 0;
                            if (dn && dn < sizeof die) {
                                memcpy(die, RACES[r].natural_weapon, dn);
                                die[dn] = '\0';
                                if (strncmp(at[k].damage, die, dn) != 0) {
                                    fail(what, "an unarmed strike that is "
                                               "not the race's own weapon");
                                }
                            }
                        }
                    }
                    if (!seen) fail(what, "no unarmed strike at all");
                }

                measure(&c, what);
            }
        }
    }
}

static void sweep_weapon_proficiency(void)
{
    static const int LEVELS[] = { 1, 5, 20 };
    Character c;
    char what[160];
    int cls, lv, i, k;

    printf("every weapon in the hands of every class\n");
    for (cls = 0; cls < CLASS_COUNT; cls++) {
        const char *line = CLASSES[cls].weapon_profs;
        int expected = -1;

        for (k = 0; k < (int)(sizeof WEAPONS_KNOWN / sizeof *WEAPONS_KNOWN);
             k++) {
            if (!strcmp(WEAPONS_KNOWN[k].name, CLASSES[cls].name)) {
                expected = WEAPONS_KNOWN[k].weapons;
            }
        }
        if (expected < 0) {
            fail(CLASSES[cls].name, "no book count is written down for it");
            continue;
        }

        for (lv = 0; lv < (int)(sizeof LEVELS / sizeof *LEVELS); lv++) {
            int prof_count = 0;

            for (i = 0; i < ITEM_COUNT; i++) {
                const ItemData *it = &ITEMS[i];
                Attack atk[MAX_ATTACKS];
                int n, j, found = 0;
                int want, ability;

                if (it->category < ITEM_SIMPLE_MELEE
                    || it->category > ITEM_MARTIAL_RANGED) continue;

                base(&c, "Combo");
                c.race_id = 0;
                add_class_at(&c, cls, LEVELS[lv], -1);
                add_prof_list(&c, line, CLASSES[cls].name);
                add_item(&c, i, 1, 1);

                snprintf(what, sizeof what, "a %s of %d with a %s",
                         CLASSES[cls].name, LEVELS[lv], it->name);
                measure(&c, what);

                want = line_covers(line, it);
                /* Ranged uses Dexterity, melee Strength, and finesse the
                   better of them -- but base() gives every score 15, so
                   whichever is chosen the modifier is the same +2. */
                ability = ability_mod(&c, ABL_STR);

                n = attacks_of(&c, atk, MAX_ATTACKS);
                for (j = 0; j < n; j++) {
                    if (strcmp(atk[j].name, it->name)) continue;
                    found = 1;
                    if (atk[j].proficient != want) {
                        fail(what, want ? "not called proficient, and the "
                                          "class's line covers it"
                                        : "called proficient, and the "
                                          "class's line does not cover it");
                    }
                    if (atk[j].proficient) prof_count++;
                    if (atk[j].bonus != ability
                        + (atk[j].proficient ? proficiency_bonus(&c) : 0)) {
                        fail(what, "an attack bonus that is not the ability "
                                   "modifier plus the proficiency bonus");
                    }
                    /* The net's damage is a dash in the book's table, and
                       formatting it as a die once produced "-+3 -". */
                    if (atk[j].damage[0] == '-' || strstr(atk[j].damage, "-+")) {
                        fail(what, "a damage line built out of the table's "
                                   "dash");
                    }
                }
                if (!found) fail(what, "the weapon it carries is not in its "
                                       "attacks");
            }

            if (prof_count != expected) {
                snprintf(what, sizeof what,
                         "a %s of %d", CLASSES[cls].name, LEVELS[lv]);
                fprintf(stderr, "  FAIL %-58s proficient with %d weapons; "
                                "the book gives it %d\n",
                        what, prof_count, expected);
                fail(what, "a weapon proficiency count the book does not give");
            }
        }
    }
}

static void sweep_settings(void)
{
    Character c;
    char what[160];
    unsigned mask;

    printf("every combination of books and optional rules (8,192)\n");
    for (mask = 0; mask < (1u << 13); mask++) {
        int i, enabled = 0;

        settings_defaults(&SETTINGS);
        for (i = 0; i < BOOK_COUNT; i++) {
            SETTINGS.book[i] = (mask >> i) & 1;
        }
        SETTINGS.book[BOOK_PHB] = 1;            /* never optional */
        SETTINGS.custom_origins    = (mask >> 7) & 1;
        SETTINGS.optional_features = (mask >> 8) & 1;
        SETTINGS.multiclassing     = (mask >> 9) & 1;
        SETTINGS.feats             = (mask >> 10) & 1;
        SETTINGS.experience        = (mask >> 11) & 1;
        SETTINGS.manual_dice       = (mask >> 12) & 1;

        for (i = 0; i < BOOK_COUNT; i++) enabled += book_enabled((SourceBook)i);
        if (enabled < 1) fail("settings", "no book is enabled at all");

        base(&c, "Combo");
        c.race_id = 0;
        c.background_id = 0;
        add_class_at(&c, CLS_FIGHTER, 5, -1);
        snprintf(what, sizeof what, "settings mask %u", mask);
        measure(&c, what);
        /* Every sixty-fourth combination is written out as well, so the
           settings line in the file is exercised across the whole space. */
        if ((mask & 63u) == 0) round_trip(&c, what);
    }
    settings_defaults(&SETTINGS);
}

int main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--print")) quiet = 0;
    }
    if (quiet && !freopen("/dev/null", "w", stdout)) {
        fprintf(stderr, "could not silence stdout\n");
        return 1;
    }
    /* Progress goes to stderr, so that the sheet may have stdout. */
    setvbuf(stderr, NULL, _IONBF, 0);

    settings_defaults(&SETTINGS);

    sweep_race_class_level();
    sweep_race_subclass_level();
    sweep_race_pairings();
    sweep_loaded_character();
    sweep_subclasses();
    sweep_multiclass();
    sweep_backgrounds_and_feats();
    sweep_spells_and_items();
    sweep_carried_but_unused();
    sweep_sidekicks();
    sweep_armour_and_dex();
    sweep_race_traits();
    sweep_weapon_proficiency();
    sweep_at_the_limits();
    sweep_settings();

    fprintf(stderr, "\n%ld combinations measured, %ld of them through the "
                    "file, %d failures\n", checked, roundtripped, failures);
    return failures ? 1 : 0;
}
