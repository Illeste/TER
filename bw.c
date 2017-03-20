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
#define BLOCK_SIZE 50
/* Alphabet used in Huffman's Algorithm has for size 2**LETTER_SIZE */
#define LETTER_SIZE 1
#define RETURN_BW "result_bw"

void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}

void print_sort_tab (uint8_t **tab, unsigned block_size) {
  unsigned i, j;
  for (i = 0; i < block_size; i++) {
    for (j = 0; j < block_size; j++) {
      printf("%c", tab[i][j]);
//      print_array (tab[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

int compare (const void *a, const void *b, unsigned block_size) {
  uint8_t *_a = (uint8_t *) a;
  uint8_t *_b = (uint8_t *) b;
  unsigned i;

  /* Little endian storage */
  for (i = 0; i < block_size; i++) {
    if (_a[i] != _b[i]) {
      return _a[i] - _b[i];
    }
  }
  return 0;
}

/* Select tri, we need to modify it, to do it faster */
void sort(uint8_t **tab, unsigned block_size) {
  unsigned i, j, min;
  uint8_t *tmp;
  for (i = 0; i < block_size - 1; i++) {
    min = i;
    for (j = i + 1; j < block_size; j++)
      if (compare(tab[min], tab[j], block_size) > 0)
        min = j;
    if (min != i) {
      tmp = tab[i];
      tab[i] = tab[min];
      tab[min] = tmp;
    }
  }
}

uint8_t *shift (uint8_t *s, unsigned block_size) {
  uint8_t *shifted_s = malloc (block_size * sizeof(uint8_t));
  unsigned i;
  /* For all uint8_t without the last */
  for (i = 0; i < block_size - 1; i++) {
    shifted_s[i + 1] = s[i];
//    strncpy(&shifted_s[i + 1], &s[i], sizeof(uint8_t));
  }
  /* And for the last uint8_t */
  shifted_s[0] = s[block_size - 1];
//  strncpy(&shifted_s[0], &s[block_size], sizeof(uint8_t));
  return shifted_s;
}

void burrows_wheeler (uint8_t *s, unsigned block_size) {
  uint8_t **strings = malloc (sizeof (uint8_t *) * (block_size));
  unsigned i;
  strings[0] = s + 1;
//  strncpy(&strings[0], &s + 1, block_size);
  for (i = 1; i < block_size; i++)
    strings[i] = shift (strings[i-1], block_size);
  //print_sort_tab (strings, block_size);
  /* Sort the array by lexicographical order */
  sort(strings, block_size);
  //print_sort_tab (strings, block_size);
  unsigned index = 1;
  /* Search pos of the original string */
  bool find_index = false;
  for (i = 0; i < block_size; i++) {
    s[i + 1] = strings[i][block_size - 1];
    if (!find_index && compare(s + 1, strings[i], block_size) == 0) {
      index = i;
      find_index = true;
    }
  }
  s[0] = index;
  /*
  for (i = 0; i < block_size; i++) {
    free(strings[i]);
  }
  free (strings);
  */
}

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
  unsigned nb_blocks = size / (LETTER_SIZE * BLOCK_SIZE);
  unsigned size_last_block = size - (nb_blocks * LETTER_SIZE * BLOCK_SIZE);
  if (size_last_block != 0)
    nb_blocks++;

  /* Choice to make:
   * write in a file to avoid to run out of memory
   * write in memory
   */
  int result_file = open (RETURN_BW, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "BW: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
//  uint8_t bw_file[size + nb_blocks];
  unsigned i, j, block_size = BLOCK_SIZE;
  /* WARNING: LETTER_SIZE <= 8, or else not handled now */
  for (i = 0; i < nb_blocks; i++) {
    if (i == nb_blocks - 1)
      block_size = size_last_block;
    for (j = 0; j < block_size; j++) {
      array[j + 1] = 0;
      /* Fill the array before passing through Burrows Wheeler */
      read(fd, array + j + 1, 1);
    }
    burrows_wheeler (array, block_size);
    write(result_file, array, block_size + 1);
    /* Print the result of bw program */
    /*    printf("%d", array[0]);
	  for (int k = 0; k < block_size; k++)
	  printf("%c", array[k + 1]);
	  printf("\n");
    */
    /* Write into memory */
    /*    for (j = 0; j < block_size + 1; j++)
	  bw_file[j + i * (block_size + 1)] = array[j];
    */
  }
  close (fd);
  close (result_file);
  return EXIT_SUCCESS;
}
