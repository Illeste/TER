#define _GNU_SOURCE

#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BLOCK_SIZE 42
#define BUFFER_SIZE 10



typedef struct {
  char *s;
  int index;
}encrypted_string;

/* Compare function */
int compare (const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp (*ia, *ib);
}

encrypted_string sort (char **s, unsigned len) {
  char *word = malloc (len * sizeof (char));
  strcpy (word, s[0]);
  qsort (s, len, sizeof (char *), compare);
  int index = 0;
  unsigned i;
  for (i = 0; i < len; i++)
    if (!strcmp (word, s[i]))
      break;
  index = i;
  for (i = 0; i < len; i++)
    word[i] = s[i][len - 1];
  return (encrypted_string){word, index};
}

char *shift (int n, char *s) {
  char *a = malloc (BLOCK_SIZE *sizeof (char));
  unsigned i;
  for (i = 0; i < BLOCK_SIZE; i++) {
    int pos = i+n;
    if (i + n >= BLOCK_SIZE)
      pos -= BLOCK_SIZE;
    a[pos] = s[i];
  }
  return a;
}


encrypted_string encrypt (char *s, unsigned len) {
  char **strings = malloc (sizeof (char *) * BLOCK_SIZE);
  unsigned i;

  /* Shift */
  for (i = 0; i < len; i++) {
    strings[i] = shift (i, s);
  }

  /* Sort */
  encrypted_string e = sort (strings, len);

  for (i = 0; i < len; i++)
    free (strings[i]);
  free (strings);
  return e;
}


void
delta_encode (char *buffer, const unsigned int length) {
 char delta = 0;
 char original;
 unsigned int i;
 for (i = 0; i < length; ++i) {
   original = buffer[i];
   buffer[i] -= delta;
   delta = original;
 }
}

int main (int argc, char **argv) {
  if (argc < 2)
    exit (EXIT_FAILURE);
  char *arg = argv[1];
  char *block = malloc (strlen(argv[1]) * 2);

  char buffer[BUFFER_SIZE];
  unsigned i;
  encrypted_string es;
  for (i = 0; i < (strlen (argv[1]) / BLOCK_SIZE); i++) {
    es = encrypt (arg, BLOCK_SIZE);
    sprintf (buffer, "%d", es.index);
    buffer[BUFFER_SIZE-1] = '\0';
    strcat (block, buffer);
    strcat (block, ":");
    strcat (block, es.s);
    strcat (block, "\n");
    arg += BLOCK_SIZE;
  }

  /* Last char to encrypt */
  es = encrypt (arg, strlen(arg));
  sprintf (buffer, "%d", es.index);
  buffer[BUFFER_SIZE-1] = '\0';
  strcat (block, buffer);
  strcat (block, ":");
  strcat (block, es.s);
  strcat (block, "\n");
  strcat (block, "\0");
  printf ("%s\n", block);

  //  printf ("Encrypted Message : %d:%s\n", es.index, es.s);

//  delta_encode (es.s, strlen (es.s));
  free (es.s);
  free (block);
  return EXIT_SUCCESS;
}
