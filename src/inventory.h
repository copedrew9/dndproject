/* inventory.h -- managing gear after a character is built. */
#ifndef INVENTORY_H
#define INVENTORY_H

#include "dnd.h"

/* Add, drop, wear and attune. Never sells anything. */
void manage_inventory(Character *c);

/* Which kind of armour a carried entry is -- ITEM_LIGHT_ARMOR through
   ITEM_SHIELD -- or -1 when it is not armour, when the entry is armour
   whose own rule does not say which kind, or when the item carries its own
   proficiency. A magic suit is placed by the Dexterity it allows, which is
   how the equipment table encodes the same thing. */
int entry_armour_category(const Character *c, int index);

#endif /* INVENTORY_H */
