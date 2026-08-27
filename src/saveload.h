/* saveload.h -- writing and reading <Charactername>.txt. */
#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "dnd.h"

#include <stdio.h>

/* Writes the sheet plus the machine-readable block. Returns 0 on success
   and fills path[] with the file actually written. */
int save_character(const Character *c, char *path, size_t pathsz);

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

#endif /* SAVELOAD_H */
