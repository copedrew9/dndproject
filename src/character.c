/* character.c -- derived statistics (PHB chapters 1, 5 and 7). */
#include "dnd.h"
#include "data.h"
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


/* Subclasses are referenced by name so the tables can grow freely. */
static int is_third_caster(int sub)
{
    return subclass_is(sub, "Eldritch Knight")
        || subclass_is(sub, "Arcane Trickster");
}

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

static int feat_id_by_name(const char *name)
{
    int i;
    for (i = 0; i < FEAT_COUNT; i++) {
        if (strcmp(FEATS[i].name, name) == 0) return i;
    }
    return -1;
}

static int has_named_feat(const Character *c, const char *name)
{
    int id = feat_id_by_name(name);
    return id >= 0 && has_feat(c, id);
}

int ability_score(const Character *c, Ability a)
{
    return c->base_score[a] + c->racial_bonus[a] + c->asi_bonus[a];
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
    } else if (class_level_of(c, CLS_BARD) >= 2) {
        bonus += pb / 2;            /* Jack of All Trades */
    }
    return bonus;
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
    if ((r->armor_base || r->shield) && !e->equipped) return NULL;
    return r;
}

/* The magic armour a character is wearing, if any. */
static const InventoryEntry *worn_magic_armour(const Character *c)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (r && r->armor_base) return &c->inventory[i];
    }
    return NULL;
}

static const InventoryEntry *worn_magic_shield(const Character *c)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        const MagicRule *r = rule_in_effect(c, i);
        if (r && r->shield) return &c->inventory[i];
    }
    return NULL;
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

int armour_class(const Character *c)
{
    const InventoryEntry *armour = NULL;
    const InventoryEntry *shield = equipped_of(c, ITEM_SHIELD);
    const InventoryEntry *magic_armour = worn_magic_armour(c);
    const InventoryEntry *magic_shield = worn_magic_shield(c);
    int dex = ability_mod(c, ABL_DEX);
    int best, cat, i;
    int wearing_armour, using_shield;

    for (cat = ITEM_LIGHT_ARMOR; cat <= ITEM_HEAVY_ARMOR && !armour; cat++) {
        armour = equipped_of(c, (ItemCategory)cat);
    }

    wearing_armour = (armour != NULL) || (magic_armour != NULL);
    using_shield = (shield != NULL) || (magic_shield != NULL);

    if (magic_armour) {
        const MagicRule *r =
            magic_rule_for(MAGIC_ITEMS[magic_armour->item_id].name);
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
        const MagicRule *r =
            magic_rule_for(MAGIC_ITEMS[magic_shield->item_id].name);
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
        best += r->ac_bonus;
        if (r->variable) best += c->inventory[i].plus;
    }

    if (has_named_feat(c, "Dual Wielder") && !using_shield) {
        /* +1 only while wielding two weapons; recorded here as the best case. */
        best += 1;
    }
    return best;
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

    if (c->race_id >= 0 && c->race_id < RACE_COUNT) {
        speed = RACES[c->race_id].speed;
        if (c->subrace_id >= 0 && c->subrace_id < SUBRACE_COUNT
            && SUBRACES[c->subrace_id].speed_override > 0) {
            speed = SUBRACES[c->subrace_id].speed_override;
        }
    }

    monk = class_level_of(c, CLS_MONK);
    if (monk >= 18)      speed += 30;
    else if (monk >= 14) speed += 25;
    else if (monk >= 10) speed += 20;
    else if (monk >= 6)  speed += 15;
    else if (monk >= 2)  speed += 10;

    barb = class_level_of(c, CLS_BARBARIAN);
    if (barb >= 5) speed += 10;         /* Fast Movement, unarmoured or light */

    if (has_named_feat(c, "Mobile")) speed += 10;
    return speed;
}

int carrying_capacity(const Character *c)
{
    return ability_score(c, ABL_STR) * 15;
}

int current_weight_tenths(const Character *c)
{
    int i, w = 0;
    for (i = 0; i < c->item_count; i++) {
        /* Magic items index a different table and carry no listed weight;
           their bulk is the DM's call. */
        if (c->inventory[i].is_magic) continue;
        w += ITEMS[c->inventory[i].item_id].weight_tenths
             * c->inventory[i].quantity;
    }
    /* 50 coins weigh a pound (PHB chapter 5). */
    w += (c->copper + c->silver + c->electrum + c->gold + c->platinum) / 5;
    return w;
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
