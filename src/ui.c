/* ui.c -- console prompts, menus and dice. */
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LINE_WIDTH 76

/* Reading input ------------------------------------------------------------ */

/* Reads a line into buf. Returns 0 on end of input, which the callers treat
   as an abort so the program never spins on a closed stdin. */
static int read_line(char *buf, size_t n)
{
    if (!fgets(buf, (int)n, stdin)) return 0;
    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

static void input_closed(void)
{
    fprintf(stderr, "\nInput ended unexpectedly; exiting without saving.\n");
    exit(1);
}

/* Output ------------------------------------------------------------------- */

void ui_rule(void)
{
    int i;
    for (i = 0; i < LINE_WIDTH; i++) putchar('-');
    putchar('\n');
}

void ui_header(const char *title)
{
    printf("\n");
    ui_rule();
    printf("  %s\n", title);
    ui_rule();
}

/* Word-wraps text to LINE_WIDTH, honouring an indent on every line. */
void ui_wrap(const char *text, int indent)
{
    int col = 0;
    const char *p = text;

    while (*p) {
        const char *word = p;
        int len = 0;
        while (*p && !isspace((unsigned char)*p)) { p++; len++; }

        if (col == 0) {
            printf("%*s", indent, "");
            col = indent;
        } else if (col + 1 + len > LINE_WIDTH) {
            printf("\n%*s", indent, "");
            col = indent;
        } else {
            putchar(' ');
            col++;
        }
        fwrite(word, 1, (size_t)len, stdout);
        col += len;

        while (*p && isspace((unsigned char)*p)) {
            if (*p == '\n') {
                putchar('\n');
                col = 0;
            }
            p++;
        }
    }
    if (col) putchar('\n');
}

void ui_para(const char *text)
{
    ui_wrap(text, 2);
}

/* Prompts ------------------------------------------------------------------ */

int ui_int(const char *prompt, int lo, int hi)
{
    char buf[128];

    for (;;) {
        char *end;
        long v;

        printf("%s [%d-%d]: ", prompt, lo, hi);
        fflush(stdout);
        if (!read_line(buf, sizeof buf)) input_closed();

        v = strtol(buf, &end, 10);
        while (*end && isspace((unsigned char)*end)) end++;
        if (end != buf && *end == '\0' && v >= lo && v <= hi) return (int)v;

        printf("  Please enter a whole number between %d and %d.\n", lo, hi);
    }
}

int ui_line(const char *prompt, char *out, size_t n)
{
    printf("%s: ", prompt);
    fflush(stdout);
    if (!read_line(out, n)) input_closed();
    return (int)strlen(out);
}

void ui_line_default(const char *prompt, const char *def, char *out, size_t n)
{
    char buf[512];

    printf("%s [%s]: ", prompt, def);
    fflush(stdout);
    if (!read_line(buf, sizeof buf)) input_closed();

    if (buf[0] == '\0') {
        strncpy(out, def, n - 1);
        out[n - 1] = '\0';
    } else {
        strncpy(out, buf, n - 1);
        out[n - 1] = '\0';
    }
}

int ui_yesno(const char *prompt, int def_yes)
{
    char buf[64];

    for (;;) {
        printf("%s [%s]: ", prompt, def_yes ? "Y/n" : "y/N");
        fflush(stdout);
        if (!read_line(buf, sizeof buf)) input_closed();

        if (buf[0] == '\0') return def_yes;
        if (buf[0] == 'y' || buf[0] == 'Y') return 1;
        if (buf[0] == 'n' || buf[0] == 'N') return 0;
        printf("  Please answer y or n.\n");
    }
}

int ui_menu(const char *prompt, const char *const *options,
            const char *const *details, int count)
{
    int i;

    printf("\n%s\n", prompt);
    for (i = 0; i < count; i++) {
        printf("  %2d) %s\n", i + 1, options[i]);
        if (details && details[i] && details[i][0]) ui_wrap(details[i], 6);
    }
    return ui_int("  Choose", 1, count) - 1;
}

int ui_multi(const char *prompt, const char *const *options,
             const int *available, int count, int n, int *picks)
{
    int chosen = 0;
    int taken[256];
    int usable_total = 0;
    int i;

    memset(taken, 0, sizeof taken);
    for (i = 0; i < n; i++) picks[i] = -1;

    /* Never ask for more than can be given. A character who already has
       every proficiency on offer would otherwise be asked forever. */
    for (i = 0; i < count; i++) {
        if (!available || available[i]) usable_total++;
    }
    if (usable_total < n) {
        if (usable_total == 0) {
            printf("\n%s\n  Nothing on this list is still available to "
                   "you.\n", prompt);
            return 0;
        }
        printf("\n%s\n  Only %d of these are still available to you.\n",
               prompt, usable_total);
        n = usable_total;
    }

    printf("\n%s (choose %d)\n", prompt, n);
    while (chosen < n) {
        int pick;

        for (i = 0; i < count; i++) {
            int usable = (!available || available[i]) && !taken[i];
            if (usable) {
                printf("  %2d) %s\n", i + 1, options[i]);
            } else {
                printf("   -  %s%s\n", options[i],
                       taken[i] ? " (already chosen)" : " (already proficient)");
            }
        }

        printf("  Selection %d of %d\n", chosen + 1, n);
        pick = ui_int("  Choose", 1, count) - 1;

        if (taken[pick]) {
            printf("  You have already chosen that. Pick another.\n");
            continue;
        }
        if (available && !available[pick]) {
            printf("  That one is not available to you. Pick another.\n");
            continue;
        }
        taken[pick] = 1;
        picks[chosen++] = pick;
    }
    return chosen;
}

/* Dice --------------------------------------------------------------------- */

static unsigned long rng_state = 1;

void rng_seed(unsigned int seed)
{
    rng_state = seed ? seed : 1;
}

/* xorshift: small, deterministic when seeded, and adequate for dice. */
static unsigned long rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

int roll_die(int sides)
{
    if (sides < 1) return 0;
    return (int)(rng_next() % (unsigned long)sides) + 1;
}

int roll_dice(int count, int sides)
{
    int i, total = 0;
    for (i = 0; i < count; i++) total += roll_die(sides);
    return total;
}

int roll_4d6_drop_lowest(void)
{
    int r[4], i, total = 0, lowest = 0;

    for (i = 0; i < 4; i++) {
        r[i] = roll_die(6);
        if (r[i] < r[lowest]) lowest = i;
    }
    for (i = 0; i < 4; i++) {
        if (i != lowest) total += r[i];
    }
    return total;
}

/* Dice ---------------------------------------------------------------------
 *
 * Every roll the program makes goes through these, so that a table which
 * rolls its own dice can switch the generator off and be asked instead. The
 * prompt names what the roll is for and accepts only what those dice could
 * actually have produced.
 */

static int manual_dice;

void ui_set_manual_dice(int on) { manual_dice = on ? 1 : 0; }
int  ui_manual_dice(void)       { return manual_dice; }

int ui_roll(int count, int sides, const char *what)
{
    char prompt[160];

    if (count < 1) count = 1;
    if (sides < 2) sides = 20;
    if (!manual_dice) return roll_dice(count, sides);

    snprintf(prompt, sizeof prompt, "  Roll %dd%d for %s, and enter the total",
             count, sides, what ? what : "this");
    return ui_int(prompt, count, count * sides);
}

int ui_roll_die(int sides, const char *what)
{
    return ui_roll(1, sides, what);
}

/* The four dice are asked for one at a time rather than as a total, because
   the lowest is dropped and the player should see which one went. */
int ui_roll_4d6_drop_lowest(const char *what)
{
    int r[4], i, lowest = 0, total = 0;
    char prompt[160];

    if (!manual_dice) return roll_4d6_drop_lowest();

    printf("\n  Roll 4d6 for %s.\n", what ? what : "an ability score");
    for (i = 0; i < 4; i++) {
        snprintf(prompt, sizeof prompt, "    die %d of 4", i + 1);
        r[i] = ui_int(prompt, 1, 6);
        if (r[i] < r[lowest]) lowest = i;
    }
    for (i = 0; i < 4; i++) {
        if (i != lowest) total += r[i];
    }
    printf("    %d, %d, %d, %d -- dropping the %d gives %d.\n",
           r[0], r[1], r[2], r[3], r[lowest], total);
    return total;
}

/* Utility ------------------------------------------------------------------ */

/* Case-insensitive substring test. */
int contains_ci(const char *haystack, const char *needle)
{
    size_t nl = strlen(needle);
    const char *h;

    if (!nl) return 1;
    for (h = haystack; *h; h++) {
        size_t k;
        for (k = 0; k < nl; k++) {
            int a = (unsigned char)h[k], b = (unsigned char)needle[k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (!h[k] || a != b) break;
        }
        if (k == nl) return 1;
    }
    return 0;
}

/* A menu with a way out: the last entry lets the answer be typed instead.
   Used wherever the books have a usual set of answers but not a closed one --
   a spell's range, a magic item's type -- so the common case is one keypress
   and the unusual case is still possible. */
/* Reads a block of text: lines until a blank one. Paragraph breaks are kept
   as newlines, which is what makes this usable for something longer than an
   answer to a question -- a character's history, a patron's demands. */
int ui_text_block(const char *prompt, char *out, size_t n)
{
    char line[512];
    size_t used = 0;

    printf("%s\n", prompt);
    printf("  Type as many lines as you like. A blank line ends it.\n");

    out[0] = '\0';
    for (;;) {
        printf("  > ");
        fflush(stdout);
        if (!read_line(line, sizeof line)) break;
        if (line[0] == '\0') break;

        {
            size_t len = strlen(line);
            if (used + len + 2 >= n) {
                printf("  (that is as much as this note holds)\n");
                break;
            }
            if (used) out[used++] = '\n';
            memcpy(out + used, line, len);
            used += len;
            out[used] = '\0';
        }
    }
    return (int)used;
}

void ui_pick_or_type(const char *prompt, const char *const *options,
                     int count, char *out, size_t n)
{
    const char *opts[64];
    int i, pick;

    if (count > 63) count = 63;
    for (i = 0; i < count; i++) opts[i] = options[i];
    opts[count] = "Something else";

    pick = ui_menu(prompt, opts, NULL, count + 1);
    if (pick == count) {
        ui_line("  Type it", out, n);
    } else {
        snprintf(out, n, "%s", options[pick]);
    }
}

/* The same idea as ui_pick_or_type, for the menus that carry a line of
   explanation beside each entry: the book's list, and below it one more
   entry for whatever the table has agreed on instead. Every list a class
   chooses from goes through this, because a DM who writes a new eldritch
   invocation should be able to put it on the sheet without waiting for the
   program to learn about it.

   Fills out[] with the answer either way, and returns the index chosen, or
   -1 when it was typed. An empty answer re-asks rather than recording a
   blank, since the choice is one the character is owed. */
int ui_menu_custom(const char *prompt, const char *const *options,
                   const char *const *details, int count,
                   const char *custom_label, char *out, size_t n)
{
    const char *opts[257];
    const char *det[257];
    int i;

    if (count > 256) count = 256;
    for (i = 0; i < count; i++) {
        opts[i] = options[i];
        det[i] = details ? details[i] : NULL;
    }
    opts[count] = custom_label;
    det[count] = "Something your table uses that is not printed above";

    for (;;) {
        int pick = ui_menu(prompt, opts, det, count + 1);
        if (pick < count) {
            snprintf(out, n, "%s", options[pick]);
            return pick;
        }
        ui_line("  Type it", out, n);
        if (out[0]) return -1;
        printf("  Nothing entered; choose again.\n");
    }
}

/* A list of things that can each be on or off, shown with checkboxes and
   toggled until the reader is done. flags[] is both the starting state and
   the answer. Returns how many ended up set. */
int ui_toggle_list(const char *prompt, const char *const *options,
                   int count, int *flags)
{
    if (count > 63) count = 63;

    for (;;) {
        const char *opts[65];
        static char labels[65][96];
        int i, pick, set = 0;

        for (i = 0; i < count; i++) {
            snprintf(labels[i], sizeof labels[i], "[%c] %s",
                     flags[i] ? 'x' : ' ', options[i]);
            opts[i] = labels[i];
            if (flags[i]) set++;
        }
        snprintf(labels[count], sizeof labels[count],
                 "Done (%d chosen)", set);
        opts[count] = labels[count];

        pick = ui_menu(prompt, opts, NULL, count + 1);
        if (pick == count) return set;
        flags[pick] = !flags[pick];
    }
}

int split_pipe(const char *src, char *buf, size_t bufsz,
               const char **out, int max)
{
    int n = 0;
    char *p;

    if (!src || !*src) return 0;
    strncpy(buf, src, bufsz - 1);
    buf[bufsz - 1] = '\0';

    p = buf;
    out[n++] = p;
    while ((p = strchr(p, '|')) != NULL) {
        *p++ = '\0';
        if (n >= max) break;
        out[n++] = p;
    }
    return n;
}
