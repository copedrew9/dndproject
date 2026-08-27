/* dump_data.c -- write the compiled tables back out as data files.
 *
 * This exists for one job, done once: the game data used to live in nineteen
 * hand-written C files, and it now lives in four hand-written text files
 * under data/. Retyping thirteen hundred verified rows would have introduced
 * errors, so instead this program links against the old tables and prints
 * them in the new format.
 *
 * It is kept because it is also the other half of a proof. Building the C
 * tables from data/ and dumping them again must produce byte-identical
 * files; `make dataverify` does exactly that. So long as that holds, the
 * text files and the compiled tables say the same thing.
 */
#include "dnd.h"
#include "data.h"
#include "data_spells.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------- printing */

/* A field may itself contain the separator: several tables pack a list of
   traits into one '|' separated string. Escaping it keeps the record format
   to a single rule rather than a set of per-column exceptions. */
static void field(FILE *f, const char *s)
{
    fputc('|', f);
    if (!s) return;
    for (; *s; s++) {
        if (*s == '|' || *s == '\\') fputc('\\', f);
        fputc(*s, f);
    }
}

static void num(FILE *f, int n) { fprintf(f, "|%d", n); }

static void rec(FILE *f, const char *tag) { fputs(tag, f); }
static void end(FILE *f) { fputc('\n', f); }

static void head(FILE *f, const char *tag, const char *cols)
{
    fprintf(f, "\n# %s | %s\n", tag, cols);
}

static const char *book_of(int b)
{
    return (b >= 0 && b < BOOK_COUNT) ? BOOK_ABBREV[b] : "PHB";
}

static const char *abl_of(int a)
{
    return (a >= 0 && a < ABL_COUNT) ? ABILITY_ABBREV[a] : "-";
}

static const char *skill_of(int s)
{
    return (s >= 0 && s < SKL_COUNT) ? SKILL_NAME[s] : "-";
}

/* A twenty-one entry progression, one number per character level. */
static void progression(FILE *f, const char *key, const unsigned char *p)
{
    int i;
    rec(f, "PROGRESSION");
    field(f, key);
    fputc('|', f);
    for (i = 0; i <= MAX_LEVEL; i++) fprintf(f, i ? ",%d" : "%d", p[i]);
    end(f);
}

/* ------------------------------------------------------------------ files */

/* Where the files are written. `make dataverify` points this at a scratch
   directory so it can compare the result with data/ without touching it. */
static const char *out_dir = "data";

static FILE *open_out(const char *name, const char *title)
{
    char path[512];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", out_dir, name);
    f = fopen(path, "w");
    if (!f) { perror(path); exit(1); }
    fprintf(f,
"# %s\n"
"#\n"
"# One record per line: a tag, then '|' separated fields. A '#' at the\n"
"# start of a line is a comment, and blank lines are ignored. The comment\n"
"# above each block names the columns.\n"
"#\n"
"# A field that needs a literal '|' or '\\' writes it as '\\|' or '\\\\'.\n"
"# A line ending in '\\' continues on the next line.\n"
"#\n"
"# tools/build_data.py turns this file into C. Run `make data` after an\n"
"# edit; it checks every cross-reference and refuses to write on an error.\n",
            title);
    return f;
}

/* ------------------------------------------------------------- characters */

static void dump_character(void)
{
    FILE *f = open_out("character.txt",
        "character.txt -- races, classes, subclasses, features, "
        "backgrounds, feats");
    int i, j;

    head(f, "LANGUAGE", "name");
    for (i = 0; i < LANGUAGE_COUNT; i++) {
        rec(f, "LANGUAGE"); field(f, LANGUAGES[i]); end(f);
    }

    head(f, "RACE", "name | book | STR | DEX | CON | INT | WIS | CHA | "
                    "speed | size | darkvision | languages | extra_languages "
                    "| extra_skills | choice_asi_count | choice_asi_amount | "
                    "bonus_feats | has_ancestry | origin_choice | traits");
    for (i = 0; i < RACE_COUNT; i++) {
        const RaceData *r = &RACES[i];
        rec(f, "RACE");
        field(f, r->name);
        field(f, book_of(r->book));
        for (j = 0; j < ABL_COUNT; j++) num(f, r->ability[j]);
        num(f, r->speed);
        field(f, SIZE_NAME[r->size]);
        num(f, r->darkvision);
        field(f, r->languages);
        num(f, r->extra_languages);
        num(f, r->extra_skills);
        num(f, r->choice_asi_count);
        num(f, r->choice_asi_amount);
        num(f, r->bonus_feats);
        num(f, r->has_ancestry);
        num(f, r->origin_choice);
        field(f, r->traits);
        end(f);
    }

    /* A subrace names its parent instead of the parent naming an index
       window, so a new one can be dropped in beside its siblings. */
    head(f, "SUBRACE", "race | name | book | STR | DEX | CON | INT | WIS | "
                       "CHA | speed_override | darkvision_override | "
                       "extra_languages | choice_asi_count | extra_skills | "
                       "bonus_feats | replaces_race_asi | traits");
    for (i = 0; i < RACE_COUNT; i++) {
        for (j = 0; j < RACES[i].subrace_count; j++) {
            const SubraceData *s = &SUBRACES[RACES[i].first_subrace + j];
            int k;
            rec(f, "SUBRACE");
            field(f, RACES[i].name);
            field(f, s->name);
            field(f, book_of(s->book));
            for (k = 0; k < ABL_COUNT; k++) num(f, s->ability[k]);
            num(f, s->speed_override);
            num(f, s->darkvision_override);
            num(f, s->extra_languages);
            num(f, s->choice_asi_count);
            num(f, s->extra_skills);
            num(f, s->bonus_feats);
            num(f, s->replaces_race_asi);
            field(f, s->traits);
            end(f);
        }
    }

    head(f, "ANCESTRY", "dragon | damage | breath");
    for (i = 0; i < ANCESTRY_COUNT; i++) {
        rec(f, "ANCESTRY");
        field(f, ANCESTRIES[i].dragon);
        field(f, ANCESTRIES[i].damage);
        field(f, ANCESTRIES[i].breath);
        end(f);
    }

    head(f, "CLASS", "name | book | hit_die | save1 | save2 | armour_profs | "
                     "weapon_profs | tool_profs | skill_options | "
                     "skill_picks | caster | prep | spell_ability | "
                     "subclass_level | subclass_label | mc_req | "
                     "mc_req_either | mc_profs | gold_dice | gold_mult | "
                     "quick_build | equipment | caster_start_level | "
                     "mc_round_up");
    for (i = 0; i < CLASS_COUNT; i++) {
        const ClassData *c = &CLASSES[i];
        static const char *const CASTER[] =
            { "none", "full", "half", "third", "pact" };
        static const char *const PREP[] =
            { "none", "known", "prepared", "spellbook" };
        char skills[512], mc[128];
        size_t n = 0;

        skills[0] = '\0';
        for (j = 0; j < c->skill_option_count; j++)
            n += (size_t)snprintf(skills + n, sizeof skills - n, "%s%s",
                                  j ? "," : "", skill_of(c->skill_options[j]));
        /* "STR 13, DEX 13" or "STR 13 or DEX 13" -- the fighter and the monk
           accept either of two scores where every other class wants both. */
        mc[0] = '\0';
        n = 0;
        for (j = 0; j < c->mc_req_count; j++)
            n += (size_t)snprintf(mc + n, sizeof mc - n, "%s%s %d",
                                  j ? "," : "", abl_of(c->mc_req[j]),
                                  c->mc_req_score[j]);

        rec(f, "CLASS");
        field(f, c->name);
        field(f, book_of(c->book));
        num(f, c->hit_die);
        field(f, abl_of(c->save_prof[0]));
        field(f, abl_of(c->save_prof[1]));
        field(f, c->armour_profs);
        field(f, c->weapon_profs);
        field(f, c->tool_profs);
        field(f, skills);
        num(f, c->skill_picks);
        field(f, CASTER[c->caster]);
        field(f, PREP[c->prep]);
        field(f, abl_of(c->spell_ability));
        num(f, c->subclass_level);
        field(f, c->subclass_label);
        field(f, mc);
        num(f, c->mc_req_either);
        field(f, c->mc_profs);
        num(f, c->gold_dice);
        num(f, c->gold_mult);
        field(f, c->quick_build);
        field(f, c->equipment);
        num(f, c->caster_start_level);
        num(f, c->mc_round_up);
        end(f);
    }

    head(f, "PROGRESSION", "key | one number per level, 0 through 20");
    for (i = 0; i < CLASS_COUNT; i++) {
        char key[128];
        if (CLASSES[i].cantrips_known) {
            snprintf(key, sizeof key, "%s cantrips", CLASSES[i].name);
            progression(f, key, CLASSES[i].cantrips_known);
        }
        if (CLASSES[i].spells_known) {
            snprintf(key, sizeof key, "%s spells known", CLASSES[i].name);
            progression(f, key, CLASSES[i].spells_known);
        }
    }
    progression(f, "third-caster cantrips", THIRD_CANTRIPS);
    progression(f, "third-caster spells known", THIRD_SPELLS_KNOWN);
    progression(f, "infusions known", INFUSIONS_KNOWN);
    progression(f, "infused items", INFUSED_ITEMS);

    head(f, "FULLSLOTS", "level | slots of 1st through 9th");
    for (i = 0; i <= MAX_LEVEL; i++) {
        rec(f, "FULLSLOTS");
        num(f, i);
        for (j = 1; j <= 9; j++) num(f, FULL_SLOTS[i][j]);
        end(f);
    }

    head(f, "PACTSLOTS", "level | slots | slot level");
    for (i = 0; i <= MAX_LEVEL; i++) {
        rec(f, "PACTSLOTS");
        num(f, i);
        num(f, PACT_SLOTS[i][0]);
        num(f, PACT_SLOTS[i][1]);
        end(f);
    }

    head(f, "SUBCLASS", "class | name | book | summary | bonus_spells | "
                        "option_label | options");
    for (i = 0; i < SUBCLASS_COUNT; i++) {
        const SubclassData *s = &SUBCLASSES[i];
        rec(f, "SUBCLASS");
        field(f, CLASSES[s->class_id].name);
        field(f, s->name);
        field(f, book_of(s->book));
        field(f, s->summary);
        field(f, s->bonus_spells);
        field(f, s->option_label);
        field(f, s->options);
        end(f);
    }

    head(f, "FEATURE", "class | subclass (blank for the class itself) | "
                       "level | name | summary");
    for (i = 0; i < FEATURE_COUNT; i++) {
        const FeatureData *d = &FEATURES[i];
        rec(f, "FEATURE");
        field(f, CLASSES[d->class_id].name);
        field(f, d->subclass_id < 0 ? "" : SUBCLASSES[d->subclass_id].name);
        num(f, d->level);
        field(f, d->name);
        field(f, d->summary);
        end(f);
    }

    head(f, "OPTFEATURE", "class | book | level | name | replaces | summary");
    for (i = 0; i < OPTIONAL_FEATURE_COUNT; i++) {
        const OptionalFeature *o = &OPTIONAL_FEATURES[i];
        rec(f, "OPTFEATURE");
        field(f, CLASSES[o->class_id].name);
        field(f, book_of(o->book));
        num(f, o->level);
        field(f, o->name);
        field(f, o->replaces);
        field(f, o->summary);
        end(f);
    }

    head(f, "ADDSPELLS", "class | spells added to the class list");
    for (i = 0; i < ADDITIONAL_SPELLS_COUNT; i++) {
        rec(f, "ADDSPELLS");
        field(f, CLASSES[ADDITIONAL_SPELLS[i].class_id].name);
        field(f, ADDITIONAL_SPELLS[i].spells);
        end(f);
    }

    head(f, "FIGHTINGSTYLES", "class | styles Tasha's opens to it");
    rec(f, "FIGHTINGSTYLES"); field(f, "Fighter");
    field(f, TASHA_FIGHTER_STYLES); end(f);
    rec(f, "FIGHTINGSTYLES"); field(f, "Paladin");
    field(f, TASHA_PALADIN_STYLES); end(f);
    rec(f, "FIGHTINGSTYLES"); field(f, "Ranger");
    field(f, TASHA_RANGER_STYLES); end(f);

    head(f, "OPTIONSPELLS", "subclass | option | levels | spells");
    for (i = 0; i < OPTION_SPELLS_COUNT; i++) {
        rec(f, "OPTIONSPELLS");
        field(f, OPTION_SPELLS[i].subclass);
        field(f, OPTION_SPELLS[i].option);
        field(f, OPTION_SPELLS[i].levels);
        field(f, OPTION_SPELLS[i].spells);
        end(f);
    }

    head(f, "INFUSION", "name | prereq | min_level | item | summary");
    for (i = 0; i < INFUSION_COUNT; i++) {
        rec(f, "INFUSION");
        field(f, INFUSIONS[i].name);
        field(f, INFUSIONS[i].prereq);
        num(f, INFUSIONS[i].min_level);
        field(f, INFUSIONS[i].item);
        field(f, INFUSIONS[i].summary);
        end(f);
    }

    head(f, "OPTIONLIST", "list | class | subclass | label | plural | "
                          "repeatable");
    head(f, "OPTION", "list | name | book | min_level | prereq | summary");
    for (i = 0; i < OPTION_LIST_COUNT; i++) {
        const OptionList *L = &OPTION_LISTS[i];
        char key[160];
        snprintf(key, sizeof key, "%s", L->plural);
        rec(f, "OPTIONLIST");
        field(f, key);
        field(f, L->class_name);
        field(f, L->subclass_name);
        field(f, L->label);
        field(f, L->plural);
        num(f, L->repeatable);
        end(f);
        progression(f, key, L->known);
        for (j = 0; j < L->count; j++) {
            rec(f, "OPTION");
            field(f, key);
            field(f, L->options[j].name);
            field(f, book_of(L->options[j].book));
            num(f, L->options[j].min_level);
            field(f, L->options[j].prereq);
            field(f, L->options[j].summary);
            end(f);
        }
    }

    head(f, "BACKGROUND", "name | book | skill1 | skill2 | tool_profs | "
                          "extra_languages | equipment | gold | "
                          "feature_name | feature_summary");
    head(f, "BGTRAIT/BGIDEAL/BGBOND/BGFLAW", "background | text");
    for (i = 0; i < BACKGROUND_COUNT; i++) {
        const BackgroundData *b = &BACKGROUNDS[i];
        static const struct { const char *tag; int n; } LISTS[] = {
            { "BGTRAIT", 8 }, { "BGIDEAL", 6 }, { "BGBOND", 6 },
            { "BGFLAW", 6 }
        };
        const char *const *src[4];
        int k;

        rec(f, "BACKGROUND");
        field(f, b->name);
        field(f, book_of(b->book));
        field(f, skill_of(b->skills[0]));
        field(f, skill_of(b->skills[1]));
        field(f, b->tool_profs);
        num(f, b->extra_languages);
        field(f, b->equipment);
        num(f, b->gold);
        field(f, b->feature_name);
        field(f, b->feature_summary);
        end(f);

        src[0] = b->traits; src[1] = b->ideals;
        src[2] = b->bonds;  src[3] = b->flaws;
        for (k = 0; k < 4; k++)
            for (j = 0; j < LISTS[k].n; j++) {
                if (!src[k][j] || !src[k][j][0]) continue;
                rec(f, LISTS[k].tag);
                field(f, b->name);
                field(f, src[k][j]);
                end(f);
            }
    }

    head(f, "FEAT", "name | book | prereq | req_ability | req_ability2 | "
                    "req_score | req_prof | req_spellcasting | STR | DEX | "
                    "CON | INT | WIS | CHA | asi_choice_count | asi_choices "
                    "| req_race | summary");
    for (i = 0; i < FEAT_COUNT; i++) {
        const FeatData *t = &FEATS[i];
        rec(f, "FEAT");
        field(f, t->name);
        field(f, book_of(t->book));
        field(f, t->prereq);
        field(f, abl_of(t->req_ability));
        field(f, abl_of(t->req_ability2));
        num(f, t->req_score);
        field(f, t->req_prof);
        num(f, t->req_spellcasting);
        for (j = 0; j < ABL_COUNT; j++) num(f, t->asi[j]);
        num(f, t->asi_choice_count);
        field(f, t->asi_choices);
        field(f, t->req_race);
        field(f, t->summary);
        end(f);
    }

    fclose(f);
}

/* -------------------------------------------------------------- equipment */

static void dump_equipment(void)
{
    FILE *f = open_out("equipment.txt",
        "equipment.txt -- gear, weapons, armour, magic items and prices");
    int i;

    head(f, "ITEM", "name | book | category | cost_cp | weight_tenths | "
                    "base_ac | dex_cap | str_req | stealth_disadvantage | "
                    "damage | damage_type | properties | contents");
    for (i = 0; i < BOOK_ITEM_COUNT; i++) {
        const ItemData *it = &BOOK_ITEMS[i];
        rec(f, "ITEM");
        field(f, it->name);
        field(f, book_of(it->book));
        field(f, ITEM_CATEGORY_NAME[it->category]);
        num(f, it->cost_cp);
        num(f, it->weight_tenths);
        num(f, it->base_ac);
        num(f, it->dex_cap);
        num(f, it->str_req);
        num(f, it->stealth_disadvantage);
        field(f, it->damage);
        field(f, it->damage_type);
        field(f, it->properties);
        field(f, it->contents);
        end(f);
    }

    head(f, "ITEMNOTE", "item | what it is and what it does");
    for (i = 0; i < ITEM_NOTE_COUNT; i++) {
        rec(f, "ITEMNOTE");
        field(f, ITEM_NOTES[i].item);
        field(f, ITEM_NOTES[i].text);
        end(f);
    }

    head(f, "WEAPONPROP", "property | what it means");
    for (i = 0; i < WEAPON_PROPERTY_COUNT; i++) {
        rec(f, "WEAPONPROP");
        field(f, WEAPON_PROPERTIES[i].item);
        field(f, WEAPON_PROPERTIES[i].text);
        end(f);
    }

    head(f, "TOOLGROUP", "group | item");
    for (i = 0; i < TOOL_GROUP_COUNT; i++) {
        rec(f, "TOOLGROUP");
        field(f, TOOL_GROUPS[i].group);
        field(f, TOOL_GROUPS[i].item);
        end(f);
    }

    head(f, "TRINKET", "text");
    for (i = 0; i < TRINKET_COUNT; i++) {
        rec(f, "TRINKET"); field(f, TRINKETS[i]); end(f);
    }

    head(f, "LIFESTYLE", "name | cost_cp_per_day | text");
    for (i = 0; i < LIFESTYLE_COUNT; i++) {
        rec(f, "LIFESTYLE");
        field(f, LIFESTYLES[i].name);
        num(f, LIFESTYLES[i].cost_cp_per_day);
        field(f, LIFESTYLES[i].text);
        end(f);
    }

    head(f, "SERVICE", "name | cost_cp");
    for (i = 0; i < SERVICE_COUNT; i++) {
        rec(f, "SERVICE");
        field(f, SERVICES[i].name);
        num(f, SERVICES[i].cost_cp);
        end(f);
    }

    head(f, "SPELLSERVICE", "name | cost_cp");
    for (i = 0; i < SPELLCASTING_SERVICE_COUNT; i++) {
        rec(f, "SPELLSERVICE");
        field(f, SPELLCASTING_SERVICES[i].name);
        num(f, SPELLCASTING_SERVICES[i].cost_cp);
        end(f);
    }

    head(f, "MAGICITEM", "name | book | type | rarity | attunement (blank "
                         "when none) | text");
    for (i = 0; i < BOOK_MAGIC_ITEM_COUNT; i++) {
        const MagicItem *m = &BOOK_MAGIC_ITEMS[i];
        rec(f, "MAGICITEM");
        field(f, m->name);
        field(f, book_of(m->book));
        field(f, m->type);
        field(f, m->rarity);
        field(f, m->attunement ? m->attunement : "");
        field(f, m->text);
        end(f);
    }

    /* Only the fields an item actually sets are written, so a rule reads as
       the short list of numbers the item changes. */
    head(f, "MAGICRULE", "item | key=value ...");
    for (i = 0; i < MAGIC_RULE_COUNT; i++) {
        const MagicRule *r = &MAGIC_RULES[i];
        static const struct { const char *key; size_t off; } INTS[] = {
            { "ac_bonus",        offsetof(MagicRule, ac_bonus) },
            { "save_bonus",      offsetof(MagicRule, save_bonus) },
            { "armor_base",      offsetof(MagicRule, armor_base) },
            { "armor_dex",       offsetof(MagicRule, armor_dex) },
            { "armor_str",       offsetof(MagicRule, armor_str) },
            { "armor_stealth",   offsetof(MagicRule, armor_stealth) },
            { "shield",          offsetof(MagicRule, shield) },
            { "only_unarmored",  offsetof(MagicRule, only_unarmored) },
            { "unarmored_base",  offsetof(MagicRule, unarmored_base) },
            { "variable",        offsetof(MagicRule, variable) },
            { "sets_ability",    offsetof(MagicRule, sets_ability) },
            { "sets_to",         offsetof(MagicRule, sets_to) },
            { "sets_speed",      offsetof(MagicRule, sets_speed) },
            { "fly_speed",       offsetof(MagicRule, fly_speed) },
            { "swim_speed",      offsetof(MagicRule, swim_speed) },
            { "climb_speed",     offsetof(MagicRule, climb_speed) },
        };
        size_t k;
        char buf[64];

        rec(f, "MAGICRULE");
        field(f, r->item);
        for (k = 0; k < sizeof INTS / sizeof INTS[0]; k++) {
            int v = *(const int *)((const char *)r + INTS[k].off);
            if (!v) continue;
            /* armor_dex of -1 is a real setting, and so is a sets_to of 0 on
               an item whose row also names an ability; both are covered
               because the row is only written when some field is non-zero
               and sets_ability is itself non-zero there. */
            snprintf(buf, sizeof buf, "%s=%d", INTS[k].key, v);
            field(f, buf);
        }
        if (r->sets_ability && !r->sets_to) field(f, "sets_to=0");
        if (r->resist)  { snprintf(buf, sizeof buf, "resist=%s", r->resist);
                          field(f, buf); }
        if (r->immune)  { snprintf(buf, sizeof buf, "immune=%s", r->immune);
                          field(f, buf); }
        end(f);
    }

    fclose(f);
}

/* ----------------------------------------------------------------- spells */

static void dump_spells(void)
{
    FILE *f = open_out("spells.txt", "spells.txt -- every spell");
    int i;

    head(f, "SCHOOL", "name, in the order the school field uses");
    for (i = 0; i < SCHOOL_COUNT; i++) {
        rec(f, "SCHOOL"); field(f, SCHOOL_NAMES[i]); end(f);
    }

    head(f, "SPELL", "name | book | level (0 = cantrip) | school | ritual | "
                     "concentration | casting_time | range | components | "
                     "duration | classes");
    for (i = 0; i < BOOK_SPELL_COUNT; i++) {
        const SpellData *s = &BOOK_SPELLS[i];
        char classes[160];

        spell_classes_text(s->classes, classes, sizeof classes);

        rec(f, "SPELL");
        field(f, s->name);
        field(f, book_of(s->book));
        num(f, s->level);
        field(f, SCHOOL_NAMES[s->school]);
        num(f, s->ritual);
        num(f, s->concentration);
        field(f, s->casting_time);
        field(f, s->range);
        field(f, s->components);
        field(f, s->duration);
        field(f, classes);
        end(f);
    }

    fclose(f);
}

/* ------------------------------------------------------------------ world */

static void dump_world(void)
{
    FILE *f = open_out("world.txt",
        "world.txt -- gods, beasts, sidekicks and the background tables");
    int i, j;

    head(f, "DEITY", "name | title | pantheon | alignment | domains | symbol");
    for (i = 0; i < DEITY_COUNT; i++) {
        rec(f, "DEITY");
        field(f, DEITIES[i].name);
        field(f, DEITIES[i].title);
        field(f, DEITIES[i].pantheon);
        field(f, DEITIES[i].alignment);
        field(f, DEITIES[i].domains);
        field(f, DEITIES[i].symbol);
        end(f);
    }

    head(f, "BEASTSIZE", "name, smallest first");
    for (i = 0; i <= BSIZE_GARGANTUAN; i++) {
        rec(f, "BEASTSIZE"); field(f, BEAST_SIZE_NAME[i]); end(f);
    }

    head(f, "BEAST", "name | size | ac | hp | challenge in eighths | speed | "
                     "STR | DEX | CON | INT | WIS | CHA | challenge | senses");
    for (i = 0; i < BEAST_COUNT_ACTUAL; i++) {
        const BeastData *b = &BEASTS[i];
        rec(f, "BEAST");
        field(f, b->name);
        field(f, BEAST_SIZE_NAME[b->size]);
        num(f, b->ac);
        num(f, b->hp);
        num(f, b->cr_eighths);
        field(f, b->speed);
        for (j = 0; j < 6; j++) num(f, b->abilities[j]);
        field(f, b->cr_text);
        field(f, b->senses);
        end(f);
    }

    head(f, "SIDEKICKCLASS", "name | blurb");
    for (i = 0; i < SK_CLASS_COUNT; i++) {
        rec(f, "SIDEKICKCLASS");
        field(f, SIDEKICK_CLASS_NAME[i]);
        field(f, SIDEKICK_CLASS_BLURB[i]);
        end(f);
    }

    head(f, "SIDEKICKROLE", "name | description");
    for (i = 0; i < SK_ROLE_COUNT; i++) {
        rec(f, "SIDEKICKROLE");
        field(f, SPELLCASTER_ROLE_NAME[i]);
        field(f, SPELLCASTER_ROLE_DESC[i]);
        end(f);
    }

    head(f, "SIDEKICKFEATURE", "sidekick class | level | name | summary");
    for (i = 0; i < SIDEKICK_FEATURE_COUNT; i++) {
        rec(f, "SIDEKICKFEATURE");
        field(f, SIDEKICK_CLASS_NAME[SIDEKICK_FEATURES[i].cls]);
        num(f, SIDEKICK_FEATURES[i].level);
        field(f, SIDEKICK_FEATURES[i].name);
        field(f, SIDEKICK_FEATURES[i].summary);
        end(f);
    }

    head(f, "PROGRESSION", "key | one number per level, 0 through 20");
    progression(f, "sidekick spellcaster cantrips", SPELLCASTER_CANTRIPS);
    progression(f, "sidekick spellcaster spells known",
                SPELLCASTER_SPELLS_KNOWN);

    head(f, "LIFETABLE", "name | die");
    head(f, "LIFEROW", "table | lowest roll | highest roll | text");
    for (i = 0; i < LIFE_TABLE_COUNT; i++) {
        rec(f, "LIFETABLE");
        field(f, LIFE_TABLES[i].name);
        field(f, LIFE_TABLES[i].die);
        end(f);
        for (j = 0; j < LIFE_TABLES[i].count; j++) {
            rec(f, "LIFEROW");
            field(f, LIFE_TABLES[i].name);
            num(f, LIFE_TABLES[i].rows[j].lo);
            num(f, LIFE_TABLES[i].rows[j].hi);
            field(f, LIFE_TABLES[i].rows[j].text);
            end(f);
        }
    }

    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc > 1) out_dir = argv[1];
    dump_character();
    dump_equipment();
    dump_spells();
    dump_world();
    return 0;
}
