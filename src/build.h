/* build.h -- character creation and advancement. */
#ifndef BUILD_H
#define BUILD_H

#include "dnd.h"
#include "data.h"

/* Top level flows. */
void wizard_create(Character *c);
void wizard_level_up(Character *c);

/* Walks every class level, granting hit points, subclasses and choices. */
void build_levels(Character *c);

/* Shared list helpers; each ignores duplicates. */
void add_language(Character *c, const char *lang);
void add_tool(Character *c, const char *tool);
void add_prof(Character *c, const char *prof);
void add_choice(Character *c, const char *label, const char *value);
void add_item(Character *c, int item_id, int qty, int equipped);
void add_item_by_name(Character *c, const char *name, int qty, int equipped);
int  has_prof(const Character *c, const char *prof);
int  has_tool(const Character *c, const char *tool);
int  has_language(const Character *c, const char *lang);

/* Steps used by both creation and level-up. */
void choose_subclass_for(Character *c, int slot);
void grant_level_hp(Character *c, int class_id, int is_first_level);
void apply_asi_or_feat(Character *c, const char *reason);
void choose_fighting_style(Character *c, int class_id);
void choose_expertise(Character *c, int count);
void manage_spells(Character *c, int class_id);
void choose_equipment(Character *c);
void choose_personality(Character *c);

/* Comma separated proficiency strings are recorded verbatim. */
void add_prof_list(Character *c, const char *csv);

/* Spellcasting helpers. */
int  spell_slots_for(const Character *c, int out[10]);
int  pact_slots_for(const Character *c, int *count, int *level);
int  spells_prepared_count(const Character *c, int class_id);
int  known_spell_count(const Character *c, int class_id, int cantrips);
int  class_of_spell(const Character *c, int spell_id);

/* True when the character meets the PHB multiclassing prerequisites. */
int  multiclass_ok_public(const Character *c, int class_id, int *why);

/* True when the character took one of Tasha's optional class features. */
int  has_optional_feature(const Character *c, const char *name);

#endif /* BUILD_H */
