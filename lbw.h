#ifndef _LBW_H
#define _LBW_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

/* Size of each block passed through Burrows Wheeler */
/* !!!!! Faire gaffe si on depasse la taille d'un uint (8 ou 16) pour index */
#define BLOCK_SIZE 500
/* Alphabet used in Huffman's Algorithm has for size 2**LETTER_SIZE */
#define LETTER_SIZE 8
#define BYTES_SIZE 8
/* Should be 2**LETTER_SIZE */
#define ALPHABET_SIZE 256
/* Amount of data scanned in Huffman */
#define DATA_READ 50000
/* Name of in betwwen files for storage */
#define RETURN_BW           "result_bw"
#define INDEX_BW            ".index_bw"
#define RETURN_ENC          "result_encode"
#define DICTIONNARY_ENC     ".dico_enc"
#define RETURN_HUF          "result_huffman"
#define ENCODE_HUF          ".code_huff"
#define SIZE_HUF            ".size_huff"


/* Indicating if we have to read byte per byte or 2 by 2 */
int byte_write = (LETTER_SIZE == 8) ? 1 : 2;

/* Huffman Structs */
typedef struct node {
  uint8_t data;
  unsigned amount;
  struct node *left, *right;
} node_t;

typedef struct {
  unsigned depth;
  uint8_t *encoding;
}transform_t;

/* Encoding Structs */
struct list {
  uint8_t data;
  struct list *next;
};

typedef struct list *list;
/* Debug functions */
void print_array (uint16_t a);
void print_array2 (uint16_t a);
void print_encoding (transform_t t);
void print_sort_tab (uint8_t **tab, unsigned block_size);

int compare (const void *a, const void *b, unsigned block_size);
void merge_sort (uint16_t **tab, unsigned tab_size, unsigned block_size);
void swap (list first, list to_change, list prev_to_change);
void cpy_data (uint16_t *array, uint16_t data);
int cpy_data2 (uint16_t *array, uint16_t data, int size_of_data, int nb_bits,
	       int nbaw);
int cpy_data3 (uint64_t *array, int size_of_array, int nbaw_array,
              uint16_t data, int size_of_data, int nbaw_data);


#endif // LBW_H
