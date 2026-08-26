/* ui.h -- console prompts, menus and dice. */
#ifndef UI_H
#define UI_H

#include <stddef.h>

void ui_rule(void);
void ui_header(const char *title);
void ui_para(const char *text);
void ui_wrap(const char *text, int indent);

/* Prompt for an integer in [lo, hi]. Re-asks until the input is valid. */
int  ui_int(const char *prompt, int lo, int hi);

/* Prompt for a line of text. Returns the number of characters stored. */
int  ui_line(const char *prompt, char *out, size_t n);

/* Prompt for a line, substituting def when the reply is empty. */
void ui_line_default(const char *prompt, const char *def, char *out, size_t n);

int  ui_yesno(const char *prompt, int def_yes);

/* Single-choice menu. options[i] is the label, details[i] may be NULL.
   Returns the selected index. */
int  ui_menu(const char *prompt, const char *const *options,
             const char *const *details, int count);

/* Choose n distinct entries from options; fills picks[] with indices.
   Entries where available[i] is 0 are shown as unavailable and cannot be
   chosen. Pass NULL for available to allow everything.

   Returns how many were actually chosen, which is less than n when the list
   has run out of available entries; unfilled picks[] slots are set to -1, so
   callers must either check the return value or skip negative indices. */
int ui_multi(const char *prompt, const char *const *options,
             const int *available, int count, int n, int *picks);

/* Dice. */
void rng_seed(unsigned int seed);
int  roll_die(int sides);
int  roll_dice(int count, int sides);
int  roll_4d6_drop_lowest(void);

/* Split a '|' separated list into pieces. Returns the number found.
   The caller supplies storage; pieces point into buf. */
int  split_pipe(const char *src, char *buf, size_t bufsz,
                const char **out, int max);

#endif /* UI_H */
