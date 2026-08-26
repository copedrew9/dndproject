/* sidekick.h -- Tasha's sidekicks. */
#ifndef SIDEKICK_H
#define SIDEKICK_H

#include <stdio.h>

#include "dnd.h"

int  create_sidekick(Sidekick *sk, int party_level);
void level_up_sidekick(Sidekick *sk);

int  sidekick_proficiency(const Sidekick *sk);
int  sidekick_ability_mod(const Sidekick *sk, int ability);

void print_sidekick(FILE *f, const Sidekick *sk, int indent);

/* The screen: add, level up, or write one out as its own sheet. */
void manage_sidekicks(Character *c);

#endif /* SIDEKICK_H */
