#define _GNU_SOURCE

#include "lbw.h"

////////////////////////////////////
//
// Burrows Wheeler Reverse
//
//////////////////////////////

void reverse_bw (uint16_t *array, uint16_t index, int n) {
  uint16_t **strings =  malloc (n * sizeof (uint16_t *));
    int i, j;
    for (i = 0; i < n; i++)
      strings[i] = calloc (n, sizeof(uint16_t));
    /* Take the string, add a letter from array and then sort them */
    for (i = n - 1; i >= 0 ; i--) {
      for (j = 0; j < n; j++)
        strings[j][i] = array[j];
      merge_sort (strings, n, n);
    }
    /* Original word is at the position index after the reverse
       transformation */
    for (i = 0; i < n; i++)
      array[i] = strings[index][i];

    for (i = 0; i < n; i++)
      free (strings[i]);
    free (strings);
}

/* Given a file and indexes, reverses the Burrows Wheeler transformation */
void undo_bw (const char *file, const char *index_file) {
  int file_fd = open (file, O_RDONLY),
    index_fd = open(index_file, O_RDONLY),
    return_ubw = open (RETURN_UBW, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file_fd == -1 || index_fd == -1) {
    fprintf (stderr, "BW: couldn't open file\n");
    exit (EXIT_FAILURE);
  }

  /* Retrieving byte size and the block size */
  int byte_read = 0, block_size = 0;
  read (index_fd, &byte_read, 1);
  read (index_fd, &block_size, 2);
  uint16_t *array = calloc (block_size, sizeof (uint16_t));
  int data_read, data;
  uint16_t index = 0;
  bool flag = false;
  while (!flag) {
    data_read = 0;
    for (int k = 0; k < block_size && !flag; k++) {
      data = read (file_fd, array + k, byte_read);
      data_read += data / byte_read;
      if (data < 1)
        flag = true;
    }
    read (index_fd, &index, byte_read);
    /* Applying the opposite of Burrows Wheeler to each block */
    reverse_bw (array, index, data_read);
    int a;
    for (a = 0; a < data_read; a++)
      write (return_ubw, array + a, byte_read);
  }
  free (array);
  close (file_fd);
  close (index_fd);
}

/////////////////////
//
// MTF Decoding
//
////////////////

void undo_mtf (const char *file_to_decode, const char *decode_dictionnary,
               const char *result_on_file) {
  int fd = open (file_to_decode, O_RDONLY),
    dictionnary_file = open (decode_dictionnary, O_RDONLY);
  if (fd == -1 || dictionnary_file == -1 ) {
    fprintf (stderr, "Undo MTF: couldn't open file\n");
    exit (EXIT_FAILURE);
  }

  /* Read and store the dictionnary used */
  list dict = malloc (sizeof (struct list));
  list tmp = dict;
  dict->next = NULL;
  uint8_t data = 0;
  uint8_t c = 0;
  while (1){
    read (dictionnary_file, &data, sizeof (uint8_t));
    tmp->data = data;
    read (dictionnary_file, &c, sizeof (uint8_t));
    if (c == ';')
      break;
    tmp->next = malloc (sizeof (struct list));
    tmp = tmp->next;
    tmp->next = NULL;
  }

  int result_file = open (result_on_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (result_file == -1) {
    fprintf (stderr, "Encoding: couldn't open to return result\n");
    exit (EXIT_FAILURE);
  }
  /* Transform each number given by MTF to the corresponding letter */
  list prev;
  while (read (fd, &data, 1)) {
    tmp = dict;
    for (; data != 0; data--) {
      prev = tmp;
      tmp = tmp->next;
    }
    write (result_file, &(tmp->data), 1);
    if (tmp != dict) {
      swap (dict, tmp, prev);
      dict = tmp;
    }
  }

  while (dict->next){
    tmp = dict->next;
    free (dict);
    dict = tmp;
  }
  free (dict);
  close (fd);
  close (result_file);
  close (dictionnary_file);
}

/////////////////////
//
// HUFFMAN Decoding
//
////////////////

node_t *create_node () {
  node_t *node = malloc (sizeof (node_t));
  node->left = NULL;
  node->right = NULL;
  return node;
}

void add_data_on_tree (node_t *tree, uint8_t size_read, uint64_t encode_read,
                       uint16_t word_read) {
  node_t *tmp = tree;
  for (uint8_t i = size_read; i > 0; i--) {
    /* if the ith bit of encode_read equal 0, it go on the left, else right */
    if ((encode_read & (1 << (i - 1))) == 0) {
      if (tmp->left == NULL)
        tmp->left = create_node ();
      tmp = tmp->left;
    }
    else {
      if (tmp->right == NULL)
        tmp->right = create_node ();
      tmp = tmp->right;
    }
  }
  tmp->data = word_read;
}

node_t *create_dictionnary (const char *encode_huf) {
  uint16_t data_read;
  unsigned i_data_read;
  data_read = data_read ^ data_read;
  /* Use to explain what we search */
  unsigned mode = 1;
  // nb de bits déjà copier
  int nb_bits_cpy = 0;
  /* Use to take the word */
  uint64_t word_read = 0;
  /* Use to take the size of encode */
  uint64_t size_read = 0;
  /* Use to take the size of encode */
  uint64_t encode_read = 0;
  unsigned i_cpy = 0;

  /* Create a tree to store the dictionnary of data and encode */
  node_t *tree = create_node();

  /* Save the encode_huffman */
  int huff_code_file = open (encode_huf, O_RDONLY);
  if (huff_code_file == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open to file of encode rules\n");
    exit (EXIT_FAILURE);
  }
  // Tant que l'on a des truc à lire; on lit
  while (read (huff_code_file, &data_read, sizeof (uint16_t)) > 0) {
    bool need_more_data = false;
    i_data_read = 0;
    while (!need_more_data) {
      switch (mode) {
        // recherche du word
        case 1:
          i_cpy = cpy_data (&word_read, 8, nb_bits_cpy,
                            (uint64_t)data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
          // S'il ne reste plus de bits à copier pour le word
          if (nb_bits_cpy == 8) {
            nb_bits_cpy = 0;
            mode = 2;
          }
          break;

        // recherche de la size de l'encode
        case 2:
          i_cpy = cpy_data (&size_read, 8, nb_bits_cpy,
                            (uint64_t)data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
          if (nb_bits_cpy == 8) {
            nb_bits_cpy = 0;
            mode = 3;
          }
          break;

        // recherche de l'encode
        case 3:
          i_cpy = cpy_data (&encode_read, (int)size_read, nb_bits_cpy,
                            (uint64_t)data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
          if (nb_bits_cpy == (int)size_read) {
            nb_bits_cpy = 0;
            mode = 4;
          }
          break;

        /* Add all data in dictionnary */
        case 4:
          add_data_on_tree (tree, size_read, encode_read, word_read);

          printf ("%lu : ", word_read);
          print_encode (word_read, 8);
          printf (" to ");
          print_encode (encode_read, size_read);
          printf ("    , depth = %d", size_read);
          printf ("\n");

          mode = 1;
          word_read = word_read ^ word_read;
          size_read = size_read ^ size_read;
          encode_read = encode_read ^ encode_read;
          break;
      }
      if (i_data_read == 16) {
        need_more_data = true;
        data_read = data_read ^ data_read;
      }
    }
  }
  close (huff_code_file);
  return tree;
}

bool is_leaf (node_t *node) {
  return (node->left == NULL && node->right == NULL);
}

uint64_t get_nb_writing_bits () {
  int size_huf_file = open (SIZE_HUF, O_RDONLY);
  if (size_huf_file == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open file SIZE_HUF\n");
    exit (EXIT_FAILURE);
  }
  uint64_t nb = 0;
  read (size_huf_file, &nb, sizeof (uint64_t));
  close (size_huf_file);
  return nb;
}

void decomp_huffman (node_t *tree, const char *file_to_decode,
                     const char *result_file) {
  uint64_t nb_bits_on_file = get_nb_writing_bits ();
  printf ("nb_bits_on_file = %lu\n", nb_bits_on_file);
  int huff_prev_result = open (file_to_decode, O_RDONLY);
  if (huff_prev_result == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open file of previous result\n");
    exit (EXIT_FAILURE);
  }
  /* Save the decode_huffman */
  int huff_decode_file = open (result_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (huff_decode_file == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open file of decode\n");
    exit (EXIT_FAILURE);
  }
  uint8_t data_read;
  uint64_t encode_read;
  data_read = data_read ^ data_read;
  encode_read = encode_read ^ encode_read;
  unsigned i_encode_read = 0;
  while (read (huff_prev_result, &data_read, sizeof (uint8_t)) > 0) {
    cpy_data (&encode_read, 64, i_encode_read, data_read, 8, 0);
    i_encode_read += 8;
    bool find_word = true;
    while (find_word) {
      find_word = false;
      node_t *tmp = tree;
      for (unsigned i = i_encode_read; i > 0 && nb_bits_on_file != 0; i--) {
        if ((encode_read & (1 << (i - 1))) != 0)
          tmp = tmp->right;
        else
          tmp = tmp->left;
        if (is_leaf (tmp)) {
          if (write(huff_decode_file, &(tmp->data), sizeof(uint8_t)) == -1) {
            fprintf (stderr, "Huffman: writing on file opened failed\n");
            exit (EXIT_FAILURE);
          }
          nb_bits_on_file -= i_encode_read - (i - 1);
          i_encode_read -= i_encode_read - (i - 1);
          find_word = true;
          break;
        }
      }
    }
  }
  close (huff_prev_result);
  close (huff_decode_file);
}

void delete_dictionnary (node_t *tree) {
  if (tree->left != NULL) {
    delete_dictionnary (tree->left);
  }
  if (tree->right != NULL) {
    delete_dictionnary (tree->right);
  }
  free (tree);
}

int main (int argc, char **argv) {
  /* Ouverture fichier compressé
   * + Huffman a l'eners : récupère le fichier post mft
   * donc mtf dans le meme sens, copier coller fonction
   * bw decompression
   *
   * Retourner, ecrire dans meme fichier/ailleurs decompression
   */
  node_t *dictionnary = create_dictionnary (ENCODE_HUF);
  decomp_huffman (dictionnary, RETURN_HUF, INV_HUFF);
  delete_dictionnary (dictionnary);
  undo_mtf(RETURN_ENC, DICTIONNARY_ENC, RETURN_UMTF);
  printf("MTF fait \n");
  undo_bw (RETURN_BW, INDEX_BW);
  return EXIT_SUCCESS;
}
