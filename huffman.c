#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define RETURN_HUF "result_huffman"
/* Amount of data scanned */
#define DATA_READ 50000
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

/* TODO: If array is sorted from decreasingly, change for loop to -- */
void get_min_amounts (node_t **cd, int *i1, int *i2) {
  int index_min, index, i;
  // Find the first valid index
  for (i = 1; i < ALPHABET_SIZE; i++)
    if (cd[i]) {
      index = i;
      break;
    }
  // Find the second valid index
  for (i++; i < ALPHABET_SIZE; i++)
    if (cd[i]) {
      index_min = i;
      break;
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
    if (amount_left <= 2) {
      get_min_amounts (cd, &i1, &i2);
      root = create_node (cd[i2], cd[i1]);
      printf("noeud entre %d et %d \n", i2, i1);
      break;
    }
    get_min_amounts (cd, &i1, &i2);
    cd[i2] = create_node (cd[i2], cd[i1]);
    printf("noeud entre %d et %d \n", i2, i1);
    cd[i1] = NULL;
//  cd[i1]->amount = DATA_READ + 1;
    amount_left --;
  }
  return root;
}


uint8_t *deep_transform (uint8_t *encoding, unsigned depth,
			     uint8_t last_bit) {
  /* 8 is the size of uint8_t in bits */
  unsigned index = depth / 8;
  uint8_t *ret = malloc (sizeof (uint8_t) * (index + 1));
  unsigned i;
  for (i = 0; i < index + 1; i++) {
    ret[i] = encoding[i];
  }
  /* Optimization for copying encoding into the array */
  if (last_bit != 2) {
    encoding[index] = (encoding[index] << 1) + last_bit;
  }

  return ret;
}


void huffman_encoding (node_t *node, transform_t *array,
		       unsigned depth, uint8_t *encoding) {
  if (node->left || node->right) {
    uint8_t *enc;
    if (node->left) {
      enc = deep_transform (encoding, depth, 0);
      huffman_encoding (node->left, array, depth + 1, encoding);
      free (enc);
    }
    if (node->right) {
      enc = deep_transform (encoding, depth, 1);
      huffman_encoding (node->right, array, depth + 1, encoding);
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
  for (i = 0; i < t.depth / 8; i++) {
    for (j = 7; j != 0; j--)
      (t.encoding[i] & (1 << j)) == 0 ?printf("0"): printf("1");
  }
  if (t.depth % 8) {
    for (j = t.depth % 8; j != 0; j--)
      (t.encoding[i] & (1 << (j-1))) == 0 ?printf("0"): printf("1");
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
  node_t **tab = malloc (sizeof (node_t *) * ALPHABET_SIZE);
  unsigned i = 0, nb_letters = 0;
  for (i = 0; read(fd, &data, sizeof(uint8_t)) > 0 && i < DATA_READ; i++) {
    if (tab[(int)data] == NULL) {
      node_t *new_data = malloc (sizeof (node_t));
      new_data->data = data;
      new_data->amount = 1;
      tab[(int)data] = new_data;
      nb_letters++;
    }
    else
      tab[(int)data]->amount++;
  }
//  ne marche pas, inverse les identifiants lignes et les data
//  sort(tab, ALPHABET_SIZE);
  unsigned k = 0;
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (tab[j] != NULL) {
      printf ("j = %d,   ",j);
      print_array (tab[j]->data);
      printf (", data : %d, amount = %u\n", tab[j]->data, tab[j]->amount);
      k += tab[j]->amount;
    }
  }
  printf("\nOut of %u data analized", i);
  printf("\n");
  node_t *root = huffman_tree (tab, nb_letters);
  /* Free "tab" */
  transform_t *array = malloc (sizeof (transform_t ) * ALPHABET_SIZE);
  uint8_t init_encoding = 0;
  huffman_encoding (root, array, 0, &init_encoding);

  /* Print encoding */
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (array[j].encoding) {
      print_array (j);
      printf (", ");
      /* print par rapport au depth le truc par lequel on remplace */
      print_encoding (array[j]);
      printf("\n");
    }
  }
  free(array);
  free(root);
  close (fd);
  return EXIT_SUCCESS;
}
