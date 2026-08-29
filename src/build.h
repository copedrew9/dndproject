/* build.h -- character creation and advancement. */
#ifndef BUILD_H
#define BUILD_H

#include "dnd.h"
#include "data.h"

/* Top level flows. */
/* Builds a character. Returns 1 when the player confirmed it at the end
   and 0 when they left without saving; the caller writes the file. */
int wizard_create(Character *c);

/* Whether a class draws its power from someone it has to name: a cleric's
   god, a paladin's oath, a warlock's patron. Those three are asked for one
   and not offered a way past; everyone else is asked whether they keep a
   faith at all. */
int class_must_name_a_patron(int class_id);
void wizard_level_up(Character *c);

/* Walks every class level, granting hit points, subclasses and choices. */
void build_levels(Character *c);

/* Menus are built into fixed arrays before being shown. One bound governs
   both the array and the loop that fills it, so a table that outgrows it
   truncates the menu instead of writing past the end -- which is what the
   race list did once it reached 42 entries against a 32-slot array, spilling
   the last ten races over the details of the first ten. The self-test checks
   that every table the wizard lists still fits. */
#define MENU_MAX 64

/* Shared list helpers; each ignores duplicates. */
void add_language(Character *c, const char *lang);
void add_tool(Character *c, const char *tool);
void add_prof(Character *c, const char *prof);
void add_choice(Character *c, const char *label, const char *value);

/* The three ways the choices list is read back. */
int  count_choices(const Character *c, const char *label);
int  has_choice_starting(const Character *c, const char *label,
                         const char *value);
int  has_choice_exactly(const Character *c, const char *label,
                        const char *value);
/* The gold a class starts with when the average is taken rather than
   rolled, which is the Player's Handbook's own average column. */
int  average_starting_gold(const ClassData *cd);

void add_item(Character *c, int item_id, int qty, int equipped);

/* Taking a pack takes what is in it, and not the pack: a pack's weight is
   the sum of its parts, so carrying both would count everything twice.
   Returns how many kinds of thing went on, or -1 when this is not a pack
   (or the bank holds none of its contents), so the caller adds it plainly
   instead. */
int  add_pack(Character *c, int pack_id, int qty);
int  inventory_has_room(const Character *c);
void add_item_by_name(Character *c, const char *name, int qty, int equipped);

/* Magic items live in the same list, flagged so they index MAGIC_ITEMS. */
void add_magic_item(Character *c, int magic_id, int qty, int attuned,
                    int plus);

/* One copy of a magic item, appended rather than stacked. Returns the new
   entry so the caller can set what only it knows -- the damage type the
   copy carries, whether the table is hiding what it is -- or NULL when
   there is no room. */
InventoryEntry *add_magic_item_copy(Character *c, int magic_id, int qty,
                                    int attuned, int plus);
void remove_inventory_entry(Character *c, int index, int qty);
int  attuned_count(const Character *c);
int  has_prof(const Character *c, const char *prof);
int  has_tool(const Character *c, const char *tool);
int  has_language(const Character *c, const char *lang);

/* Steps used by both creation and level-up. */
void choose_subclass_for(Character *c, int slot);
void grant_level_hp(Character *c, int class_id, int is_first_level);
void apply_asi_or_feat(Character *c, const char *reason);

/* Whether a feat's prerequisites are met, which is what decides whether the
   level-up offers it. Exported so that a test can ask the same question the
   menu asks, rather than a copy of it. */
int  feat_offered(const Character *c, int feat_id);
void choose_fighting_style(Character *c, int class_id);
void choose_expertise(Character *c, int count);
void manage_spells(Character *c, int class_id);
void choose_equipment(Character *c);
void choose_personality(Character *c);

/* A class's comma separated proficiency line. Each piece is filed where it
   belongs: armour and weapons as proficiencies, tools as tools, and an open
   tool choice as a menu. `source` names whatever granted it. */
void add_prof_list(Character *c, const char *csv, const char *source);

/* Spellcasting helpers. */
/* True for the two subclasses that cast off a third of their level. */
int  is_third_caster(int subclass_id);

int  spell_slots_for(const Character *c, int out[10]);
int  pact_slots_for(const Character *c, int *count, int *level);
int  spells_prepared_count(const Character *c, int class_id);
int  known_spell_count(const Character *c, int class_id, int cantrips);

/* A warlock patron's expanded spell list: the spells the patron adds to the
   list this character may choose from, comma separated and lowercase-safe,
   or NULL for anyone else. It widens the choice and is never granted --
   which is the whole difference between a patron and a cleric's domain. */
const char *warlock_expanded_list(const Character *c, int class_id);

/* True when the character meets the PHB multiclassing prerequisites. */
int  multiclass_ok_public(const Character *c, int class_id, int *why);

/* The feats that grant a further choice -- an invocation, a fighting style,
   a spell -- which the builder asks for when the feat is taken. */
extern const char *const FEATS_WITH_CHOICES[];
extern const int FEATS_WITH_CHOICES_COUNT;

/* True when the character took one of Tasha's optional class features. */
int  has_optional_feature(const Character *c, const char *name);

#endif /* BUILD_H */
