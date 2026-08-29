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

/* What one blow did. The screen turns this into a sentence; the rule that
   decides it is take_damage() and is checked by the selftest. */
typedef enum {
    HURT_NOTHING,       /* no damage, or none left after temporary hit points */
    HURT_SOAKED,        /* the whole blow went on temporary hit points */
    HURT_STANDING,      /* hurt, still up */
    HURT_DOWNED,        /* brought to 0 -- death saves start */
    HURT_SAVE_FAILED,   /* hit while already at 0: a failed death save */
    HURT_DEAD           /* the remainder reached the hit point maximum */
} HurtResult;

HurtResult take_damage(Character *c, int hurt, int critical,
                       int *absorbed, int *spare);

/* How many hit dice of one class are left to spend. */
int  hit_dice_left(const Character *c, int slot);

/* The most one sheet can carry, in copper. A character file holds each
   coin count in its own field and reads it back through a range check, so
   a purse too large for those fields would come back smaller than it went
   out -- which tools/stress.py caught as a sheet that changed on reload
   after a character was paid eleven times over. */
#define MAX_PURSE_CP ((long)MAX_COINS * 1000 + 999)

/* The purse, counted in copper so that mixed change works out, and put
   back into coins afterwards. Everything that takes money -- the inn, the
   shop, a straight payment -- goes through these two, so that no screen
   invents its own idea of what a silver piece is worth. */
long purse_in_copper(const Character *c);
void purse_from_copper(Character *c, long cp);

/* Write a line in the ledger: negative copper for money out, positive for
   money in, and what it was for. */
void remember(Character *c, int copper, const char *what);

#endif
