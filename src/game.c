/* game.c -- game mode: a character in play rather than in the making.
 *
 * Every other screen in this program answers "what is this character?".
 * This one answers "what has happened to them?", which is a different
 * question with different arithmetic: hit points go down and come back,
 * uses of things run out and are restored by a rest, money is spent on
 * something somebody will want to remember, and conditions arrive and
 * leave. None of it changes what the character is.
 *
 * What it deliberately does NOT do is play the game. It rolls a hit die
 * when asked and totals the result, and it will not decide that a hit
 * landed, that a save was made, or that a condition has ended. The table
 * decides; this keeps the count. The one place that judgement would be
 * tempting -- equipment wearing out -- has no rule in the Player's
 * Handbook at all, so wear here is a note the table writes rather than
 * anything the program computes.
 */
#include "game.h"
#include "build.h"
#include "data.h"
#include "details.h"
#include "inventory.h"
#include "reference.h"
#include "saveload.h"
#include "shop.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------- hit points */

int hit_points_now(const Character *c)
{
    int now = hit_points_max(c) - c->damage;
    return now > 0 ? now : 0;
}

int hit_dice_left(const Character *c, int slot)
{
    int left;
    if (slot < 0 || slot >= c->class_count) return 0;
    left = c->classes[slot].level - c->hit_dice_used[slot];
    return left > 0 ? left : 0;
}

/* The one-line state of a character in play, printed above every screen so
   a decision is never made against a number that has scrolled away. */
static void vitals(const Character *c)
{
    int max = hit_points_max(c);
    int now = hit_points_now(c);
    int i;

    printf("\n  %s -- %d/%d hit points", c->name, now, max);
    if (c->temp_hp) printf(" (+%d temporary)", c->temp_hp);
    printf(", AC %d, speed %d\n", armour_class(c), speed_of(c));

    if (now == 0) {
        if (c->death_fail >= 3) {
            printf("  Dead: three failed death saves.\n");
        } else if (c->death_success >= 3) {
            printf("  Stable at 0 hit points, %d failure%s on the way.\n",
                   c->death_fail, c->death_fail == 1 ? "" : "s");
        } else {
            printf("  Down: %d success%s and %d failure%s on death saves.\n",
                   c->death_success, c->death_success == 1 ? "" : "es",
                   c->death_fail, c->death_fail == 1 ? "" : "s");
        }
    }
    if (c->exhaustion) {
        printf("  Exhaustion level %d.\n", c->exhaustion);
    }
    {
        int wrote = 0;
        for (i = 0; i < CONDITION_COUNT; i++) {
            if (!(c->conditions & (1u << i))) continue;
            printf("%s%s", wrote ? ", " : "  Conditions: ",
                   CONDITIONS[i].name);
            wrote = 1;
        }
        if (wrote) printf("\n");
    }
    {
        int wrote = 0;
        for (i = 0; i < c->resource_count; i++) {
            const Resource *r = &c->resources[i];
            printf("%s%s %d/%d", wrote ? "   " : "  ", r->name,
                   r->max - r->used, r->max);
            wrote = 1;
        }
        if (wrote) printf("\n");
    }
    printf("  Coins: %d pp, %d gp, %d ep, %d sp, %d cp\n",
           c->platinum, c->gold, c->electrum, c->silver, c->copper);
}

/* One blow, and what the rules make of it.
 *
 * Kept apart from the screen because it is the only part with a right
 * answer: everything else here records what the table decided, and this
 * works something out. The Player's Handbook (p.197, "Damage at 0 Hit
 * Points") says three things, and the program used to do none of them --
 * it printed that enough damage in one blow kills outright and then never
 * worked out whether this blow was enough.
 *
 *   Damage taken at 0 hit points is a failed death save, and two of them
 *   if the blow was a critical.
 *
 *   Damage left over once the hit points are gone kills outright when it
 *   reaches the hit point maximum. That remainder has to be measured
 *   before the damage is clamped to the maximum, because the clamp is
 *   exactly what throws it away.
 *
 * `absorbed` and `spare` are filled in for the caller's message; either
 * may be NULL.
 */
HurtResult take_damage(Character *c, int hurt, int critical,
                       int *absorbed, int *spare)
{
    int max = hit_points_max(c);
    int was_down = (hit_points_now(c) == 0);
    int soak = 0, left;

    if (absorbed) *absorbed = 0;
    if (spare) *spare = 0;
    if (hurt <= 0) return HURT_NOTHING;

    /* Temporary hit points are lost first and are not healed back, which
       is the whole of the rule. */
    if (c->temp_hp) {
        soak = hurt < c->temp_hp ? hurt : c->temp_hp;
        c->temp_hp -= soak;
        hurt -= soak;
        if (absorbed) *absorbed = soak;
    }
    if (hurt == 0) return HURT_SOAKED;

    left = hurt - hit_points_now(c);
    if (left < 0) left = 0;
    if (spare) *spare = left;

    c->damage += hurt;
    if (c->damage > max) c->damage = max;

    if (hit_points_now(c) > 0) return HURT_STANDING;

    if (left >= max) {
        c->death_success = 0;
        c->death_fail = 3;
        return HURT_DEAD;
    }
    if (!was_down) return HURT_DOWNED;

    c->death_fail += critical ? 2 : 1;
    if (c->death_fail > 3) c->death_fail = 3;
    return HURT_SAVE_FAILED;
}

static void hurt_and_heal(Character *c)
{
    for (;;) {
        static const char *const what[] = {
            "Take damage", "Heal", "Gain temporary hit points",
            "Death saving throw", "Back"
        };
        int max = hit_points_max(c);

        vitals(c);
        switch (ui_menu("  Hit points:", what, NULL, 5)) {
        case 0: {
            int hurt = ui_int("  How much damage", 0, 9999);
            int crit = 0, absorbed = 0, spare = 0;

            /* Whether the blow was a critical only changes anything for a
               character already at 0, so it is only asked there. */
            if (hurt > 0 && hurt > c->temp_hp && hit_points_now(c) == 0) {
                crit = ui_yesno("  Was that a critical hit?", 0);
            }
            switch (take_damage(c, hurt, crit, &absorbed, &spare)) {
            case HURT_NOTHING:
                break;
            case HURT_SOAKED:
                printf("  %d absorbed by temporary hit points.\n", absorbed);
                break;
            case HURT_STANDING:
                if (absorbed) {
                    printf("  %d absorbed by temporary hit points.\n",
                           absorbed);
                }
                break;
            case HURT_DOWNED:
                printf("  %s is down. Death saves start now, and enough "
                       "damage in one blow kills outright.\n", c->name);
                break;
            case HURT_SAVE_FAILED:
                printf("  Hit while down: %s.\n",
                       crit ? "two death save failures"
                            : "a death save failure");
                if (c->death_fail >= 3) {
                    printf("  Three failures: %s dies.\n", c->name);
                }
                break;
            case HURT_DEAD:
                printf("  %d past nothing, against a maximum of %d: %s dies "
                       "outright.\n", spare, max, c->name);
                break;
            }
            break;
        }
        case 1: {
            int heal = ui_int("  How many hit points", 0, 9999);
            if (heal > c->damage) heal = c->damage;
            c->damage -= heal;
            /* Healing above zero ends the dying, and the saves so far
               stop counting. */
            if (heal > 0 && hit_points_now(c) > 0) {
                c->death_success = c->death_fail = 0;
            }
            printf("  Healed %d.\n", heal);
            break;
        }
        case 2:
            /* They do not stack: a second lot replaces the first, and the
               player keeps whichever is better. */
            {
                int got = ui_int("  How many temporary hit points", 0, 999);
                if (c->temp_hp && got < c->temp_hp) {
                    printf("  You already have %d, which is more; keeping "
                           "those.\n", c->temp_hp);
                } else {
                    c->temp_hp = got;
                }
            }
            break;
        case 3: {
            static const char *const roll[] = {
                "A success", "A failure", "A natural 20 -- up with 1 hit point",
                "Clear the count", "Back"
            };
            if (hit_points_now(c) > 0) {
                printf("  %s is not dying.\n", c->name);
                break;
            }
            switch (ui_menu("  Death saving throw:", roll, NULL, 5)) {
            case 0:
                if (c->death_success < 3) c->death_success++;
                if (c->death_success >= 3) {
                    printf("  Three successes: stable, still at 0 hit "
                           "points.\n");
                }
                break;
            case 1:
                if (c->death_fail < 3) c->death_fail++;
                if (c->death_fail >= 3) printf("  Three failures.\n");
                break;
            case 2:
                c->death_success = c->death_fail = 0;
                c->damage = max - 1;
                printf("  Up with 1 hit point.\n");
                break;
            case 3:
                c->death_success = c->death_fail = 0;
                break;
            default:
                break;
            }
            break;
        }
        default:
            return;
        }
    }
}

/* ---------------------------------------------------------- rests */

static void spend_hit_die(Character *c)
{
    const char *opts[MAX_CLASSES + 1];
    static char lines[MAX_CLASSES][80];
    int map[MAX_CLASSES], n = 0, i, pick;

    for (i = 0; i < c->class_count; i++) {
        int left = hit_dice_left(c, i);
        if (left <= 0) continue;
        snprintf(lines[n], sizeof lines[n], "%s -- %d d%d left",
                 CLASSES[c->classes[i].class_id].name, left,
                 CLASSES[c->classes[i].class_id].hit_die);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    if (n == 0) {
        printf("  No hit dice left; a long rest brings some back.\n");
        return;
    }
    opts[n] = "Back";
    pick = ui_menu("  Spend which hit die?", opts, NULL, n + 1);
    if (pick == n) return;

    {
        int slot = map[pick];
        int die = CLASSES[c->classes[slot].class_id].hit_die;
        int con = ability_mod(c, ABL_CON);
        int got = ui_roll_die(die, "a hit die");
        int heal = got + con;
        /* A hit die never heals less than nothing, however bad the
           Constitution. */
        if (heal < 0) heal = 0;
        if (heal > c->damage) heal = c->damage;
        c->hit_dice_used[slot]++;
        c->damage -= heal;
        printf("  Rolled %d%+d -- healed %d.\n", got, con, heal);
    }
}

static void take_a_rest(Character *c)
{
    for (;;) {
        static const char *const what[] = {
            "Spend a hit die", "Finish a short rest", "Finish a long rest",
            "Back"
        };
        int i;

        vitals(c);
        switch (ui_menu("  Rest:", what, NULL, 4)) {
        case 0:
            spend_hit_die(c);
            break;
        case 1:
            /* A short rest restores nothing on its own -- hit dice are
               spent one at a time above. What it does restore is anything
               the table said comes back on one. */
            {
                int back = 0;
                for (i = 0; i < c->resource_count; i++) {
                    if (c->resources[i].per_long_rest) continue;
                    if (!c->resources[i].used) continue;
                    c->resources[i].used = 0;
                    back++;
                }
                printf("  Short rest over. %d thing%s came back; hit points "
                       "only come back from hit dice.\n", back,
                       back == 1 ? "" : "s");
            }
            break;
        case 2: {
            /* The long rest, as the book gives it: all hit points, half
               your hit dice rounded down and at least one, everything that
               recharges, and one level of exhaustion off if you have eaten
               and drunk. */
            int total = total_level(c), give = total / 2;
            if (give < 1) give = 1;
            c->damage = 0;
            c->temp_hp = 0;
            c->death_success = c->death_fail = 0;
            for (i = 0; i < c->class_count && give > 0; i++) {
                int back = c->hit_dice_used[i];
                if (back > give) back = give;
                c->hit_dice_used[i] -= back;
                give -= back;
            }
            for (i = 0; i < c->resource_count; i++) c->resources[i].used = 0;
            printf("  Long rest over: hit points full, half your hit dice "
                   "back, everything recharged.\n");
            if (c->exhaustion) {
                if (ui_yesno("  Did you have food and drink?", 1)) {
                    c->exhaustion--;
                    printf("  Exhaustion is now level %d.\n", c->exhaustion);
                } else {
                    printf("  Without them the exhaustion stays at level "
                           "%d.\n", c->exhaustion);
                }
            }
            break;
        }
        default:
            return;
        }
    }
}

/* ----------------------------------------------------- conditions */

static void set_conditions(Character *c)
{
    for (;;) {
        const char *opts[32];
        static char lines[32][96];
        int flags[32], map[32], n = 0, i, before;

        /* Exhaustion is the odd one: six levels rather than a yes or no,
           so it is asked for separately below rather than shown as a
           checkbox that could only ever mean "level 1". */
        for (i = 0; i < CONDITION_COUNT && n < 30; i++) {
            if (!strcmp(CONDITIONS[i].name, "Exhaustion")) continue;
            snprintf(lines[n], sizeof lines[n], "%s", CONDITIONS[i].name);
            opts[n] = lines[n];
            flags[n] = (c->conditions & (1u << i)) ? 1 : 0;
            map[n] = i;
            n++;
        }

        vitals(c);
        printf("  Toggle a condition on or off. What each one does is in "
               "the reference screen.\n");
        ui_toggle_list("  Conditions:", opts, n, flags);

        before = 0;
        for (i = 0; i < n; i++) {
            if (flags[i]) c->conditions |= (1u << map[i]);
            else c->conditions &= ~(1u << map[i]);
            if (flags[i]) before++;
        }

        printf("\n  Exhaustion is level %d of %d.\n", c->exhaustion,
               MAX_EXHAUSTION);
        if (ui_yesno("  Change it?", 0)) {
            c->exhaustion = ui_int("  Which level", 0, MAX_EXHAUSTION);
            if (c->exhaustion >= MAX_EXHAUSTION) {
                printf("  Level %d is death.\n", MAX_EXHAUSTION);
            }
        }
        if (!ui_yesno("  Change anything else here?", 0)) return;
    }
}

/* ------------------------------------------------------- resources */

/* Names worth suggesting, drawn from what the character actually has.
 * A feature called "Bardic Inspiration (d6)" is a thing with uses; one
 * called "Expertise" is not. Rather than guess from the name, this offers
 * every feature the character has and lets the table say which are
 * countable and how many there are -- the books put the count in prose
 * that varies by level, subclass and feat, and reading it out of the
 * summary line would be right for some classes and wrong for others.
 */
static void track_resources(Character *c)
{
    for (;;) {
        const char *opts[MAX_RESOURCES + 3];
        static char lines[MAX_RESOURCES][96];
        int i, n = 0, pick;

        vitals(c);
        for (i = 0; i < c->resource_count; i++) {
            const Resource *r = &c->resources[i];
            snprintf(lines[n], sizeof lines[n], "%s -- %d of %d left (back "
                     "on a %s rest)", r->name, r->max - r->used, r->max,
                     r->per_long_rest ? "long" : "short");
            opts[n] = lines[n];
            n++;
        }
        opts[n] = "Start tracking something new";
        opts[n + 1] = "Back";

        pick = ui_menu("  Uses:", opts, NULL, n + 2);
        if (pick == n + 1) return;

        if (pick == n) {
            Resource *r;
            char name[MAX_NAME];
            if (c->resource_count >= MAX_RESOURCES) {
                printf("  That is as many as this program tracks.\n");
                continue;
            }
            ui_line("  What is it called (Ki, Rage, Bardic Inspiration)",
                    name, sizeof name);
            if (!name[0]) continue;
            r = &c->resources[c->resource_count++];
            memset(r, 0, sizeof *r);
            snprintf(r->name, sizeof r->name, "%s", name);
            r->max = ui_int("  How many uses", 1, 999);
            r->per_long_rest = !ui_yesno("  Back on a short rest?", 0);
            continue;
        }

        {
            static const char *const what[] = {
                "Use one", "Use several", "Get one back", "Refill it",
                "Change the maximum", "Stop tracking it", "Back"
            };
            Resource *r = &c->resources[pick];
            switch (ui_menu("  Do what?", what, NULL, 7)) {
            case 0:
                if (r->used >= r->max) printf("  None left.\n");
                else r->used++;
                break;
            case 1: {
                int left = r->max - r->used;
                if (left <= 0) { printf("  None left.\n"); break; }
                r->used += ui_int("  How many", 0, left);
                break;
            }
            case 2:
                if (r->used > 0) r->used--;
                break;
            case 3:
                r->used = 0;
                break;
            case 4:
                r->max = ui_int("  New maximum", 0, 999);
                if (r->used > r->max) r->used = r->max;
                break;
            case 5:
                for (i = pick; i + 1 < c->resource_count; i++) {
                    c->resources[i] = c->resources[i + 1];
                }
                c->resource_count--;
                break;
            default:
                break;
            }
        }
    }
}

/* ----------------------------------------------------------- money */

/* Everything is counted in copper so that mixed change works out, and
   turned back into coins at the end. */
long purse_in_copper(const Character *c)
{
    return (long)c->copper + (long)c->silver * 10 + (long)c->electrum * 50
         + (long)c->gold * 100 + (long)c->platinum * 1000;
}

void purse_from_copper(Character *c, long cp)
{
    if (cp < 0) cp = 0;
    /* Held to what the five fields of a saved sheet can hold. Every screen
       that pays a character checks first and says so, so nothing reaches
       here that would lose money quietly; this is the backstop. */
    if (cp > MAX_PURSE_CP) cp = MAX_PURSE_CP;
    c->platinum = (int)(cp / 1000); cp %= 1000;
    c->gold     = (int)(cp / 100);  cp %= 100;
    c->electrum = (int)(cp / 50);   cp %= 50;
    c->silver   = (int)(cp / 10);   cp %= 10;
    c->copper   = (int)cp;
}

void remember(Character *c, int copper, const char *what)
{
    LedgerEntry *e;
    int i;

    if (c->ledger_count >= MAX_LEDGER) {
        /* Keep the newest, since the oldest is the one nobody is arguing
           about any more. */
        for (i = 0; i + 1 < MAX_LEDGER; i++) c->ledger[i] = c->ledger[i + 1];
        c->ledger_count = MAX_LEDGER - 1;
    }
    e = &c->ledger[c->ledger_count++];
    e->copper = copper;
    snprintf(e->what, sizeof e->what, "%s", what && what[0] ? what
                                            : "(not written down)");
}

static void money(Character *c)
{
    for (;;) {
        static const char *const what[] = {
            "Spend", "Earn or find", "What has been spent", "Back"
        };

        vitals(c);
        switch (ui_menu("  Money:", what, NULL, 4)) {
        case 0: {
            long have = purse_in_copper(c);
            int gp = ui_int("  How many gold pieces", 0, MAX_COINS);
            int sp = ui_int("  And how many silver", 0, 99);
            int cp = ui_int("  And how many copper", 0, 99);
            long cost = (long)gp * 100 + (long)sp * 10 + cp;
            char note[MAX_NAME];

            if (cost == 0) break;
            if (cost > have) {
                printf("  That is %ld copper and you have %ld.\n", cost,
                       have);
                break;
            }
            ui_line("  What on (blank to leave it unsaid)", note,
                    sizeof note);
            purse_from_copper(c, have - cost);
            remember(c, (int)-cost, note);
            printf("  Spent. ");
            break;
        }
        case 1: {
            long have = purse_in_copper(c);
            int gp = ui_int("  How many gold pieces", 0, MAX_COINS);
            int sp = ui_int("  And how many silver", 0, 99);
            int cp = ui_int("  And how many copper", 0, 99);
            long got = (long)gp * 100 + (long)sp * 10 + cp;
            char note[MAX_NAME];

            if (got == 0) break;
            if (have + got > MAX_PURSE_CP) {
                printf("  That is more coin than one character can carry "
                       "about; put the rest somewhere safe.\n");
                break;
            }
            ui_line("  Where from (blank to leave it unsaid)", note,
                    sizeof note);
            purse_from_copper(c, have + got);
            remember(c, (int)got, note);
            break;
        }
        case 2: {
            int i;
            if (!c->ledger_count) {
                printf("  Nothing written down yet.\n");
                break;
            }
            printf("\n  The ledger, newest last:\n");
            for (i = 0; i < c->ledger_count; i++) {
                int cp = c->ledger[i].copper;
                printf("    %c%3d gp %2d sp %2d cp   %s\n",
                       cp < 0 ? '-' : '+',
                       (cp < 0 ? -cp : cp) / 100,
                       ((cp < 0 ? -cp : cp) / 10) % 10,
                       (cp < 0 ? -cp : cp) % 10,
                       c->ledger[i].what);
            }
            break;
        }
        default:
            return;
        }
    }
}

/* -------------------------------------------------- food and lodging */

/* Everything a night in an inn or a day's meals costs is already priced
 * in the Player's Handbook's services table, so this buys from that rather
 * than asking the player for a number: choosing "Meals, modest (per day)"
 * is one decision and typing 30 copper is two, one of which can be wrong.
 */
static void spend_at_the_inn(Character *c)
{
    for (;;) {
        const char *opts[64];
        static char lines[64][96];
        int map[64], n = 0, i, pick;

        for (i = 0; i < SERVICE_COUNT && n < 62; i++) {
            snprintf(lines[n], sizeof lines[n], "%-36s %d gp %d sp %d cp",
                     SERVICES[i].name, SERVICES[i].cost_cp / 100,
                     (SERVICES[i].cost_cp / 10) % 10,
                     SERVICES[i].cost_cp % 10);
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
        opts[n] = "Back";

        vitals(c);
        pick = ui_menu("  Food, drink, lodging and hire:", opts, NULL, n + 1);
        if (pick == n) return;

        {
            const PriceEntry *p = &SERVICES[map[pick]];
            long have = purse_in_copper(c);
            int qty = ui_int("  How many", 1, 99);
            long cost = (long)p->cost_cp * qty;

            if (cost > have) {
                printf("  That costs %ld copper and you have %ld.\n", cost,
                       have);
                continue;
            }
            purse_from_copper(c, have - cost);
            {
                char note[MAX_NAME];
                snprintf(note, sizeof note, "%s x%d", p->name, qty);
                remember(c, (int)-cost, note);
            }
            printf("  Bought %d x %s.\n", qty, p->name);
        }
    }
}

/* ------------------------------------------------------- wear and tear */

static const char *const WEAR_WORD[] = { "sound", "damaged", "broken" };

static void wear_and_repair(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][110];
        int map[MAX_ITEMS], n = 0, i, pick;

        for (i = 0; i < c->item_count && n < MAX_ITEMS; i++) {
            const InventoryEntry *e = &c->inventory[i];
            const char *name = e->is_magic ? MAGIC_ITEMS[e->item_id].name
                                           : ITEMS[e->item_id].name;
            snprintf(lines[n], sizeof lines[n], "%-40s %s%s", name,
                     WEAR_WORD[e->wear < 0 || e->wear > 2 ? 0 : e->wear],
                     e->equipped ? " (worn)" : "");
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  Nothing carried.\n");
            return;
        }
        opts[n] = "Back";

        vitals(c);
        printf("  The Player's Handbook has no general rule for gear "
               "wearing out, so this records what your table decided "
               "rather than deciding anything.\n");
        pick = ui_menu("  Which thing?", opts, NULL, n + 1);
        if (pick == n) return;

        {
            static const char *const state[] = {
                "Sound", "Damaged", "Broken", "Back"
            };
            int got = ui_menu("  What state is it in?", state, NULL, 4);
            if (got < 3) c->inventory[map[pick]].wear = got;
        }
    }
}

/* --------------------------------------------------- spells for the day */

/* Casting a spell is three separate things at a table, and the program is
 * only asked about two of them. Which spells are prepared is settled when
 * the character wakes up (progression.c asks, and the answer is on the
 * sheet); what a spell does is the DM's business. What is left, and what
 * gets lost track of, is how many slots are gone and whether the material
 * component was consumed -- so those are what this counts.
 */
static void cast_and_slots(Character *c)
{
    for (;;) {
        int slots[10], have, lvl, i, n = 0, pick;
        const char *opts[16];
        static char lines[16][80];
        int map[16];
        int pact_count = 0, pact_level = 0;

        have = spell_slots_for(c, slots);
        pact_slots_for(c, &pact_count, &pact_level);

        vitals(c);
        if (!have && !pact_count) {
            printf("  %s has no spell slots.\n", c->name);
            return;
        }

        for (lvl = 1; lvl <= 9 && n < 12; lvl++) {
            if (!slots[lvl]) continue;
            snprintf(lines[n], sizeof lines[n],
                     "Level %d -- %d of %d left", lvl,
                     slots[lvl] - c->slots_used[lvl], slots[lvl]);
            opts[n] = lines[n];
            map[n] = lvl;
            n++;
        }
        if (pact_count) {
            snprintf(lines[n], sizeof lines[n],
                     "Pact magic (level %d) -- %d of %d left", pact_level,
                     pact_count - c->pact_used, pact_count);
            opts[n] = lines[n];
            map[n] = -1;
            n++;
        }
        opts[n] = "Use a material component";
        opts[n + 1] = "Get every slot back (a long rest)";
        opts[n + 2] = "Back";

        pick = ui_menu("  Spell slots:", opts, NULL, n + 3);
        if (pick == n + 2) return;

        if (pick == n + 1) {
            for (i = 0; i <= 9; i++) c->slots_used[i] = 0;
            c->pact_used = 0;
            printf("  Every slot is back.\n");
            continue;
        }
        if (pick == n) {
            /* A material component that the spell consumes is gone, and
               the sheet has to show it gone. Which spells consume one is
               in the components line, so the player is shown it and picks
               from what they are carrying. */
            const char *what[MAX_ITEMS + 1];
            static char lab[MAX_ITEMS][96];
            int wmap[MAX_ITEMS], wn = 0, k;

            for (k = 0; k < c->item_count && wn < MAX_ITEMS; k++) {
                const InventoryEntry *e = &c->inventory[k];
                if (e->is_magic) continue;
                snprintf(lab[wn], sizeof lab[wn], "%d x %s", e->quantity,
                         ITEMS[e->item_id].name);
                what[wn] = lab[wn];
                wmap[wn] = k;
                wn++;
            }
            if (wn == 0) {
                printf("  You are carrying nothing to use.\n");
                continue;
            }
            what[wn] = "Back";
            k = ui_menu("  Use up what?", what, NULL, wn + 1);
            if (k == wn) continue;
            {
                InventoryEntry *e = &c->inventory[wmap[k]];
                int used = ui_int("  How many", 1,
                                  e->quantity < 1 ? 1 : e->quantity);
                e->quantity -= used;
                printf("  Used %d.\n", used);
                if (e->quantity <= 0) {
                    printf("  That was the last of them.\n");
                    for (i = wmap[k]; i + 1 < c->item_count; i++) {
                        c->inventory[i] = c->inventory[i + 1];
                    }
                    c->item_count--;
                }
            }
            continue;
        }

        {
            int which = map[pick];
            if (which < 0) {
                if (c->pact_used >= pact_count) printf("  None left.\n");
                else c->pact_used++;
            } else if (c->slots_used[which] >= slots[which]) {
                printf("  No level %d slots left.\n", which);
            } else {
                c->slots_used[which]++;
            }
        }
    }
}

/* ------------------------------------------------------------ the screen */

void game_mode(Character *c)
{
    ui_header("Game Mode");
    ui_para("The character in play rather than in the making: what they "
            "have lost, spent, used and caught. Nothing here changes who "
            "they are, and nothing here decides anything -- it keeps the "
            "count so the table does not have to.");

    for (;;) {
        static const char *const what[] = {
            "Show the whole sheet",
            "Hit points, healing and death saves",
            "Rests and hit dice",
            "Spell slots and components",
            "Uses of things",
            "Conditions",
            "Money",
            "Visit a shop",
            "Food, drink and lodging",
            "Equipment",
            "Wear and repair",
            "Notes",
            "Done"
        };

        vitals(c);
        switch (ui_menu("  At the table:", what, NULL, 13)) {
        case 0:  ui_header("Character Sheet"); print_sheet(c); break;
        case 1:  hurt_and_heal(c);      break;
        case 2:  take_a_rest(c);        break;
        case 3:  cast_and_slots(c);     break;
        case 4:  track_resources(c);    break;
        case 5:  set_conditions(c);     break;
        case 6:  money(c);              break;
        case 7:  shop_visit(c);         break;
        case 8:  spend_at_the_inn(c);   break;
        case 9:  manage_inventory(c);   break;
        case 10: wear_and_repair(c);    break;
        case 11: edit_details(c);       break;
        default: return;
        }
    }
}
