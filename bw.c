#define _GNU_SOURCE

#include "lbw.h"

int byte_write = (LETTER_SIZE == 8) ? 1 : 2;

////////////////////////////////////
//
// Burrows Wheeler Transformation
//
//////////////////////////////

uint16_t *shift (uint16_t *s, unsigned block_size) {
  uint16_t *shifted_s = malloc (block_size * sizeof(uint16_t));
  unsigned i;
  /* For all uint8_t without the last */
  for (i = 0; i < block_size - 1; i++) {
    shifted_s[i + 1] = s[i];
  }
  /* And for the last uint8_t */
  shifted_s[0] = s[block_size - 1];
  return shifted_s;
}

void burrows_wheeler (uint16_t *s, unsigned block_size) {
  uint16_t **strings = malloc (sizeof (uint16_t *) * (block_size));
  unsigned i;
  strings[0] = malloc (sizeof (uint16_t) * block_size);
  for (i = 0; i < block_size; i++) {
    strings[0][i] = s[i + 1];
  }
  for (i = 1; i < block_size; i++)
    strings[i] = shift (strings[i - 1], block_size);
  /* Sort the array by lexicographical order */
  merge_sort (strings, block_size, block_size);
  unsigned index = 0;
  /* Search pos of the original string */
  for (i = 0; i < block_size; i++) {
    if (compare(s + 1, strings[i], block_size) == 0) {
      index = i;
      break;
    }
  }
  s[0] = index;
  for (i = 0; i < block_size; i++) {
    s[i + 1] = strings[i][block_size - 1];
    free(strings[i]);
  }
  free (strings);
}

void bw (char *file) {
  int fd = open (file, O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "BW: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  /* Getting file size */
  size_t size = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);

  /* Should contain the index then the word */
  uint16_t *array = malloc (sizeof (uint16_t) * (BLOCK_SIZE + 1));
  unsigned nb_blocks = size * BYTES_SIZE / (LETTER_SIZE * BLOCK_SIZE);
  unsigned size_last_block = (size * BYTES_SIZE -
                             (nb_blocks * LETTER_SIZE * BLOCK_SIZE))
                             / BYTES_SIZE;
  if (size_last_block != 0)
    nb_blocks++;

  /* Print the result into a file and index to another */
  int result_file = open (RETURN_BW, O_WRONLY | O_CREAT | O_TRUNC, 0666),
    index_file = open (INDEX_BW, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1 || index_file == -1) {
    fprintf (stderr, "BW: couldn't open a file\n");
    exit (EXIT_FAILURE);
  }

  /* Write in header of the index file the size of read and the block
    size used in bw */
  array[0] = byte_write, array[1] = BLOCK_SIZE;
  write (index_file, array, 1);
  write (index_file, array + 1, 2);
  unsigned i, j, data_count = 0;
  uint16_t data_read;
  for (i = 0; i < BLOCK_SIZE + 1; i++)
    array[i] = 0;

  /* WARNING: LETTER_SIZE == 8 or 16, or else not handled now */
  for (i = 0; i < nb_blocks; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      data_read = 0;
      /* Fill the array before passing through Burrows Wheeler */
      read (fd, &data_read, 1);

      data_count += byte_write * BYTES_SIZE;
      /* Store on array, all data with sizeof LETTER_SIZE */
      while (data_count >= LETTER_SIZE && j < BLOCK_SIZE) {
        array[j + 1] = 0;
        cpy_data (array + j + 1, data_read);
        j++;
        data_count -= LETTER_SIZE;
        data_read = data_read >> LETTER_SIZE;
      }
      j--;
    }
    int block_size;
    if (i == nb_blocks - 1 && size_last_block != 0)
      block_size = size_last_block;
    else
      block_size = BLOCK_SIZE;
    burrows_wheeler (array, block_size);
    write (index_file, array, byte_write);
    /* Write into result file */
    for (int k = 1; k < block_size + 1; k++)
      write (result_file, array + k, byte_write);
  }
  free (array);
  close (fd);
  close (index_file);
  close (result_file);
}

//////////////////////
//
// MTF Encoding 
//
//////////////////

void move_to_front () {
  int fd = open (RETURN_BW, O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Encoding: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  uint8_t original;
  uint8_t *reading_tab = malloc (sizeof (uint8_t) * (ALPHABET_SIZE));
  int i = 0;
  for (i = 0; i < ALPHABET_SIZE; i++)
    reading_tab[i] = 0;
  while (read(fd, &original, sizeof(uint8_t)) > 0)
    reading_tab[original] = 1;

  /* Count each letter of the file */
  int count = 0;
  uint8_t first;
  for (i = 0; i < ALPHABET_SIZE; i++)
    if (reading_tab[i] != 0) {
      reading_tab[i] = count;
      count++;
      if (count == 1)
        first = (uint8_t)i;
    }
  /* Making the dictionnary for MTF */
  list begin = malloc (sizeof (struct list));
  begin->data = first;
  begin->next = NULL;
  list tmp = begin;
  for (i = 0; i < ALPHABET_SIZE; i++)
    if (reading_tab[i] != 0) {
      list next = malloc (sizeof (struct list));
      next->data = (uint8_t)i;
      next->next = NULL;
      tmp->next = next;
      tmp = next;
    }
  /* DEBUG */
  tmp = begin;
  printf ("There are %d letters in the dictionnary :(", count);
  for (i = 0; i < count; i++) {
    printf ("%u,", tmp->data);
    tmp = tmp->next;
  }
  printf (")\n");

  /* Write the transformed file */
  int dictionnary_file = open (DICTIONNARY_ENC,
                              O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (dictionnary_file == -1) {
    fprintf (stderr, "Encoding: couldn't open to return the dictionnary\n");
    exit (EXIT_FAILURE);
  }
  /* Print dictionnary on file */
  tmp = begin;
  while (tmp != NULL) {
    write (dictionnary_file, &tmp->data, sizeof (uint8_t));
    if (tmp->next != NULL)
      write (dictionnary_file, ":", sizeof (uint8_t));
    else
      write (dictionnary_file, ";", sizeof (uint8_t));
    tmp = tmp->next;
  }
  close (dictionnary_file);

  /* Write the transformed file */
  lseek (fd, 0, SEEK_SET);
  int result_file = open (RETURN_ENC, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "Encoding: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  list prev;
  uint8_t new_val;
  while (read(fd, &original, sizeof(uint8_t)) > 0) {
    tmp = begin;
    /* Find the index of the letter original in the dictionnary */
    for (i = 0; tmp->data != original; i++) {
      prev = tmp;
      tmp = tmp->next;
    }
    new_val = (uint8_t)i;
    /* Move tmp to the front of the dictionnary */
    if (i != 0) {
      swap(begin, tmp, prev);
      begin = tmp;
    }
    if (write(result_file, &new_val, sizeof(uint8_t)) == -1) {
      fprintf (stderr, "Encoding: writing on file opened failed\n");
      exit (EXIT_FAILURE);
    }
  }

  while (begin->next != NULL) {
    tmp = begin->next;
    free (begin);
    begin = tmp;
  }
  free (begin);
  free (reading_tab);
  close (fd);
  close (result_file);
}

////////////////////
//
//   Huffman Tree
//
///////////////

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
  /* Find two valid indexes */
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
  /* Sort them */
  if (cd[index]->amount < cd[index_min]->amount) {
    int tmp = index_min;
    index_min = index;
    index = tmp;
  }
  /* Find, if possible, other index with lower amount */
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
  uint8_t *ret = malloc (sizeof (uint8_t) * (index + 2));
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

/* Create the Huffman encoding, for a given node
 * If it has a left child,  we pass the child a 0
 * If it has a right child, we pass the child a 1
 */
void huffman_encoding (node_t *node, transform_t *array,
           unsigned depth, uint8_t *encoding) {
  if (node->left || node->right) {
    /* Intern node */
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

/*
nb_bits = nb de bits dans le buffer à écrire "to_write"
nb_cpy = nb de bits copier dans le buffer. (au cas où il ne copierai pas tout)
nbaw = nb_bits_already_wrote = nb de bits qui a déjà été écrit
(ce dernier peut être non utile en fonction de la taille de l'entier à copier,
c'est à dire si la taille ne dépasse pas sizeof (uint16_t))
*/
int cpy_data_huffman (int file, uint16_t *write_buf, uint16_t to_cpy, int size,
                      int nb_bits_write) {
  uint16_t *to_write = write_buf;
  int nb_bits = nb_bits_write;
  int nb_cpy = 0;
  /* nbaw = nb_bits_already_wrote */
  int nbaw = 0;

  nb_cpy = cpy_data2 (to_write, to_cpy, size,
                      nb_bits, nbaw);
  nb_bits += nb_cpy;
  while (nb_bits == 16) {
/*    printf("\nwriting :");
    print_array2(*to_write);
    printf("\n");
*/    if (write (file, to_write, sizeof (uint16_t)) == -1) {
      fprintf (stderr, "Huffman: writing on file .code_huff failed\n");
      exit (EXIT_FAILURE);
    }
    nb_bits = 0;
    nbaw += nb_cpy;
    if (nbaw != size) {
      nb_cpy = cpy_data2 (to_write, to_cpy, size,
                          nb_bits, nbaw);
      nb_bits += nb_cpy;
    }
  }
  return nb_bits;
}

void huffman () {
  int fd = open (RETURN_ENC, O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "Huffman: couldn't open file\n");
    exit (EXIT_FAILURE);
  }
  uint8_t data;
  node_t **tab = malloc (sizeof (node_t *) * ALPHABET_SIZE);
  unsigned i;
  for (i = 0; i < ALPHABET_SIZE; i++)
    tab[i] = NULL;
  unsigned nb_letters = 0;
  /* Count the amount of each letter */
  for (i = 0; read (fd, &data, sizeof(uint8_t)) > 0 && i < DATA_READ; i++) {
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
  /* DEBUG */
  unsigned k = 0;
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (tab[j] != NULL) {
      printf ("j = %d,   ",j);
      print_array (tab[j]->data);
      printf (", amount = %u\n", tab[j]->amount);
      k += tab[j]->amount;
    }
  }
  printf ("\nOut of %u data analyzed", i);
  printf ("\n");

  /* Creation of the Huffman Tree */
  node_t *root = huffman_tree (tab, nb_letters);
  transform_t *array = malloc (sizeof (transform_t ) * ALPHABET_SIZE);
  for (i = 0; i < ALPHABET_SIZE; i++)
    array[i].encoding = 0;
  uint8_t init_encoding = 0;
  huffman_encoding (root, array, 0, &init_encoding);
  /* Print encoding */
  printf ("\nEncoding: <word> to <encoding>\n");
  for (int j = 0; j < ALPHABET_SIZE; j++) {
    if (array[j].encoding) {
      printf ("%d : ", j);
      print_array (j);
      printf (" to ");
      print_encoding (array[j]);
      printf("    , depth = %d", array[j].depth);
      printf("\n");
    }
  }

  /* Print the code into a file */
  int huff_code_file = open (ENCODE_HUF,
                              O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (huff_code_file == -1) {
    fprintf (stderr, "Huffman: couldn't open to return the encode\n");
    exit (EXIT_FAILURE);
  }
  uint16_t to_write = 0;
  int nb_bits = 0;
  uint8_t *encoding;
  for (uint16_t j = 0; j < ALPHABET_SIZE; j++) {
    encoding = array[j].encoding;
    if (encoding) {
      /* Print the word */
/*      printf("j : ");
      print_array2 (j);
      printf("\n");
*/      nb_bits = cpy_data_huffman (huff_code_file, &to_write, j,
                                  sizeof (uint8_t) * BYTES_SIZE, nb_bits);
      /* Print the size of encoding */
/*      printf("depth : ");
      print_array (array[j].depth);
      printf("\n");
*/      nb_bits = cpy_data_huffman (huff_code_file, &to_write, array[j].depth,
                                  sizeof (uint8_t) * BYTES_SIZE, nb_bits);
      /* Print the encoding */
      int max_size = sizeof (uint8_t) * BYTES_SIZE;
      int nb_bits_encoding = array[j].depth;
      while (nb_bits_encoding != 0) {
/*        printf("encode : ");
        print_array (*encoding);
        printf("\n");
*/        if (nb_bits_encoding > max_size) {
          nb_bits = cpy_data_huffman (huff_code_file, &to_write, *encoding,
                                      max_size, nb_bits);
          nb_bits_encoding -= max_size;
          encoding++;
        }
        else {
          nb_bits = cpy_data_huffman (huff_code_file, &to_write, *encoding,
                                      nb_bits_encoding, nb_bits);
          nb_bits_encoding = 0;
        }
      }
    }
  }
  uint16_t end = 0;
  cpy_data_huffman (huff_code_file, &to_write, end,
                                    sizeof (uint16_t) * BYTES_SIZE, nb_bits);

  close (huff_code_file);

  /* Write encoded letters into the final file */
  lseek (fd, 0, SEEK_SET);
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
  uint64_t nb_bits_writing;
  nb_bits_writing = nb_bits_writing ^ nb_bits_writing;
  for (i = 0; read (fd, &data_read, sizeof(uint8_t)) > 0; i++) {
    /* Write the encoding */
    unsigned k, l;
    for (k = 0; k < (array[data_read].depth / 8); k++) {
      for (l = 8; l != 0; l--) {
        if (!(array[data_read].encoding[k] & (1 << (l - 1))))
          data_to_write = data_to_write << 1;
        else
          data_to_write = (data_to_write << 1) + 1;
        width++;
        if (width == 8) {
          if (write (result_file, &data_to_write, sizeof(uint8_t)) == -1) {
            fprintf (stderr, "Huffman: writing on file opened failed\n");
            exit (EXIT_FAILURE);
          }
          nb_bits_writing += width;
          width = 0;
          data_to_write = 0;
        }
      }
    }
    /* Write the rest of the encoding, if there is */
    if (array[data_read].depth  % 8) {
      for (l = array[data_read].depth % 8; l != 0; l--) {
        if (!(array[data_read].encoding[k] & (1 << (l - 1))))
          data_to_write = data_to_write << 1;
        else
          data_to_write = (data_to_write << 1) + 1;
        width++;
        if (width == 8) {
          if (write (result_file, &data_to_write, sizeof(uint8_t)) == -1) {
            fprintf (stderr, "Huffman: writing on file opened failed\n");
            exit (EXIT_FAILURE);
          }
          nb_bits_writing += width;
          width = 0;
          data_to_write = 0;
        }
      }
    }
  }
  // One last value to write
  if (width != 0) {
    nb_bits_writing += width;
    while (width != 8) {
      data_to_write = data_to_write << 1;
      width++;
    }
    if (write (result_file, &data_to_write, sizeof(uint8_t)) == -1) {
      fprintf (stderr, "Huffman: writing on file opened failed\n");
      exit (EXIT_FAILURE);
    }
  }
  int numb_write_file = open (SIZE_HUF, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (numb_write_file == -1) {
    fprintf (stderr, "Huffman: couldn't open SIZE_HUF file\n");
    exit (EXIT_FAILURE);
  }
  if (write (numb_write_file, &nb_bits_writing, sizeof(uint64_t)) == -1) {
    fprintf (stderr, "Huffman: writing on file SIZE_HUF failed\n");
    exit (EXIT_FAILURE);
  }
  close (numb_write_file);
  for (i = 0; i < ALPHABET_SIZE; i++)
    if (array[i].encoding)
      free(array[i].encoding);
  free (array);
  free_tree (root);
  free (tab);
  close (result_file);
  close (fd);
  printf("nb_bits_writing = %lu\n", nb_bits_writing);
}
////////////////////
//
//  Main Function
//
///////////////


int main (int argc, char **argv) {
  if (argc < 2) {
    fprintf (stderr, "BW: usage: bw <file>\n");
    exit (EXIT_FAILURE);
  }
  bw (argv[1]);
  move_to_front ();
  huffman ();
  return EXIT_SUCCESS;
}
