/* saveload.h -- writing and reading <Charactername>.txt. */
#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "dnd.h"

/* Writes the sheet plus the machine-readable block. Returns 0 on success
   and fills path[] with the file actually written. */
int save_character(const Character *c, char *path, size_t pathsz);

/* Reads a file written by save_character. Returns 0 on success. */
int load_character(const char *path, Character *c);

/* Prints the same sheet to stdout. */
void print_sheet(const Character *c);

#endif /* SAVELOAD_H */
