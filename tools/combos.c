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
static int has_named(const Character *c, const char *name)
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

    /* Three of the derived numbers are a formula the PHB states outright,
       so they are checked against the formula rather than against a range.
       Whatever a race, a class, a feat or a magic item does to the score
       underneath, the number on the sheet has to follow it. */
    if (carrying_capacity(c) != ability_score(c, ABL_STR) * 15) {
        fail(what, "a carrying capacity that is not Strength x 15");
    }
    {
        int want = 10 + skill_bonus(c, SKL_PERCEPTION)
                 + (has_named(c, "Observant") ? 5 : 0);
        if (passive_perception(c) != want) {
            fail(what, "a passive Perception that is not 10 + Perception");
        }
    }
    {
        int want = ability_mod(c, ABL_DEX) + (has_named(c, "Alert") ? 5 : 0);
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

/* Every combination of the seven books and the six optional rules. The
   banks are rebuilt as the books change, and every menu the wizard shows is
   filtered by them, so a combination that leaves a bank empty is exactly
   where an off-by-one goes unnoticed. */
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
    sweep_sidekicks();
    sweep_at_the_limits();
    sweep_settings();

    fprintf(stderr, "\n%ld combinations measured, %ld of them through the "
                    "file, %d failures\n", checked, roundtripped, failures);
    return failures ? 1 : 0;
}
