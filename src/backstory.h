/* backstory.h -- Xanathar's "This Is Your Life" tables. */
#ifndef BACKSTORY_H
#define BACKSTORY_H

#include "dnd.h"

/* Walks the tables, rolling or choosing, and writes the result into the
   character's backstory line. */
void build_backstory(Character *c);

#endif /* BACKSTORY_H */
