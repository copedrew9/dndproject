/* shop.h -- a shop the DM builds, and buying from one at the table.
 *
 * A shop is a list of what is for sale and at what price. It is NOT a
 * filter over the equipment tables: a shop that offered "the gear
 * category" would restock itself every time the tables changed and would
 * never hold anything the tables do not have. What it holds is whatever
 * the DM put in it, one line at a time, each with its own price -- taken
 * from the book by default, because that is right nine times in ten, and
 * overridable because the tenth is the interesting one.
 */
#ifndef SHOP_H
#define SHOP_H

#include "dnd.h"

#include <stddef.h>

#define MAX_SHOP_LINES 128
#define MAX_SHOP_NAME 48

/* One thing for sale. `item_id` indexes ITEMS when `from_book` is set, and
   is -1 otherwise; a line the DM typed carries only its name. */
typedef struct {
    char name[MAX_NAME];
    int from_book;
    int item_id;
    int category;           /* ItemCategory, so the shelves stay sorted */
    int price_cp;
    int stock;              /* -1 for as many as you like */
    char note[MAX_TEXT];
} ShopLine;

typedef struct {
    char name[MAX_SHOP_NAME];
    char keeper[MAX_NAME];
    char about[MAX_TEXT];
    ShopLine lines[MAX_SHOP_LINES];
    int line_count;
} Shop;

/* The DM's screen: build a shop and write it to <name>.txt. */
void shopbuilder_menu(void);

/* Writes a shop to <name>.txt and fills path[] with the file written.
   Returns 0 on success, -1 when the file cannot be opened, and -2 when a
   character of that name already has the file. */
int  shop_save(const Shop *s, char *path, size_t n);

/* The file name a shop of this name is written to, on the same manners as
   a character sheet: what a filesystem will not take is dropped. */
void shop_name_to_path(const char *name, char *out, size_t n);

/* Reads a shop written by the builder. Returns 0, or -1 when the file
   cannot be opened, or -2 when it is not a shop file. */
int  shop_load(const char *path, Shop *s);

/* The player's screen: spend money in a shop, at the table. */
void shop_visit(Character *c);

#endif
