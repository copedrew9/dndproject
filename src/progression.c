/* progression.c -- levels, subclasses, ability improvements, feats, spells. */
#include "build.h"
#include "ui.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>


/* Subclasses are referenced by name so the tables can grow freely. Shared
   with character.c, which needs it for the multiclass caster level. */
int is_third_caster(int sub)
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
        if (!cantrips) return THIRD_SPELLS_KNOWN[lvl];
        /* The two third casters do not learn the same number of cantrips:
           the Arcane Trickster's three include mage hand. */
        return subclass_is(sub, "Arcane Trickster")
             ? TRICKSTER_CANTRIPS[lvl] : THIRD_CANTRIPS[lvl];
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
        gain = ui_roll_die(die, "this level's hit points");
        if (!ui_manual_dice()) {
            printf("      Hit die (d%d) rolled: %d\n", die, gain);
        }
    }
    c->hp_rolls[c->hp_roll_count++] = gain;
}

/* ------------------------------------------------------------- ability score */

int feat_offered(const Character *c, int f)
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

    /* Xanathar's racial feats are limited to a race, sometimes a subrace.
       The requirement lists every race that qualifies, separated by '|'. */
    if (fd->req_race[0]) {
        char buf[128];
        const char *parts[8];
        int n, k, ok = 0;
        const char *race = (c->race_id >= 0) ? RACES[c->race_id].name : "";
        const char *sub = (c->subrace_id >= 0) ? SUBRACES[c->subrace_id].name
                                               : "";

        n = split_pipe(fd->req_race, buf, sizeof buf, parts, 8);
        for (k = 0; k < n; k++) {
            /* "Small" is a size, not a race, and it is how Xanathar's words
               the one feat that asks for it: Squat Nimbleness is for "a
               dwarf or a small race". Naming the small races one at a time
               refuses the feat to every small race added after the list was
               written, which by now is four of the six. */
            if (!strcmp(parts[k], "Small")) {
                if (c->race_id >= 0 && RACES[c->race_id].size == SZ_SMALL) {
                    ok = 1;
                }
            } else if (!strcmp(parts[k], race) || !strcmp(parts[k], sub)) {
                ok = 1;
            }
        }
        if (!ok) return 0;
    }
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
static void feat_extras(Character *c, int feat_id);

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
            if (!feat_offered(c, i)) continue;
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
        feat_extras(c, map[pick]);

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
static void choose_infusions(Character *c, int artificer_level)
{
    int want = INFUSIONS_KNOWN[artificer_level] - count_choices(c, "Infusion");

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
            if (!repeatable && has_choice_starting(c, "Infusion", INFUSIONS[i].name)) continue;

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

        {
            char answer[MAX_TEXT];

            pick = ui_menu_custom("  Choose an artificer infusion:", opts, det,
                                  n, "Another infusion (type it in)",
                                  answer, sizeof answer);
            if (pick < 0) {
                add_choice(c, "Infusion", answer);
            } else if (strcmp(INFUSIONS[map[pick]].name,
                              "Replicate Magic Item") == 0) {
                /* Which items may be replicated is its own table, by
                   artificer level, so the item is named rather than picked
                   from the magic item bank. */
                char item[MAX_NAME], value[MAX_TEXT];
                ui_line("    Which magic item does it replicate", item,
                        sizeof item);
                snprintf(value, sizeof value, "Replicate Magic Item (%s)",
                         item[0] ? item : "to be chosen");
                add_choice(c, "Infusion", value);
            } else {
                add_choice(c, "Infusion", INFUSIONS[map[pick]].name);
            }
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

    /* What the subclass makes you proficient with.
     *
     * This was six subclasses written out by hand while thirteen print the
     * promise in their feature text, so seven of them -- the Hexblade, the
     * Armorer, the Battle Smith, the Artillerist, the Forge Domain, the
     * College of Swords and the Bladesinger -- said "gain proficiency with
     * martial weapons" on the sheet and left the attack bonus short. It
     * comes off the row now, so a subclass added to data/ brings its
     * proficiencies with it. */
    {
        int sub = c->classes[slot].subclass_id;
        char buf[256], *cursor = buf, *piece;

        snprintf(buf, sizeof buf, "%s", SUBCLASSES[sub].grants);
        while ((piece = next_csv(&cursor)) != NULL) {
            if (!*piece) continue;
            /* A kit or a set of tools is a tool; everything else -- armour,
               a shield, a weapon, a skill -- is a proficiency. */
            if (strstr(piece, "tools") || strstr(piece, "kit")) {
                add_tool(c, piece);
            } else {
                int sk = skill_by_name(piece);
                if (sk >= 0) c->skill_prof[sk] = 1;
                else add_prof(c, piece);
            }
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
        opts[n++] = all[i];
    }

    /* Tasha's Fighting Style Options widens the list. */
    if (has_optional_feature(c, "Fighting Style Options")) {
        const char *src = (class_id == CLS_PALADIN) ? TASHA_PALADIN_STYLES
                        : (class_id == CLS_RANGER)  ? TASHA_RANGER_STYLES
                        : TASHA_FIGHTER_STYLES;
        extra_n = split_pipe(src, extra_buf, sizeof extra_buf, extra_parts, 8);
        for (i = 0; i < extra_n && n < 16; i++) opts[n++] = extra_parts[i];
    }

    {
        /* ui_menu_custom copies the label it was shown, so a Tasha's style
           and a typed one are recorded the same way a PHB one is. */
        char answer[MAX_TEXT];
        ui_menu_custom("  Fighting Style:", opts, NULL, n,
                       "Another fighting style (type it in)",
                       answer, sizeof answer);
        add_choice(c, "Fighting Style", answer);
    }
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
int has_optional_feature(const Character *c, const char *name)
{
    return has_choice_starting(c, "Optional feature", name);
}

static void offer_optional_features(Character *c, int class_id, int class_level)
{
    int i;

    for (i = 0; i < OPTIONAL_FEATURE_COUNT; i++) {
        const OptionalFeature *of = &OPTIONAL_FEATURES[i];
        char prompt[MAX_NAME + 64];

        if (of->class_id != class_id || of->level != class_level) continue;
        if (!SETTINGS.optional_features || !book_enabled(of->book)) continue;
        if (has_choice_starting(c, "Optional feature", of->name)) continue;

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
            if (has_choice_starting(c, "Optional feature", NAMES[k])) {
                /* Only the one matching this class can be taken by it. */
                const ClassData *cd = &CLASSES[class_id];
                char expect[MAX_NAME];
                snprintf(expect, sizeof expect, "Additional %s Spells",
                         cd->name);
                if (has_choice_starting(c, "Optional feature", expect)) return ADDITIONAL_SPELLS[i].spells;
            }
        }
        return NULL;
    }
    return NULL;
}

/* True when `name` appears in a comma separated list. */
static const int *bonus_spell_levels(int class_id);

/* A warlock's patron does not hand over its spells. The Player's Handbook
 * says the expanded list is "added to the warlock spell list for you" --
 * it widens what you may choose, and you still choose every spell you know.
 * Every other subclass with bonus spells is the opposite: a cleric's
 * domain, a paladin's oath, a druid's circle are handed over and always
 * prepared, and do not touch the number the class may learn.
 *
 * The program granted them to a warlock as well, with the always-prepared
 * flag off. That was wrong twice over. The spells arrived unchosen, and
 * because they were not flagged they counted against the allowance --
 * recorded_for() sees them, manage_spells subtracts them, and the total
 * reaches zero. A 5th-level warlock was told "You know 6 spells" and then
 * asked nothing at all: every one of its six was the patron's.
 *
 * So the same rows are read back out here and offered as the extra list
 * instead. Returns NULL for anyone else.
 */
const char *warlock_expanded_list(const Character *c, int class_id)
{
    static char out[2048];
    const SubclassData *sc;
    char buf[1024];
    const char *groups[8];
    const int *at;
    int slot, sub, lvl, n, g;
    size_t used = 0;

    if (class_id != CLS_WARLOCK) return NULL;
    slot = find_class_slot(c, class_id);
    if (slot < 0) return NULL;
    sub = c->classes[slot].subclass_id;
    if (sub < 0) return NULL;

    lvl = c->classes[slot].level;
    at = bonus_spell_levels(class_id);
    out[0] = '\0';

    sc = &SUBCLASSES[sub];
    if (sc->bonus_spells && sc->bonus_spells[0]) {
        n = split_pipe(sc->bonus_spells, buf, sizeof buf, groups, 8);
        for (g = 0; g < n && g < 5; g++) {
            if (lvl < at[g]) break;
            used += (size_t)snprintf(out + used, sizeof out - used, "%s%s",
                                     used ? "," : "", groups[g]);
            if (used >= sizeof out) { used = sizeof out - 1; break; }
        }
    }

    /* The Genie warlock's kind hangs off the option rather than the
       subclass, exactly as it does for a druid's terrain. */
    {
        int opt = c->classes[slot].subclass_option;
        int i;
        for (i = 0; opt >= 0 && i < OPTION_SPELLS_COUNT; i++) {
            char obuf[512], sbuf[1024], lbuf[64];
            const char *onames[16], *ogroups[8];
            int no, ng, oat[8], nat = 0;
            const char *q;

            if (!subclass_is(sub, OPTION_SPELLS[i].subclass)) continue;
            no = split_pipe(SUBCLASSES[sub].options, obuf, sizeof obuf,
                            onames, 16);
            if (opt >= no) continue;
            if (strcmp(onames[opt], OPTION_SPELLS[i].option) != 0) continue;

            strncpy(lbuf, OPTION_SPELLS[i].levels, sizeof lbuf - 1);
            lbuf[sizeof lbuf - 1] = '\0';
            for (q = lbuf; *q && nat < 8; ) {
                while (*q == ' ' || *q == ',') q++;
                if (*q >= '0' && *q <= '9') {
                    int v = 0;
                    while (*q >= '0' && *q <= '9') v = v * 10 + (*q++ - '0');
                    oat[nat++] = v;
                } else if (*q) {
                    q++;
                }
            }
            ng = split_pipe(OPTION_SPELLS[i].spells, sbuf, sizeof sbuf,
                            ogroups, 8);
            for (g = 0; g < ng && g < nat; g++) {
                if (lvl < oat[g]) break;
                used += (size_t)snprintf(out + used, sizeof out - used,
                                         "%s%s", used ? "," : "", ogroups[g]);
                if (used >= sizeof out) { used = sizeof out - 1; break; }
            }
            break;
        }
    }

    return out[0] ? out : NULL;
}

/* Everything this class may choose from beyond its own list: Tasha's
   additional spells, and a warlock patron's expanded list. */
static const char *choosable_extras(const Character *c, int class_id)
{
    static char both[3072];
    const char *tasha = additional_spells_for(c, class_id);
    const char *patron = warlock_expanded_list(c, class_id);

    if (!tasha) return patron;
    if (!patron) return tasha;
    snprintf(both, sizeof both, "%s,%s", tasha, patron);
    return both;
}

/* Whether a comma-separated list names this spell.
 *
 * Case-insensitively, because the two lists it reads are written by
 * different hands. Tasha's additional spells are stored entirely in lower
 * case, so a case-sensitive compare worked for as long as that was the only
 * caller. A warlock patron's expanded list is stored the way the book
 * prints it, which keeps the capital in "Tasha's hideous laughter" and
 * "Evard's black tentacles" -- and those two spells silently fell off the
 * list the moment a patron's list was offered rather than granted. */
static int name_in_list(const char *list, const char *name)
{
    const char *p = list;
    size_t len = strlen(name);

    while (p && *p) {
        size_t i;
        while (*p == ' ' || *p == ',') p++;
        for (i = 0; i < len; i++) {
            int a = (unsigned char)p[i], b = (unsigned char)name[i];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) break;
        }
        if (i == len && (p[len] == '\0' || p[len] == ',')) return 1;
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
    for (i = 0; i < SPELL_COUNT; i++) {
        if (same_fold(SPELLS[i].name, name)) return i;
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
    const char *extra = choosable_extras(c, class_id);
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
    char names[256], *cursor = names, *piece;

    strncpy(names, group, sizeof names - 1);
    names[sizeof names - 1] = '\0';
    while ((piece = next_csv(&cursor)) != NULL) {
        if (*piece) {
            int sid = find_spell_by_name(piece);
            if (sid >= 0) add_spell(c, sid, class_id, 1, always);
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
    if (id == CLS_WARLOCK) return;      /* a menu, not a grant */

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
            grant_spell_group(c, groups[g], id, 1);
        }
        return;
    }
}

/* The class levels at which each group of a subclass's bonus spells opens.
   Domain and origin spells arrive at 1/3/5/7/9; druid circle spells at
   2/3/5/7/9; oath, archetype and specialist spells at 3/5/9/13/17. A
   warlock patron's expanded list unlocks with the spell level, which for a
   warlock is class level 1/3/5/7/9. */
static const int *bonus_spell_levels(int class_id)
{
    static const int early_at[5]   = {1, 3, 5, 7, 9};
    static const int druid_at[5]   = {2, 3, 5, 7, 9};
    static const int martial_at[5] = {3, 5, 9, 13, 17};

    if (class_id == CLS_PALADIN || class_id == CLS_RANGER
        || class_id == CLS_ARTIFICER) {
        return martial_at;
    }
    if (class_id == CLS_DRUID) return druid_at;
    return early_at;
}

/* Adds the always-prepared domain, oath, circle or patron spells earned so
   far. A warlock is not here: see warlock_expanded_list(). */
static void add_bonus_spells(Character *c, int slot)
{
    const SubclassData *sc;
    int sub = c->classes[slot].subclass_id;
    int id = c->classes[slot].class_id;
    int lvl = c->classes[slot].level;
    char buf[1024];
    const char *groups[8];
    const int *at = bonus_spell_levels(id);
    int n, g;

    if (sub < 0) return;
    if (id == CLS_WARLOCK) return;
    sc = &SUBCLASSES[sub];
    if (!sc->bonus_spells || !sc->bonus_spells[0]) return;

    n = split_pipe(sc->bonus_spells, buf, sizeof buf, groups, 8);
    for (g = 0; g < n && g < 5; g++) {
        if (lvl < at[g]) break;
        grant_spell_group(c, groups[g], id, 1);
    }
}

/* How many of this class's spells are actually ticked as prepared, as
   against spells_prepared_count(), which is how many the rules allow. */
static int spells_marked_prepared(const Character *c, int class_id)
{
    int i, n = 0;
    for (i = 0; i < c->spell_count; i++) {
        if (c->spells[i].class_id != class_id) continue;
        if (c->spells[i].always_prepared) continue;
        if (SPELLS[c->spells[i].spell_id].level == 0) continue;
        if (c->spells[i].prepared) n++;
    }
    return n;
}

/* Removes one spell from the character, closing the gap behind it. */
static void forget_spell(Character *c, int at)
{
    int i;
    for (i = at; i + 1 < c->spell_count; i++) c->spells[i] = c->spells[i + 1];
    c->spell_count--;
}

/* Trading a spell for another on the day you gain a level.
 *
 * The Player's Handbook gives this to the classes that know a fixed list --
 * a bard, ranger, sorcerer or warlock -- because for them a spell chosen at
 * 1st level is otherwise carried to 20th. A wizard adds to a spellbook
 * rather than replacing anything in it, and a cleric, druid or paladin
 * prepares afresh from the whole list every day, so for those three there
 * is nothing a swap would mean.
 */
static void offer_spell_swap(Character *c, int class_id)
{
    const ClassData *cd = &CLASSES[class_id];
    const char *opts[MAX_SPELLS];
    static char lines[MAX_SPELLS][120];
    int map[MAX_SPELLS], n = 0, i, slot, bit, maxlvl, pick;

    slot = find_class_slot(c, class_id);
    if (slot < 0) return;

    /* The classes that know a fixed list, and the two archetypes that do:
       "Whenever you gain a level in this class, you can replace one of the
       wizard spells you know with another" is printed for the Eldritch
       Knight and the Arcane Trickster in the same words the bard, ranger,
       sorcerer and warlock get it in. The fighter and the rogue are
       PREP_NONE in the class table, so the guard on prep alone never
       reached them. */
    if (cd->prep != PREP_KNOWN
        && !is_third_caster(c->classes[slot].subclass_id)) {
        return;
    }
    bit = spell_class_bit(class_id, c->classes[slot].subclass_id);
    if (!bit) return;
    maxlvl = max_spell_level_for(c, slot);
    if (maxlvl < 1) return;

    for (i = 0; i < c->spell_count && n < MAX_SPELLS; i++) {
        if (c->spells[i].class_id != class_id) continue;
        if (c->spells[i].always_prepared) continue;
        if (SPELLS[c->spells[i].spell_id].level == 0) continue;
        snprintf(lines[n], sizeof lines[n], "%s (level %d)",
                 SPELLS[c->spells[i].spell_id].name,
                 SPELLS[c->spells[i].spell_id].level);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    if (n == 0) return;

    printf("\n  Gaining a level lets you replace one %s spell you know "
           "with another.\n",
           is_third_caster(c->classes[slot].subclass_id)
               ? SUBCLASSES[c->classes[slot].subclass_id].name : cd->name);
    if (!ui_yesno("  Swap one out?", 0)) return;

    opts[n] = "Change my mind";
    pick = ui_menu("  Replace which spell?", opts, NULL, n + 1);
    if (pick == n) return;

    {
        int gone = map[pick];
        int level = SPELLS[c->spells[gone].spell_id].level;
        const char *name = SPELLS[c->spells[gone].spell_id].name;
        /* The replacement has to be one this character could have learned
           in the first place, so it is drawn from the same list at a level
           they can already cast. */
        int want = level <= maxlvl ? level : maxlvl;

        if (available_count(c, bit, want,
                            choosable_extras(c, class_id)) == 0) {
            printf("  There is no other %s spell of level %d to take "
                   "instead, so %s stays.\n", cd->name, want, name);
            return;
        }
        printf("  Forgetting %s.\n", name);
        forget_spell(c, gone);
        pick_spells(c, bit, class_id, want, 1, "replacement");
    }
}

/* Which spells are prepared today.
 *
 * A cleric, druid, paladin or artificer prepares a fresh list each day from
 * the whole class list, and a wizard prepares from the spellbook. The
 * program recorded every spell as prepared and never asked, which is right
 * on the day the character is made and wrong every day after. Cantrips are
 * always ready and domain and oath spells are always prepared, so neither
 * appears here -- only the spells there is a decision to make about.
 *
 * The list is offered a spell level at a time because ui_toggle_list holds
 * 63 entries and a wizard's spellbook can hold more than that.
 */
static void mark_prepared(Character *c, int class_id)
{
    const ClassData *cd = &CLASSES[class_id];
    int slot = find_class_slot(c, class_id);
    int allowed, maxlvl, lvl, total = 0;

    if (cd->prep != PREP_PREPARED && cd->prep != PREP_SPELLBOOK) return;
    if (slot < 0) return;
    maxlvl = max_spell_level_for(c, slot);
    if (maxlvl < 1) return;

    allowed = spells_prepared_count(c, class_id);
    for (lvl = 1; lvl <= maxlvl; lvl++) {
        int i;
        for (i = 0; i < c->spell_count; i++) {
            if (c->spells[i].class_id != class_id) continue;
            if (c->spells[i].always_prepared) continue;
            if (SPELLS[c->spells[i].spell_id].level == lvl) total++;
        }
    }
    if (total == 0) return;

    printf("\n  You may have %d %s spell%s prepared at once.\n",
           allowed, cd->name, allowed == 1 ? "" : "s");
    if (!ui_yesno("  Say which ones are prepared now?", 0)) return;

    for (lvl = 1; lvl <= maxlvl; lvl++) {
        const char *opts[64];
        static char lines[64][120];
        int flags[64], map[64], n = 0, i;
        char prompt[96];

        for (i = 0; i < c->spell_count && n < 63; i++) {
            if (c->spells[i].class_id != class_id) continue;
            if (c->spells[i].always_prepared) continue;
            if (SPELLS[c->spells[i].spell_id].level != lvl) continue;
            snprintf(lines[n], sizeof lines[n], "%s",
                     SPELLS[c->spells[i].spell_id].name);
            opts[n] = lines[n];
            flags[n] = c->spells[i].prepared;
            map[n] = i;
            n++;
        }
        if (n == 0) continue;

        snprintf(prompt, sizeof prompt,
                 "  Level %d -- prepared today (%d of %d used):",
                 lvl, spells_marked_prepared(c, class_id), allowed);
        ui_toggle_list(prompt, opts, n, flags);
        for (i = 0; i < n; i++) c->spells[map[i]].prepared = flags[i];
    }

    {
        int have = spells_marked_prepared(c, class_id);
        if (have > allowed) {
            printf("  That is %d prepared where you may have %d. The sheet "
                   "records what you chose; agree the rest with your DM.\n",
                   have, allowed);
        }
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
        const char *extra = choosable_extras(c, class_id);
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
                            choosable_extras(c, class_id)) == 0) {
            printf("    No %s spells of that level are left to learn.\n",
                   cd->name);
            /* If nothing is left anywhere, stop rather than ask again. */
            {
                const char *extra2 = choosable_extras(c, class_id);
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

/* ------------------------------------------------- class option lists */

/* Offer everything this class and subclass draw from, at this class level.
 * Each list says how many are known by now; the difference from what has
 * already been recorded is what is still owed, so a character who levels up
 * in stages is asked exactly once for each. */
/* Records `count` choices from one option list.
 *
 * `label` is what they are filed under, and `alt_label` a second label whose
 * entries also count as already taken -- an eldritch invocation is an
 * eldritch invocation whether the warlock class or the Eldritch Adept feat
 * supplied it, and the same one cannot be had twice.
 *
 * `allow_prereq` is 0 when the list is being drawn on from outside the class
 * that owns it: Tasha's lets a non-warlock take an invocation through a feat
 * only if it has no prerequisite at all.
 *
 * Below the book's entries there is always one more, for whatever the table
 * has agreed on instead.
 */
static void pick_from_option_list(Character *c, const OptionList *ol,
                                  const char *label, const char *alt_label,
                                  int class_level, int allow_prereq,
                                  int count)
{
    while (count-- > 0) {
        const char *opts[256];
        const char *det[256];
        static char labels[256][128];
        int map[256], n = 0, i, pick;
        char prompt[96], custom[128], answer[MAX_TEXT];

        for (i = 0; i < ol->count && n < 256; i++) {
            const ClassOption *o = &ol->options[i];
            if (!book_enabled(o->book)) continue;
            if (!allow_prereq && (o->prereq[0] || o->min_level > 0)) continue;
            if (o->min_level > class_level) continue;
            if (!ol->repeatable
                && (has_choice_exactly(c, label, o->name)
                    || (alt_label
                        && has_choice_exactly(c, alt_label, o->name))))
                continue;

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

        snprintf(prompt, sizeof prompt, "  Choose a %s:", ol->label);
        snprintf(custom, sizeof custom, "Another %s (type it in)", ol->label);

        if (n == 0) {
            /* Every printed entry is taken or out of reach. The choice is
               still owed, so ask for it rather than dropping it silently. */
            printf("    Nothing further is printed for you to take.\n");
            ui_line("  Name one your table uses", answer, sizeof answer);
            if (!answer[0]) return;
            add_choice(c, label, answer);
            continue;
        }

        pick = ui_menu_custom(prompt, opts, det, n, custom,
                              answer, sizeof answer);
        if (pick >= 0) {
            add_choice(c, label, ol->options[map[pick]].name);
        } else if (!ol->repeatable
                   && (has_choice_exactly(c, label, answer)
                       || (alt_label
                           && has_choice_exactly(c, alt_label, answer)))) {
            printf("    You already have that; choose again.\n");
            count++;
        } else {
            add_choice(c, label, answer);
        }
    }
}

/* The list a class or a feat draws on, found by the plural name its registry
   entry carries. */
static const OptionList *option_list_named(const char *plural)
{
    int i;
    for (i = 0; i < OPTION_LIST_COUNT; i++) {
        if (strcmp(OPTION_LISTS[i].plural, plural) == 0)
            return &OPTION_LISTS[i];
    }
    return NULL;
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

        want = (int)ol->known[class_level] - count_choices(c, ol->label);
        if (want <= 0) continue;

        printf("\n  %s knows %d %s at level %d.\n", cd->name,
               (int)ol->known[class_level], ol->plural, class_level);
        pick_from_option_list(c, ol, ol->label, NULL, class_level, 1, want);
    }
}


/* --------------------------------------------- what a feat then asks for */

/* Several feats hand you a further choice: an eldritch invocation, two
 * metamagic options, a fighting style, a tool, a spell. Taking the feat used
 * to record only its name and any ability increase, which left the player to
 * remember the rest. What a feat grants is now asked for and recorded beside
 * it -- as a choice, a proficiency or a spell -- so it reaches the sheet and
 * the save file the way anything else does.
 */

/* A spell a feat grants outright. It is filed under no class, so it never
   counts against a class's cantrips or spells known. */
static void grant_feat_spell(Character *c, const char *name)
{
    int id = find_spell_by_name(name);

    if (id < 0) return;
    if (already_known(c, id)) return;
    add_spell(c, id, -1, 1, 1);
    printf("    You learn %s.\n", SPELLS[id].name);
}

/* One spell of a given level, from a class's list, a pair of schools, or
   both. A mask of 0 means "no restriction on that axis". */
static void pick_feat_spell(Character *c, const char *prompt, int level,
                            unsigned class_mask, unsigned school_mask)
{
    const char *opts[256];
    static char labels[256][96];
    int map[256], n = 0, i;
    char answer[MAX_TEXT];

    for (i = 0; i < SPELL_COUNT && n < 256; i++) {
        if (SPELLS[i].level != level) continue;
        if (!book_enabled((SourceBook)SPELLS[i].book)) continue;
        if (class_mask && !(SPELLS[i].classes & class_mask)) continue;
        if (school_mask && !(school_mask & (1u << SPELLS[i].school))) continue;
        if (already_known(c, i)) continue;
        snprintf(labels[n], sizeof labels[n], "%s (%s)", SPELLS[i].name,
                 SCHOOL_NAMES[SPELLS[i].school]);
        opts[n] = labels[n];
        map[n] = i;
        n++;
    }
    if (n == 0) {
        printf("    Nothing is left for you to learn there.\n");
        return;
    }
    i = ui_menu_custom(prompt, opts, NULL, n, "Another spell (type it in)",
                       answer, sizeof answer);
    if (i >= 0) {
        add_spell(c, map[i], -1, 1, 1);
        printf("    You learn %s.\n", SPELLS[map[i]].name);
        return;
    }
    /* A spell the tables do not carry has no entry to point at, so it is
       recorded as a note beside the feat instead. */
    add_choice(c, "Spell from a feat", answer);
}

/* One proficiency from a group of tools. */
static void pick_feat_tool(Character *c, const char *prompt, const char *group)
{
    const char *all[64];
    const char *opts[64];
    int total = tools_in_group(group, all, 64), n = 0, i;
    char answer[MAX_NAME];

    for (i = 0; i < total; i++) {
        if (!has_tool(c, all[i])) opts[n++] = all[i];
    }
    if (n == 0) {
        ui_line("  Name a tool you are not already proficient with",
                answer, sizeof answer);
    } else {
        ui_menu_custom(prompt, opts, NULL, n, "Another tool (type it in)",
                       answer, sizeof answer);
    }
    add_tool(c, answer);
}

/* The feats feat_extras() below asks a further question for. It is a
   separate list only so the self-test can check that each still names a real
   feat: the branches match on the printed name, so a rename in the data
   would otherwise stop the questions being asked without anything failing. */
const char *const FEATS_WITH_CHOICES[] = {
    "Artificer Initiate", "Chef", "Eldritch Adept", "Fey Touched",
    "Fighting Initiate", "Gunner", "Metamagic Adept", "Poisoner",
    "Shadow Touched", "Skill Expert", "Telekinetic", "Telepathic"
};
const int FEATS_WITH_CHOICES_COUNT =
    (int)(sizeof(FEATS_WITH_CHOICES) / sizeof(FEATS_WITH_CHOICES[0]));

static void feat_extras(Character *c, int feat_id)
{
    const char *name = FEATS[feat_id].name;
    const OptionList *ol;

    if (!strcmp(name, "Eldritch Adept")) {
        /* Tasha's, p.79: an invocation with a prerequisite is open only to
           a warlock who meets it. */
        int wl = class_level_of(c, CLS_WARLOCK);
        ol = option_list_named("eldritch invocations");
        if (ol) pick_from_option_list(c, ol, "Eldritch Adept invocation",
                                      "Eldritch Invocation", wl, wl > 0, 1);

    } else if (!strcmp(name, "Metamagic Adept")) {
        ol = option_list_named("metamagic options");
        if (ol) pick_from_option_list(c, ol, "Metamagic Adept option",
                                      "Metamagic option", MAX_LEVEL, 1, 2);

    } else if (!strcmp(name, "Fighting Initiate")) {
        choose_fighting_style(c, CLS_FIGHTER);

    } else if (!strcmp(name, "Skill Expert")) {
        const char *opts[SKL_COUNT];
        int avail[SKL_COUNT], picks[1], i;
        for (i = 0; i < SKL_COUNT; i++) {
            opts[i] = SKILL_NAME[i];
            avail[i] = !c->skill_prof[i];
        }
        printf("    Skill Expert: a skill to learn, and expertise in one"
               " you already have.\n");
        if (ui_multi("  A skill to become proficient in:",
                     opts, avail, SKL_COUNT, 1, picks) > 0 && picks[0] >= 0) {
            c->skill_prof[picks[0]] = 1;
        }
        choose_expertise(c, 1);

    } else if (!strcmp(name, "Chef")) {
        add_tool(c, "Cook's utensils");
        printf("    You gain proficiency with cook's utensils.\n");

    } else if (!strcmp(name, "Poisoner")) {
        add_tool(c, "Poisoner's kit");
        printf("    You gain proficiency with a poisoner's kit.\n");

    } else if (!strcmp(name, "Gunner")) {
        add_prof(c, "Firearms");
        printf("    You gain proficiency with firearms.\n");

    } else if (!strcmp(name, "Artificer Initiate")) {
        pick_feat_spell(c, "  An artificer cantrip:", 0, SPL_ARTIFICER, 0);
        pick_feat_spell(c, "  A 1st-level artificer spell:", 1,
                        SPL_ARTIFICER, 0);
        pick_feat_tool(c, "  Artisan's tools to become proficient with:",
                       "Artisan's tools");

    } else if (!strcmp(name, "Fey Touched")) {
        grant_feat_spell(c, "Misty Step");
        pick_feat_spell(c, "  A 1st-level divination or enchantment spell:",
                        1, 0, (1u << SCHOOL_DIVINATION)
                            | (1u << SCHOOL_ENCHANTMENT));

    } else if (!strcmp(name, "Shadow Touched")) {
        grant_feat_spell(c, "Invisibility");
        pick_feat_spell(c, "  A 1st-level illusion or necromancy spell:",
                        1, 0, (1u << SCHOOL_ILLUSION)
                            | (1u << SCHOOL_NECROMANCY));

    } else if (!strcmp(name, "Telekinetic")) {
        grant_feat_spell(c, "Mage Hand");

    } else if (!strcmp(name, "Telepathic")) {
        grant_feat_spell(c, "Detect Thoughts");
    }
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
        && !count_choices(c, "Animal Companion")) {
        printf("\n  Your animal companion is a beast of challenge 1/4 or "
               "lower, no larger than Medium and with no flying speed.\n");
        choose_beast(c, "Animal Companion", "  Animal companion:", 2,
                     BSIZE_MEDIUM, 0);
    }

    /* Pact of the Chain names its own familiar forms on top of the usual
       ones; the base list is offered to anyone who has find familiar. */
    if (has_choice_exactly(c, "Pact Boon", "Pact of the Chain")
        && !count_choices(c, "Familiar")) {
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

    /* Multiclassing is asked before the class list rather than left for
       the player to spot inside it. A list of all thirteen classes with
       "(continue)" and "(new class)" mixed together makes taking a level in
       something new look like the same kind of choice as carrying on, when
       it is the one decision on this screen that cannot be undone by
       carrying on differently next time.
     *
     * Answering no narrows the list to the classes already taken, and where
     * there is only one there is nothing to narrow to -- that level goes
     * where the only class is, with no menu at all. */
    {
        int have = c->class_count;
        int branching = 0;

        if (SETTINGS.multiclassing && total_level(c) < MAX_LEVEL) {
            branching = ui_yesno("\n  Are you going to multiclass?", 0);
        }

        if (!branching && have == 1) {
            pick = c->classes[0].class_id;
            printf("  Another level of %s.\n", CLASSES[pick].name);
        } else {
            int map[CLASS_COUNT], n = 0;
            for (i = 0; i < CLASS_COUNT; i++) {
                int cur = class_level_of(c, i);
                int why, ok;
                if (branching && cur > 0) continue;   /* already have it */
                if (!branching && cur == 0) continue; /* not one of theirs */
                ok = (cur > 0) || multiclass_ok_public(c, i, &why);
                snprintf(labels[n], sizeof labels[n], "%s%s%s",
                         CLASSES[i].name,
                         cur ? " (continue)" : " (new class)",
                         ok ? "" : " -- prerequisites not met");
                opts[n] = labels[n];
                det[n] = NULL;
                map[n] = i;
                n++;
            }
            /* Every class already taken, so there is nothing new to take. */
            if (n == 0) {
                printf("  You already have a level in every class this "
                       "program tracks.\n");
                return;
            }
            for (;;) {
                int why;
                pick = map[ui_menu(branching ? "Start which new class?"
                                             : "Take your next level in:",
                                   opts, det, n)];
                if (class_level_of(c, pick) > 0) break;
                if (multiclass_ok_public(c, pick, &why)) break;
                printf("  You do not meet the multiclassing prerequisites "
                       "for %s.\n", CLASSES[pick].name);
                if (!ui_yesno("  Choose a different class?", 1)) break;
            }
        }
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
        add_prof_list(c, CLASSES[pick].mc_profs, CLASSES[pick].name);
    }

    if (ui_yesno("\n  Roll the hit die for this level?", 0)) hp_use_average = 0;
    else hp_use_average = 1;

    newlvl = ++c->classes[slot].level;
    apply_class_level(c, slot, newlvl, 0);

    manage_spells(c, pick);
    offer_spell_swap(c, pick);
    mark_prepared(c, pick);

    /* Experience is the table's business, not the program's: it never
       awards any and never levels anybody up on its own. What it can do is
       keep the running total beside the threshold, which is the only part
       a player has to look up. */
    if (SETTINGS.experience) {
        int lv = total_level(c);
        printf("\n  Experience so far: %d.", c->xp);
        if (lv < MAX_LEVEL) {
            printf(" Level %d wants %d.", lv + 1, XP_FOR_LEVEL[lv + 1]);
        }
        printf("\n");
        if (ui_yesno("  Add experience earned?", 0)) {
            int add = ui_int("  How much", 0, 999999);
            if (add > 9999999 - c->xp) add = 9999999 - c->xp;
            c->xp += add;
            printf("  Experience is now %d.\n", c->xp);
            if (lv < MAX_LEVEL && c->xp >= XP_FOR_LEVEL[lv + 1]) {
                printf("  That is enough for level %d whenever your DM "
                       "says so.\n", lv + 1);
            }
        }
    }

    printf("\n  %s is now ", c->name);
    for (i = 0; i < c->class_count; i++) {
        printf("%s%s %d", i ? " / " : "",
               CLASSES[c->classes[i].class_id].name, c->classes[i].level);
    }
    printf(" -- %d hit points.\n", hit_points_max(c));
}
