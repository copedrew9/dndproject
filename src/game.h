/* game.h -- game mode: a character in play rather than in the making. */
#ifndef GAME_H
#define GAME_H

#include "dnd.h"

/* Runs the table screen for one character. Everything it changes is on the
   character, so the caller saves as it would after any other edit. */
void game_mode(Character *c);

/* Current hit points, which is the maximum less what has been lost. Never
   below zero: a character at zero is dying, not on negative numbers. */
int  hit_points_now(const Character *c);

/* How many hit dice of one class are left to spend. */
int  hit_dice_left(const Character *c, int slot);

#endif
