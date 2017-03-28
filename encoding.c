#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define RETURN_ENC "result_encode"

#define NB_CHAR 256

void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}
/* delta_encode */

int main (int argc, char **argv) {
  if (argc < 2) {
    fprintf (stderr, "Encoding: usage: encoding <file>\n");
    exit (EXIT_FAILURE);
  }
  int fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Encoding: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  uint8_t original;
  uint8_t *reading_tab = malloc (sizeof (uint8_t) * (NB_CHAR));
  for (int i = 0; i < NB_CHAR; i++)
    reading_tab[i] = 0;
  while (read(fd, &original, sizeof(uint8_t)) > 0)
    reading_tab[original] = 1;
  close(fd);
  /* Create a new language only with the alphabet of file */
  int count = 0;
  uint8_t first;
  for (int i = 0; i < NB_CHAR; i++)
    if (reading_tab[i] != 0) {
      reading_tab[i] = count;
      count++;
      if (count == 1)
        first = (uint8_t)i;
    }

/*
  printf("On a %u lettres avec comme language :\n", count);
  for (int i = 0; i < NB_CHAR; i++)
    if (reading_tab[i] != 0) {
      printf("%u ---> %d   = ", i, reading_tab[i]);
      print_array(reading_tab[i]);
      printf("\n");
    }
*/

  uint8_t *dictionary = malloc (sizeof (uint8_t) * count);
  dictionary[0] = first;
  int j = 1;
  for (int i = 0; i < NB_CHAR; i++)
    if (reading_tab[i] != 0) {
      dictionary[j] = (uint8_t)i ;
      j++;
    }

  printf("On a %u lettres dans le dictionnaire :(", count);
  for (int i = 0; i < count; i++)
    printf("%u,", dictionary[i]);
  printf(")\n");


  fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Encoding: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  int result_file = open (RETURN_ENC, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "Encoding: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  uint8_t delta;
  if (read(fd, &original, sizeof(uint8_t)) <= 0) {
    fprintf (stderr, "Encoding: file opened is empty\n");
    exit (EXIT_FAILURE);
  }
  uint8_t new_val = reading_tab[original];
  if (write(result_file, &new_val, sizeof(uint8_t)) == -1) {
    fprintf (stderr, "Encoding: writing on file opened failed\n");
    exit (EXIT_FAILURE);
  }
  delta = original;
  while (read(fd, &original, sizeof(uint8_t)) > 0) {
    new_val = (((reading_tab[original] - reading_tab[delta]) % count)
              + count) % count;
/*
    printf("original = ");
    print_array(original);
    printf(", delta = ");
    print_array(delta);
    printf(", xor = ");
    print_array(new_val);
    printf("\n");
    */
//    if (original > 128)
//    printf("original = %u, delta = %u, xor = %u\n", original, delta, new_val);
    if (write(result_file, &new_val, sizeof(uint8_t)) == -1) {
      fprintf (stderr, "Encoding: writing on file opened failed\n");
      exit (EXIT_FAILURE);
    }
    delta = original;
  }
  close (fd);
  close (result_file);
  free (reading_tab);
  return EXIT_SUCCESS;
}
