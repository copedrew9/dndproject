/* saveload.h -- writing and reading <Charactername>.txt. */
#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "dnd.h"

#include <stdio.h>

/* Writes the sheet plus the machine-readable block. Returns 0 on success
   and fills path[] with the file actually written. */
int save_character(const Character *c, char *path, size_t pathsz);

/* The file name a sheet of this name is written to, without the ".txt":
   the characters a file name cannot hold are dropped, and a name left empty
   by that becomes "Unnamed". */
void sheet_filename(const char *name, char *out, size_t n);

/* Whether a file already holds a character, so that a sheet which is not
   one is not written over the top of it. */
int file_is_character(const char *path);

/* Reads a file written by save_character. Returns 0 on success. */
int load_character(const char *path, Character *c);

/* Prints the same sheet to stdout. */
void print_sheet(const Character *c);

/* The '|' separated record format, which homebrew.txt shares with the
   character file. A field is written with the separator, the escape and a
   newline escaped, and read back with record_unescape(); a field carrying
   none of the three is unchanged either way, which is what lets a file
   written before any of this was escaped still read correctly. */
void record_put(FILE *f, const char *text);
void record_unescape(char *field);

/* Splits a line on '|' in place, filling out[] with at most max pieces and
   returning how many there were. The last piece keeps whatever is left,
   separators and all, when the line has more fields than there is room for. */
int  record_split(char *line, char **out, int max);

/* A number out of one of those fields, held to the range the program's own
   prompts allow. Both files are meant to be readable and so are edited, and
   an unbounded number here is an overflow later rather than a big number:
   two billion of an item, multiplied by its weight. strtol rather than
   atoi, which is itself undefined on a number too large to hold. */
int  record_int(const char *field, int lo, int hi);

/* The prose wrapper the readable half of these files is written with. It
   breaks only at spaces and never rewrites the text, which is what lets
   tools/roundtrip.py compare a sheet against itself. Shared with shop.c,
   whose files are written the same way. */
void record_wrap(FILE *f, const char *text, int indent);

#endif /* SAVELOAD_H */
