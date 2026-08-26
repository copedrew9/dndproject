/* homebrew.h -- the DM's own items and spells. */
#ifndef HOMEBREW_H
#define HOMEBREW_H

/* Reads homebrew.txt and folds what it holds into the item, magic item and
   spell banks. Returns how many entries were added. Call once at startup,
   before anything reads those banks. */
int  homebrew_load(void);
int  homebrew_save(void);

int  homebrew_item_count(void);
int  homebrew_magic_count(void);
int  homebrew_spell_count(void);
int  homebrew_total(void);

/* The DM's screen: add, list and remove. */
void homebrew_menu(void);

#endif /* HOMEBREW_H */
