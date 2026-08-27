/* sidekick.c -- creating and levelling Tasha's sidekicks.
 *
 * A sidekick is a creature of challenge 1/2 or lower with levels in Expert,
 * Spellcaster or Warrior. It belongs to a character and is saved inside
 * their file, and can also be written out as a sheet of its own for whoever
 * is running it at the table.
 *
 * The creature's own numbers come from the beast tables where possible, so
 * the wolf a ranger befriends really is the Monster Manual wolf. Anything
 * not in those tables can be typed in instead, since a sidekick may be any
 * stat block at all.
 */
#include "dnd.h"
#include "data.h"
#include "data_spells.h"
#include "sidekick.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

/* "a wolf" but "an ape": the creature name is data, so the article has to be
   worked out rather than written into the string. */
static const char *article_for(const char *word)
{
    char c = word[0];
    if (c >= 'A' && c <= 'Z') c += 32;
    return (c=='a'||c=='e'||c=='i'||c=='o'||c=='u') ? "an" : "a";
}

/* A sidekick's proficiency bonus follows its class level, as characters do. */
int sidekick_proficiency(const Sidekick *sk)
{
    return 2 + (sk->level - 1) / 4;
}

int sidekick_ability_mod(const Sidekick *sk, int ability)
{
    return (sk->abilities[ability] - 10) / 2;
}

/* ------------------------------------------------------------ the creature */

static int choose_creature(Sidekick *sk)
{
    static const char *const sources[] = {
        "Choose a beast (challenge 1/2 or lower)",
        "Type in another stat block by hand",
        "Cancel"
    };

    switch (ui_menu("  What creature is it?", sources, NULL, 3)) {
    case 0: {
        const char *opts[128];
        static char labels[128][96];
        int map[128], n = 0, i, pick;

        for (i = 0; i < BEAST_COUNT_ACTUAL && n < 128; i++) {
            const BeastData *b = &BEASTS[i];
            if (b->cr_eighths > 4) continue;        /* CR 1/2 is 4 eighths */
            snprintf(labels[n], sizeof labels[n],
                     "%-22s CR %-4s AC %-3d HP %-4d %s", b->name, b->cr_text,
                     b->ac, b->hp, b->speed);
            opts[n] = labels[n];
            map[n] = i;
            n++;
        }
        if (n == 0) return 0;

        pick = ui_menu("  Creature:", opts, NULL, n);
        {
            const BeastData *b = &BEASTS[map[pick]];
            int a;
            sk->beast_id = map[pick];
            snprintf(sk->creature, sizeof sk->creature, "%s", b->name);
            for (a = 0; a < 6; a++) sk->abilities[a] = b->abilities[a];
            sk->hp = b->hp;
            sk->ac = b->ac;
            snprintf(sk->speed, sizeof sk->speed, "%s", b->speed);
        }
        return 1;
    }
    case 1: {
        int a;
        sk->beast_id = -1;
        ui_line("  Creature name", sk->creature, sizeof sk->creature);
        if (!sk->creature[0]) return 0;
        printf("  Its stat block must be challenge 1/2 or lower.\n");
        for (a = 0; a < 6; a++) {
            char prompt[48];
            snprintf(prompt, sizeof prompt, "  %s", ABILITY_NAME[a]);
            sk->abilities[a] = ui_int(prompt, 1, 30);
        }
        sk->hp = ui_int("  Hit points", 1, 200);
        sk->ac = ui_int("  Armor Class", 1, 25);
        ui_line_default("  Speed", "30 ft.", sk->speed, sizeof sk->speed);
        return 1;
    }
    default:
        return 0;
    }
}

/* ------------------------------------------------------------ class choices */

static void add_sk_choice(Sidekick *sk, const char *label, const char *value)
{
    if (sk->choice_count >= MAX_SK_CHOICES) return;
    snprintf(sk->choices[sk->choice_count].label,
             sizeof sk->choices[0].label, "%s", label);
    snprintf(sk->choices[sk->choice_count].value,
             sizeof sk->choices[0].value, "%s", value);
    sk->choice_count++;
}

static int has_sk_choice(const Sidekick *sk, const char *label)
{
    int i;
    for (i = 0; i < sk->choice_count; i++) {
        if (strcmp(sk->choices[i].label, label) == 0) return 1;
    }
    return 0;
}

/* The saving throw each class lets it become proficient in. */
static void choose_save(Sidekick *sk)
{
    static const char *const expert[]  = { "Dexterity", "Intelligence",
                                           "Charisma" };
    static const char *const caster[]  = { "Wisdom", "Intelligence",
                                           "Charisma" };
    static const char *const warrior[] = { "Strength", "Dexterity",
                                           "Constitution" };
    const char *const *list = (sk->cls == SK_EXPERT) ? expert
                            : (sk->cls == SK_SPELLCASTER) ? caster : warrior;

    add_sk_choice(sk, "Saving throw proficiency",
                  list[ui_menu("  Proficient in which saving throw?", list,
                               NULL, 3)]);
}

/* A Spellcaster's role decides its spell list and spellcasting ability. */
static void choose_role(Sidekick *sk)
{
    const char *opts[SK_ROLE_COUNT];
    const char *det[SK_ROLE_COUNT];
    int i;

    for (i = 0; i < SK_ROLE_COUNT; i++) {
        opts[i] = SPELLCASTER_ROLE_NAME[i];
        det[i] = SPELLCASTER_ROLE_DESC[i];
    }
    sk->role = ui_menu("  Which role?", opts, det, SK_ROLE_COUNT);
    add_sk_choice(sk, "Role", SPELLCASTER_ROLE_NAME[sk->role]);
}

static void choose_martial_role(Sidekick *sk)
{
    static const char *const roles[] = { "Attacker", "Defender" };
    static const char *const det[] = {
        "+2 to all attack rolls",
        "Use its reaction to impose disadvantage on an attack by a creature "
        "within 5 feet aimed at someone else"
    };
    add_sk_choice(sk, "Martial Role",
                  roles[ui_menu("  Attacker or Defender?", roles, det, 2)]);
}

/* Which spell lists the role draws on. */
static unsigned spell_bits_for_role(int role)
{
    switch (role) {
    case SK_MAGE:    return SPL_WIZARD;
    case SK_HEALER:  return SPL_CLERIC | SPL_DRUID;
    default:         return SPL_BARD | SPL_WARLOCK;
    }
}

/* Half-caster slots, the same shape the artificer uses: the sidekick has
   slots from 1st level and tops out at 5th-level spells. */
static int sk_max_spell_level(int level)
{
    int n = (level + 1) / 2;            /* rounds up, as the artificer does */
    int lvl = (n + 1) / 2;
    if (lvl < 1) lvl = 1;
    if (lvl > 5) lvl = 5;
    return lvl;
}

static void pick_sidekick_spells(Sidekick *sk)
{
    unsigned bits = spell_bits_for_role(sk->role);
    int want_cantrips = SPELLCASTER_CANTRIPS[sk->level];
    int want_spells = SPELLCASTER_SPELLS_KNOWN[sk->level];
    int maxlvl = sk_max_spell_level(sk->level);
    int have_cantrips = 0, have_spells = 0, i;

    for (i = 0; i < sk->spell_count; i++) {
        if (SPELLS[sk->spells[i]].level == 0) have_cantrips++;
        else have_spells++;
    }

    printf("\n  A %s %s knows %d cantrips and %d spells at level %d, up to "
           "%s level.\n", SPELLCASTER_ROLE_NAME[sk->role],
           SIDEKICK_CLASS_NAME[sk->cls], want_cantrips, want_spells,
           sk->level, maxlvl == 1 ? "1st" : maxlvl == 2 ? "2nd"
                    : maxlvl == 3 ? "3rd" : maxlvl == 4 ? "4th" : "5th");

    while (have_cantrips < want_cantrips || have_spells < want_spells) {
        int cantrip = have_cantrips < want_cantrips;
        const char *opts[256];
        static char labels[256][96];
        int map[256], n = 0, pick, k;

        for (i = 0; i < SPELL_COUNT && n < 256; i++) {
            int already = 0;
            if (!(SPELLS[i].classes & bits)) continue;
            if (!book_enabled((SourceBook)SPELLS[i].book)) continue;
            if (cantrip ? SPELLS[i].level != 0
                        : (SPELLS[i].level == 0 || SPELLS[i].level > maxlvl))
                continue;
            for (k = 0; k < sk->spell_count; k++) {
                if (sk->spells[k] == i) already = 1;
            }
            if (already) continue;

            snprintf(labels[n], sizeof labels[n], "%-30s %s", SPELLS[i].name,
                     SCHOOL_NAMES[SPELLS[i].school]);
            opts[n] = labels[n];
            map[n] = i;
            n++;
        }
        if (n == 0) break;

        pick = ui_menu(cantrip ? "  Choose a cantrip:" : "  Choose a spell:",
                       opts, NULL, n);
        if (sk->spell_count >= (int)(sizeof sk->spells / sizeof sk->spells[0]))
            break;
        sk->spells[sk->spell_count++] = map[pick];
        if (cantrip) have_cantrips++; else have_spells++;
    }
}

/* ---------------------------------------------- ability score improvements */

static void sidekick_asi(Sidekick *sk, int level)
{
    static const char *const modes[] = {
        "+2 to one ability", "+1 to each of two abilities"
    };
    const char *opts[6];
    int avail[6], picks[2], a, m;

    printf("\n  %s level %d: Ability Score Improvement.\n",
           SIDEKICK_CLASS_NAME[sk->cls], level);

    m = ui_menu("  How?", modes, NULL, 2);
    for (a = 0; a < 6; a++) {
        opts[a] = ABILITY_NAME[a];
        avail[a] = sk->abilities[a] < (m == 0 ? 19 : 20);
    }
    if (m == 0) {
        if (ui_multi("  Raise which ability by 2?", opts, avail, 6, 1, picks)
            && picks[0] >= 0) {
            sk->abilities[picks[0]] += 2;
            if (sk->abilities[picks[0]] > 20) sk->abilities[picks[0]] = 20;
        }
    } else {
        ui_multi("  Raise which two abilities by 1?", opts, avail, 6, 2,
                 picks);
        for (a = 0; a < 2; a++) {
            if (picks[a] >= 0) sk->abilities[picks[a]] += 1;
        }
    }
}

/* ------------------------------------------------------------ levelling up */

/* Applies one class level: hit points, then anything the level grants. */
static void apply_sidekick_level(Sidekick *sk, int level, int roll_hp)
{
    int i;

    if (level > 1) {
        /* The hit die is the creature's own; d8 stands in when the stat
           block came from the beast tables without one stated. */
        int die = 8;
        int gain = roll_hp ? roll_die(die) : (die / 2 + 1);
        gain += sidekick_ability_mod(sk, ABL_CON);
        if (gain < 1) gain = 1;
        sk->hp += gain;
    }

    for (i = 0; i < SIDEKICK_FEATURE_COUNT; i++) {
        const SidekickFeature *f = &SIDEKICK_FEATURES[i];
        if ((int)f->cls != sk->cls || f->level != level) continue;

        printf("\n  %s (level %d)\n", f->name, f->level);
        ui_wrap(f->summary, 4);

        if (strcmp(f->name, "Ability Score Improvement") == 0) {
            sidekick_asi(sk, level);
        } else if (strcmp(f->name, "Bonus Proficiencies") == 0) {
            choose_save(sk);
        } else if (strcmp(f->name, "Martial Role") == 0
                   && !has_sk_choice(sk, "Martial Role")) {
            choose_martial_role(sk);
        } else if (strcmp(f->name, "Spellcasting") == 0
                   && !has_sk_choice(sk, "Role")) {
            choose_role(sk);
        }
    }
}

void level_up_sidekick(Sidekick *sk)
{
    if (sk->level >= MAX_LEVEL) {
        printf("  %s is already 20th level.\n", sk->name);
        return;
    }
    sk->level++;
    printf("\n  %s reaches %s level %d.\n", sk->name,
           SIDEKICK_CLASS_NAME[sk->cls], sk->level);
    apply_sidekick_level(sk, sk->level, 1);
    if (sk->cls == SK_SPELLCASTER) pick_sidekick_spells(sk);
}

/* ------------------------------------------------------------- creating one */

int create_sidekick(Sidekick *sk, int party_level)
{
    const char *opts[SK_CLASS_COUNT];
    const char *det[SK_CLASS_COUNT];
    int i, lvl;

    memset(sk, 0, sizeof *sk);
    sk->beast_id = -1;
    sk->role = -1;

    ui_header("Add a Sidekick");
    ui_para("A sidekick is a creature with a stat block of challenge 1/2 or "
            "lower, given levels in one of three simple classes. It starts "
            "at the party's average level and gains a level whenever that "
            "average does.");

    ui_line("  What is the sidekick called", sk->name, sizeof sk->name);
    if (!sk->name[0]) return 0;

    if (!choose_creature(sk)) return 0;

    for (i = 0; i < SK_CLASS_COUNT; i++) {
        opts[i] = SIDEKICK_CLASS_NAME[i];
        det[i] = SIDEKICK_CLASS_BLURB[i];
    }
    sk->cls = ui_menu("  Which sidekick class?", opts, det, SK_CLASS_COUNT);

    printf("\n  The party is level %d, so the sidekick starts there.\n",
           party_level);
    lvl = ui_int("  Starting level", 1, MAX_LEVEL);

    /* Walk the levels so every choice each one grants is actually made. */
    sk->level = 0;
    for (i = 1; i <= lvl; i++) {
        sk->level = i;
        apply_sidekick_level(sk, i, i > 1);
    }
    if (sk->cls == SK_SPELLCASTER) pick_sidekick_spells(sk);
    return 1;
}

/* ------------------------------------------------------------- the sheet */

void print_sidekick(FILE *f, const Sidekick *sk, int indent)
{
    static const char *const abbrev[6] = { "STR", "DEX", "CON", "INT",
                                           "WIS", "CHA" };
    int i;
    char pad[16];

    snprintf(pad, sizeof pad, "%*s", indent, "");

    fprintf(f, "%s%s -- %s %d, built on %s %s\n", pad, sk->name,
            SIDEKICK_CLASS_NAME[sk->cls], sk->level,
            article_for(sk->creature), sk->creature);
    fprintf(f, "%s  AC %d, %d hit points, speed %s, proficiency +%d\n", pad,
            sk->ac, sk->hp, sk->speed, sidekick_proficiency(sk));

    fprintf(f, "%s  ", pad);
    for (i = 0; i < 6; i++) {
        int mod = sidekick_ability_mod(sk, i);
        fprintf(f, "%s %d (%+d)%s", abbrev[i], sk->abilities[i], mod,
                i == 5 ? "\n" : "  ");
    }

    for (i = 0; i < sk->choice_count; i++) {
        fprintf(f, "%s  %-24s %s\n", pad, sk->choices[i].label,
                sk->choices[i].value);
    }

    if (sk->cls == SK_SPELLCASTER && sk->role >= 0) {
        int dc = 8 + sidekick_proficiency(sk)
               + sidekick_ability_mod(sk, sk->role == SK_MAGE ? ABL_INT
                                        : sk->role == SK_HEALER ? ABL_WIS
                                        : ABL_CHA);
        fprintf(f, "%s  Spell save DC %d, spell attack %+d\n", pad, dc,
                dc - 8);
    }
    for (i = 0; i < sk->spell_count; i++) {
        const SpellData *sp = &SPELLS[sk->spells[i]];
        fprintf(f, "%s    %-28s %s %s\n", pad, sp->name,
                sp->level ? "level" : "cantrip",
                sp->level ? (sp->level == 1 ? "1" : sp->level == 2 ? "2"
                           : sp->level == 3 ? "3" : sp->level == 4 ? "4" : "5")
                          : "");
    }

    fprintf(f, "\n%s  Features\n", pad);
    for (i = 0; i < SIDEKICK_FEATURE_COUNT; i++) {
        const SidekickFeature *ft = &SIDEKICK_FEATURES[i];
        if ((int)ft->cls != sk->cls || ft->level > sk->level) continue;
        fprintf(f, "%s    %2d  %s\n", pad, ft->level, ft->name);
    }
}

/* ---------------------------------------------------------- the sidekick screen */

/* Writes a sidekick out as a sheet of its own, for whoever runs it at the
 * table. The owner's file keeps the authoritative copy; this is a printout,
 * not a second source of truth, and it says so. */
static int export_sidekick(const Sidekick *sk, const Character *owner)
{
    char path[MAX_NAME + 8];
    FILE *f;
    int i;

    snprintf(path, sizeof path, "%s.txt", sk->name);
    f = fopen(path, "w");
    if (!f) return -1;

    for (i = 0; i < 64; i++) fputc('=', f);
    fprintf(f, "\n %s\n", sk->name);
    for (i = 0; i < 64; i++) fputc('=', f);
    fprintf(f, "\n\n");
    print_sidekick(f, sk, 0);
    fprintf(f, "\n  Sidekick of %s. The character file %s.txt holds the\n"
               "  authoritative copy; this sheet is a printout of it.\n",
            owner->name, owner->name);
    fclose(f);
    printf("  Wrote %s\n", path);
    return 0;
}

void manage_sidekicks(Character *c)
{
    ui_header("Sidekicks");
    ui_para("A sidekick is an NPC of challenge 1/2 or lower with levels in "
            "Expert, Spellcaster or Warrior. It starts at the party's "
            "average level and gains a level whenever that average does.");

    for (;;) {
        static const char *const modes[] = {
            "Add a sidekick",
            "Level one up",
            "Write one out as its own sheet",
            "Done"
        };
        int i, pick;

        if (c->sidekick_count == 0) {
            printf("\n  %s has no sidekicks.\n", c->name);
        } else {
            printf("\n");
            for (i = 0; i < c->sidekick_count; i++) {
                const Sidekick *sk = &c->sidekicks[i];
                printf("    %-20s %s %d, built on %s %s\n", sk->name,
                       SIDEKICK_CLASS_NAME[sk->cls], sk->level,
                       article_for(sk->creature), sk->creature);
            }
        }

        pick = ui_menu("  Sidekicks:", modes, NULL, 4);
        if (pick == 3) return;

        if (pick == 0) {
            if (c->sidekick_count >= MAX_SIDEKICKS) {
                printf("  That is as many as this program tracks.\n");
                continue;
            }
            if (create_sidekick(&c->sidekicks[c->sidekick_count],
                                total_level(c))) {
                c->sidekick_count++;
            }
            continue;
        }

        if (c->sidekick_count == 0) {
            printf("  There are none yet.\n");
            continue;
        }
        {
            const char *opts[MAX_SIDEKICKS + 1];
            static char labels[MAX_SIDEKICKS][80];
            int which;

            for (i = 0; i < c->sidekick_count; i++) {
                snprintf(labels[i], sizeof labels[i], "%s (%s %d)",
                         c->sidekicks[i].name,
                         SIDEKICK_CLASS_NAME[c->sidekicks[i].cls],
                         c->sidekicks[i].level);
                opts[i] = labels[i];
            }
            opts[c->sidekick_count] = "Back";

            which = ui_menu("  Which one?", opts, NULL, c->sidekick_count + 1);
            if (which == c->sidekick_count) continue;

            if (pick == 1) level_up_sidekick(&c->sidekicks[which]);
            else if (export_sidekick(&c->sidekicks[which], c) != 0) {
                printf("  Could not write that sheet.\n");
            }
        }
    }
}
