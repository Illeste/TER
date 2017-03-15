#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define RETURN_HUF "result_huffman"

void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
}

typedef struct count_data {
  uint8_t data;
  int count;
} count_data;

/* Select tri, we need to modify it, to do it faster */
void sort(count_data **tab, unsigned size) {
  unsigned i, j, max;
  count_data *tmp;
  for (i = 0; i < size - 1; i++) {
    if (tab[i] != NULL) {
      max = i;
      for (j = i + 1; j < size; j++)
        if (tab[j] != NULL)
          if (tab[max]->count < tab[j]->count)
            max = j;
      if (max != i) {
        tmp = tab[i];
        tab[i] = tab[max];
        tab[max] = tmp;
      }
    }
  }
}

int main (int argc, char **argv) {
  if (argc < 2) {
    fprintf (stderr, "Huffman: usage: huffman <file>\n");
    exit (EXIT_FAILURE);
  }
  int fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Huffman: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  /*
  int result_file = open (RETURN_HUF, O_WRONLY | O_CREAT | O_TRUNC);
  if (result_file == -1) {
    fprintf (stderr, "Encode: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  */
  /*
  if (write(result_file, &original, sizeof(uint8_t)) == -1) {
    fprintf (stderr, "Encode: writing on file opened failed\n");
    exit (EXIT_FAILURE);
  }
  */
  uint8_t data;
  count_data **tab = malloc (sizeof (count_data *) * 256);
  unsigned i = 0;
  for (i = 0; read(fd, &data, sizeof(uint8_t)) > 0 && i < 50000; i++) {
    if (tab[(int)data] == NULL) {
      count_data *new_data = malloc (sizeof (count_data));
      new_data->data = data;
      new_data->count = 1;
      tab[(int)data] = new_data;
    }
    else
      tab[(int)data]->count++;
  }
  sort(tab, 256);
  unsigned k = 0;
  for (int j = 0; j < 256; j++) {
    if (tab[j] != NULL) {
      print_array (tab[j]->data);
      printf (", data : %d, count = %d\n", tab[j]->data, tab[j]->count);
      k += tab[j]->count;
    }
  }
  printf("\nThere are %d data read, and %d data analized", i, k);
  printf("\n");
  close (fd);
  return EXIT_SUCCESS;
}
