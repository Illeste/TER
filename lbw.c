#include "lbw.h"

/* Display the helped message */
void usage(int status, char *argv)
{
  if (status == EXIT_SUCCESS)
  {
    fprintf(stdout,
      "Usage: %s [OPTION] FILE\n"
      "Compress your file.\n"
      "-v, --verbose		verbose output\n"
      "-h, --help		display this help\n", argv);
  }
  else
    fprintf(stderr, "Try '%s --help' for more information.\n", argv);
  exit(status);
}

int _open (char *file, int m) {
  int fd;
  if (m == 1)
    fd = open (file, O_RDONLY);
  else
    fd = open (file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1) {
    perror ("BW: error opening file");
    exit (EXIT_FAILURE);
  }
  return fd;
}


void print_array (uint16_t a) {
  int i;
  for (i = BW_SIZE - 1; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}

void print_array2 (uint16_t a) {
  int i;
  for (i = 15; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}

void print_encoding (transform_t t) {
  unsigned i, j;
  for (i = 0; i < t.depth / 8; i++) {
    for (j = 8; j != 0; j--)
      (t.encoding[i] & (1 << (j - 1))) == 0 ?printf("0"): printf("1");
  }
  if (t.depth % 8) {
    for (j = t.depth % 8; j != 0; j--)
      (t.encoding[i] & (1 << (j-1))) == 0 ?printf("0"): printf("1");
  }
}

void print_encode (uint64_t a, int size) {
  int i;
  for (i = size - 1; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
}

void print_sort_tab (uint8_t **tab, unsigned block_size) {
  unsigned i, j;
  for (i = 0; i < block_size; i++) {
    for (j = 0; j < block_size; j++) {
      printf("%c", tab[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}


void swap (list first, list to_change, list prev_to_change) {
  list tmp = to_change;
  prev_to_change->next = to_change->next;
  tmp->next = first;
}

int cpy_data (uint64_t *array, int size_of_array, int nbaw_array,
         uint64_t data, int size_of_data, int nbaw_data) {
  int i;
  int cpy_bits = 0;
  for (i = size_of_data - nbaw_data - 1;
      i >= 0 && (nbaw_array + cpy_bits) != size_of_array; i--) {
    if ((data & (1UL << i)) != 0)
      *array = *array | (1UL << (size_of_array - (nbaw_array + cpy_bits) - 1));
    cpy_bits++;
  }
  return (cpy_bits);
}
