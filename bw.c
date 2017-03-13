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
#define BLOCK_SIZE 7
#define BUFFER_SIZE 10
/* Alphabet used in Huffman's Algorithm has for size 2**LETTER_SIZE */
#define LETTER_SIZE 8 

void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}


int compare (const void *a, const void *b) {
  uint8_t *_a = (uint8_t *) a;
  uint8_t *_b = (uint8_t *) b;
  int i;
  for (i = 0; i < BLOCK_SIZE; i++)
    print_array (_a[i]);
  printf("\n");
  for (i = 0; i < BLOCK_SIZE; i++)
    print_array (_b[i]);
  printf("\n");


  /* Little endian storage */
  for (i = 0; i < BLOCK_SIZE; i++) {
    if (_a[i] != _b[i]) {
      printf("%d\n\n", _a[i] - _b[i]);
      return _a[i] - _b[i];
    }
  }
  printf("0\n\n");
  return 0;
}

void sort(uint8_t *to_sort[], uint8_t s[]) {
  /* Sort the array by lexicographical order */
  qsort (to_sort, BLOCK_SIZE, sizeof (uint8_t *), compare);
  unsigned index = 65;
  int i, j;
  for (i = 0; i < BLOCK_SIZE; i++) {
    for (j = 0; j < BLOCK_SIZE; j++)
      print_array (to_sort[i][j]);
    printf("\n");
  }
  /* Search pos of the original string */
  s[0] = index;
}


uint8_t *shift (uint8_t *s) {
  uint8_t *shifted_s = malloc (BLOCK_SIZE * sizeof(uint8_t)); 
  int i;
  for (i = 0; i < BLOCK_SIZE; i++) {
    int pos = i + 1;
    if (pos == BLOCK_SIZE)
      pos = 0;
    shifted_s[pos] = s[i];
  }
  return shifted_s;
}


void burrows_wheeler (uint8_t *s) {
  uint8_t **strings = malloc (sizeof (uint8_t *) * (BLOCK_SIZE));
  unsigned i, j;
  strings[0] = s + 1 ;
  for (i = 1; i < BLOCK_SIZE; i++)
    strings[i] = shift (strings[i-1]);
  for (i = 0; i < BLOCK_SIZE; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      print_array (strings[i][j]);
    }
    printf("\n");
  }
  printf("\n");
  sort (strings, s);
  /* for (i = 0; i < BLOCK_SIZE; i++) */
  /*   free(strings[i]); */
  free (strings);
}

/* delta_encode */

int main (int argc, char **argv) {
  if (argc < 2) {
    fprintf (stderr, "BW: usage: bw <file>\n");
    exit (EXIT_FAILURE);
  } 
  int fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "BW: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  /* Getting file size */
  size_t size = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);
  
  /* Should contain the index then the word */
  uint8_t array[BLOCK_SIZE + 1]; 
  unsigned nb_blocks = size * 8 / (LETTER_SIZE * BLOCK_SIZE);  
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
    for (j = 0; j < BLOCK_SIZE; j++) {
      array[j + 1] = 0;
      /* Fill the array before passing through Burrows Wheeler */
      read(fd, array + j + 1, 1);
    }
    burrows_wheeler (array);
    /* Write into memory */
    for (j = 0; j < BLOCK_SIZE + 1; j++)
      bw_file[j + i * (BLOCK_SIZE + 1)] = array[j];
  }
  printf("%s\n", bw_file);
  return EXIT_SUCCESS;
}
