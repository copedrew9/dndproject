/* details.c -- the parts of a character that are prose.
 *
 * Personality, appearance, backstory and notes were settled once during
 * creation and then fixed. They are the things most likely to change in
 * play, so they get a screen of their own that a saved character can be
 * loaded into.
 *
 * Notes are a list rather than one long line: a contact, a debt, the name of
 * the innkeeper's dog. They are free text on purpose -- the books have
 * nothing to say about them, so there is nothing to pick from.
 */
#include "dnd.h"
#include "data.h"
#include "details.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ notes */

/* One line per note when listing them, since a note may run to paragraphs
   and the list is for finding one, not reading it. */
static void note_summary(const Note *nt, char *out, size_t n)
{
    const char *nl = strchr(nt->body, '\n');
    int first = nl ? (int)(nl - nt->body) : (int)strlen(nt->body);

    if (first > 46) first = 46;
    if (!strcmp(nt->title, nt->body)) {         /* a one-line note */
        snprintf(out, n, "%s", nt->title);
        return;
    }
    snprintf(out, n, "%-22s %.*s%s", nt->title, first, nt->body,
             (nl || (int)strlen(nt->body) > 46) ? "..." : "");
}

static void list_notes(const Character *c)
{
    int i;

    if (c->note_count == 0) {
        printf("\n  No notes yet.\n");
        return;
    }
    printf("\n");
    for (i = 0; i < c->note_count; i++) {
        char line[120];
        note_summary(&c->notes[i], line, sizeof line);
        printf("  %2d. %s\n", i + 1, line);
    }
}

/* Prints a note in full, paragraph breaks and all. */
static void show_note(const Note *nt)
{
    char buf[MAX_LORE];
    char *start, *p;

    printf("\n  %s\n", nt->title);
    if (!strcmp(nt->title, nt->body)) return;   /* a one-line note */
    snprintf(buf, sizeof buf, "%s", nt->body);
    start = buf;
    for (p = buf;; p++) {
        if (*p == '\n' || *p == '\0') {
            int end = (*p == '\0');
            *p = '\0';
            if (*start) ui_wrap(start, 4);
            else printf("\n");
            if (end) break;
            start = p + 1;
        }
    }
}

static void add_note(Character *c)
{
    if (c->note_count >= MAX_NOTES) {
        printf("  That is as many notes as this program holds. Remove one "
               "first.\n");
        return;
    }
    {
        Note *nt = &c->notes[c->note_count];
        nt->title[0] = nt->body[0] = '\0';

        ui_line("  A short title for it", nt->title, sizeof nt->title);
        if (!nt->title[0]) return;
        ui_text_block("  The note:", nt->body, sizeof nt->body);
        if (!nt->body[0]) {
            /* A title with nothing under it is still worth keeping.
               Copied rather than printed, because the two are members of
               the same struct and the compiler cannot see that they do not
               overlap. */
            size_t tn = strlen(nt->title);
            if (tn >= sizeof nt->body) tn = sizeof nt->body - 1;
            memcpy(nt->body, nt->title, tn);
            nt->body[tn] = '\0';
        }
        c->note_count++;
        printf("  Noted.\n");
    }
}

static void remove_note(Character *c)
{
    const char *opts[MAX_NOTES + 1];
    static char labels[MAX_NOTES][80];
    int i, pick;

    if (c->note_count == 0) {
        printf("  There are no notes to remove.\n");
        return;
    }
    for (i = 0; i < c->note_count; i++) {
        note_summary(&c->notes[i], labels[i], sizeof labels[i]);
        opts[i] = labels[i];
    }
    opts[c->note_count] = "Back";

    pick = ui_menu("  Remove which?", opts, NULL, c->note_count + 1);
    if (pick == c->note_count) return;

    show_note(&c->notes[pick]);
    if (!ui_yesno("  Remove it?", 0)) return;

    for (i = pick; i < c->note_count - 1; i++) {
        c->notes[i] = c->notes[i + 1];
    }
    c->note_count--;
    printf("  Removed.\n");
}

static void edit_note(Character *c)
{
    const char *opts[MAX_NOTES + 1];
    static char labels[MAX_NOTES][80];
    int i, pick;

    if (c->note_count == 0) {
        printf("  There are no notes to edit.\n");
        return;
    }
    for (i = 0; i < c->note_count; i++) {
        note_summary(&c->notes[i], labels[i], sizeof labels[i]);
        opts[i] = labels[i];
    }
    opts[c->note_count] = "Back";

    pick = ui_menu("  Which note?", opts, NULL, c->note_count + 1);
    if (pick == c->note_count) return;

    show_note(&c->notes[pick]);

    {
        static const char *const what[] = {
            "Add more to the end", "Replace what it says",
            "Rename it", "Leave it alone"
        };
        Note *nt = &c->notes[pick];

        switch (ui_menu("  What would you like to do?", what, NULL, 4)) {
        case 0: {
            char more[MAX_LORE];
            size_t used = strlen(nt->body);
            if (ui_text_block("  What to add:", more, sizeof more)
                && used + strlen(more) + 2 < sizeof nt->body) {
                nt->body[used] = '\n';
                snprintf(nt->body + used + 1, sizeof nt->body - used - 1,
                         "%s", more);
            }
            break;
        }
        case 1:
            ui_text_block("  The note:", nt->body, sizeof nt->body);
            break;
        case 2: {
            char t[MAX_NAME];
            ui_line_default("  Title", nt->title, t, sizeof t);
            snprintf(nt->title, sizeof nt->title, "%s", t);
            break;
        }
        default:
            break;
        }
    }
}

static void notes_menu(Character *c)
{
    for (;;) {
        static const char *const modes[] = {
            "Add a note", "Read or change one", "Remove one", "Done"
        };

        list_notes(c);
        switch (ui_menu("  Notes:", modes, NULL, 4)) {
        case 0: add_note(c);    break;
        case 1: edit_note(c);   break;
        case 2: remove_note(c); break;
        default: return;
        }
    }
}

/* ------------------------------------------------------------ personality */

/* Offers the background's suggestions where there are any, and always
 * allows something else -- including for a character with no background at
 * all, which the creation wizard used to skip over entirely. */
static void edit_line(const Character *c, const char *label,
                      const char *const *suggestions, char *out)
{
    char prompt[96];

    if (out[0]) {
        printf("\n  %s is currently: ", label);
        ui_wrap(out, 4);
        if (!ui_yesno("  Change it?", 0)) return;
    }

    snprintf(prompt, sizeof prompt, "  %s:", label);
    if (suggestions && suggestions[0]) {
        int n = 0;
        while (n < 9 && suggestions[n]) n++;
        ui_pick_or_type(prompt, suggestions, n, out, MAX_TEXT);
    } else {
        ui_line(prompt, out, MAX_TEXT);
    }
    (void)c;
}

static void personality_menu(Character *c)
{
    const BackgroundData *bg = (c->background_id >= 0)
                             ? &BACKGROUNDS[c->background_id] : NULL;

    if (bg) {
        printf("\n  Suggestions come from the %s background; you can always "
               "write your own.\n", bg->name);
    } else if (c->background_name[0]) {
        printf("\n  %s is a background of your own, so there is nothing to "
               "suggest -- write what fits.\n", c->background_name);
    }

    edit_line(c, "Personality trait", bg ? bg->traits : NULL, c->trait);
    edit_line(c, "Ideal",             bg ? bg->ideals : NULL, c->ideal);
    edit_line(c, "Bond",              bg ? bg->bonds  : NULL, c->bond);
    edit_line(c, "Flaw",              bg ? bg->flaws  : NULL, c->flaw);
    edit_line(c, "Appearance",        NULL, c->appearance);
    edit_line(c, "Backstory",         NULL, c->backstory);
}

/* ---------------------------------------------------------------- the screen */

void edit_details(Character *c)
{
    ui_header("Notes and Details");
    ui_para("The parts of a character that are prose rather than numbers: "
            "who they are, what they look like, and whatever you want to "
            "remember between sessions.");

    for (;;) {
        static const char *const modes[] = {
            "Notes",
            "Personality, appearance and backstory",
            "Done"
        };

        printf("\n  %s", c->name);
        if (c->background_id >= 0) {
            printf(" -- %s", BACKGROUNDS[c->background_id].name);
        } else if (c->background_name[0]) {
            printf(" -- %s", c->background_name);
        }
        printf(", %d note%s\n", c->note_count,
               c->note_count == 1 ? "" : "s");

        switch (ui_menu("  What would you like to change?", modes, NULL, 3)) {
        case 0: notes_menu(c);       break;
        case 1: personality_menu(c); break;
        default: return;
        }
    }
}
