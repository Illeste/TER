#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LETTER_SIZE 8

#define RESULT_FILE "aaa"

int main (int argc, char **argv) {

  /* Catch parameters and use them */
  if (argc < 3) {
    fprintf (stderr, "transform: usage: transform <file> <int[|1 ; 7|]>\n");
    exit (EXIT_FAILURE);
  }
  int fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    perror ("error opening file to transform");
    exit (EXIT_FAILURE);
  }
  int result_file = open (RESULT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    perror ("error opening file RESULT_FILE");
    exit (EXIT_FAILURE);
  }

  int new_size = atoi (argv[2]);
  uint8_t data_read = 0;
  uint8_t data_to_write = 0;
  unsigned nb_data_read = 0;
  unsigned nb_data_write = 0;
  while (read (fd, &data_read, sizeof (uint8_t)) > 0) {
    bool need_more_data = false;
    nb_data_read += LETTER_SIZE;
    unsigned l;
    while (!need_more_data) {
      for (int i = nb_data_write; i < new_size; i++) {
        if (nb_data_read == 0) {
          need_more_data = true;
          break;
        }
        if (data_read & (1 << (nb_data_read - 1)))
          data_to_write = (data_to_write << 1) + 1;
        else
          data_to_write = data_to_write << 1;
        nb_data_read--;
        nb_data_write++;
      }
      if (nb_data_write == new_size) {
        while (nb_data_write != LETTER_SIZE) {
          data_to_write = data_to_write << 1;
          nb_data_write++;
        }
        write (result_file, &data_to_write, sizeof(uint8_t));
        data_to_write = data_to_write ^ data_to_write;
        nb_data_write = 0;
      }
    }
  }
  /* If the last character wasn't wroten */
  if (nb_data_write != 0) {
    while (nb_data_write != LETTER_SIZE) {
      data_to_write = data_to_write << 1;
      nb_data_write++;
    }
    write (result_file, &data_to_write, sizeof(uint8_t));
    data_to_write = data_to_write ^ data_to_write;
    nb_data_write = 0;
  }

  close (fd);
  close (result_file);
  return EXIT_SUCCESS;
}
