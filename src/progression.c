/* progression.c -- levels, subclasses, ability improvements, feats, spells. */
#include "build.h"
#include "ui.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Subclasses are referenced by name so the tables can grow freely. */
static int is_third_caster(int sub)
{
    return subclass_is(sub, "Eldritch Knight")
        || subclass_is(sub, "Arcane Trickster");
}

/* 0 = roll hit dice, 1 = take the fixed average. Asked once per build. */
static int hp_use_average = 1;

/* ------------------------------------------------------------- spell slots */

/* Highest spell level this class can learn at its own level. */
static int max_spell_level_for(const Character *c, int slot)
{
    int id = c->classes[slot].class_id;
    int lvl = c->classes[slot].level;
    int sub = c->classes[slot].subclass_id;
    int eff = 0, i;

    if (CLASSES[id].caster_start_level == 0
        || lvl < CLASSES[id].caster_start_level) {
        return 0;
    }

    switch (CLASSES[id].caster) {
    case CAST_FULL: eff = lvl; break;
    case CAST_HALF: eff = (lvl + 1) / 2; break;
    case CAST_PACT: return PACT_SLOTS[lvl][1];
    default:
        if (is_third_caster(sub)) {
            eff = (lvl + 2) / 3;
        } else {
            return 0;
        }
        break;
    }
    if (eff > MAX_LEVEL) eff = MAX_LEVEL;
    for (i = 9; i >= 1; i--) {
        if (FULL_SLOTS[eff][i]) return i;
    }
    return 0;
}

/* Fills out[1..9] with spell slots. Uses the single-class table when only
 * one class grants Spellcasting, and the multiclass table otherwise
 * (PHB chapter 6). Pact Magic is reported separately. */
int spell_slots_for(const Character *c, int out[10])
{
    int i, casters = 0, single = -1, eff = 0;

    memset(out, 0, sizeof(int) * 10);

    for (i = 0; i < c->class_count; i++) {
        int id = c->classes[i].class_id;
        int sub = c->classes[i].subclass_id;
        CasterType t = CLASSES[id].caster;

        if (t == CAST_FULL || t == CAST_HALF) {
            casters++; single = i;
        } else if (is_third_caster(sub)) {
            casters++; single = i;
        }
    }
    if (casters == 0) return 0;

    if (casters == 1) {
        int id = c->classes[single].class_id;
        int lvl = c->classes[single].level;
        int sub = c->classes[single].subclass_id;

        /* Paladins and rangers have no slots at 1st level, and the
           third-casters none before 3rd. */
        if (lvl < CLASSES[id].caster_start_level) return 0;

        switch (CLASSES[id].caster) {
        case CAST_FULL: eff = lvl; break;
        case CAST_HALF: eff = (lvl + 1) / 2; break;
        default:
            if (is_third_caster(sub)) eff = (lvl + 2) / 3;
            break;
        }
    } else {
        eff = caster_level(c);
    }

    if (eff < 1) return 0;
    if (eff > MAX_LEVEL) eff = MAX_LEVEL;
    for (i = 1; i <= 9; i++) out[i] = FULL_SLOTS[eff][i];
    return eff;
}

int pact_slots_for(const Character *c, int *count, int *level)
{
    int lvl = class_level_of(c, CLS_WARLOCK);
    *count = *level = 0;
    if (lvl < 1) return 0;
    *count = PACT_SLOTS[lvl][0];
    *level = PACT_SLOTS[lvl][1];
    return 1;
}

int spells_prepared_count(const Character *c, int class_id)
{
    int lvl = class_level_of(c, class_id);
    int mod = ability_mod(c, CLASSES[class_id].spell_ability);
    int n;

    /* Half-casters that prepare (paladin, artificer) use half their level. */
    if (CLASSES[class_id].caster == CAST_HALF) n = mod + lvl / 2;
    else                                       n = mod + lvl;
    return n < 1 ? 1 : n;
}

int known_spell_count(const Character *c, int class_id, int cantrips)
{
    int slot = find_class_slot(c, class_id);
    int lvl, sub;

    if (slot < 0) return 0;
    lvl = c->classes[slot].level;
    sub = c->classes[slot].subclass_id;

    if (is_third_caster(sub)) {
        return cantrips ? THIRD_CANTRIPS[lvl] : THIRD_SPELLS_KNOWN[lvl];
    }
    if (cantrips) {
        return CLASSES[class_id].cantrips_known
             ? CLASSES[class_id].cantrips_known[lvl] : 0;
    }
    if (CLASSES[class_id].prep == PREP_SPELLBOOK) {
        return 6 + 2 * (lvl - 1);           /* wizard spellbook */
    }
    return CLASSES[class_id].spells_known
         ? CLASSES[class_id].spells_known[lvl] : 0;
}

/* ---------------------------------------------------------------- hit points */

void grant_level_hp(Character *c, int class_id, int is_first_level)
{
    int die = CLASSES[class_id].hit_die;
    int gain;

    if (c->hp_roll_count >= MAX_LEVEL) return;

    if (is_first_level) {
        gain = die;                          /* maximum at 1st level */
    } else if (hp_use_average) {
        gain = die / 2 + 1;
    } else {
        gain = roll_die(die);
        printf("      Hit die (d%d) rolled: %d\n", die, gain);
    }
    c->hp_rolls[c->hp_roll_count++] = gain;
}

/* ------------------------------------------------------------- ability score */

static int feat_available(const Character *c, int f)
{
    const FeatData *fd = &FEATS[f];
    int i;

    if (has_feat(c, f)) return 0;

    if (fd->req_ability != ABL_COUNT) {
        int ok = ability_score(c, fd->req_ability) >= fd->req_score;
        if (!ok && fd->req_ability2 != ABL_COUNT) {
            ok = ability_score(c, fd->req_ability2) >= fd->req_score;
        }
        if (!ok) return 0;
    }
    if (fd->req_prof[0] && !has_prof(c, fd->req_prof)) return 0;
    if (fd->req_spellcasting) {
        int casts = 0;
        for (i = 0; i < c->class_count; i++) {
            if (CLASSES[c->classes[i].class_id].caster != CAST_NONE) casts = 1;
            if (is_third_caster(c->classes[i].subclass_id)) casts = 1;
        }
        if (!casts) return 0;
    }
    return 1;
}

/* Applies a feat's own ability increase, asking when the feat allows a choice. */
static void apply_feat_asi(Character *c, int f)
{
    const FeatData *fd = &FEATS[f];
    int a;

    for (a = 0; a < ABL_COUNT; a++) {
        if (fd->asi[a] && ability_score(c, (Ability)a) < 20) {
            c->asi_bonus[a] += fd->asi[a];
        }
    }
    if (fd->asi_choice_count > 0) {
        const char *opts[ABL_COUNT];
        int avail[ABL_COUNT], picks[2], n = 0, i;

        for (a = 0; a < ABL_COUNT; a++) {
            int allowed = 1;
            if (fd->asi_choices[0]) {
                allowed = strstr(fd->asi_choices, ABILITY_ABBREV[a]) != NULL;
            }
            opts[n] = ABILITY_NAME[a];
            avail[n] = allowed && ability_score(c, (Ability)a) < 20;
            n++;
        }
        ui_multi("  This feat also raises an ability by 1:", opts, avail,
                 n, fd->asi_choice_count, picks);
        for (i = 0; i < fd->asi_choice_count; i++) {
            if (picks[i] < 0) continue;
            c->asi_bonus[picks[i]] += 1;
            if (strcmp(fd->name, "Resilient") == 0) {
                c->save_prof[picks[i]] = 1;
            }
        }
    }
}

void apply_asi_or_feat(Character *c, const char *reason)
{
    static const char *const modes[] = {
        "Increase one ability score by 2",
        "Increase two ability scores by 1 each",
        "Take a feat instead"
    };
    int m;

    printf("\n  %s\n", reason);
    /* Feats are an optional rule; without them only the two increases apply. */
    m = ui_menu("  Ability Score Improvement:", modes, NULL,
                SETTINGS.feats ? 3 : 2);

    if (m == 0 || m == 1) {
        const char *opts[ABL_COUNT];
        int avail[ABL_COUNT], picks[2], a, want = (m == 0) ? 1 : 2;
        int step = (m == 0) ? 2 : 1;
        int any = 0;

        for (a = 0; a < ABL_COUNT; a++) {
            char label[64];
            (void)label;
            opts[a] = ABILITY_NAME[a];
            avail[a] = (ability_score(c, (Ability)a) + step) <= 20;
            if (avail[a]) any++;
        }
        if (any < want) {
            printf("  Not enough room below the maximum of 20; take a feat "
                   "instead.\n");
            m = 2;
        } else {
            ui_multi("  Which ability score(s)?", opts, avail, ABL_COUNT,
                     want, picks);
            for (a = 0; a < want; a++) {
                if (picks[a] >= 0) c->asi_bonus[picks[a]] += step;
            }
            return;
        }
    }

    {
        const char *opts[64];
        const char *det[64];
        int map[64], n = 0, i, pick;

        for (i = 0; i < FEAT_COUNT; i++) {
            if (!book_enabled(FEATS[i].book)) continue;
            if (!feat_available(c, i)) continue;
            opts[n] = FEATS[i].name;
            det[n] = FEATS[i].summary;
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  No feats are available to you; taking +1 to two "
                   "abilities instead.\n");
            c->asi_bonus[ABL_CON] += 1;
            c->asi_bonus[ABL_DEX] += 1;
            return;
        }
        pick = ui_menu("  Feats you qualify for:", opts, det, n);
        if (c->feat_count < MAX_FEATS) c->feats[c->feat_count++] = map[pick];
        apply_feat_asi(c, map[pick]);

        /* Feats that grant proficiencies. */
        if (strcmp(FEATS[map[pick]].name, "Heavily Armored") == 0) {
            add_prof(c, "Heavy armor");
        } else if (strcmp(FEATS[map[pick]].name, "Lightly Armored") == 0) {
            add_prof(c, "Light armor");
        } else if (strcmp(FEATS[map[pick]].name, "Moderately Armored") == 0) {
            add_prof(c, "Medium armor");
            add_prof(c, "Shields");
        }
    }
}


/* Artificer infusions. The number known rises at 2nd, 6th, 10th, 14th and
 * 18th level; each has a minimum artificer level, and only Replicate Magic
 * Item may be learned more than once (Tasha's, p.20). */
static int infusions_recorded(const Character *c)
{
    int i, n = 0;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, "Infusion") == 0) n++;
    }
    return n;
}

static int infusion_taken(const Character *c, const char *name)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, "Infusion") != 0) continue;
        if (strncmp(c->choices[i].value, name, strlen(name)) == 0) return 1;
    }
    return 0;
}

static void choose_infusions(Character *c, int artificer_level)
{
    int want = INFUSIONS_KNOWN[artificer_level] - infusions_recorded(c);

    if (want <= 0) return;

    printf("\n  Infusions known: %d; you can infuse %d item%s at a time.\n",
           (int)INFUSIONS_KNOWN[artificer_level],
           (int)INFUSED_ITEMS[artificer_level],
           INFUSED_ITEMS[artificer_level] == 1 ? "" : "s");

    while (want-- > 0) {
        const char *opts[64];
        const char *det[64];
        static char labels[64][160];
        int map[64], n = 0, i, pick;

        for (i = 0; i < INFUSION_COUNT && n < 64; i++) {
            int repeatable = strcmp(INFUSIONS[i].name,
                                    "Replicate Magic Item") == 0;
            if (INFUSIONS[i].min_level > artificer_level) continue;
            if (!repeatable && infusion_taken(c, INFUSIONS[i].name)) continue;

            snprintf(labels[n], sizeof labels[n], "%s -- %s",
                     INFUSIONS[i].name, INFUSIONS[i].item);
            opts[n] = labels[n];
            det[n] = INFUSIONS[i].summary;
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("    No further infusions are available to you.\n");
            return;
        }

        pick = ui_menu("  Choose an artificer infusion:", opts, det, n);

        if (strcmp(INFUSIONS[map[pick]].name, "Replicate Magic Item") == 0) {
            char item[MAX_NAME];
            char value[MAX_TEXT];
            ui_line("    Which magic item does it replicate", item, sizeof item);
            snprintf(value, sizeof value, "Replicate Magic Item (%s)",
                     item[0] ? item : "to be chosen");
            add_choice(c, "Infusion", value);
        } else {
            add_choice(c, "Infusion", INFUSIONS[map[pick]].name);
        }
    }
}

/* ------------------------------------------------------------- subclass etc. */

void choose_subclass_for(Character *c, int slot)
{
    const ClassData *cd = &CLASSES[c->classes[slot].class_id];
    const char *opts[64];
    const char *det[64];
    int ids[64];
    int i, pick, n;

    n = subclasses_of(c->classes[slot].class_id, ids, 64);
    if (n <= 0) return;

    /* Hide subclasses from books this character is not using. */
    {
        int keep = 0;
        for (i = 0; i < n; i++) {
            if (book_enabled(SUBCLASSES[ids[i]].book)) ids[keep++] = ids[i];
        }
        n = keep;
        if (n <= 0) return;
    }

    for (i = 0; i < n; i++) {
        opts[i] = SUBCLASSES[ids[i]].name;
        det[i] = SUBCLASSES[ids[i]].summary;
    }
    {
        char prompt[128];
        snprintf(prompt, sizeof prompt, "  %s -- %s:", cd->name,
                 cd->subclass_label);
        pick = ui_menu(prompt, opts, det, n);
    }
    c->classes[slot].subclass_id = ids[pick];

    /* Some subclasses carry a further choice. */
    {
        const SubclassData *sc = &SUBCLASSES[c->classes[slot].subclass_id];
        if (sc->options && sc->options[0]) {
            char buf[512];
            const char *parts[16];
            int n = split_pipe(sc->options, buf, sizeof buf, parts, 16);
            char prompt[128];
            snprintf(prompt, sizeof prompt, "  %s:", sc->option_label);
            c->classes[slot].subclass_option = ui_menu(prompt, parts, NULL, n);
            add_choice(c, sc->option_label, parts[c->classes[slot].subclass_option]);
        }
    }

    /* Subclass proficiency grants. */
    {
        int sub = c->classes[slot].subclass_id;
        if (subclass_is(sub, "College of Valor")) {
            add_prof(c, "Medium armor");
            add_prof(c, "Shields");
            add_prof(c, "Martial weapons");
        }
        /* Life, Nature, Tempest and War domains grant heavy armour; the
           latter two also grant martial weapons. */
        if (strcmp(SUBCLASSES[sub].name, "Life Domain") == 0
            || strcmp(SUBCLASSES[sub].name, "Nature Domain") == 0) {
            add_prof(c, "Heavy armor");
        }
        if (strcmp(SUBCLASSES[sub].name, "Tempest Domain") == 0
            || strcmp(SUBCLASSES[sub].name, "War Domain") == 0) {
            add_prof(c, "Heavy armor");
            add_prof(c, "Martial weapons");
        }
        if (strcmp(SUBCLASSES[sub].name, "Assassin") == 0) {
            add_tool(c, "Disguise kit");
            add_tool(c, "Poisoner's kit");
        }
    }

    /* Lore bards gain three extra skills. */
    if (subclass_is(c->classes[slot].subclass_id, "College of Lore")) {
        const char *sopts[SKL_COUNT];
        int avail[SKL_COUNT], picks[3], i2;
        for (i2 = 0; i2 < SKL_COUNT; i2++) {
            sopts[i2] = SKILL_NAME[i2];
            avail[i2] = !c->skill_prof[i2];
        }
        ui_multi("  College of Lore bonus proficiencies:", sopts, avail,
                 SKL_COUNT, 3, picks);
        for (i2 = 0; i2 < 3; i2++) {
            if (picks[i2] >= 0) c->skill_prof[picks[i2]] = 1;
        }
    }
}

void choose_fighting_style(Character *c, int class_id)
{
    static const char *const all[] = {
        "Archery (+2 to ranged weapon attack rolls)",
        "Defense (+1 AC while wearing armor)",
        "Dueling (+2 damage with a single one-handed weapon)",
        "Great Weapon Fighting (reroll 1s and 2s on two-handed damage)",
        "Protection (impose disadvantage on an attack against an ally)",
        "Two-Weapon Fighting (add your ability modifier to the off-hand)"
    };
    int allowed[6], n = 0, i;
    const char *opts[16];
    int map[16], pick;
    char extra_buf[1024];
    const char *extra_parts[8];
    int extra_n = 0;

    /* Fighters may take any; paladins exclude Archery and Two-Weapon;
       rangers exclude Great Weapon Fighting and Protection. */
    for (i = 0; i < 6; i++) allowed[i] = 1;
    if (class_id == CLS_PALADIN) { allowed[0] = 0; allowed[5] = 0; }
    if (class_id == CLS_RANGER)  { allowed[3] = 0; allowed[4] = 0; }

    for (i = 0; i < 6; i++) {
        if (!allowed[i]) continue;
        opts[n] = all[i];
        map[n] = i;
        n++;
    }

    /* Tasha's Fighting Style Options widens the list. */
    if (has_optional_feature(c, "Fighting Style Options")) {
        const char *src = (class_id == CLS_PALADIN) ? TASHA_PALADIN_STYLES
                        : (class_id == CLS_RANGER)  ? TASHA_RANGER_STYLES
                        : TASHA_FIGHTER_STYLES;
        extra_n = split_pipe(src, extra_buf, sizeof extra_buf, extra_parts, 8);
        for (i = 0; i < extra_n && n < 16; i++) {
            opts[n] = extra_parts[i];
            map[n] = -1 - i;                /* negative marks a Tasha's style */
            n++;
        }
    }

    pick = ui_menu("  Fighting Style:", opts, NULL, n);
    add_choice(c, "Fighting Style",
               map[pick] >= 0 ? all[map[pick]] : extra_parts[-1 - map[pick]]);
}

void choose_expertise(Character *c, int count)
{
    const char *opts[SKL_COUNT + 1];
    int avail[SKL_COUNT + 1], picks[4], i, n = 0;
    int map[SKL_COUNT + 1];

    for (i = 0; i < SKL_COUNT; i++) {
        if (!c->skill_prof[i] || c->skill_expertise[i]) continue;
        opts[n] = SKILL_NAME[i];
        avail[n] = 1;
        map[n] = i;
        n++;
    }
    if (n == 0) return;
    if (count > n) count = n;

    ui_multi("  Expertise -- double your proficiency bonus for:",
             opts, avail, n, count, picks);
    for (i = 0; i < count; i++) {
        if (picks[i] >= 0) c->skill_expertise[map[picks[i]]] = 1;
    }
}


/* Tasha's optional class features are opt-in, one at a time, at the class
 * level that offers them. Taking one is recorded as a choice so it appears
 * on the sheet along with anything it replaces. */
static int optional_taken(const Character *c, const char *name)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, "Optional feature") != 0) continue;
        if (strncmp(c->choices[i].value, name, strlen(name)) == 0) return 1;
    }
    return 0;
}

int has_optional_feature(const Character *c, const char *name)
{
    return optional_taken(c, name);
}

static void offer_optional_features(Character *c, int class_id, int class_level)
{
    int i;

    for (i = 0; i < OPTIONAL_FEATURE_COUNT; i++) {
        const OptionalFeature *of = &OPTIONAL_FEATURES[i];
        char prompt[MAX_NAME + 64];

        if (of->class_id != class_id || of->level != class_level) continue;
        if (!SETTINGS.optional_features || !book_enabled(of->book)) continue;
        if (optional_taken(c, of->name)) continue;

        printf("\n  Optional class feature (Tasha's): %s\n", of->name);
        ui_wrap(of->summary, 6);
        if (of->replaces[0]) {
            printf("      Replaces: %s\n", of->replaces);
        }
        snprintf(prompt, sizeof prompt, "  Take %s?", of->name);
        if (!ui_yesno(prompt, 0)) continue;

        if (of->replaces[0]) {
            char value[MAX_TEXT];
            snprintf(value, sizeof value, "%s (replaces %s)", of->name,
                     of->replaces);
            add_choice(c, "Optional feature", value);
        } else {
            add_choice(c, "Optional feature", of->name);
        }
    }
}

/* The extra spells an "Additional <Class> Spells" feature grants, if taken. */
static const char *additional_spells_for(const Character *c, int class_id)
{
    static const char *const NAMES[] = {
        "Additional Bard Spells", "Additional Cleric Spells",
        "Additional Druid Spells", "Additional Paladin Spells",
        "Additional Ranger Spells", "Additional Sorcerer Spells",
        "Additional Warlock Spells", "Additional Wizard Spells",
    };
    int i;
    size_t k;

    for (i = 0; i < ADDITIONAL_SPELLS_COUNT; i++) {
        if (ADDITIONAL_SPELLS[i].class_id != class_id) continue;
        for (k = 0; k < sizeof NAMES / sizeof NAMES[0]; k++) {
            if (optional_taken(c, NAMES[k])) {
                /* Only the one matching this class can be taken by it. */
                const ClassData *cd = &CLASSES[class_id];
                char expect[MAX_NAME];
                snprintf(expect, sizeof expect, "Additional %s Spells",
                         cd->name);
                if (optional_taken(c, expect)) return ADDITIONAL_SPELLS[i].spells;
            }
        }
        return NULL;
    }
    return NULL;
}

/* True when `name` appears in a comma separated list. */
static int name_in_list(const char *list, const char *name)
{
    const char *p = list;
    size_t len = strlen(name);

    while (p && *p) {
        while (*p == ' ' || *p == ',') p++;
        if (strncmp(p, name, len) == 0
            && (p[len] == '\0' || p[len] == ',')) {
            return 1;
        }
        p = strchr(p, ',');
    }
    return 0;
}

/* ------------------------------------------------------------------- spells */

static int spell_class_bit(int class_id, int subclass_id)
{
    if (is_third_caster(subclass_id)) return SPL_WIZARD;
    switch (class_id) {
    case CLS_BARD:     return SPL_BARD;
    case CLS_CLERIC:   return SPL_CLERIC;
    case CLS_DRUID:    return SPL_DRUID;
    case CLS_PALADIN:  return SPL_PALADIN;
    case CLS_RANGER:   return SPL_RANGER;
    case CLS_SORCERER: return SPL_SORCERER;
    case CLS_WARLOCK:  return SPL_WARLOCK;
    case CLS_WIZARD:   return SPL_WIZARD;
    case CLS_ARTIFICER: return SPL_ARTIFICER;
    default:           return 0;
    }
}

static int already_known(const Character *c, int spell_id)
{
    int i;
    for (i = 0; i < c->spell_count; i++) {
        if (c->spells[i].spell_id == spell_id) return 1;
    }
    return 0;
}

static void add_spell(Character *c, int spell_id, int class_id,
                      int prepared, int always)
{
    if (c->spell_count >= MAX_SPELLS || already_known(c, spell_id)) return;
    c->spells[c->spell_count].spell_id = spell_id;
    c->spells[c->spell_count].class_id = class_id;
    c->spells[c->spell_count].prepared = prepared;
    c->spells[c->spell_count].always_prepared = always;
    c->spell_count++;
}

/* Spells already recorded for one class, optionally only cantrips. */
static int recorded_for(const Character *c, int class_id, int cantrips_only)
{
    int i, n = 0;
    for (i = 0; i < c->spell_count; i++) {
        if (c->spells[i].class_id != class_id) continue;
        if (cantrips_only) {
            if (SPELLS[c->spells[i].spell_id].level == 0) n++;
        } else {
            if (SPELLS[c->spells[i].spell_id].level == 0) continue;
            if (c->spells[i].always_prepared) continue;
            n++;
        }
    }
    return n;
}

static int find_spell_by_name(const char *name)
{
    int i;
    size_t j;
    for (i = 0; i < SPELL_COUNT; i++) {
        const char *a = SPELLS[i].name, *b = name;
        size_t la = strlen(a), lb = strlen(b);
        if (la != lb) continue;
        for (j = 0; j < la; j++) {
            int ca = a[j], cb = b[j];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) break;
        }
        if (j == la) return i;
    }
    return -1;
}

/* A spell is offered when the class list holds it, or when Tasha's
 * "Additional <Class> Spells" feature has widened that list to include it. */
static int spell_offered(int spell_id, int bit, const char *extra)
{
    char lower[64];
    size_t i;

    if (SPELLS[spell_id].classes & bit) return 1;
    if (!extra) return 0;

    for (i = 0; SPELLS[spell_id].name[i] && i + 1 < sizeof lower; i++) {
        int ch = (unsigned char)SPELLS[spell_id].name[i];
        lower[i] = (char)((ch >= 'A' && ch <= 'Z') ? ch + 32 : ch);
    }
    lower[i] = '\0';
    return name_in_list(extra, lower);
}

/* How many spells of this level the class list still offers. */
static int available_count(const Character *c, int bit, int level,
                           const char *extra)
{
    int i, n = 0;
    for (i = 0; i < SPELL_COUNT; i++) {
        if (!spell_offered(i, bit, extra)) continue;
        if (!book_enabled((SourceBook)SPELLS[i].book)) continue;
        if (SPELLS[i].level != level) continue;
        if (already_known(c, i)) continue;
        n++;
    }
    return n;
}

/* Picks `want` spells of exactly `level` from the class list. */
static void pick_spells(Character *c, int bit, int class_id, int level,
                        int want, const char *what)
{
    const char *extra = additional_spells_for(c, class_id);
    const char *opts[400];
    const char *det[400];
    static char lines[400][160];
    int map[400], n = 0, i, taken = 0;

    for (i = 0; i < SPELL_COUNT && n < 400; i++) {
        if (!spell_offered(i, bit, extra)) continue;
        if (!book_enabled((SourceBook)SPELLS[i].book)) continue;
        if (SPELLS[i].level != level) continue;
        if (already_known(c, i)) continue;

        snprintf(lines[n], sizeof lines[n], "%s (%s%s)",
                 SPELLS[i].name, SCHOOL_NAMES[SPELLS[i].school],
                 SPELLS[i].ritual ? ", ritual" : "");
        opts[n] = lines[n];
        det[n] = NULL;
        map[n] = i;
        n++;
    }
    if (n == 0) {
        printf("    (no more %s available at that level)\n", what);
        return;
    }
    if (want > n) want = n;

    while (taken < want) {
        char prompt[160];
        int pick;

        snprintf(prompt, sizeof prompt, "  Choose %s %d of %d (level %d):",
                 what, taken + 1, want, level);
        pick = ui_menu(prompt, opts, det, n);

        if (already_known(c, map[pick])) {
            printf("    You already have that one.\n");
            continue;
        }
        add_spell(c, map[pick], class_id, 1, 0);
        printf("    Added %s -- %s, %s, %s, %s\n",
               SPELLS[map[pick]].name, SPELLS[map[pick]].casting_time,
               SPELLS[map[pick]].range, SPELLS[map[pick]].components,
               SPELLS[map[pick]].duration);
        taken++;
    }
}

/* Grants one group of comma-separated spell names. */
static void grant_spell_group(Character *c, const char *group, int class_id,
                              int always)
{
    char names[256], *p, *start;

    strncpy(names, group, sizeof names - 1);
    names[sizeof names - 1] = '\0';
    start = names;
    for (p = names;; p++) {
        if (*p == ',' || *p == '\0') {
            int end = (*p == '\0');
            *p = '\0';
            while (*start == ' ') start++;
            if (*start) {
                int sid = find_spell_by_name(start);
                if (sid >= 0) add_spell(c, sid, class_id, 1, always);
            }
            if (end) break;
            start = p + 1;
        }
    }
}

/* Circle of the Land terrain spells and Genie kind spells hang off the
 * option the player chose, not the subclass, so they need their own pass. */
static void add_option_spells(Character *c, int slot)
{
    int sub = c->classes[slot].subclass_id;
    int opt = c->classes[slot].subclass_option;
    int id = c->classes[slot].class_id;
    int lvl = c->classes[slot].level;
    int i;

    if (sub < 0 || opt < 0) return;

    for (i = 0; i < OPTION_SPELLS_COUNT; i++) {
        char obuf[512], sbuf[1024], lbuf[64];
        const char *onames[16], *groups[8];
        int no, ng, g, at[8], nat = 0;
        const char *q;

        if (!subclass_is(sub, OPTION_SPELLS[i].subclass)) continue;

        /* Match by the option's name rather than its index, so the option
           lists in data_subclasses.c can be reordered safely. */
        no = split_pipe(SUBCLASSES[sub].options, obuf, sizeof obuf, onames, 16);
        if (opt >= no) continue;
        if (strcmp(onames[opt], OPTION_SPELLS[i].option) != 0) continue;

        strncpy(lbuf, OPTION_SPELLS[i].levels, sizeof lbuf - 1);
        lbuf[sizeof lbuf - 1] = '\0';
        for (q = lbuf; *q && nat < 8; ) {
            while (*q == ' ' || *q == ',') q++;
            if (*q >= '0' && *q <= '9') {
                int v = 0;
                while (*q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
                at[nat++] = v;
            } else if (*q) {
                q++;
            }
        }

        ng = split_pipe(OPTION_SPELLS[i].spells, sbuf, sizeof sbuf, groups, 8);
        for (g = 0; g < ng && g < nat; g++) {
            if (lvl < at[g]) break;
            /* A warlock's expanded list is a menu, not a grant; everyone
               else has these always prepared. */
            grant_spell_group(c, groups[g], id, id != CLS_WARLOCK);
        }
        return;
    }
}

/* Adds the always-prepared domain, oath, circle or patron spells earned so far. */
static void add_bonus_spells(Character *c, int slot)
{
    const SubclassData *sc;
    int sub = c->classes[slot].subclass_id;
    int id = c->classes[slot].class_id;
    int lvl = c->classes[slot].level;
    char buf[1024];
    const char *groups[8];
    int n, g;
    /* Domain and origin spells arrive at 1/3/5/7/9; druid circle spells at
       2/3/5/7/9; oath, archetype and specialist spells at 3/5/9/13/17. A
       warlock patron's expanded list unlocks with the spell level, which for
       a warlock is class level 1/3/5/7/9. */
    static const int early_at[5]   = {1, 3, 5, 7, 9};
    static const int druid_at[5]   = {2, 3, 5, 7, 9};
    static const int martial_at[5] = {3, 5, 9, 13, 17};
    const int *at;

    if (sub < 0) return;
    sc = &SUBCLASSES[sub];
    if (!sc->bonus_spells || !sc->bonus_spells[0]) return;

    if (id == CLS_PALADIN || id == CLS_RANGER || id == CLS_ARTIFICER) {
        at = martial_at;
    } else if (id == CLS_DRUID) {
        at = druid_at;
    } else {
        at = early_at;
    }

    n = split_pipe(sc->bonus_spells, buf, sizeof buf, groups, 8);
    for (g = 0; g < n && g < 5; g++) {
        if (lvl < at[g]) break;
        /* Warlock expanded lists are options, not grants. */
        grant_spell_group(c, groups[g], id, id != CLS_WARLOCK);
    }
}

void manage_spells(Character *c, int class_id)
{
    int slot = find_class_slot(c, class_id);
    const ClassData *cd;
    int bit, maxlvl, cantrips, known, lvl;

    if (slot < 0) return;
    cd = &CLASSES[class_id];
    bit = spell_class_bit(class_id, c->classes[slot].subclass_id);
    if (!bit) return;

    maxlvl = max_spell_level_for(c, slot);
    if (maxlvl < 1 && cd->caster != CAST_FULL) {
        /* Rangers and paladins have no spells at 1st level. */
        if (known_spell_count(c, class_id, 1) == 0 && maxlvl < 1) return;
    }

    printf("\n");
    ui_rule();
    printf("  Spells -- %s (level %d)\n", cd->name, c->classes[slot].level);
    ui_rule();
    printf("  Spellcasting ability: %s   Save DC %d   Attack +%d\n",
           ABILITY_NAME[cd->spell_ability],
           spell_save_dc(c, class_id), spell_attack_bonus(c, class_id));

    add_bonus_spells(c, slot);
    add_option_spells(c, slot);

    cantrips = known_spell_count(c, class_id, 1);
    if (cantrips > 0) {
        int have = recorded_for(c, class_id, 1);
        if (cantrips > have) {
            pick_spells(c, bit, class_id, 0, cantrips - have, "cantrip");
        }
    }

    if (maxlvl < 1) return;

    if (cd->prep == PREP_PREPARED) {
        known = spells_prepared_count(c, class_id);
        printf("\n  You prepare %d spell%s each day from the whole %s list.\n",
               known, known == 1 ? "" : "s", cd->name);
    } else if (cd->prep == PREP_SPELLBOOK) {
        known = known_spell_count(c, class_id, 0);
        printf("\n  Your spellbook holds %d spells; you prepare %d each day.\n",
               known, spells_prepared_count(c, class_id));
    } else {
        known = known_spell_count(c, class_id, 0);
        printf("\n  You know %d spell%s.\n", known, known == 1 ? "" : "s");
    }

    /* Only this class's own spells count against its allowance. */
    known -= recorded_for(c, class_id, 0);
    if (known <= 0) return;

    /* Never ask for more than the class list can still supply, or than the
       sheet has room for. */
    {
        const char *extra = additional_spells_for(c, class_id);
        int supply = 0;
        for (lvl = 1; lvl <= maxlvl; lvl++) {
            supply += available_count(c, bit, lvl, extra);
        }
        if (known > supply) known = supply;
        if (known > MAX_SPELLS - c->spell_count) {
            known = MAX_SPELLS - c->spell_count;
        }
    }
    if (known <= 0) return;

    printf("  Choose %d spell%s of level 1 to %d.\n", known,
           known == 1 ? "" : "s", maxlvl);

    while (known > 0) {
        int want, choose_level, before;
        char prompt[96];

        snprintf(prompt, sizeof prompt,
                 "  Spell level to pick from (%d left to choose)", known);
        choose_level = ui_int(prompt, 1, maxlvl);

        if (available_count(c, bit, choose_level,
                            additional_spells_for(c, class_id)) == 0) {
            printf("    No %s spells of that level are left to learn.\n",
                   cd->name);
            /* If nothing is left anywhere, stop rather than ask again. */
            {
                const char *extra2 = additional_spells_for(c, class_id);
                int supply = 0;
                for (lvl = 1; lvl <= maxlvl; lvl++) {
                    supply += available_count(c, bit, lvl, extra2);
                }
                if (supply == 0) break;
            }
            continue;
        }

        snprintf(prompt, sizeof prompt, "  How many at level %d",
                 choose_level);
        want = (known == 1) ? 1 : ui_int(prompt, 1, known);

        before = c->spell_count;
        pick_spells(c, bit, class_id, choose_level, want, "spell");
        known -= (c->spell_count - before);
        if (c->spell_count == before) break;   /* no progress: stop */
    }
}

int class_of_spell(const Character *c, int spell_id)
{
    int i;
    for (i = 0; i < c->class_count; i++) {
        int bit = spell_class_bit(c->classes[i].class_id,
                                  c->classes[i].subclass_id);
        if (SPELLS[spell_id].classes & bit) return c->classes[i].class_id;
    }
    return c->class_count ? c->classes[0].class_id : 0;
}

/* ------------------------------------------------- class option lists */

/* How many entries with this label the character has already recorded. */
static int options_recorded(const Character *c, const char *label)
{
    int i, n = 0;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0) n++;
    }
    return n;
}

static int option_already_taken(const Character *c, const char *label,
                                const char *name)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0
            && strcmp(c->choices[i].value, name) == 0) return 1;
    }
    return 0;
}

/* Offer everything this class and subclass draw from, at this class level.
 * Each list says how many are known by now; the difference from what has
 * already been recorded is what is still owed, so a character who levels up
 * in stages is asked exactly once for each. */
static void offer_class_options(Character *c, int slot, int class_level)
{
    const ClassData *cd = &CLASSES[c->classes[slot].class_id];
    int sub = c->classes[slot].subclass_id;
    int li;

    for (li = 0; li < OPTION_LIST_COUNT; li++) {
        const OptionList *ol = &OPTION_LISTS[li];
        int want;

        if (strcmp(ol->class_name, cd->name) != 0) continue;
        if (ol->subclass_name[0]) {
            if (sub < 0 || !subclass_is(sub, ol->subclass_name)) continue;
        }

        want = (int)ol->known[class_level] - options_recorded(c, ol->label);
        if (want <= 0) continue;

        printf("\n  %s knows %d %s at level %d.\n", cd->name,
               (int)ol->known[class_level], ol->plural, class_level);

        while (want-- > 0) {
            const char *opts[128];
            const char *det[128];
            static char labels[128][128];
            int map[128], n = 0, i, pick;

            for (i = 0; i < ol->count && n < 128; i++) {
                const ClassOption *o = &ol->options[i];
                if (!book_enabled(o->book)) continue;
                if (o->min_level > class_level) continue;
                if (!ol->repeatable
                    && option_already_taken(c, ol->label, o->name)) continue;

                if (o->prereq[0]) {
                    snprintf(labels[n], sizeof labels[n], "%s (needs %s)",
                             o->name, o->prereq);
                } else {
                    snprintf(labels[n], sizeof labels[n], "%s", o->name);
                }
                opts[n] = labels[n];
                det[n] = o->summary[0] ? o->summary : NULL;
                map[n] = i;
                n++;
            }
            if (n == 0) {
                printf("    Nothing further is available to you yet.\n");
                break;
            }
            {
                char prompt[96];
                snprintf(prompt, sizeof prompt, "  Choose a %s:", ol->label);
                pick = ui_menu(prompt, opts, det, n);
            }
            add_choice(c, ol->label, ol->options[map[pick]].name);
        }
    }
}

/* Has this character already recorded a choice under this label? */
static int has_choice(const Character *c, const char *label)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0) return 1;
    }
    return 0;
}

static int has_choice_value(const Character *c, const char *label,
                            const char *value)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0
            && strcmp(c->choices[i].value, value) == 0) return 1;
    }
    return 0;
}

/* ----------------------------------------------------------- beast forms */

/* Renders a challenge rating held in eighths. */
static const char *cr_text(int eighths)
{
    static char buf[16];
    switch (eighths) {
    case 0: return "0";
    case 1: return "1/8";
    case 2: return "1/4";
    case 4: return "1/2";
    default: break;
    }
    snprintf(buf, sizeof buf, "%d", eighths / 8);
    return buf;
}

/* A druid's Wild Shape is limited by challenge rating and by movement:
 * CR 1/4 and no swimming or flying at 2nd level, CR 1/2 with swimming at
 * 4th, CR 1 with flying at 8th. A Circle of the Moon druid uses a better
 * table: CR 1 from 2nd level, then CR equal to a third of the druid level
 * rounded down, and it may take swimmers and fliers on the same schedule. */
static int wild_shape_max_cr(const Character *c, int slot, int druid_level)
{
    if (subclass_is(c->classes[slot].subclass_id, "Circle of the Moon")) {
        int third = druid_level / 3;
        int cr = druid_level >= 6 ? (third > 0 ? third : 1) : 1;
        return cr * 8;
    }
    if (druid_level >= 8) return 8;         /* CR 1  */
    if (druid_level >= 4) return 4;         /* CR 1/2 */
    return 2;                               /* CR 1/4 */
}

/* Shows the forms available now. Wild Shape is used at the table rather than
 * recorded once, so this lists what the druid can become instead of asking
 * them to pick one and writing it down. */
static void show_wild_shape_forms(const Character *c, int slot,
                                  int druid_level)
{
    int max_cr = wild_shape_max_cr(c, slot, druid_level);
    int allow_swim = druid_level >= 4
        || subclass_is(c->classes[slot].subclass_id, "Circle of the Moon");
    int allow_fly = druid_level >= 8;
    int i, shown = 0;

    printf("\n  Wild Shape: beasts of challenge %s or lower", cr_text(max_cr));
    if (!allow_swim)     printf(", with no swimming speed");
    else if (!allow_fly) printf(", with no flying speed");
    printf(".\n");

    for (i = 0; i < BEAST_COUNT_ACTUAL; i++) {
        const BeastData *b = &BEASTS[i];
        if (b->cr_eighths > max_cr) continue;
        if (!allow_swim && beast_swims(b)) continue;
        if (!allow_fly && beast_flies(b)) continue;
        printf("    %-24s CR %-4s AC %-3d HP %-4d %s\n",
               b->name, b->cr_text, b->ac, b->hp, b->speed);
        shown++;
    }
    if (!shown) printf("    No beast in the tables fits those limits.\n");
    printf("  While transformed you keep your Intelligence, Wisdom and "
           "Charisma, your alignment, personality and skill and saving "
           "throw proficiencies.\n");
}

/* Lists the beasts that fit a set of limits and records the one chosen.
 * Used for the Beast Master's companion; Wild Shape only lists, because it
 * is used at the table rather than settled once. */
static void choose_beast(Character *c, const char *label, const char *prompt,
                         int max_cr_eighths, BeastSize max_size, int allow_fly)
{
    const char *opts[96];
    static char labels[96][96];
    int map[96], n = 0, i, pick;

    for (i = 0; i < BEAST_COUNT_ACTUAL && n < 96; i++) {
        const BeastData *b = &BEASTS[i];
        if (b->cr_eighths > max_cr_eighths) continue;
        if (b->size > max_size) continue;
        if (!allow_fly && beast_flies(b)) continue;

        snprintf(labels[n], sizeof labels[n],
                 "%-22s CR %-4s AC %-3d HP %-4d %s", b->name, b->cr_text,
                 b->ac, b->hp, b->speed);
        opts[n] = labels[n];
        map[n] = i;
        n++;
    }
    if (n == 0) return;

    pick = ui_menu(prompt, opts, NULL, n);
    add_choice(c, label, BEASTS[map[pick]].name);
}

/* The forms find familiar offers, plus the extra ones a Pact of the Chain
 * warlock may take. */
static const char *const FAMILIAR_FORMS[] = {
    "Bat", "Cat", "Crab", "Frog", "Hawk", "Lizard", "Octopus", "Owl",
    "Poisonous Snake", "Fish", "Rat", "Raven", "Sea Horse", "Spider",
    "Weasel"
};

static void offer_beast_choices(Character *c, int slot, int class_level)
{
    int id = c->classes[slot].class_id;
    int sub = c->classes[slot].subclass_id;

    /* Only when the limits actually change; otherwise a druid would see the
       same list at every level. */
    if (id == CLS_DRUID && class_level >= 2) {
        int now_cr = wild_shape_max_cr(c, slot, class_level);
        int was_cr = class_level > 2
            ? wild_shape_max_cr(c, slot, class_level - 1) : -1;
        int moon = subclass_is(c->classes[slot].subclass_id,
                               "Circle of the Moon");
        int now_swim = class_level >= 4 || moon;
        int was_swim = class_level > 2 && ((class_level - 1) >= 4 || moon);
        int now_fly = class_level >= 8;
        int was_fly = class_level > 2 && (class_level - 1) >= 8;

        if (now_cr != was_cr || now_swim != was_swim || now_fly != was_fly) {
            show_wild_shape_forms(c, slot, class_level);
        }
    }

    /* A Beast Master's companion is a beast of CR 1/4 or lower with no
       flying speed. */
    if (subclass_is(sub, "Beast Master") && class_level == 3
        && !has_choice(c, "Animal Companion")) {
        printf("\n  Your animal companion is a beast of challenge 1/4 or "
               "lower, no larger than Medium and with no flying speed.\n");
        choose_beast(c, "Animal Companion", "  Animal companion:", 2,
                     BSIZE_MEDIUM, 0);
    }

    /* Pact of the Chain names its own familiar forms on top of the usual
       ones; the base list is offered to anyone who has find familiar. */
    if (has_choice_value(c, "Pact Boon", "Pact of the Chain")
        && !has_choice(c, "Familiar")) {
        static const char *const chain[] = {
            "Imp", "Pseudodragon", "Quasit", "Sprite"
        };
        const char *opts[24];
        int i, n = 0, pick;
        for (i = 0; i < (int)(sizeof FAMILIAR_FORMS / sizeof FAMILIAR_FORMS[0]);
             i++) {
            if (find_beast(FAMILIAR_FORMS[i]) >= 0 ||
                strcmp(FAMILIAR_FORMS[i], "Fish") == 0) {
                opts[n++] = FAMILIAR_FORMS[i];
            }
        }
        for (i = 0; i < 4; i++) opts[n++] = chain[i];
        pick = ui_menu("  Your familiar's form:", opts, NULL, n);
        add_choice(c, "Familiar", opts[pick]);
    }
}

/* --------------------------------------------------------------- the ladder */

/* Applies everything a single class level grants. */
static void apply_class_level(Character *c, int slot, int class_level,
                              int is_first_character_level)
{
    int id = c->classes[slot].class_id;
    const ClassData *cd = &CLASSES[id];
    int asi[8], nasi, i;

    grant_level_hp(c, id, is_first_character_level);

    if (class_level == cd->subclass_level) {
        choose_subclass_for(c, slot);
    }

    /* Tasha's options come first: taking Fighting Style Options at this very
       level has to widen the menu before the style is chosen. */
    offer_optional_features(c, id, class_level);

    /* Fighting styles. */
    if ((id == CLS_FIGHTER && class_level == 1)
        || (id == CLS_PALADIN && class_level == 2)
        || (id == CLS_RANGER && class_level == 2)) {
        choose_fighting_style(c, id);
    }
    if (subclass_is(c->classes[slot].subclass_id, "Champion") && class_level == 10) {
        printf("\n  Champion: Additional Fighting Style.\n");
        choose_fighting_style(c, id);
    }

    /* Artificer infusions arrive at 2nd level and grow from there. */
    if (id == CLS_ARTIFICER && class_level >= 2) {
        choose_infusions(c, class_level);
    }

    /* Invocations, metamagic, maneuvers, runes, favoured enemies and the
       rest of the lists a class draws from as it advances. */
    offer_class_options(c, slot, class_level);

    /* Wild Shape forms, a Beast Master's companion and a chain warlock's
       familiar all come out of the beast tables. */
    offer_beast_choices(c, slot, class_level);

    /* Expertise. */
    if (id == CLS_ROGUE && (class_level == 1 || class_level == 6)) {
        choose_expertise(c, 2);
    }
    if (id == CLS_BARD && (class_level == 3 || class_level == 10)) {
        choose_expertise(c, 2);
    }

    /* Ability Score Improvements. */
    nasi = asi_levels_for(id, asi, 8);
    for (i = 0; i < nasi; i++) {
        if (asi[i] == class_level) {
            char reason[96];
            snprintf(reason, sizeof reason, "%s level %d:", cd->name,
                     class_level);
            apply_asi_or_feat(c, reason);
        }
    }
}

void build_levels(Character *c)
{
    static const char *const hpmodes[] = {
        "Take the fixed average each level (the common table rule)",
        "Roll the hit die each level"
    };
    int i, l, character_level = 0;

    ui_header("Hit Points, Features and Choices");
    ui_para("Your first level gives the maximum roll of your class's hit die. "
            "For each level after that, choose how to determine hit points.");

    if (total_level(c) > 1) {
        hp_use_average = (ui_menu("Hit points after 1st level:", hpmodes,
                                  NULL, 2) == 0);
    }

    for (i = 0; i < c->class_count; i++) {
        printf("\n");
        ui_rule();
        printf("  %s levels 1-%d\n", CLASSES[c->classes[i].class_id].name,
               c->classes[i].level);
        ui_rule();

        for (l = 1; l <= c->classes[i].level; l++) {
            character_level++;
            apply_class_level(c, i, l, character_level == 1);
        }
    }

    /* Variant humans take a feat at 1st level. */
    if (c->subrace_id >= 0 && SUBRACES[c->subrace_id].bonus_feats > 0) {
        printf("\n  Variant Human: you begin with one feat.\n");
        apply_asi_or_feat(c, "Variant Human bonus feat:");
    }

    for (i = 0; i < c->class_count; i++) {
        manage_spells(c, c->classes[i].class_id);
    }
}

/* ------------------------------------------------------------- level upward */

void wizard_level_up(Character *c)
{
    const char *opts[16];
    const char *det[16];
    int i, pick, slot, newlvl;
    char labels[16][96];

    if (total_level(c) >= MAX_LEVEL) {
        printf("\n%s is already 20th level, the maximum.\n", c->name);
        return;
    }

    ui_header("Level Up");
    printf("  %s is currently ", c->name);
    for (i = 0; i < c->class_count; i++) {
        printf("%s%s %d", i ? " / " : "",
               CLASSES[c->classes[i].class_id].name, c->classes[i].level);
    }
    printf(" (character level %d).\n", total_level(c));

    for (i = 0; i < CLASS_COUNT; i++) {
        int cur = class_level_of(c, i);
        int why, ok = (cur > 0) || multiclass_ok_public(c, i, &why);
        snprintf(labels[i], sizeof labels[i], "%s%s%s",
                 CLASSES[i].name,
                 cur ? " (continue)" : " (new class)",
                 ok ? "" : " -- prerequisites not met");
        opts[i] = labels[i];
        det[i] = NULL;
    }

    for (;;) {
        int why;
        pick = ui_menu("Take your next level in:", opts, det, CLASS_COUNT);
        if (class_level_of(c, pick) > 0) break;
        if (multiclass_ok_public(c, pick, &why)) break;
        printf("  You do not meet the multiclassing prerequisites for %s.\n",
               CLASSES[pick].name);
        if (!ui_yesno("  Choose a different class?", 1)) break;
    }

    slot = find_class_slot(c, pick);
    if (slot < 0) {
        if (c->class_count >= MAX_CLASSES) {
            printf("  Too many classes to track.\n");
            return;
        }
        slot = c->class_count++;
        c->classes[slot].class_id = pick;
        c->classes[slot].level = 0;
        c->classes[slot].subclass_id = -1;
        c->classes[slot].subclass_option = -1;
        add_prof_list(c, CLASSES[pick].mc_profs);
    }

    if (ui_yesno("\n  Roll the hit die for this level?", 0)) hp_use_average = 0;
    else hp_use_average = 1;

    newlvl = ++c->classes[slot].level;
    apply_class_level(c, slot, newlvl, 0);

    manage_spells(c, pick);

    printf("\n  %s is now ", c->name);
    for (i = 0; i < c->class_count; i++) {
        printf("%s%s %d", i ? " / " : "",
               CLASSES[c->classes[i].class_id].name, c->classes[i].level);
    }
    printf(" -- %d hit points.\n", hit_points_max(c));
}
