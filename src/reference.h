/* reference.h -- read-only item lookup. */
#ifndef REFERENCE_H
#define REFERENCE_H

#include <stddef.h>

#include "dnd.h"

/* Menu list sizes. The equipment catalogue is the longest list shown. */
#define REF_MAX  400
#define REF_LINE 96

extern const char *const CATEGORY_LABEL[];

/* The longest a spell summary can run: five lines of fields and the
   description, with room to spare for a homebrew spell's long name. */
#define SPELL_INFO_LEN 768

/* Everything about a spell in one paragraph, for a menu's "N info". */
void spell_summary(int spell_id, char *out, size_t n);

void format_price(int cp, char *out, size_t n);
void format_weight(int tenths, char *out, size_t n);

void show_item_detail(int idx);          /* index into ITEMS */
void show_magic_item_detail(int idx);    /* index into MAGIC_ITEMS */

/* The standalone browser, and the same detail view over what a character
   is actually carrying. */
void reference_menu(void);
void inventory_reference(const Character *c);

#endif /* REFERENCE_H */
