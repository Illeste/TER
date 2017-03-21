#define _GNU_SOURCE

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define RETURN_HUF "result_huffman"
/* Amount of data scanned */
#define DATA_READ 100000
/* Should be 2**LETTER_SIZE */
#define ALPHABET_SIZE 256

/* Huffman tree */

typedef struct node {
  uint8_t data;
  unsigned amount;
  struct node *left, *right;
} node_t;


typedef struct {
  unsigned depth;
  uint8_t *encoding;
}transform_t;


void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
}

/* Huffman coding */
node_t *create_leaf (node_t *n) {
  node_t *leaf = malloc (sizeof (*leaf));
  leaf->left = NULL;
  leaf->right = NULL;
  leaf->amount = n->amount;
  leaf->data = n->data;
  return leaf;
}
/* Creates a node with subnodes n1 and n2,
 * and overwrites in with the amount of children */
node_t *create_node (node_t *n1, node_t *n2) {
  node_t *node = malloc (sizeof (*node));
  node->amount = n1->amount + n2->amount;
  node->left = n1;
  node->right = n2;
  return node;
}

void get_min_amounts (node_t **cd, int *i1, int *i2) {
  int index_min = 0, index = 0, i;
  bool first = false;
  // Find two valid indexes
  for (i = 0; i < ALPHABET_SIZE; i++)
    if (cd[i]) {
      if (first) {
        index_min = i;
        break;
      }
      else {
        index = i;
        first = true;
      }
    }
  // Sort them
  if (cd[index]->amount < cd[index_min]->amount) {
    int tmp = index_min;
    index_min = index;
    index = tmp;
  }
  // Find, if possible, other index with lower amount
  for (i++; i < ALPHABET_SIZE; i++) {
    if (cd[i] && cd[i]->amount < cd[index]->amount) {
      if (cd[i]->amount < cd[index_min]->amount) {
        index = index_min;
        index_min = i;
      }
      else {
        index = i;
      }
    }
  }
  *i1 = index_min;
  *i2 = index;
}

node_t *huffman_tree (node_t **cd, unsigned amount_left) {
  /* Sub tree construction */
  node_t *root;
  int i1, i2;
  while (1) {
    if (amount_left <= 2)
      break;
    /* Create a node from the 2 lowest frequencies of apparition */
    get_min_amounts (cd, &i1, &i2);
    cd[i2] = create_node (cd[i2], cd[i1]);
    cd[i1] = NULL;
    amount_left --;
  }
  /* Form root with the last nodes */
  get_min_amounts (cd, &i1, &i2);
  root = create_node (cd[i2], cd[i1]);
  return root;
}


uint8_t *deep_transform (uint8_t *encoding, unsigned depth, uint8_t last_bit) {
  /* 8 is the size of uint8_t in bits */
  unsigned index = depth / 8;
  uint8_t *ret = malloc (sizeof (uint8_t) * (index + 1));
  unsigned i;
  for (i = 0; i < index + 1; i++) {
    ret[i] = encoding[i];
  }
  /* Optimization for copying encoding into the array */
  if (last_bit != 2) {
    ret[index] = (encoding[index] << 1) + last_bit;
  }

  return ret;
}


void huffman_encoding (node_t *node, transform_t *array,
           unsigned depth, uint8_t *encoding) {
  if (node->left || node->right) {
    uint8_t *enc;
    if (node->left) {
      enc = deep_transform (encoding, depth, 0);
      huffman_encoding (node->left, array, depth + 1, enc);
      free (enc);
    }
    if (node->right) {
      enc = deep_transform (encoding, depth, 1);
      huffman_encoding (node->right, array, depth + 1, enc);
      free (enc);
    }
  }
  else {
    /* Leaf */
    array[node->data].depth = depth;
    array[node->data].encoding = deep_transform (encoding, depth, 2);
  }
}


/* Select a better/faster sort */
void sort(node_t **tab, unsigned size) {
  unsigned i, j;
  node_t *tmp;
  for (i = 0; i < size - 1; i++) {
    if (tab[i] != NULL) {
      unsigned max = i;
      for (j = i + 1; j < size; j++)
        if (tab[j] != NULL)
          if (tab[max]->amount < tab[j]->amount)
            max = j;
      if (max != i) {
        tmp = tab[i];
        tab[i] = tab[max];
        tab[max] = tmp;
      }
    }
  }
}

void print_encoding (transform_t t) {
  unsigned i, j;
  for (i = 0; i < (t.depth + 1) / 8; i++) {
    for (j = 8; j != 0; j--)
      (t.encoding[i] & (1 << (j - 1))) == 0 ?printf("0"): printf("1");
  }
  if ((t.depth + 1)  % 8) {
    for (j = t.depth % 8; j != 0; j--)
      (t.encoding[i] & (1 << (j-1))) == 0 ?printf("0"): printf("1");
  }
}

void free_tree (node_t *node) {
  if (node) {
    if (node->left || node->right) {
      if (node->left)
        free_tree (node->left);
      if (node->right)
        free_tree (node->right);
    }
    free (node);
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
  uint8_t data;
  node_t **tab = malloc (sizeof (node_t *) * ALPHABET_SIZE);
  unsigned i = 0, nb_letters = 0;
  for (i = 0; read(fd, &data, sizeof(uint8_t)) > 0 && i < DATA_READ; i++) {
    if (tab[(int)data] == NULL) {
      node_t *new_data = malloc (sizeof (node_t));
      new_data->data = data;
      new_data->amount = 1;
      new_data->left = NULL;
      new_data->right = NULL;
      tab[(int)data] = new_data;
      nb_letters++;
    }
    else
      tab[(int)data]->amount++;
  }
  close (fd);
//  ne marche pas, inverse les identifiants lignes et les data
//  sort(tab, ALPHABET_SIZE);
  unsigned k = 0;
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (tab[j] != NULL) {
      printf ("j = %d,   ",j);
      print_array (tab[j]->data);
      printf (", amount = %u\n", tab[j]->amount);
      k += tab[j]->amount;
    }
  }
  printf("\nOut of %u data analyzed", i);
  printf("\n");
  node_t *root = huffman_tree (tab, nb_letters);
  /* Free "tab" */
  transform_t *array = malloc (sizeof (transform_t ) * ALPHABET_SIZE);
  uint8_t init_encoding = 0;
  huffman_encoding (root, array, 0, &init_encoding);

  /* Print encoding */
  printf("\nEncoding: <word> to <encoding>\n");
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (array[j].encoding) {
      printf("%u : ", j);
      print_array (j);
      printf (" to ");
      /* print par rapport au depth le truc par lequel on remplace */
      print_encoding (array[j]);
      printf("\n");
    }
  }

  /* Now we read again the file and write the translate thanks to array */
  fd = open (argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Huffman: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  int result_file = open (RETURN_HUF, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "Huffman: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  uint8_t data_read, data_to_write = 0;
  unsigned width = 0;
  for (i = 0; read(fd, &data_read, sizeof(uint8_t)) > 0; i++) {
    /* Code of function print_encoding, with some modification */
    unsigned k, l;
    for (k = 0; k < (array[data_read].depth + 1) / 8; k++) {
      for (l = 8; l != 0; l--) {
        if (!(array[data_read].encoding[k] & (1 << (l - 1))))
          data_to_write = data_to_write << 1;
        else
          data_to_write = (data_to_write << 1) + 1;
        width++;
        if (width == 8) {
          if (write(result_file, &data_to_write, sizeof(uint8_t)) == -1) {
            fprintf (stderr, "Huffman: writing on file opened failed\n");
            exit (EXIT_FAILURE);
          }
          width = 0;
          data_to_write = 0;
        }
      }
    }
    if ((array[data_read].depth + 1)  % 8) {
      for (l = array[data_read].depth % 8; l != 0; l--) {
        if (!(array[data_read].encoding[k] & (1 << (l - 1))))
          data_to_write = data_to_write << 1;
        else
          data_to_write = (data_to_write << 1) + 1;
        width++;
        if (width == 8) {
          if (write(result_file, &data_to_write, sizeof(uint8_t)) == -1) {
            fprintf (stderr, "Huffman: writing on file opened failed\n");
            exit (EXIT_FAILURE);
          }
          width = 0;
          data_to_write = 0;
        }
      }
    }
  }
  // One last value to write
  if (width != 0) {
    while (width != 8) {
      data_to_write = data_to_write << 1;
      width++;
    }
    if (write(result_file, &data_to_write, sizeof(uint8_t)) == -1) {
      fprintf (stderr, "Huffman: writing on file opened failed\n");
      exit (EXIT_FAILURE);
    }
  }

  free (array);
  free_tree (root);
  close (result_file);
  close (fd);
  return EXIT_SUCCESS;
}
