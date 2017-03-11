#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

/* Size of each block passed through Burrows Wheeler */
#define BLOCK_SIZE 40
#define BUFFER_SIZE 10
/* Alphabet used in Huffman's Algorithm has for size 2**LETTER_SIZE */
#define LETTER_SIZE 8 


typedef struct {
  char *s;
  int index;
}encrypted_string;

int compare (const void *a, const void *b) {
  uint8_t *_a = (uint8_t *) a;
  uint8_t *_b = (uint8_t *) b;
  unsigned i;
  for (i = 0; i < BLOCK_SIZE; i++)
    if (_a[i] != _b[i])
      return _a[i] - _b[i];
  return 0;
}

/* Sorts to_sort array, and writes index then last letter of each sorted word */
void sort (uint8_t **to_sort, unsigned len, uint8_t *s) {
  qsort (to_sort, len, sizeof (uint8_t), compare);
  printf("qsort\n");
  unsigned i;
  for (i = 0; i < len; i++)
    if (*to_sort[i] == *s)
      break;
  printf("a\n");
  uint8_t index = i;
  s[0] = (uint8_t) index;
  for (i = 0; i < len; i++)
    s[index + 1] = to_sort[i][len + 1];
}

/* Returns the input shifted one to the right, 
 * leaving the first block for the future index */
uint8_t *shift (int n, uint8_t *s) {
  uint8_t *a = malloc (BLOCK_SIZE *sizeof (uint8_t));
  unsigned i;
  for (i = 0; i < BLOCK_SIZE; i++) {
    int pos = i + 1 + n;
    if (i + 1 + n >= BLOCK_SIZE)
      pos -= BLOCK_SIZE;
    a[pos] = s[i + 1];
  }
  return a;
}


void burrows_wheeler (uint8_t *s, unsigned len) {
  uint8_t **strings = malloc (sizeof (uint8_t) * BLOCK_SIZE);
  unsigned i;
  for (i = 0; i < len; i++) 
    strings[i] = shift (i, s);
  sort (strings, len, s);
  free (strings);
}

/* void delta_encode (char *buffer, const unsigned int length) { */
/*  char delta = 0; */
/*  char original; */
/*  unsigned int i; */
/*  for (i = 0; i < length; ++i) { */
/*    original = buffer[i]; */
/*    buffer[i] -= delta; */
/*    delta = original; */
/*  } */
/* } */

int main (int argc, char **argv) {
  if (argc < 2) {
    fprintf (stderr, "BW: usage: bw <file>\n");
    exit (EXIT_FAILURE);
  }
  
  int fd = open (argv[1], O_WRONLY);
  if (fd == -1) {
    fprintf (stderr, "BW: couldn't open file\n");
    exit (EXIT_FAILURE);
  }

  /* Getting file size */
  size_t size = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);

  /* Should contain the index then the word */
  uint8_t array[BLOCK_SIZE + 1]; 

  unsigned nb_blocks = size / (LETTER_SIZE * BLOCK_SIZE);  
  if (nb_blocks * LETTER_SIZE * BLOCK_SIZE != size)
    nb_blocks++;

  /* Choice to make: 
   * write in a file to avoid to run out of memory
   * write in memory
   */
  uint8_t bw_file[size + nb_blocks];
  unsigned i, j;
  /* WARNING: LETTER_SIZE <= 8, or else not handled now */
  for (i = 0; i < nb_blocks; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) 
      /* Fill the array before passing through Burrows Wheeler */
      read(fd, &array[j + 1], LETTER_SIZE);
    burrows_wheeler (array, BLOCK_SIZE);
    /* Write into memory */
    for (j = 0; j < BLOCK_SIZE; j++)
      bw_file[j + i * (BLOCK_SIZE + 1)] = array[j];
  }
  
  return EXIT_SUCCESS;
}
