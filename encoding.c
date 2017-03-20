#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define RETURN_ENC "result_encode"

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
  int result_file = open (RETURN_ENC, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "Encoding: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  uint8_t original, delta;
  if (read(fd, &original, sizeof(uint8_t)) <= 0) {
    fprintf (stderr, "Encoding: file opened is empty\n");
    exit (EXIT_FAILURE);
  }
  if (write(result_file, &original, sizeof(uint8_t)) == -1) {
    fprintf (stderr, "Encoding: writing on file opened failed\n");
    exit (EXIT_FAILURE);
  }
  delta = original;
  printf("%d ", original);
  while (read(fd, &original, sizeof(uint8_t)) > 0) {
    uint8_t new_val = original - delta;
    printf("%d ", new_val);
    if (write(result_file, &new_val, sizeof(uint8_t)) == -1) {
      fprintf (stderr, "Encoding: writing on file opened failed\n");
      exit (EXIT_FAILURE);
    }
    delta = original;

  }

  printf("\n");
  close (fd);
  close (result_file);
  return EXIT_SUCCESS;
}
