/* saveload.c -- the character file: a readable sheet plus a data block.
 *
 * The sheet at the top is for the table. Everything below the marker is a
 * key/value block the loader parses to rebuild the character exactly, so a
 * saved file can be levelled up later. Names rather than table indices are
 * stored so the file stays valid if the game data is extended.
 */
#include "saveload.h"
#include "ui.h"
#include "data.h"
#include "sidekick.h"
#include "build.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DATA_BEGIN "#BEGIN-DNDDATA v1"
#define DATA_END   "#END-DNDDATA"

/* ------------------------------------------------------------- name lookups */

static int index_of_race(const char *n)
{
    int i;
    for (i = 0; i < RACE_COUNT; i++) if (!strcmp(RACES[i].name, n)) return i;
    return -1;
}
static int index_of_subrace(const char *n)
{
    int i;
    for (i = 0; i < SUBRACE_COUNT; i++) if (!strcmp(SUBRACES[i].name, n)) return i;
    return -1;
}
static int index_of_background(const char *n)
{
    int i;
    for (i = 0; i < BACKGROUND_COUNT; i++) {
        if (!strcmp(BACKGROUNDS[i].name, n)) return i;
    }
    return -1;
}
static int index_of_feat(const char *n)
{
    int i;
    for (i = 0; i < FEAT_COUNT; i++) if (!strcmp(FEATS[i].name, n)) return i;
    return -1;
}
static int index_of_spell(const char *n)
{
    int i;
    for (i = 0; i < SPELL_COUNT; i++) if (!strcmp(SPELLS[i].name, n)) return i;
    return -1;
}
static int index_of_alignment(const char *n)
{
    int i;
    for (i = 0; i < ALIGN_COUNT; i++) {
        if (!strcmp(ALIGNMENT_NAME[i], n)) return i;
    }
    return ALIGN_TN;
}

/* ------------------------------------------------------------ sheet writing */

static void hr(FILE *f)
{
    fprintf(f, "================================================================\n");
}

static void section(FILE *f, const char *title)
{
    fprintf(f, "\n----------------------------------------------------------------\n");
    fprintf(f, " %s\n", title);
    fprintf(f, "----------------------------------------------------------------\n");
}

/* Wraps a long line of prose to the sheet's width at a given indent. The
   sheet is compared against itself by tools/roundtrip.py, so this has to be
   deterministic -- it breaks only at spaces and never rewrites the text. */
/* A saved character stores names, not table indices, which is what lets the
   game data grow without invalidating files. The cost is that a name the
   banks no longer hold -- homebrew the DM has since removed, or a book
   switched off -- would otherwise vanish from the sheet without a word. */
static void warn_unknown(const char *kind, const char *name)
{
    fprintf(stderr, "  note: this character's %s \"%s\" is not in the "
                    "current data; it was left out.\n", kind, name);
}

static void wrap_to(FILE *f, const char *text, int indent)
{
    const int width = 76;
    int col = 0;
    const char *p = text;

    while (*p) {
        const char *word = p;
        int len = 0;

        while (*p && *p != ' ') { p++; len++; }
        while (*p == ' ') p++;

        if (col == 0) {
            fprintf(f, "%*s", indent, "");
            col = indent;
        } else if (col + 1 + len > width) {
            fprintf(f, "\n%*s", indent, "");
            col = indent;
        } else {
            fputc(' ', f);
            col++;
        }
        fprintf(f, "%.*s", len, word);
        col += len;
    }
    if (col) fputc('\n', f);
}


static void class_line(const Character *c, char *out, size_t n)
{
    int i;
    size_t used = 0;

    out[0] = '\0';
    for (i = 0; i < c->class_count; i++) {
        const char *sub = (c->classes[i].subclass_id >= 0)
                        ? SUBCLASSES[c->classes[i].subclass_id].name : NULL;
        int w = snprintf(out + used, n - used, "%s%s %d%s%s%s",
                         i ? " / " : "",
                         CLASSES[c->classes[i].class_id].name,
                         c->classes[i].level,
                         sub ? " (" : "", sub ? sub : "", sub ? ")" : "");
        if (w < 0 || (size_t)w >= n - used) break;
        used += (size_t)w;
    }
}

/* Prints every feature the character has earned, in class then level order. */
static void write_features(FILE *f, const Character *c)
{
    int i, k;

    for (i = 0; i < c->class_count; i++) {
        int id = c->classes[i].class_id;
        int lvl = c->classes[i].level;
        int sub = c->classes[i].subclass_id;

        fprintf(f, "\n  %s %d\n", CLASSES[id].name, lvl);
        for (k = 0; k < FEATURE_COUNT; k++) {
            const FeatureData *fd = &FEATURES[k];
            if (fd->class_id != id) continue;
            if (fd->level > lvl) continue;
            if (fd->subclass_id != -1 && fd->subclass_id != sub) continue;
            fprintf(f, "    %2d  %-32s %s\n", fd->level, fd->name, fd->summary);
        }
    }
}

static void write_spells(FILE *f, const Character *c)
{
    int slots[10];
    int i, lvl, eff, pact_n, pact_l;

    if (c->spell_count == 0) {
        int any = 0;
        for (i = 0; i < c->class_count; i++) {
            if (CLASSES[c->classes[i].class_id].caster != CAST_NONE) any = 1;
        }
        if (!any) return;
    }

    section(f, "SPELLCASTING");

    for (i = 0; i < c->class_count; i++) {
        int id = c->classes[i].class_id;
        if (CLASSES[id].caster == CAST_NONE
            && c->classes[i].subclass_id != 15
            && c->classes[i].subclass_id != 26) continue;
        fprintf(f, "  %-10s ability %-13s save DC %2d   attack +%d\n",
                CLASSES[id].name, ABILITY_NAME[CLASSES[id].spell_ability],
                spell_save_dc(c, id), spell_attack_bonus(c, id));
    }

    eff = spell_slots_for(c, slots);
    if (eff > 0) {
        fprintf(f, "\n  Spell slots (caster level %d):\n   ", eff);
        for (lvl = 1; lvl <= 9; lvl++) {
            if (slots[lvl]) fprintf(f, "  %d%s: %d", lvl,
                                    lvl == 1 ? "st" : lvl == 2 ? "nd"
                                    : lvl == 3 ? "rd" : "th", slots[lvl]);
        }
        fprintf(f, "\n");
    }
    if (pact_slots_for(c, &pact_n, &pact_l)) {
        fprintf(f, "  Pact Magic: %d slot%s of level %d "
                   "(recharges on a short rest)\n",
                pact_n, pact_n == 1 ? "" : "s", pact_l);
    }

    if (c->spell_count == 0) return;

    for (lvl = 0; lvl <= 9; lvl++) {
        int printed = 0;
        for (i = 0; i < c->spell_count; i++) {
            const SpellData *s = &SPELLS[c->spells[i].spell_id];
            if (s->level != lvl) continue;

            if (!printed) {
                if (lvl == 0) fprintf(f, "\n  Cantrips\n");
                else fprintf(f, "\n  Level %d\n", lvl);
                printed = 1;
            }
            fprintf(f, "    %-26s %s%s%s%s%s\n", s->name,
                    SCHOOL_NAMES[s->school],
                    s->ritual ? ", ritual" : "",
                    c->spells[i].always_prepared ? ", always prepared" : "",
                    c->class_count > 1 ? " -- " : "",
                    c->class_count > 1
                        ? (c->spells[i].class_id < 0
                               ? "from a feat"
                               : CLASSES[c->spells[i].class_id].name)
                        : "");
            fprintf(f, "        Casting Time: %-18s Range: %s\n",
                    s->casting_time, s->range);
            fprintf(f, "        Components:   %s\n", s->components);
            fprintf(f, "        Duration:     %s\n", s->duration);
        }
    }
}

static void write_sheet(FILE *f, const Character *c)
{
    char buf[512];
    int i, w;

    hr(f);
    fprintf(f, " %s\n", c->name);
    class_line(c, buf, sizeof buf);
    fprintf(f, " %s\n", buf);
    hr(f);

    fprintf(f, " Player: %-24s Race: %s%s%s\n",
            c->player[0] ? c->player : "-",
            c->race_id >= 0 ? RACES[c->race_id].name : "-",
            c->subrace_id >= 0 ? " / " : "",
            c->subrace_id >= 0 ? SUBRACES[c->subrace_id].name : "");
    fprintf(f, " Background: %-20s Alignment: %s\n",
            c->background_id >= 0 ? BACKGROUNDS[c->background_id].name
                : (c->background_name[0] ? c->background_name : "-"),
            ALIGNMENT_NAME[c->alignment]);
    fprintf(f, " Level: %-25d Proficiency Bonus: +%d\n",
            total_level(c), proficiency_bonus(c));
    {
        char sbuf[256];
        settings_summary(&SETTINGS, sbuf, sizeof sbuf);
        fprintf(f, " Sources: ");
        fprintf(f, "%s\n", sbuf);
    }
    if (c->ancestry_id >= 0) {
        fprintf(f, " Draconic Ancestry: %s (%s, %s)\n",
                ANCESTRIES[c->ancestry_id].dragon,
                ANCESTRIES[c->ancestry_id].damage,
                ANCESTRIES[c->ancestry_id].breath);
    }

    section(f, "ABILITY SCORES");
    for (i = 0; i < ABL_COUNT; i++) {
        int score = ability_score(c, (Ability)i);
        int mod = ability_mod(c, (Ability)i);
        fprintf(f, "  %-14s %2d  (%+d)     Saving throw %+d%s\n",
                ABILITY_NAME[i], score, mod, save_bonus(c, (Ability)i),
                c->save_prof[i] ? "  (proficient)" : "");
    }

    section(f, "COMBAT");
    fprintf(f, "  Armor Class      %d\n", armour_class(c));
    fprintf(f, "  Initiative       %+d\n", initiative_bonus(c));
    fprintf(f, "  Speed            %d ft.", speed_of(c));
    {
        /* Movement a magic item grants, alongside the walking speed. */
        int fly = magic_fly_speed(c), swim = magic_swim_speed(c);
        int climb = magic_climb_speed(c);
        if (fly)   fprintf(f, ", fly %d ft.", fly);
        if (swim)  fprintf(f, ", swim %d ft.", swim);
        if (climb) fprintf(f, ", climb %d ft.", climb);
    }
    fprintf(f, "\n");
    {
        char defences[256];
        if (magic_defences(c, defences, sizeof defences)) {
            fprintf(f, "  From your gear:  %s\n", defences);
        }
    }
    fprintf(f, "  Hit Points       %d\n", hit_points_max(c));
    fprintf(f, "  Hit Dice         ");
    for (i = 0; i < c->class_count; i++) {
        fprintf(f, "%s%dd%d", i ? " + " : "", c->classes[i].level,
                CLASSES[c->classes[i].class_id].hit_die);
    }
    fprintf(f, "\n");
    fprintf(f, "  Passive Perception %d\n", passive_perception(c));
    {
        /* What has been earned, and what the next level costs. The two are
           printed together because the useful number is the difference. */
        int lv = total_level(c);
        if (SETTINGS.experience && lv >= 0 && lv <= MAX_LEVEL) {
            fprintf(f, "  Experience       %d", c->xp);
            if (lv < MAX_LEVEL) {
                int need = XP_FOR_LEVEL[lv + 1] - c->xp;
                fprintf(f, " -- %d for level %d", XP_FOR_LEVEL[lv + 1],
                        lv + 1);
                if (need > 0) fprintf(f, ", %d to go", need);
                else fprintf(f, ", enough to level up");
            } else {
                fprintf(f, " -- 20th level is the maximum");
            }
            fprintf(f, "\n");
        }
    }

    {
        Attack atk[MAX_ATTACKS];
        int n = attacks_of(c, atk, MAX_ATTACKS);
        if (n) {
            section(f, "ATTACKS");
            for (i = 0; i < n; i++) {
                fprintf(f, "  %-24s %+3d to hit   %s\n",
                        atk[i].name, atk[i].bonus, atk[i].damage);
                if (atk[i].note[0]) {
                    fprintf(f, "        %s\n", atk[i].note);
                }
            }
            fprintf(f, "\n  A fighting style or a feature that adds to one "
                       "of these -- Dueling,\n  Great Weapon Fighting, "
                       "Sneak Attack, Rage -- is listed under class\n"
                       "  features and applies on top.\n");
        }
    }

    section(f, "SKILLS");
    for (i = 0; i < SKL_COUNT; i++) {
        fprintf(f, "  %c%c %-18s %+d  (%s)\n",
                c->skill_prof[i] ? '*' : ' ',
                c->skill_expertise[i] ? 'E' : ' ',
                SKILL_NAME[i], skill_bonus(c, (Skill)i),
                ABILITY_ABBREV[SKILL_ABILITY[i]]);
    }
    fprintf(f, "  (* proficient, E expertise)\n");

    section(f, "PROFICIENCIES AND LANGUAGES");
    fprintf(f, "  Armor and weapons:\n");
    for (i = 0, w = 0; i < c->other_prof_count; i++) {
        fprintf(f, "%s%s", w ? ", " : "    ", c->other_profs[i]);
        w = 1;
    }
    fprintf(f, "%s\n", c->other_prof_count ? "" : "    none");
    fprintf(f, "  Tools:\n");
    for (i = 0, w = 0; i < c->tool_prof_count; i++) {
        fprintf(f, "%s%s", w ? ", " : "    ", c->tool_profs[i]);
        w = 1;
    }
    fprintf(f, "%s\n", c->tool_prof_count ? "" : "    none");
    fprintf(f, "  Languages:\n");
    for (i = 0, w = 0; i < c->language_count; i++) {
        fprintf(f, "%s%s", w ? ", " : "    ", c->languages[i]);
        w = 1;
    }
    fprintf(f, "\n");

    if (c->race_id >= 0) {
        section(f, "RACIAL TRAITS");
        {
            char tbuf[2048];
            const char *parts[16];
            int n = 0, k;
            n = 0;
            strncpy(tbuf, RACES[c->race_id].traits, sizeof tbuf - 1);
            tbuf[sizeof tbuf - 1] = '\0';
            {
                char *p = tbuf;
                parts[n++] = p;
                while ((p = strchr(p, '|')) != NULL && n < 16) {
                    *p++ = '\0';
                    parts[n++] = p;
                }
            }
            for (k = 0; k < n; k++) fprintf(f, "  - %s\n", parts[k]);
        }
        if (c->subrace_id >= 0 && SUBRACES[c->subrace_id].traits[0]) {
            char tbuf[1024];
            char *p;
            strncpy(tbuf, SUBRACES[c->subrace_id].traits, sizeof tbuf - 1);
            tbuf[sizeof tbuf - 1] = '\0';
            p = tbuf;
            fprintf(f, "  - %s\n", p);
            while ((p = strchr(p, '|')) != NULL) {
                *p++ = '\0';
                fprintf(f, "  - %s\n", p);
            }
        }
    }

    if (c->background_id >= 0) {
        section(f, "BACKGROUND FEATURE");
        fprintf(f, "  %s: %s\n", BACKGROUNDS[c->background_id].feature_name,
                BACKGROUNDS[c->background_id].feature_summary);
    } else if (c->background_feature[0]) {
        fprintf(f, "  %s: %s\n", c->background_feature,
                c->background_feature_text);
    }

    section(f, "CLASS FEATURES");
    write_features(f, c);

    {
        int art = class_level_of(c, class_by_name("Artificer"));
        if (art >= 2) {
            section(f, "ARTIFICER INFUSIONS");
            fprintf(f, "  Infusions known: %d   Items infused at once: %d\n",
                    (int)INFUSIONS_KNOWN[art], (int)INFUSED_ITEMS[art]);
            for (i = 0; i < c->choice_count; i++) {
                if (strcmp(c->choices[i].label, "Infusion") != 0) continue;
                fprintf(f, "    %s\n", c->choices[i].value);
            }
        }
    }

    if (c->feat_count) {
        section(f, "FEATS");
        for (i = 0; i < c->feat_count; i++) {
            fprintf(f, "  %s\n      %s\n", FEATS[c->feats[i]].name,
                    FEATS[c->feats[i]].summary);
        }
    }

    {
        int optional = 0;
        for (i = 0; i < c->choice_count; i++) {
            if (strcmp(c->choices[i].label, "Optional feature") == 0) optional++;
        }
        if (optional) {
            section(f, "OPTIONAL CLASS FEATURES (TASHA'S)");
            for (i = 0; i < c->choice_count; i++) {
                if (strcmp(c->choices[i].label, "Optional feature") != 0) continue;
                fprintf(f, "  %s\n", c->choices[i].value);
            }
        }
    }

    {
        /* Infusions and optional features have their own sections above. */
        int others = 0;
        for (i = 0; i < c->choice_count; i++) {
            if (strcmp(c->choices[i].label, "Infusion") == 0) continue;
            if (strcmp(c->choices[i].label, "Optional feature") == 0) continue;
            others++;
        }
        if (others) {
            section(f, "CHOICES");
            for (i = 0; i < c->choice_count; i++) {
                if (strcmp(c->choices[i].label, "Infusion") == 0) continue;
                if (strcmp(c->choices[i].label, "Optional feature") == 0) continue;
                fprintf(f, "  %-22s %s\n", c->choices[i].label,
                        c->choices[i].value);
            }
        }
    }

    write_spells(f, c);

    section(f, "EQUIPMENT");
    for (i = 0; i < c->item_count; i++) {
        const ItemData *it;
        if (c->inventory[i].is_magic) continue;
        it = &ITEMS[c->inventory[i].item_id];
        fprintf(f, "  %3d x %-26s%s", c->inventory[i].quantity, it->name,
                c->inventory[i].equipped ? " (equipped)" : "");
        if (it->damage[0] && strcmp(it->damage, "-")) {
            fprintf(f, "  %s %s", it->damage, it->damage_type);
        }
        fprintf(f, "\n");
        if (it->contents[0]) fprintf(f, "        contains: %s\n", it->contents);
    }
    fprintf(f, "\n  Coins: %d pp, %d gp, %d ep, %d sp, %d cp\n",
            c->platinum, c->gold, c->electrum, c->silver, c->copper);
    fprintf(f, "  Carried weight: %d.%d lb of a %d lb capacity\n",
            current_weight_tenths(c) / 10, current_weight_tenths(c) % 10,
            carrying_capacity(c));
    {
        /* The PHB's optional encumbrance rule (p.176), which most tables
           that use it want on the sheet rather than in their heads. */
        int str = ability_score(c, ABL_STR);
        int carried = current_weight_tenths(c);
        fprintf(f, "  Encumbered above %d lb, heavily encumbered above %d lb"
                   " (variant rule)%s\n",
                str * 5, str * 10,
                carried > str * 100 ? " -- you are heavily encumbered"
                    : carried > str * 50 ? " -- you are encumbered" : "");
    }

    /* Magic items get their own section with what each one does, since the
       whole point of carrying one is the rule it brings. */
    {
        int magic = 0;
        for (i = 0; i < c->item_count; i++) {
            if (c->inventory[i].is_magic) magic++;
        }
        if (magic) {
            section(f, "MAGIC ITEMS");
            fprintf(f, "  Attuned to %d of %d\n", attuned_count(c),
                    MAX_ATTUNED);
            {
                /* Say which ones the numbers above already account for, so
                   nobody counts a ring of protection twice. */
                int counted = 0, k;
                for (k = 0; k < c->item_count; k++) {
                    const MagicItem *mm;
                    const MagicRule *rr;
                    if (!c->inventory[k].is_magic) continue;
                    mm = &MAGIC_ITEMS[c->inventory[k].item_id];
                    rr = magic_rule_for(mm->name);
                    if (!rr) continue;
                    if (mm->attunement && !c->inventory[k].attuned) continue;
                    if ((rr->armor_base || rr->shield)
                        && !c->inventory[k].equipped) continue;
                    if (!counted) {
                        fprintf(f, "\n  Already counted in the Armor Class "
                                   "and saving throws above:\n");
                        counted = 1;
                    }
                    fprintf(f, "    %s\n", mm->name);
                }
            }
            fprintf(f, "\n  Anything not listed as counted is applied at the "
                       "table: most of what a magic\n"
                       "  item grants depends on being worn, charged, or in "
                       "the right situation.\n");
            for (i = 0; i < c->item_count; i++) {
                const MagicItem *m;
                const InventoryEntry *e = &c->inventory[i];
                if (!e->is_magic) continue;
                m = &MAGIC_ITEMS[e->item_id];
                fprintf(f, "\n  %d x %s%s\n", e->quantity,
                        m->name, e->attuned ? " (attuned)" : "");
                /* An item the table has not identified for this character
                   prints as a name and nothing else. What it does is still
                   done -- the numbers reached Armor Class and the attacks
                   block above -- but the sheet does not say why, which is
                   the point of handing one over unidentified. */
                if (e->concealed) {
                    fprintf(f, "    (not yet identified)\n");
                    continue;
                }
                fprintf(f, "    %s, %s%s%s\n", m->type, m->rarity,
                        m->attunement ? " -- " : "",
                        m->attunement ? m->attunement : "");
                wrap_to(f, m->text, 4);
                if (m->curse && !e->curse_hidden) wrap_to(f, m->curse, 4);
            }
        }
    }

    if (c->sidekick_count) {
        section(f, "SIDEKICKS");
        for (i = 0; i < c->sidekick_count; i++) {
            if (i) fprintf(f, "\n");
            print_sidekick(f, &c->sidekicks[i], 2);
        }
    }

    section(f, "PERSONALITY");
    fprintf(f, "  Age %d, %d ft %d in, %d lb, %s eyes, %s skin, %s hair\n",
            c->age, c->height_in / 12, c->height_in % 12, c->weight_lb,
            c->eyes, c->skin, c->hair);
    if (c->trait[0])      fprintf(f, "\n  Trait:  %s\n", c->trait);
    if (c->ideal[0])      fprintf(f, "  Ideal:  %s\n", c->ideal);
    if (c->bond[0])       fprintf(f, "  Bond:   %s\n", c->bond);
    if (c->flaw[0])       fprintf(f, "  Flaw:   %s\n", c->flaw);
    if (c->appearance[0]) {
        fprintf(f, "\n  Appearance:\n");
        wrap_to(f, c->appearance, 4);
    }
    if (c->backstory[0]) {
        fprintf(f, "\n  Backstory:\n");
        wrap_to(f, c->backstory, 4);
    }

    if (c->note_count) {
        int k;
        section(f, "NOTES");
        for (k = 0; k < c->note_count; k++) {
            /* A note may run to paragraphs, so its title heads it and the
               body keeps the breaks it was typed with. */
            char buf[MAX_LORE], *start, *p;

            fprintf(f, "\n  %d. %s\n", k + 1, c->notes[k].title);
            /* A one-line note is its own title; do not print it twice. */
            if (!strcmp(c->notes[k].title, c->notes[k].body)) continue;
            snprintf(buf, sizeof buf, "%s", c->notes[k].body);
            start = buf;
            for (p = buf;; p++) {
                if (*p == '\n' || *p == '\0') {
                    int end = (*p == '\0');
                    *p = '\0';
                    if (*start) wrap_to(f, start, 6);
                    else fprintf(f, "\n");
                    if (end) break;
                    start = p + 1;
                }
            }
        }
    }
}

/* -------------------------------------------------------- the data block */

/* The block is one record per line and '|' separated, so any text the player
   typed has to be written with those two characters escaped, and with the
   escape itself escaped. Notes did this from the start, a note being the one
   field expected to run to paragraphs. Everything else was written raw, and
   a single '|' in a name, an appearance or a tool a table invented shifted
   every field after it: the character came back as whatever was left before
   the stray separator, silently, with the rest dropped. */
void record_put(FILE *f, const char *s)
{
    for (; s && *s; s++) {
        if (*s == '\n')      fputs("\\n", f);
        else if (*s == '|')  fputs("\\p", f);
        else if (*s == '\\') fputs("\\\\", f);
        else                 fputc(*s, f);
    }
}

/* A record of one text field, which is most of them. */
static void put_line(FILE *f, const char *tag, const char *text)
{
    fprintf(f, "%s|", tag);
    record_put(f, text);
    fputc('\n', f);
}

/* Reverses put_text() in place; the result is never longer than the input.
   An escape the writer never emits keeps the character and loses the
   backslash, which is what notes have always done. */
void record_unescape(char *s)
{
    char *out = s;

    for (; *s; s++) {
        if (*s == '\\' && s[1]) {
            s++;
            *out++ = (*s == 'n') ? '\n' : (*s == 'p') ? '|' : *s;
        } else {
            *out++ = *s;
        }
    }
    *out = '\0';
}

static void write_data(FILE *f, const Character *c)
{
    int i;

    fprintf(f, "\n\n");
    hr(f);
    fprintf(f, " MACHINE-READABLE DATA -- the creator reads this to reload\n");
    fprintf(f, " the character for levelling up. Editing it by hand is fine,\n"
               " but keep the field names and the '|' separators intact.\n");
    hr(f);
    fprintf(f, "%s\n", DATA_BEGIN);

    /* The books are named rather than counted off, so that adding one to
       the enum does not silently shift every flag in an older file. */
    fprintf(f, "SETTINGS|");
    {
        int w = 0;
        for (i = 0; i < BOOK_COUNT; i++) {
            if (!SETTINGS.book[i]) continue;
            fprintf(f, "%s%s", w++ ? "," : "", BOOK_ABBREV[i]);
        }
        if (!w) fprintf(f, "%s", BOOK_ABBREV[BOOK_PHB]);
    }
    fprintf(f, "|%d|%d|%d|%d|%d|%d\n", SETTINGS.custom_origins,
            SETTINGS.optional_features, SETTINGS.multiclassing,
            SETTINGS.feats, SETTINGS.experience, SETTINGS.manual_dice);
    put_line(f, "NAME", c->name);
    put_line(f, "PLAYER", c->player);
    if (c->race_id >= 0) fprintf(f, "RACE|%s\n", RACES[c->race_id].name);
    if (c->subrace_id >= 0) {
        fprintf(f, "SUBRACE|%s\n", SUBRACES[c->subrace_id].name);
    }
    if (c->background_id >= 0) {
        fprintf(f, "BACKGROUND|%s\n", BACKGROUNDS[c->background_id].name);
    } else if (c->background_name[0]) {
        /* A background built with the customization rules has no table row
           to point at, so everything it granted is written out. */
        fputs("CUSTOMBG|", f);
        record_put(f, c->background_name);
        fputc('|', f);
        record_put(f, c->background_feature);
        fputc('|', f);
        record_put(f, c->background_feature_text);
        fputc('|', f);
        record_put(f, c->background_equipment);
        fputc('\n', f);
    }
    fprintf(f, "ALIGNMENT|%s\n", ALIGNMENT_NAME[c->alignment]);
    if (c->ancestry_id >= 0) {
        fprintf(f, "ANCESTRY|%s\n", ANCESTRIES[c->ancestry_id].dragon);
    }

    for (i = 0; i < c->class_count; i++) {
        fprintf(f, "CLASS|%s|%d|%s|%d\n",
                CLASSES[c->classes[i].class_id].name,
                c->classes[i].level,
                c->classes[i].subclass_id >= 0
                    ? SUBCLASSES[c->classes[i].subclass_id].name : "-",
                c->classes[i].subclass_option);
    }

    fprintf(f, "BASE");
    for (i = 0; i < ABL_COUNT; i++) fprintf(f, "|%d", c->base_score[i]);
    fprintf(f, "\nRACIALBONUS");
    for (i = 0; i < ABL_COUNT; i++) fprintf(f, "|%d", c->racial_bonus[i]);
    fprintf(f, "\nASIBONUS");
    for (i = 0; i < ABL_COUNT; i++) fprintf(f, "|%d", c->asi_bonus[i]);
    fprintf(f, "\nSAVEPROF");
    for (i = 0; i < ABL_COUNT; i++) fprintf(f, "|%d", c->save_prof[i]);
    fprintf(f, "\n");

    for (i = 0; i < SKL_COUNT; i++) {
        if (c->skill_prof[i] || c->skill_expertise[i]) {
            fprintf(f, "SKILL|%s|%d|%d\n", SKILL_NAME[i],
                    c->skill_prof[i], c->skill_expertise[i]);
        }
    }

    for (i = 0; i < c->language_count; i++) {
        put_line(f, "LANG", c->languages[i]);
    }
    for (i = 0; i < c->tool_prof_count; i++) {
        put_line(f, "TOOL", c->tool_profs[i]);
    }
    for (i = 0; i < c->other_prof_count; i++) {
        put_line(f, "PROF", c->other_profs[i]);
    }
    for (i = 0; i < c->feat_count; i++) {
        fprintf(f, "FEAT|%s\n", FEATS[c->feats[i]].name);
    }
    for (i = 0; i < c->choice_count; i++) {
        fprintf(f, "CHOICE|");
        record_put(f, c->choices[i].label);
        fputc('|', f);
        record_put(f, c->choices[i].value);
        fputc('\n', f);
    }
    for (i = 0; i < c->item_count; i++) {
        if (c->inventory[i].is_magic) {
            fprintf(f, "MAGICITEM|%d|%d|", c->inventory[i].quantity,
                    c->inventory[i].attuned);
            record_put(f, MAGIC_ITEMS[c->inventory[i].item_id].name);
            fprintf(f, "|%d|%d|", c->inventory[i].plus,
                    c->inventory[i].equipped);
            record_put(f, c->inventory[i].variant);
            fprintf(f, "|%d|%d", c->inventory[i].concealed,
                    c->inventory[i].curse_hidden);
            fputc('\n', f);
        } else {
            fprintf(f, "ITEM|%d|%d|", c->inventory[i].quantity,
                    c->inventory[i].equipped);
            record_put(f, ITEMS[c->inventory[i].item_id].name);
            fputc('\n', f);
        }
    }
    if (c->xp) fprintf(f, "XP|%d\n", c->xp);
    fprintf(f, "COINS|%d|%d|%d|%d|%d\n", c->copper, c->silver, c->electrum,
            c->gold, c->platinum);
    for (i = 0; i < c->sidekick_count; i++) {
        const Sidekick *sk = &c->sidekicks[i];
        int k;
        fputs("SIDEKICK|", f);
        record_put(f, sk->name);
        fputc('|', f);
        record_put(f, sk->creature);
        fprintf(f, "|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|",
                SIDEKICK_CLASS_NAME[sk->cls],
                sk->level, sk->role, sk->abilities[0], sk->abilities[1],
                sk->abilities[2], sk->abilities[3], sk->abilities[4],
                sk->abilities[5], sk->hp);
        record_put(f, sk->speed);
        fputc('\n', f);
        fputs("SKAC|", f);
        record_put(f, sk->name);
        fprintf(f, "|%d\n", sk->ac);
        for (k = 0; k < sk->choice_count; k++) {
            fputs("SKCHOICE|", f);
            record_put(f, sk->name);
            fputc('|', f);
            record_put(f, sk->choices[k].label);
            fputc('|', f);
            record_put(f, sk->choices[k].value);
            fputc('\n', f);
        }
        for (k = 0; k < sk->spell_count; k++) {
            fputs("SKSPELL|", f);
            record_put(f, sk->name);
            fputc('|', f);
            record_put(f, SPELLS[sk->spells[k]].name);
            fputc('\n', f);
        }
    }
    for (i = 0; i < c->spell_count; i++) {
        /* A spell a feat granted belongs to no class, and must not be
           counted against any class's spells known when it is read back. */
        fprintf(f, "SPELL|%d|%d|%s|%s\n", c->spells[i].prepared,
                c->spells[i].always_prepared,
                c->spells[i].class_id < 0
                    ? "-" : CLASSES[c->spells[i].class_id].name,
                SPELLS[c->spells[i].spell_id].name);
    }

    fprintf(f, "HPROLLS|%d", c->hp_roll_count);
    for (i = 0; i < c->hp_roll_count; i++) fprintf(f, "|%d", c->hp_rolls[i]);
    fprintf(f, "\n");

    fprintf(f, "BODY|%d|%d|%d|", c->age, c->height_in, c->weight_lb);
    record_put(f, c->eyes);
    fputc('|', f);
    record_put(f, c->skin);
    fputc('|', f);
    record_put(f, c->hair);
    fputc('\n', f);
    put_line(f, "TRAIT", c->trait);
    put_line(f, "IDEAL", c->ideal);
    put_line(f, "BOND", c->bond);
    put_line(f, "FLAW", c->flaw);
    put_line(f, "APPEARANCE", c->appearance);
    {
        int k;
        for (k = 0; k < c->note_count; k++) {
            fputs("NOTE|", f);
            record_put(f, c->notes[k].title);
            fputc('|', f);
            record_put(f, c->notes[k].body);
            fputc('\n', f);
        }
    }
    put_line(f, "BACKSTORY", c->backstory);

    fprintf(f, "%s\n", DATA_END);
}

/* ------------------------------------------------------------------ saving */

/* Whether a file already holds a character. Used before writing a sheet
   that is not one, so that it cannot be written over the top of one. */
int file_is_character(const char *path)
{
    char line[256];
    FILE *f = fopen(path, "r");
    int found = 0;

    if (!f) return 0;
    while (!found && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (!strcmp(line, DATA_BEGIN)) found = 1;
    }
    fclose(f);
    return found;
}

void sheet_filename(const char *name, char *out, size_t n)
{
    size_t i, k = 0;
    for (i = 0; name[i] && k + 1 < n; i++) {
        unsigned char ch = (unsigned char)name[i];
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?'
            || ch == '"' || ch == '<' || ch == '>' || ch == '|' || ch < 32) {
            continue;
        }
        out[k++] = (char)ch;
    }
    out[k] = '\0';
    if (k == 0) {
        strncpy(out, "Unnamed", n - 1);
        out[n - 1] = '\0';
    }
}

int save_character(const Character *c, char *path, size_t pathsz)
{
    char safe[MAX_NAME];
    FILE *f;

    sheet_filename(c->name, safe, sizeof safe);
    snprintf(path, pathsz, "%s.txt", safe);

    f = fopen(path, "w");
    if (!f) return -1;

    write_sheet(f, c);
    write_data(f, c);
    fclose(f);
    return 0;
}

void print_sheet(const Character *c)
{
    write_sheet(stdout, c);
}

/* ----------------------------------------------------------------- loading */

/* Splits a '|' separated record in place. Returns the field count. */
/* A number out of one of those fields, held to a range. A hand-edited file
 * is not an attack, but it is not checked either, and an unbounded number
 * here became undefined behaviour further on: a quantity of two billion
 * multiplied by an item's weight, five coin counts of two billion added
 * together, a hit point roll of two billion added to the last. strtol
 * rather than atoi because atoi is itself undefined on a number too large
 * to hold, which is the case this exists for.
 */
int record_int(const char *field, int lo, int hi)
{
    long v = strtol(field, NULL, 10);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return (int)v;
}

int record_split(char *line, char **out, int max)
{
    int n = 0;
    char *p = line;

    out[n++] = p;
    while ((p = strchr(p, '|')) != NULL && n < max) {
        *p++ = '\0';
        out[n++] = p;
    }
    return n;
}

static void copy_field(char *dst, size_t n, const char *src)
{
    strncpy(dst, src ? src : "", n - 1);
    dst[n - 1] = '\0';
}

/* The sidekick a SKAC, SKCHOICE or SKSPELL record belongs to.
 *
 * The writer emits a SIDEKICK record and then that sidekick's own records
 * immediately after it, so the one being read is almost always the one most
 * recently added -- and that is what has to decide it, because a name does
 * not. Two sidekicks may share a name, and when they did, every record for
 * either was applied to both: the first of them ended up with the last one's
 * Armor Class, and with both of their choices and spells. Matching by name
 * is kept only as the answer for a file whose records were reordered by
 * hand, which the program itself never writes. */
static Sidekick *sidekick_named(Character *c, const char *name)
{
    int k;

    if (c->sidekick_count > 0) {
        Sidekick *last = &c->sidekicks[c->sidekick_count - 1];
        if (!strcmp(last->name, name)) return last;
    }
    for (k = 0; k < c->sidekick_count; k++) {
        if (!strcmp(c->sidekicks[k].name, name)) return &c->sidekicks[k];
    }
    return NULL;
}

int load_character(const char *path, Character *c)
{
    FILE *f = fopen(path, "r");
    /* Long enough for the longest record the writer can produce: a note,
       whose title and body are both escaped and so can double in length.
       At 1024 a note of more than about a thousand characters was cut in
       half, and the half that was left over was read as a record of its
       own. */
    char line[2 * MAX_LORE + 2 * MAX_NAME + 64];
    int in_data = 0;

    if (!f) return -1;

    memset(c, 0, sizeof *c);
    c->race_id = c->subrace_id = c->background_id = c->ancestry_id = -1;

    while (fgets(line, sizeof line, f)) {
        char *fields[32];
        int n, field;

        /* A hand-edited record longer than the buffer would arrive as two,
           and its tail would be read as a record of its own. Drop the tail
           and say so rather than inventing a record out of it. */
        if (!strchr(line, '\n')) {
            int ch, dropped = 0;
            while ((ch = fgetc(f)) != EOF && ch != '\n') dropped++;
            if (dropped) {
                fprintf(stderr, "  note: a line in %s was too long to read; "
                                "%d characters were dropped.\n", path, dropped);
            }
        }
        line[strcspn(line, "\r\n")] = '\0';

        if (!in_data) {
            if (!strcmp(line, DATA_BEGIN)) in_data = 1;
            continue;
        }
        if (!strcmp(line, DATA_END)) break;
        if (line[0] == '\0') continue;

        n = record_split(line, fields, 32);
        if (n < 1) continue;
        /* Every field is written escaped, so every field is read unescaped.
           One that carries no escape is unchanged, which is what makes a
           file written before this still read correctly. */
        for (field = 0; field < n; field++) record_unescape(fields[field]);

        if (!strcmp(fields[0], "SETTINGS") && n >= 2) {
            /* Two shapes. Files written now name the books they allow. Older
               ones carried one flag per book in enum order, which is exactly
               why this is stored by name instead: adding SCAG to the middle
               of that enum moved every book after it, so a positional read
               would take an old file's Homebrew flag for SCAG's. An older
               file is known by its first field being a digit, and its flags
               are read against the enum as it stood when they were written. */
            static const int LEGACY[] = {
                BOOK_PHB, BOOK_XGE, BOOK_TCE, BOOK_DMG,
                BOOK_MPMM, BOOK_MM, BOOK_HOMEBREW
            };
            const int legacy_books = (int)(sizeof LEGACY / sizeof LEGACY[0]);
            int k, at;

            if (fields[1][0] >= '0' && fields[1][0] <= '9') {
                /* Books the older file never knew about stay switched on. */
                for (k = 0; k < BOOK_COUNT; k++) SETTINGS.book[k] = 1;
                for (k = 0; k < legacy_books && k + 1 < n; k++) {
                    SETTINGS.book[LEGACY[k]] = atoi(fields[k + 1]);
                }
                at = legacy_books + 1;
            } else {
                char *p = fields[1];
                for (k = 0; k < BOOK_COUNT; k++) SETTINGS.book[k] = 0;
                while (p && *p) {
                    char *comma = strchr(p, ',');
                    if (comma) *comma = '\0';
                    for (k = 0; k < BOOK_COUNT; k++) {
                        if (!strcmp(BOOK_ABBREV[k], p)) SETTINGS.book[k] = 1;
                    }
                    p = comma ? comma + 1 : NULL;
                }
                at = 2;
            }
            SETTINGS.book[BOOK_PHB] = 1;        /* always available */

            if (at + 3 < n) {
                SETTINGS.custom_origins    = atoi(fields[at]);
                SETTINGS.optional_features = atoi(fields[at + 1]);
                SETTINGS.multiclassing     = atoi(fields[at + 2]);
                SETTINGS.feats             = atoi(fields[at + 3]);
            }
            /* Written since the experience line became optional; a file
               from before that has the line, as it always did. */
            SETTINGS.experience = (at + 4 < n) ? atoi(fields[at + 4]) : 1;
            SETTINGS.manual_dice = (at + 5 < n) ? atoi(fields[at + 5]) : 0;
            ui_set_manual_dice(SETTINGS.manual_dice);
        } else if (!strcmp(fields[0], "NAME") && n >= 2) {
            copy_field(c->name, sizeof c->name, fields[1]);
        } else if (!strcmp(fields[0], "PLAYER") && n >= 2) {
            copy_field(c->player, sizeof c->player, fields[1]);
        } else if (!strcmp(fields[0], "RACE") && n >= 2) {
            c->race_id = index_of_race(fields[1]);
        } else if (!strcmp(fields[0], "SUBRACE") && n >= 2) {
            c->subrace_id = index_of_subrace(fields[1]);
        } else if (!strcmp(fields[0], "BACKGROUND") && n >= 2) {
            c->background_id = index_of_background(fields[1]);
        } else if (!strcmp(fields[0], "CUSTOMBG") && n >= 5) {
            c->background_id = -1;
            snprintf(c->background_name, sizeof c->background_name, "%s",
                     fields[1]);
            snprintf(c->background_feature, sizeof c->background_feature,
                     "%s", fields[2]);
            snprintf(c->background_feature_text,
                     sizeof c->background_feature_text, "%s", fields[3]);
            snprintf(c->background_equipment,
                     sizeof c->background_equipment, "%s", fields[4]);
        } else if (!strcmp(fields[0], "NOTE") && n >= 2) {
            if (c->note_count < MAX_NOTES) {
                Note *nt = &c->notes[c->note_count++];

                snprintf(nt->title, sizeof nt->title, "%s", fields[1]);
                snprintf(nt->body, sizeof nt->body, "%s",
                         (n >= 3) ? fields[2] : "");
                /* Notes written before they had a title and a body of
                   their own are a single line; keep it as both. */
                if (!nt->body[0]) {
                    size_t tn = strlen(nt->title);
                    if (tn >= sizeof nt->body) tn = sizeof nt->body - 1;
                    memcpy(nt->body, nt->title, tn);
                    nt->body[tn] = '\0';
                }
            }
        } else if (!strcmp(fields[0], "ALIGNMENT") && n >= 2) {
            c->alignment = (Alignment)index_of_alignment(fields[1]);
        } else if (!strcmp(fields[0], "ANCESTRY") && n >= 2) {
            int i;
            for (i = 0; i < ANCESTRY_COUNT; i++) {
                if (!strcmp(ANCESTRIES[i].dragon, fields[1])) c->ancestry_id = i;
            }
        } else if (!strcmp(fields[0], "CLASS") && n >= 5) {
            if (c->class_count < MAX_CLASSES) {
                int id = class_by_name(fields[1]);
                if (id >= 0) {
                    int k = c->class_count++;
                    int lvl = atoi(fields[2]);
                    c->classes[k].class_id = id;
                    /* Every table shaped like a level runs 0 to MAX_LEVEL,
                       and several are read with a class level as the index.
                       A hand-edited file claiming level 99 read past the end
                       of the experience table. */
                    c->classes[k].level = lvl < 0 ? 0
                                        : lvl > MAX_LEVEL ? MAX_LEVEL : lvl;
                    c->classes[k].subclass_id =
                        strcmp(fields[3], "-") ? subclass_by_name(fields[3]) : -1;
                    c->classes[k].subclass_option = atoi(fields[4]);
                }
            }
        } else if (!strcmp(fields[0], "BASE") && n >= 7) {
            int i;
            for (i = 0; i < ABL_COUNT; i++) {
                c->base_score[i] = record_int(fields[i + 1], 1, 30);
            }
        } else if (!strcmp(fields[0], "RACIALBONUS") && n >= 7) {
            int i;
            for (i = 0; i < ABL_COUNT; i++) {
                c->racial_bonus[i] = record_int(fields[i + 1], -10, 10);
            }
        } else if (!strcmp(fields[0], "ASIBONUS") && n >= 7) {
            int i;
            for (i = 0; i < ABL_COUNT; i++) {
                c->asi_bonus[i] = record_int(fields[i + 1], -10, 20);
            }
        } else if (!strcmp(fields[0], "SAVEPROF") && n >= 7) {
            int i;
            for (i = 0; i < ABL_COUNT; i++) c->save_prof[i] = atoi(fields[i + 1]);
        } else if (!strcmp(fields[0], "SKILL") && n >= 4) {
            int s = skill_by_name(fields[1]);
            if (s >= 0) {
                c->skill_prof[s] = atoi(fields[2]);
                c->skill_expertise[s] = atoi(fields[3]);
            }
        } else if (!strcmp(fields[0], "LANG") && n >= 2) {
            add_language(c, fields[1]);
        } else if (!strcmp(fields[0], "TOOL") && n >= 2) {
            add_tool(c, fields[1]);
        } else if (!strcmp(fields[0], "PROF") && n >= 2) {
            add_prof(c, fields[1]);
        } else if (!strcmp(fields[0], "FEAT") && n >= 2) {
            int id = index_of_feat(fields[1]);
            if (id >= 0 && c->feat_count < MAX_FEATS) {
                c->feats[c->feat_count++] = id;
            }
        } else if (!strcmp(fields[0], "CHOICE") && n >= 3) {
            add_choice(c, fields[1], fields[2]);
        } else if (!strcmp(fields[0], "ITEM") && n >= 4) {
            int id = find_item(fields[3]);
            if (id >= 0) {
                add_item(c, id, record_int(fields[1], 1, MAX_STACK),
                         record_int(fields[2], 0, 1));
            }
            else warn_unknown("item", fields[3]);
        } else if (!strcmp(fields[0], "SIDEKICK") && n >= 14) {
            if (c->sidekick_count < MAX_SIDEKICKS) {
                Sidekick *sk = &c->sidekicks[c->sidekick_count++];
                int k;
                memset(sk, 0, sizeof *sk);
                snprintf(sk->name, sizeof sk->name, "%s", fields[1]);
                snprintf(sk->creature, sizeof sk->creature, "%s", fields[2]);
                        sk->cls = SK_EXPERT;
                for (k = 0; k < SK_CLASS_COUNT; k++) {
                    if (!strcmp(SIDEKICK_CLASS_NAME[k], fields[3])) sk->cls = k;
                }
                {   /* Bounded for the same reason: the sidekick's spells
                       known are read out of tables indexed by its level. */
                    int lvl = atoi(fields[4]);
                    sk->level = lvl < 0 ? 0
                              : lvl > MAX_LEVEL ? MAX_LEVEL : lvl;
                }
                /* The role indexes the role tables, and -1 is what a
                   sidekick that is not a spellcaster carries. */
                sk->role  = record_int(fields[5], -1, SK_ROLE_COUNT - 1);
                /* Zero is what a stat block the program has not filled
                   in carries, so the floor is nothing rather than one. */
                for (k = 0; k < 6; k++) {
                    sk->abilities[k] = record_int(fields[6 + k], 0, 30);
                }
                sk->hp = record_int(fields[12], 0, 1000);
                snprintf(sk->speed, sizeof sk->speed, "%s", fields[13]);
            }
        } else if (!strcmp(fields[0], "SKAC") && n >= 3) {
            Sidekick *sk = sidekick_named(c, fields[1]);
            if (sk) sk->ac = record_int(fields[2], 0, 40);
        } else if (!strcmp(fields[0], "SKCHOICE") && n >= 4) {
            Sidekick *sk = sidekick_named(c, fields[1]);
            if (sk && sk->choice_count < MAX_SK_CHOICES) {
                snprintf(sk->choices[sk->choice_count].label,
                         sizeof sk->choices[0].label, "%s", fields[2]);
                snprintf(sk->choices[sk->choice_count].value,
                         sizeof sk->choices[0].value, "%s", fields[3]);
                sk->choice_count++;
            }
        } else if (!strcmp(fields[0], "SKSPELL") && n >= 3) {
            int id = index_of_spell(fields[2]);
            if (id < 0) {
                warn_unknown("sidekick spell", fields[2]);
            } else {
                Sidekick *sk = sidekick_named(c, fields[1]);
                int max = (int)(sizeof sk->spells / sizeof sk->spells[0]);
                if (sk && sk->spell_count < max) {
                    sk->spells[sk->spell_count++] = id;
                }
            }
        } else if (!strcmp(fields[0], "MAGICITEM") && n >= 4) {
            int id = find_magic_item(fields[3]);
            if (id >= 0) {
                int before = c->item_count;
                /* Older files have no plus or worn column; both read as 0. */
                /* You cannot be attuned to something that asks for no
                   attunement. A file written before an item's row lost its
                   clause -- the gem of brightness had one it should never
                   have had -- would otherwise keep spending one of the
                   three slots on it. */
                /* Each record is one entry, not one more of an entry
                   already read: two rings of resistance made against
                   different damage, or two copies of one item the table is
                   telling the player different things about, are distinct
                   and merging them would drop the second's attunement. */
                add_magic_item_copy(c, id,
                                    record_int(fields[1], 1, MAX_STACK),
                                    MAGIC_ITEMS[id].attunement
                                        ? record_int(fields[2], 0, 1) : 0,
                                    /* The copy's plus -- except for a belt
                                       of giant strength, whose column
                                       carries the Strength it sets. */
                                    n >= 5 ? record_int(fields[4], 0, 30) : 0);
                /* A row the bank declines -- a quantity of zero or less, or
                   an inventory already full -- adds nothing, and the columns
                   after the name then have no entry of their own to land on.
                   Writing them anyway indexed one before the array. */
                if (c->item_count > before) {
                    InventoryEntry *e = &c->inventory[c->item_count - 1];
                    if (n >= 6 && atoi(fields[5])) e->equipped = 1;
                    if (n >= 7) {
                        snprintf(e->variant, sizeof e->variant, "%s",
                                 fields[6]);
                    }
                    /* What the table has chosen not to tell the player.
                       Older files carry neither column, and an item nobody
                       hid anything about reads the same either way. */
                    if (n >= 8) e->concealed = record_int(fields[7], 0, 1);
                    if (n >= 9) e->curse_hidden = record_int(fields[8], 0, 1);
                }
            } else {
                warn_unknown("magic item", fields[3]);
            }
        } else if (!strcmp(fields[0], "XP") && n >= 2) {
            /* Written only when non-zero, so a file without the record is a
               character who has earned none. */
            c->xp = record_int(fields[1], 0, 9999999);
        } else if (!strcmp(fields[0], "COINS") && n >= 6) {
            c->copper   = record_int(fields[1], 0, MAX_COINS);
            c->silver   = record_int(fields[2], 0, MAX_COINS);
            c->electrum = record_int(fields[3], 0, MAX_COINS);
            c->gold     = record_int(fields[4], 0, MAX_COINS);
            c->platinum = record_int(fields[5], 0, MAX_COINS);
        } else if (!strcmp(fields[0], "SPELL") && n >= 5) {
            int id = index_of_spell(fields[4]);
            int from_feat = !strcmp(fields[3], "-");
            int owner = from_feat ? -1 : class_by_name(fields[3]);
            if (id < 0) warn_unknown("spell", fields[4]);
            if (id >= 0 && c->spell_count < MAX_SPELLS) {
                c->spells[c->spell_count].spell_id = id;
                c->spells[c->spell_count].class_id =
                    (owner < 0 && !from_feat) ? 0 : owner;
                c->spells[c->spell_count].prepared = atoi(fields[1]);
                c->spells[c->spell_count].always_prepared = atoi(fields[2]);
                c->spell_count++;
            }
        } else if (!strcmp(fields[0], "HPROLLS") && n >= 2) {
            int count = record_int(fields[1], 0, MAX_LEVEL), i;
            for (i = 0; i < count && i + 2 < n; i++) {
                /* A hit die rolls at most 12, and the first level takes it
                   whole; a homebrew class may state a larger one, so the
                   ceiling is generous rather than exact. */
                c->hp_rolls[i] = record_int(fields[i + 2], 0, 100);
            }
            c->hp_roll_count = count;
        } else if (!strcmp(fields[0], "BODY") && n >= 7) {
            c->age = record_int(fields[1], 0, 100000);
            c->height_in = record_int(fields[2], 0, 10000);
            c->weight_lb = record_int(fields[3], 0, 100000);
            copy_field(c->eyes, sizeof c->eyes, fields[4]);
            copy_field(c->skin, sizeof c->skin, fields[5]);
            copy_field(c->hair, sizeof c->hair, fields[6]);
        } else if (!strcmp(fields[0], "TRAIT") && n >= 2) {
            copy_field(c->trait, sizeof c->trait, fields[1]);
        } else if (!strcmp(fields[0], "IDEAL") && n >= 2) {
            copy_field(c->ideal, sizeof c->ideal, fields[1]);
        } else if (!strcmp(fields[0], "BOND") && n >= 2) {
            copy_field(c->bond, sizeof c->bond, fields[1]);
        } else if (!strcmp(fields[0], "FLAW") && n >= 2) {
            copy_field(c->flaw, sizeof c->flaw, fields[1]);
        } else if (!strcmp(fields[0], "APPEARANCE") && n >= 2) {
            copy_field(c->appearance, sizeof c->appearance, fields[1]);
        } else if (!strcmp(fields[0], "BACKSTORY") && n >= 2) {
            copy_field(c->backstory, sizeof c->backstory, fields[1]);
        }
    }

    fclose(f);
    if (!in_data) return -2;        /* no data block: not one of our files */
    if (c->class_count == 0) return -3;
    return 0;
}
