#define _GNU_SOURCE

#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
  char *s;
  int index;
}encrypted_string;

/* Compare function */
int compare (const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
}

encrypted_string sort (char **s, unsigned len) {
  char *word = malloc (sizeof (char *));
  strcpy (word, s[0]);
  qsort(s, len, sizeof(char *), compare);
  int index = 0;
  unsigned i;
  for (i = 0; i < len; i++)
    printf("%s\n", s[i]);
  for (i = 0; i < len; i++) 
    if (!strcmp (word, s[i]))
      break;
  index = i;
  for (i = 0; i < len; i++)
    word[i] = s[i][len - 1];
  return (encrypted_string){word, index};
}

char *shift (int n, char *s) {
  char *a = malloc (sizeof (char *));
  unsigned i;
  for (i = 0; i < strlen (s); i++) {
    int pos = i+n;
    if (i + n >= strlen(s))
      pos -= strlen(s);
    a[pos] = s[i];
  }
  return a;
}


encrypted_string encrypt (char *s) {
  char **strings = malloc (sizeof (char *) * strlen (s));
  unsigned i, len = strlen (s);
  
  /* Shift */
  for (i = 0; i < len; i++) {
    strings[i] = shift(i, s);
  }
  
  /* Sort */
  encrypted_string e = sort (strings, len);

  for (i = 0; i < len; i++)
    free (strings[i]);
  free (strings);
  return e;
}


void
delta_encode (char *buffer, const unsigned int length)
{
 char delta = 0;
 char original;
 unsigned int i;
 for (i = 0; i < length; ++i)
 {
   original = buffer[i];
   buffer[i] -= delta;
   delta = original;
 }
}

int main (int argc, char **argv) {
  if (argc < 2)
    exit(EXIT_FAILURE);
  
  encrypted_string es = encrypt (argv[1]);
  printf ("Encrypted Message : %d%s\n", es.index, es.s);

  delta_encode(es.s, strlen(es.s));
  printf("After Differential Encoding %x\n", es.s);
  free (es.s);
  return EXIT_SUCCESS;
}
