/* inventory.h -- managing gear after a character is built. */
#ifndef INVENTORY_H
#define INVENTORY_H

#include "dnd.h"

/* Add, drop, wear and attune. Never sells anything. */
void manage_inventory(Character *c);

#endif /* INVENTORY_H */
