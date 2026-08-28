/* character.c -- derived statistics (PHB chapters 1, 5 and 7). */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const ABILITY_NAME[ABL_COUNT] = {
    "Strength", "Dexterity", "Constitution",
    "Intelligence", "Wisdom", "Charisma"
};
const char *const ABILITY_ABBREV[ABL_COUNT] = {
    "STR", "DEX", "CON", "INT", "WIS", "CHA"
};

const char *const SKILL_NAME[SKL_COUNT] = {
    "Acrobatics", "Animal Handling", "Arcana", "Athletics", "Deception",
    "History", "Insight", "Intimidation", "Investigation", "Medicine",
    "Nature", "Perception", "Performance", "Persuasion", "Religion",
    "Sleight of Hand", "Stealth", "Survival"
};
/* A skill by the name the data files use for it. -1 for anything else, so a
 * misspelling in a data file shows up as a skill nobody can pick rather than
 * as a proficiency in the wrong thing. */
int skill_by_name(const char *name)
{
    int i;
    for (i = 0; i < SKL_COUNT; i++) {
        if (strcmp(SKILL_NAME[i], name) == 0) return i;
    }
    return -1;
}

const Ability SKILL_ABILITY[SKL_COUNT] = {
    ABL_DEX, ABL_WIS, ABL_INT, ABL_STR, ABL_CHA,
    ABL_INT, ABL_WIS, ABL_CHA, ABL_INT, ABL_WIS,
    ABL_INT, ABL_WIS, ABL_CHA, ABL_CHA, ABL_INT,
    ABL_DEX, ABL_DEX, ABL_WIS
};

const char *const SIZE_NAME[] = { "Small", "Medium" };

const char *const ALIGNMENT_NAME[ALIGN_COUNT] = {
    "Lawful Good", "Neutral Good", "Chaotic Good",
    "Lawful Neutral", "True Neutral", "Chaotic Neutral",
    "Lawful Evil", "Neutral Evil", "Chaotic Evil"
};


int total_level(const Character *c)
{
    int i, n = 0;
    for (i = 0; i < c->class_count; i++) n += c->classes[i].level;
    return n;
}

int find_class_slot(const Character *c, int class_id)
{
    int i;
    for (i = 0; i < c->class_count; i++) {
        if (c->classes[i].class_id == class_id) return i;
    }
    return -1;
}

int class_level_of(const Character *c, int class_id)
{
    int i = find_class_slot(c, class_id);
    return i < 0 ? 0 : c->classes[i].level;
}

static int has_subclass_named(const Character *c, const char *name)
{
    int i;
    for (i = 0; i < c->class_count; i++) {
        if (subclass_is(c->classes[i].subclass_id, name)) return 1;
    }
    return 0;
}

int has_feat(const Character *c, int feat_id)
{
    int i;
    for (i = 0; i < c->feat_count; i++) {
        if (c->feats[i] == feat_id) return 1;
    }
    return 0;
}

static int has_named_feat(const Character *c, const char *name)
{
    int i;
    for (i = 0; i < FEAT_COUNT; i++) {
        if (strcmp(FEATS[i].name, name) == 0) return has_feat(c, i);
    }
    return 0;
}

/* The rule a carried magic item brings, if it brings one and it is actually
 * in effect: an item needing attunement does nothing until attuned, and
 * armour or a shield does nothing until worn. */
static const MagicRule *rule_in_effect(const Character *c, int i)
{
    const InventoryEntry *e = &c->inventory[i];
    const MagicItem *m;
    const MagicRule *r;

    if (!e->is_magic) return NULL;
    m = &MAGIC_ITEMS[e->item_id];
    r = magic_rule_for(m->name);
    if (!r) return NULL;

    if (m->attunement && !e->attuned) return NULL;
    if (magic_rule_is_worn(r) && !e->equipped) return NULL;
    return r;
}

/* The magic armour, or the magic shield, a character is wearing. The two
   differ only in which column of the rule has to be set, and both callers
   want the rule as well as the entry, so it is handed back rather than
   looked up a second time. */
static const InventoryEntry *worn_magic(const Character *c, int shield,
                                        const MagicRule **rule)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (r && (shield ? r->shield : r->armor_base)) {
            if (rule) *rule = r;
            return &c->inventory[i];
        }
    }
    if (rule) *rule = NULL;
    return NULL;
}

int ability_score(const Character *c, Ability a)
{
    int score = c->base_score[a] + c->racial_bonus[a] + c->asi_bonus[a];
    int i;

    /* An amulet of health or gauntlets of ogre power set a score outright,
       and only help if the score is not already higher. */
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        int set;
        if (!r || r->sets_ability != (int)a + 1) continue;
        /* A belt of giant strength carries its own score. */
        set = r->sets_to ? r->sets_to : c->inventory[i].plus;
        if (set > score) score = set;
    }

    /* "the range of possible ability scores, from 1 to 30" (PHB p.173).
       Nothing the program itself builds comes near either end -- an
       improvement stops at 20 and the largest belt sets 29 -- but a sheet
       is a text file, and a hand-edited one that says two billion here used
       to be multiplied by fifteen for the carrying capacity. */
    if (score < 1) score = 1;
    if (score > 30) score = 30;
    return score;
}

int ability_mod_of(int score)
{
    /* Floor division: the modifier for a score below 10 rounds down. */
    int d = score - 10;
    return (d >= 0) ? d / 2 : -(((-d) + 1) / 2);
}

int ability_mod(const Character *c, Ability a)
{
    return ability_mod_of(ability_score(c, a));
}

int proficiency_bonus(const Character *c)
{
    int lvl = total_level(c);
    if (lvl < 1) lvl = 1;
    return 2 + (lvl - 1) / 4;
}

int skill_bonus(const Character *c, Skill s)
{
    int pb = proficiency_bonus(c);
    int bonus = ability_mod(c, SKILL_ABILITY[s]);

    if (c->skill_prof[s]) {
        bonus += pb;
        if (c->skill_expertise[s]) bonus += pb;
        return bonus;
    }

    /* Two features add half the proficiency bonus to a check that does not
       already use it: the bard's Jack of All Trades, rounded down and on
       any of them, and the Champion's Remarkable Athlete, rounded up and
       only on Strength, Dexterity and Constitution. A character with both
       adds the better of the two rather than both -- they are the same
       half of the same bonus. */
    {
        Ability a = SKILL_ABILITY[s];
        int half = 0;

        if (class_level_of(c, CLS_BARD) >= 2) half = pb / 2;
        if (class_level_of(c, CLS_FIGHTER) >= 7
            && has_subclass_named(c, "Champion")
            && (a == ABL_STR || a == ABL_DEX || a == ABL_CON)
            && (pb + 1) / 2 > half) {
            half = (pb + 1) / 2;
        }
        bonus += half;
    }
    return bonus;
}

int save_bonus(const Character *c, Ability a)
{
    int bonus = ability_mod(c, a);
    int i;

    /* A monk of 14th level is proficient in every saving throw. */
    if (c->save_prof[a] || class_level_of(c, CLS_MONK) >= 14) {
        bonus += proficiency_bonus(c);
    }
    /* A ring or cloak of protection raises every save, not just one. */
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (r) bonus += r->save_bonus;
    }
    return bonus;
}

int passive_perception(const Character *c)
{
    int p = 10 + skill_bonus(c, SKL_PERCEPTION);
    if (has_named_feat(c, "Observant")) p += 5;
    return p;
}

static const InventoryEntry *equipped_of(const Character *c, ItemCategory cat)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        if (!c->inventory[i].equipped) continue;
        if (c->inventory[i].is_magic) continue;
        if (ITEMS[c->inventory[i].item_id].category == cat) {
            return &c->inventory[i];
        }
    }
    return NULL;
}

/* What the character is actually wearing. Three rules of the PHB turn on
   it: Unarmored Defense, Unarmored Movement and Fast Movement, each with
   its own idea of what counts, so all three answers are worked out here and
   the callers take the ones they need.

   Heavy is judged from the armour's own row, and from a magic armour only
   when the rule states a base and admits no Dexterity -- which is what
   heavy armour is. A "+1, +2, or +3" suit states no base and could be any
   armour at all, so it is not called heavy on a guess. */
static void worn_armour_state(const Character *c, int *armour, int *shield,
                              int *heavy, int *too_heavy)
{
    const InventoryEntry *worn = NULL;
    int is_heavy, is_too_heavy;
    int cat, i;

    /* The last two are what only the speed cares about, and are passed as
       NULL by the caller that does not. */
    if (!heavy) heavy = &is_heavy;
    if (!too_heavy) too_heavy = &is_too_heavy;

    for (cat = ITEM_LIGHT_ARMOR; cat <= ITEM_HEAVY_ARMOR && !worn; cat++) {
        worn = equipped_of(c, (ItemCategory)cat);
    }
    *armour = (worn != NULL);
    *shield = (equipped_of(c, ITEM_SHIELD) != NULL);
    *heavy = (worn && ITEMS[worn->item_id].category == ITEM_HEAVY_ARMOR);

    /* "If the Armor table shows Str 13 or Str 15 for an armor, the armor
       reduces the wearer's speed by 10 feet unless the wearer has a
       Strength score equal to or higher than the listed score" (PHB p.144).
       The equipment table carries the requirement and the reference screen
       reads it out; until now nothing subtracted the ten feet. */
    *too_heavy = (worn && ITEMS[worn->item_id].str_req
                  && ability_score(c, ABL_STR) < ITEMS[worn->item_id].str_req);

    /* Magic armour counts as armour whether or not its rule states a base
       Armor Class: a "+1, +2, or +3" suit states none, and a monk wearing
       one is wearing armour as surely as one in plate. */
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (!r || !magic_rule_is_worn(r)) continue;
        if (r->shield) {
            *shield = 1;
        } else {
            *armour = 1;
            if (r->armor_base && r->armor_dex == 0) *heavy = 1;
            /* Magic armour carries its Strength requirement too: dwarven
               plate is plate, and slows a wearer who cannot bear it. */
            if (r->armor_str && ability_score(c, ABL_STR) < r->armor_str) {
                *too_heavy = 1;
            }
        }
    }
}

static int has_choice_containing(const Character *c, const char *label,
                                 const char *needle)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) != 0) continue;
        if (strstr(c->choices[i].value, needle)) return 1;
    }
    return 0;
}

int armour_class(const Character *c)
{
    const InventoryEntry *armour = NULL;
    const InventoryEntry *shield = equipped_of(c, ITEM_SHIELD);
    const MagicRule *armour_rule, *shield_rule;
    const InventoryEntry *magic_armour = worn_magic(c, 0, &armour_rule);
    const InventoryEntry *magic_shield = worn_magic(c, 1, &shield_rule);
    int dex = ability_mod(c, ABL_DEX);
    int best, cat, i;
    int wearing_armour, using_shield;

    for (cat = ITEM_LIGHT_ARMOR; cat <= ITEM_HEAVY_ARMOR && !armour; cat++) {
        armour = equipped_of(c, (ItemCategory)cat);
    }

    /* What is worn is asked of the one place that works it out, rather
       than judged from the two entries found above: a "+1, +2, or +3" suit
       states no base Armor Class and so is not one of them, and a monk
       wearing one is still wearing armour. */
    worn_armour_state(c, &wearing_armour, &using_shield, NULL, NULL);

    if (magic_armour) {
        const MagicRule *r = armour_rule;
        int add = (r->armor_dex < 0) ? dex
                : (r->armor_dex == 0) ? 0
                : (dex < r->armor_dex ? dex : r->armor_dex);
        best = r->armor_base + add;
    } else if (armour) {
        const ItemData *it = &ITEMS[armour->item_id];
        int add = (it->dex_cap < 0) ? dex
                : (it->dex_cap == 0) ? 0
                : (dex < it->dex_cap ? dex : it->dex_cap);
        best = it->base_ac + add;
    } else {
        best = 10 + dex;
    }

    /* Unarmored Defense and Draconic Resilience apply only without armour;
       the monk's version also forbids a shield. */
    if (!wearing_armour) {
        int alt;
        if (class_level_of(c, CLS_BARBARIAN) >= 1) {
            alt = 10 + dex + ability_mod(c, ABL_CON);
            if (alt > best) best = alt;
        }
        if (has_subclass_named(c, "Draconic Bloodline")) {
            alt = 13 + dex;
            if (alt > best) best = alt;
        }
        if (class_level_of(c, CLS_MONK) >= 1 && !using_shield) {
            alt = 10 + dex + ability_mod(c, ABL_WIS);
            if (alt > best) best = alt;
        }

        /* A robe that sets the base outright, rather than adding to it. */
        for (i = 0; i < c->item_count; i++) {
            const MagicRule *r = rule_in_effect(c, i);
            if (r && r->unarmored_base) {
                alt = r->unarmored_base + dex;
                if (alt > best) best = alt;
            }
        }
    }

    if (magic_shield) {
        const MagicRule *r = shield_rule;
        best += r->shield + (r->variable ? magic_shield->plus : 0);
    } else if (shield) {
        best += ITEMS[shield->item_id].base_ac;
    }

    /* Everything else worn or attuned that adds flatly. */
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (!r) continue;
        if (r->armor_base || r->shield) continue;      /* already counted */
        if (r->only_unarmored && (wearing_armour || using_shield)) continue;
        if (r->unarmored_base) continue;               /* handled above */
        /* A weapon's plus is a bonus to hit and to damage; the attacks
           block spends it. It never belonged in the Armor Class, where it
           was quietly making a +3 longsword worth three points of armour. */
        if (r->weapon) continue;
        best += r->ac_bonus;
        if (r->variable) best += c->inventory[i].plus;
    }

    /* The Defense fighting style, which is worth a point while armour is
       worn. Its companion Archery is already spent in the attacks block,
       and the two are chosen the same way, so both belong in the numbers
       rather than in a note. */
    if (wearing_armour
        && has_choice_containing(c, "Fighting Style", "Defense")) {
        best += 1;
    }

    if (has_named_feat(c, "Dual Wielder") && !using_shield) {
        /* +1 only while wielding two weapons; recorded here as the best case. */
        best += 1;
    }
    return best;
}

/* ------------------------------------------------------------- attacks */

/* A character sheet's attack lines: for each weapon carried, what it hits
 * at and what it does. The arithmetic is small but it is the arithmetic a
 * player redoes at the table more often than any other, and getting it from
 * the sheet rather than from memory is the point of writing one.
 */

/* Proficiency is granted three ways: by name, by category, or by "All
 * weapons". A class grants a plural ("longswords") where the equipment
 * table has the singular, so a trailing s is ignored on both sides. */
static int same_weapon(const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);

    if (la && (a[la - 1] == 's' || a[la - 1] == 'S')) la--;
    if (lb && (b[lb - 1] == 's' || b[lb - 1] == 'S')) lb--;
    if (la != lb) return 0;
    while (la--) {
        int ca = a[la], cb = b[la];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
    }
    return 1;
}

static int weapon_proficient(const Character *c, const ItemData *it)
{
    int simple = (it->category == ITEM_SIMPLE_MELEE
                  || it->category == ITEM_SIMPLE_RANGED);
    int i;

    /* The books do not word the category line the same way twice: the
       fighter's reads "Simple weapons, martial weapons" and the cleric's
       "All simple weapons". Matching the whole line against one spelling
       left the cleric proficient with nothing -- so the phrase is looked
       for inside the line, which is what has_prof() already does. */
    if (has_prof(c, "All weapons")) return 1;
    if (has_prof(c, simple ? "Simple weapons" : "Martial weapons")) return 1;

    /* A monk is proficient with shortswords through Martial Arts, and a
       druid's list is worded as a set of names: both are matched by name. */
    for (i = 0; i < c->other_prof_count; i++) {
        if (same_weapon(c->other_profs[i], it->name)) return 1;
    }
    return 0;
}

/* The +N of a magic weapon the character has said this one is. The book's
   entry covers every weapon at once, so the copy names which. */
static int magic_weapon_bonus(const Character *c, const char *weapon)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r;
        if (!c->inventory[i].is_magic) continue;
        r = magic_rule_for(MAGIC_ITEMS[c->inventory[i].item_id].name);
        if (!r || !r->weapon) continue;
        if (!c->inventory[i].variant[0]) continue;
        if (same_weapon(c->inventory[i].variant, weapon)) {
            return c->inventory[i].plus;
        }
    }
    return 0;
}

/* The monk's Martial Arts die, which replaces an unarmed strike's damage
   and that of any monk weapon. Zero for anyone else. */
static int martial_arts_die(const Character *c)
{
    int lv = class_level_of(c, CLS_MONK);
    if (lv <= 0) return 0;
    if (lv >= 17) return 10;
    if (lv >= 11) return 8;
    if (lv >= 5) return 6;
    return 4;
}

/* "1d8" -> 8. Zero when the damage is a flat number or absent. */
static int die_of(const char *damage)
{
    const char *d = strchr(damage, 'd');
    return d ? atoi(d + 1) : 0;
}

static void add_note(char *note, size_t n, const char *text)
{
    size_t used = strlen(note);
    snprintf(note + used, n - used, "%s%s", used ? ", " : "", text);
}

int attacks_of(const Character *c, Attack *out, int max)
{
    int prof = proficiency_bonus(c);
    int str = ability_mod(c, ABL_STR);
    int dex = ability_mod(c, ABL_DEX);
    int monk_die = martial_arts_die(c);
    int archery = has_choice_containing(c, "Fighting Style", "Archery");
    int i, n = 0;

    for (i = 0; i < c->item_count && n < max; i++) {
        const ItemData *it;
        const char *props;
        int ranged, finesse, ability, plus, hit, dmg, die, no_damage;
        Attack *a;

        if (c->inventory[i].is_magic) continue;
        it = &ITEMS[c->inventory[i].item_id];
        if (it->category < ITEM_SIMPLE_MELEE
            || it->category > ITEM_MARTIAL_RANGED) continue;

        /* The PHB's weapons table prints a dash where a weapon deals no
           damage, and the net is the one that does. Its attack is real --
           throwing a net is a ranged weapon attack -- so it stays in the
           block, with the damage said in words rather than run through the
           format below, which would have written it "-+3 -". */
        no_damage = (!it->damage[0] || !strcmp(it->damage, "-"));

        props = it->properties;
        ranged = (it->category == ITEM_SIMPLE_RANGED
                  || it->category == ITEM_MARTIAL_RANGED);
        finesse = contains_ci(props, "finesse");

        /* Finesse lets either modifier be used, so the sheet shows the
           better one; a ranged weapon uses Dexterity and everything else
           Strength. */
        ability = ranged ? dex : str;
        if (finesse && dex > ability) ability = dex;
        if (finesse && str > ability) ability = str;

        plus = magic_weapon_bonus(c, it->name);
        hit = ability + plus;
        if (weapon_proficient(c, it)) hit += prof;
        if (archery && ranged) hit += 2;

        a = &out[n++];
        snprintf(a->name, sizeof a->name, "%s", it->name);
        a->bonus = hit;
        a->proficient = weapon_proficient(c, it);

        die = die_of(it->damage);
        dmg = ability + plus;
        {
            /* A monk weapon -- a simple melee weapon that is neither
               two-handed nor heavy, and the shortsword -- may use the
               Martial Arts die when it is the larger one. */
            const char *dice = it->damage;
            char monk[8];
            if (monk_die && die && die < monk_die
                && (it->category == ITEM_SIMPLE_MELEE
                    || same_weapon(it->name, "Shortsword"))
                && !contains_ci(props, "two-handed")
                && !contains_ci(props, "heavy")) {
                snprintf(monk, sizeof monk, "1d%d", monk_die);
                dice = monk;
            }
            if (no_damage) {
                snprintf(a->damage, sizeof a->damage, "no damage");
            } else if (dmg) {
                snprintf(a->damage, sizeof a->damage, "%s%+d %s",
                         dice, dmg, it->damage_type);
            } else {
                snprintf(a->damage, sizeof a->damage, "%s %s",
                         dice, it->damage_type);
            }
        }

        a->note[0] = '\0';
        if (props[0]) add_note(a->note, sizeof a->note, props);
        if (!a->proficient) {
            add_note(a->note, sizeof a->note,
                     "you are not proficient with it");
        }
        if (plus) {
            char buf[32];
            snprintf(buf, sizeof buf, "+%d weapon", plus);
            add_note(a->note, sizeof a->note, buf);
        }
    }

    /* Everyone can punch, and for a monk it is a real attack. */
    if (n < max) {
        Attack *a = &out[n++];
        int ability = str;
        if (monk_die && dex > str) ability = dex;
        snprintf(a->name, sizeof a->name, "Unarmed strike");
        a->bonus = ability + prof;
        a->proficient = 1;
        if (monk_die) {
            snprintf(a->damage, sizeof a->damage, "1d%d%+d bludgeoning",
                     monk_die, ability);
            snprintf(a->note, sizeof a->note,
                     "Martial Arts; a bonus-action strike after the Attack "
                     "action");
        } else {
            /* An unarmed strike deals 1 plus the Strength modifier, and
               never less than nothing. */
            int hurt = 1 + ability;
            snprintf(a->damage, sizeof a->damage, "%d bludgeoning",
                     hurt > 0 ? hurt : 0);
            a->note[0] = '\0';
        }
    }
    return n;
}

int initiative_bonus(const Character *c)
{
    int b = ability_mod(c, ABL_DEX);
    if (has_named_feat(c, "Alert")) b += 5;
    return b;
}

int speed_of(const Character *c)
{
    int speed = 30, monk, barb;
    int wearing_armour, using_shield, wearing_heavy, armour_too_heavy;

    if (c->race_id >= 0 && c->race_id < RACE_COUNT) {
        speed = RACES[c->race_id].speed;
        if (c->subrace_id >= 0 && c->subrace_id < SUBRACE_COUNT
            && SUBRACES[c->subrace_id].speed_override > 0) {
            speed = SUBRACES[c->subrace_id].speed_override;
        }
    }

    worn_armour_state(c, &wearing_armour, &using_shield, &wearing_heavy,
                      &armour_too_heavy);

    /* Unarmored Movement: "while you are not wearing armor and are not
       wielding a shield" (PHB p.78). Both halves used to go unread, so a
       monk in plate with a shield still moved thirty feet faster. */
    monk = class_level_of(c, CLS_MONK);
    if (wearing_armour || using_shield) monk = 0;
    if (monk >= 18)      speed += 30;
    else if (monk >= 14) speed += 25;
    else if (monk >= 10) speed += 20;
    else if (monk >= 6)  speed += 15;
    else if (monk >= 2)  speed += 10;

    /* Fast Movement: "while you aren't wearing heavy armor" (PHB p.48).
       Medium armour is allowed, which is why this is not the same test the
       monk's step makes. */
    barb = class_level_of(c, CLS_BARBARIAN);
    if (barb >= 5 && !wearing_heavy) speed += 10;

    if (has_named_feat(c, "Mobile")) speed += 10;

    /* Armour too heavy for its wearer costs ten feet, and can never take
       the speed below nothing. */
    if (armour_too_heavy) speed -= 10;
    if (speed < 0) speed = 0;

    /* Boots of striding and springing set a floor rather than adding. */
    {
        int i;
        for (i = 0; i < c->item_count; i++) {
            const MagicRule *r = rule_in_effect(c, i);
            if (r && r->sets_speed > speed) speed = r->sets_speed;
        }
    }
    return speed;
}

/* Movement a magic item grants that the character would not otherwise have.
 * A speed of -1 in the table means "the same as your walking speed". Returns
 * 0 when the character has none of that kind. */
/* The three kinds differ only in which column of the rule they read, so
   they share the walk: `which` names the column. */
static int best_magic_speed(const Character *c,
                            int (*column)(const MagicRule *))
{
    int i, best = 0;
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        int v, listed;
        if (!r) continue;
        listed = column(r);
        if (!listed) continue;
        v = (listed < 0) ? speed_of(c) : listed;
        if (v > best) best = v;
    }
    return best;
}

static int rule_fly(const MagicRule *r)   { return r->fly_speed; }
static int rule_swim(const MagicRule *r)  { return r->swim_speed; }
static int rule_climb(const MagicRule *r) { return r->climb_speed; }

int magic_fly_speed(const Character *c)   { return best_magic_speed(c, rule_fly); }
int magic_swim_speed(const Character *c)  { return best_magic_speed(c, rule_swim); }
int magic_climb_speed(const Character *c) { return best_magic_speed(c, rule_climb); }

/* Collects what the character's worn magic items make them resist or ignore
 * into one line, so the sheet can state it rather than leaving it buried in
 * each item's description. Returns the number of entries written. */
int magic_defences(const Character *c, char *out, size_t n)
{
    int i, found = 0;
    size_t used = 0;

    out[0] = '\0';
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        int k;

        if (!r) continue;
        for (k = 0; k < 2; k++) {
            const char *what = k ? r->immune : r->resist;
            const char *how = k ? "immune to" : "resistant to";
            int w;

            if (!what) continue;
            /* A "*" means the copy carries the type it was made against. */
            if (!strcmp(what, "*")) {
                what = c->inventory[i].variant[0]
                     ? c->inventory[i].variant : "a chosen damage type";
            }
            w = snprintf(out + used, n - used, "%s%s %s", used ? "; " : "",
                         how, what);
            if (w < 0 || (size_t)w >= n - used) return found;
            used += (size_t)w;
            found++;
        }
    }
    return found;
}

int carrying_capacity(const Character *c)
{
    return ability_score(c, ABL_STR) * 15;
}

/* What is carried, in tenths of a pound.
 *
 * Added up in a wider number than it is returned in. Ninety-six lines of an
 * inventory, each holding hundreds of something a homebrew file is free to
 * say weighs half a ton, is a product no int is required to hold -- and an
 * overflowed one is undefined behaviour rather than a heavy pack. The
 * ceiling is never reached by anything a game produces; it is there so that
 * a number typed into homebrew.txt cannot make the arithmetic undefined. */
int current_weight_tenths(const Character *c)
{
    long long w = 0;
    int i;

    for (i = 0; i < c->item_count; i++) {
        /* Magic items index a different table and carry no listed weight;
           their bulk is the DM's call. */
        if (c->inventory[i].is_magic) continue;
        w += (long long)ITEMS[c->inventory[i].item_id].weight_tenths
             * c->inventory[i].quantity;
    }
    /* 50 coins weigh a pound (PHB chapter 5). */
    w += ((long long)c->copper + c->silver + c->electrum
          + c->gold + c->platinum) / 5;

    if (w > 1000000000LL) return 1000000000;
    if (w < 0) return 0;
    return (int)w;
}

int hit_points_max(const Character *c)
{
    int i, hp = 0;
    int lvl = total_level(c);
    int con = ability_mod(c, ABL_CON);

    for (i = 0; i < c->hp_roll_count && i < MAX_LEVEL; i++) hp += c->hp_rolls[i];
    hp += con * lvl;

    /* Hill dwarves gain 1 extra hit point per level. */
    if (c->subrace_id >= 0 && c->subrace_id < SUBRACE_COUNT
        && strcmp(SUBRACES[c->subrace_id].name, "Hill Dwarf") == 0) {
        hp += lvl;
    }
    /* Tough grants 2 per level. */
    if (has_named_feat(c, "Tough")) hp += 2 * lvl;
    /* Draconic Resilience grants 1 per sorcerer level. */
    if (has_subclass_named(c, "Draconic Bloodline")) {
        hp += class_level_of(c, CLS_SORCERER);
    }

    return hp;
}

/* Combined caster level for the multiclass spellcaster table (PHB p.164):
 * full levels for full casters, half rounded down for paladin and ranger,
 * a third rounded down for the Eldritch Knight and Arcane Trickster. */
int caster_level(const Character *c)
{
    int i, lvl = 0;

    for (i = 0; i < c->class_count; i++) {
        int id = c->classes[i].class_id;
        int n = c->classes[i].level;
        int sub = c->classes[i].subclass_id;

        switch (CLASSES[id].caster) {
        case CAST_FULL: lvl += n; break;
        case CAST_HALF:
            /* Tasha's has the artificer round up where the paladin and the
               ranger round down. */
            lvl += CLASSES[id].mc_round_up ? (n + 1) / 2 : n / 2;
            break;
        default:
            if (is_third_caster(sub)) lvl += n / 3;
            break;
        }
    }
    return lvl;
}

int spell_save_dc(const Character *c, int class_id)
{
    Ability a = CLASSES[class_id].spell_ability;
    return 8 + proficiency_bonus(c) + ability_mod(c, a);
}

int spell_attack_bonus(const Character *c, int class_id)
{
    Ability a = CLASSES[class_id].spell_ability;
    return proficiency_bonus(c) + ability_mod(c, a);
}
