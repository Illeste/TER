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

int compare (const void *a, const void *b, unsigned block_size) {
  uint16_t *_a = (uint16_t *) a;
  uint16_t *_b = (uint16_t *) b;
  unsigned i;
  /* Little endian storage */
  for (i = 0; i < block_size; i++)
    if (_a[i] != _b[i])
      return _a[i] - _b[i];
  return 0;
}

void merge (uint16_t **tab, uint16_t **tab2,
            unsigned tab_size1, unsigned tab_size2, unsigned block_size) {
  uint16_t *tmp1[tab_size1];
  uint16_t *tmp2[tab_size2];
  unsigned i;
  for (i = 0; i < tab_size1; i++)
    tmp1[i] = tab[i];
  for (i = 0; i < tab_size2; i++)
    tmp2[i] = tab2[i];
  unsigned i_tab2 = 0;
  unsigned i_tab1 = 0;
  unsigned c = 0;
  while (i_tab1 < tab_size1 && i_tab2 < tab_size2) {
    if (compare(tmp1[i_tab1], tmp2[i_tab2], block_size) > 0) {
      tab[c] = tmp2[i_tab2];
      c++;
      i_tab2++;
    }
    else {
      tab[c] = tmp1[i_tab1];
      c++;
      i_tab1++;
    }
  }
  if (i_tab1 == tab_size1)
    for (i = i_tab2; i < tab_size2; i++) {
      tab[c] = tmp2[i];
      c++;
    }
  else
    for (i = i_tab1; i < tab_size1; i++) {
      tab[c] = tmp1[i];
      c++;
    }
}

void merge_sort (uint16_t **tab, unsigned tab_size, unsigned block_size) {
  if (tab_size > 1){
    unsigned tab_size1 = tab_size / 2;
    unsigned tab_size2 = tab_size - tab_size1;
    merge_sort (tab, tab_size1, block_size);
    merge_sort (tab + tab_size1, tab_size2, block_size);
    merge (tab, tab + tab_size1, tab_size1, tab_size2, block_size);
  }
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
