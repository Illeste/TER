#define _GNU_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
/* Get the min values to build the next node in the tree 
 * c1 should be lower than c2 */

/* TODO: If array is sorted from decreasingly, change for loop to -- */
void get_min_amounts (node_t **cd, node_t *c1, node_t *c2 ,
		      unsigned *i1, unsigned *i2) {
  unsigned index1 = 0, index2 = 1;
  c1 = cd[0], c2 = cd[1];
  for (unsigned i = 1; i < ALPHABET_SIZE; i++)
    if (cd[i] && c1->amount > cd[i]->amount) {
      c2 = c1, index2 = index1;
      c1 = cd[i], index1 = i;
    }
  if (c1->amount > c2->amount) {
    c2 = c1;
    c1 = cd[index2];
    unsigned tmp = index1;
    index1 = index2;
    index2 = tmp;
  }
  *i1 = index1, *i2 = index2;
}
/* Replace in the array node c1 and c2 by a new node composed by both */
void add_subtree (node_t **cd, node_t *c1, node_t *c2 ,
		  unsigned *i1, unsigned *i2) {
  node_t *node = create_node (c2, c1);
  cd[*i2] = node;
  cd[*i1] = NULL;
}

node_t *huffman_tree (node_t **cd) {
  unsigned amount_left = ALPHABET_SIZE;
  /* Sub tree construction */
  node_t *c1 = NULL, *c2 = NULL, *root;
  unsigned i1, i2;
  while (1) {
    if (amount_left <= 2) {
      get_min_amounts (cd, c1, c2, &i1, &i2);
      root = create_node (c2, c1);
      break;
    }
    get_min_amounts (cd, c1, c2, &i1, &i2);
    /* Create new subtree from the lowest values and remove values from array */
    add_subtree (cd, c1, c2, &i1, &i2);
    amount_left --;
  }
  return root;
}

void print_array (uint8_t a) {
  int i;
  for (i = 7; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
}


/* Select tri, we need to modify it, to do it faster */
void sort(node_t **tab, unsigned size) {
  unsigned i, j, max;
  node_t *tmp;
  for (i = 0; i < size - 1; i++) {
    if (tab[i] != NULL) {
      max = i;
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
  unsigned i = 0;
  for (i = 0; read(fd, &data, sizeof(uint8_t)) > 0 && i < DATA_READ; i++) {
    if (tab[(int)data] == NULL) {
      node_t *new_data = malloc (sizeof (node_t));
      new_data->data = data;
      new_data->amount = 1;
      tab[(int)data] = new_data;
    }
    else
      tab[(int)data]->amount++;
  }
  sort(tab, ALPHABET_SIZE);
  unsigned k = 0;
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (tab[j] != NULL) {
      print_array (tab[j]->data);
      printf (", data : %d, amount = %d\n", tab[j]->data, tab[j]->amount);
      k += tab[j]->amount;
    }
  }
  printf("\nThere are %d data read, and %d data analized", i, k);
  printf("\n");
  close (fd);
  return EXIT_SUCCESS;
}
