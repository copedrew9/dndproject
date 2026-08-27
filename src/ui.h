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

/* Prompt for several lines, ending at a blank one. Newlines are kept, so
   the result can hold paragraphs. Returns the number of characters stored. */
int  ui_text_block(const char *prompt, char *out, size_t n);

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

/* Dice at the table rather than in the program. When this is switched on,
   every roll the program would make is asked for instead, so a player who
   rolls their own can enter what came up. `what` names what is being rolled
   for, since a prompt out of context is no use. */
void ui_set_manual_dice(int on);
int  ui_manual_dice(void);
int  ui_roll(int count, int sides, const char *what);
int  ui_roll_die(int sides, const char *what);
int  ui_roll_4d6_drop_lowest(const char *what);

/* A menu whose last entry lets the answer be typed instead. */
void ui_pick_or_type(const char *prompt, const char *const *options,
                     int count, char *out, size_t n);

/* The same, for menus that show a line of explanation beside each entry.
   Fills out[] with the answer and returns the index chosen, or -1 when it
   was typed in. An empty answer re-asks. */
int  ui_menu_custom(const char *prompt, const char *const *options,
                    const char *const *details, int count,
                    const char *custom_label, char *out, size_t n);

/* Checkboxes: toggle entries on and off until done. flags[] is both the
   starting state and the answer; returns how many ended up set. */
int  ui_toggle_list(const char *prompt, const char *const *options,
                    int count, int *flags);

/* Case-insensitive substring test; returns 1 when needle is empty. */
int  contains_ci(const char *haystack, const char *needle);

/* Split a '|' separated list into pieces. Returns the number found.
   The caller supplies storage; pieces point into buf. */
int  split_pipe(const char *src, char *buf, size_t bufsz,
                const char **out, int max);

#endif /* UI_H */
