/* ui.h -- console prompts, menus and dice. */
#ifndef UI_H
#define UI_H

#include <stddef.h>

void ui_rule(void);
void ui_header(const char *title);
void ui_para(const char *text);
void ui_wrap(const char *text, int indent);

/* ------------------------------------------------- going back, and giving up
 *
 * A player part-way through building a character wants two things a plain
 * menu cannot give them: to undo the last choice, and to abandon the whole
 * thing and start again. Both are answered by typing a word rather than a
 * number -- "b" or "back", "q" or "quit" -- because every menu's numbers
 * are already spoken for by its options, and a number that means something
 * different on every screen is worse than a word that means the same on
 * all of them.
 *
 * No prompt function returns an escape. A prompt that is escaped never
 * returns at all: the registered handler is called and does not come back,
 * which is what keeps this out of the other hundred and fifty call sites
 * in the program. They cannot mishandle a sentinel they can never see.
 *
 * Escapes are off until a handler is registered, so a prompt outside
 * character creation -- setting coins, entering a dice roll -- neither
 * offers the words nor accepts them.
 */
typedef enum { UI_ESC_BACK = 1, UI_ESC_QUIT } UiEscape;

/* Registered by the flow that can honour an escape. THE HANDLER MUST NOT
   RETURN: it longjmps past the prompt that raised it. Pass NULL to turn
   escapes off again. */
void ui_set_escape(void (*fn)(UiEscape));

/* The dice, so that going back can put them back too. Undoing a choice has
   to undo the rolls it consumed, or backing up and coming forward again
   would be a way to reroll hit points until they suited. Starting a new
   character is the opposite case and does not restore them: that is a new
   character, not a replay of the old one. */
unsigned long ui_rng_state(void);
void ui_rng_restore(unsigned long state);

/* The text "3 info" shows for entry 3 of the menu now on screen, set for
   the duration of one prompt. ui_menu does this for its callers, passing
   the details it was already given, so every existing menu answers "info"
   with the line it already shows. */
void ui_set_info(const char *const *info, int count);

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

/* The same menu, with a separate array of longer answers to "N info".
   Pass NULL for info and the details array is used, which is what ui_menu
   does -- so a menu that already prints a detail line needs no change to
   answer a question about it. */
int  ui_menu_info(const char *prompt, const char *const *options,
                  const char *const *details, int count,
                  const char *const *info);

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

/* Walks a comma-separated line in place. Set a cursor to the start of the
   buffer and call this until it returns NULL; each call gives the next
   piece with its leading spaces trimmed. An empty piece is returned as the
   empty string rather than skipped, because the lines this reads are the
   books' own and a caller that cares says so itself. The buffer is written
   through, so it must be a copy the caller owns. */
char *next_csv(char **cursor);

/* Split a '|' separated list into pieces. Returns the number found.
   The caller supplies storage; pieces point into buf. */
int  split_pipe(const char *src, char *buf, size_t bufsz,
                const char **out, int max);

#endif /* UI_H */
