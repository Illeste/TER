#define _GNU_SOURCE

#include "lbw.h"

static bool verbose = false;
static unsigned BLOCK_SIZE = SIZE_BLOCK;

////////////////////////////////////
//
// Burrows Wheeler Reverse
//
//////////////////////////////

/* Given a file and indexes, reverses the Burrows Wheeler transformation */

void undo_bw (char *file) {
  int file_fd = _open (file, 1),
    return_ubw = _open (RETURN_UBW, 2),
    index = _open (INDEX_BW, 1);
  
  uint8_t buffer [BLOCK_SIZE + 1];
  unsigned int alp_size = 256;
  unsigned int transform_vector[BLOCK_SIZE + 1],
    count[alp_size + 1],
    total[alp_size + 1];
  
  while (1) {
    /* Retrieving block and first/last */
    unsigned int length;
    if (read (index, &length, sizeof (int)) == 0)
      break;
    read (file_fd, (char *)buffer, length);
    unsigned int first, last;
    read (index, &first, sizeof (int));
    read (index, &last, sizeof (int));

    unsigned int i;
    
    for (i = 0; i < alp_size + 1; i++)
      count[i] = 0;
    for (i = 0; i < length; i++) {
      if (i == last)
	count[alp_size]++;
      else
	count[buffer[i]]++;
    }

     int sum = 0;
     for (i = 0 ; i < alp_size + 1 ; i++) {
       total[i] = sum;
       sum += count[i];
       count[i] = 0;
     }

     for (i = 0 ; i <  length ; i++) {
       int index;
       if (i == last)
	 index = alp_size;
       else
	 index = buffer[i];
       transform_vector[count[index] + total[index]] = i;
       count[index]++;
     }

      unsigned int j;
      i = first;
      for (j = 0 ; j <  (length - 1) ; j++) {
	write (return_ubw, &buffer[i], 1);
	i = transform_vector[i];
      }
  }
  close (file_fd);
  close (return_ubw);
}  

/////////////////////
//
// MTF Decoding
//
////////////////

void undo_mtf (char *file_to_decode, char *decode_dictionnary,
               char *result_on_file) {
  int fd = _open (file_to_decode, 1),
    dictionnary_file = _open (decode_dictionnary, 1);

  /* Read and store the dictionnary used */
  list dict = malloc (sizeof (struct list));
  list tmp = dict;
  dict->next = NULL;
  uint16_t data = 0;
  uint8_t c = 0;
  int rw_size = (MTF_SIZE == 8) ? 1 : 2;
  while (1) {
    read (dictionnary_file, &data, rw_size);
    tmp->data = data;
    read (dictionnary_file, &c, 1);
    if (c == ';')
      break;
    tmp->next = malloc (sizeof (struct list));
    tmp = tmp->next;
    tmp->next = NULL;
  }

  int result_file = _open (result_on_file, 2);
  /* Transform each number given by MTF to the corresponding letter */
  list prev;
  while (read (fd, &data, rw_size)) {
    tmp = dict;
    for (; data != 0; data--) {
      prev = tmp;
      tmp = tmp->next;
    }
    write (result_file, &(tmp->data), rw_size);
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

void add_data_on_tree (node_t *tree, uint16_t size_read, uint64_t encode_read,
                       uint16_t word_read) {
  node_t *tmp = tree;
  for (uint16_t i = size_read; i > 0; i--) {
    /* if the ith bit of encode_read equal 0, it go on the left, else right */
    if ((encode_read & (1U << (i - 1))) == 0) {
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

node_t *create_dictionnary (char *encode_huf) {
  uint16_t data_read = 0;
  int size_data_read = sizeof (uint16_t) * BYTES_SIZE;
  unsigned i_data_read = 0;
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
  int huff_code_file = _open (encode_huf, 1);

  // Tant que l'on a des truc à lire; on lit
  while (read (huff_code_file, &data_read, sizeof (uint16_t)) > 0) {
    bool need_more_data = false;
    i_data_read = 0;
    while (!need_more_data) {
      switch (mode) {
        // recherche du word
        case 1:
          i_cpy = cpy_data (&word_read, HUFF_SIZE, nb_bits_cpy,
                            (uint64_t)data_read, size_data_read, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
          // S'il ne reste plus de bits à copier pour le word
          if (nb_bits_cpy == HUFF_SIZE) {
            nb_bits_cpy = 0;
            mode = 2;
          }
          break;

        // recherche de la size de l'encode
        case 2:
          i_cpy = cpy_data (&size_read, HUFF_SIZE, nb_bits_cpy,
                            (uint64_t)data_read, size_data_read, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
          if (nb_bits_cpy == HUFF_SIZE) {
            nb_bits_cpy = 0;
            mode = 3;
          }
          break;

        // recherche de l'encode
        case 3:
          i_cpy = cpy_data (&encode_read, (int)size_read, nb_bits_cpy,
                            (uint64_t)data_read, size_data_read, i_data_read);
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

          if (verbose) {
            printf ("%lu : ", word_read);
            print_encode (word_read, HUFF_SIZE);
            printf (" to ");
            print_encode (encode_read, size_read);
            printf ("    , depth = %d", size_read);
            printf ("\n");
          }

          mode = 1;
          word_read = word_read ^ word_read;
          size_read = size_read ^ size_read;
          encode_read = encode_read ^ encode_read;
          break;
      }
      if (i_data_read == size_data_read) {
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
  int size_huf_file = _open (SIZE_HUF, 1);
  uint64_t nb = 0;
  read (size_huf_file, &nb, sizeof (uint64_t));
  close (size_huf_file);
  return nb;
}

void decomp_huffman (node_t *tree, int file_to_decode,
                     char *result_file) {
  uint64_t nb_bits_on_file = get_nb_writing_bits ();
  if (verbose)
    printf ("\nnb_bits_on_file = %lu\n", nb_bits_on_file);
  int huff_prev_result = file_to_decode;
  /* Save the decode_huffman */
  int huff_decode_file = _open (result_file, 2);
  uint8_t data_read = 0;
  uint64_t encode_read = 0;
  int size_data_read = sizeof (data_read) * BYTES_SIZE;
  int size_encode_read = sizeof (encode_read) * BYTES_SIZE;
  unsigned i_encode_read = 0;
  while (read (huff_prev_result, &data_read, sizeof (uint8_t)) > 0) {
    cpy_data (&encode_read, size_encode_read, i_encode_read,
              data_read, size_data_read, 0);
    i_encode_read += size_data_read;
    bool find_word = true;
    while (find_word) {
      find_word = false;
      node_t *tmp = tree;
      for (unsigned i = 1; i <= i_encode_read && nb_bits_on_file != 0; i++) {
        if ((encode_read & (1UL << (size_encode_read - i))) != 0)
          tmp = tmp->right;
        else
          tmp = tmp->left;
        if (is_leaf (tmp)) {
          write(huff_decode_file, &(tmp->data), HUFF_SIZE / BYTES_SIZE);
          nb_bits_on_file -= i;
          i_encode_read -= i;
          encode_read = encode_read << i;
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

int uncompress_archive (char *file) {
  int len = strlen (file);
  /* If the file isn't a ".bw" file */
  if (file[len - 3] != '.' ||
      file[len - 2] != 'b' ||
      file[len - 1] != 'w') {
    perror ("BW: Cannot uncompress this file \n");
    exit (EXIT_FAILURE);
  }
  int fd =  _open (file, 1);

  int code_h = _open (ENCODE_HUF, 2),
    dico_enc = _open (DICTIONNARY_ENC, 2),
    size_huff= _open (SIZE_HUF, 2),
    index_bw = _open (INDEX_BW, 2);

  int files[4] = {code_h, dico_enc, index_bw, size_huff};
  int size;
  char buff[1];
  for (int i = 0; i < 4; i++) {
    if (i != 3) {
      read (fd, &size, 4);
      for (int j = 0; j < size; j++) {
	read (fd, &buff, 1);
	write (files[i], &buff, 1);
      }
    }
    /* size_huff file is just 8 bytes */
    else {
      uint64_t b;
      read (fd, &b, sizeof (uint64_t));
      write (size_huff, &b, sizeof (uint64_t));
    }
  }
  close (code_h);
  close (dico_enc);
  close (size_huff);
  close (index_bw);
  return fd;
}

int main (int argc, char **argv) {
  int optc;
  static struct option long_opts[] =
  {
    {"verbose",   no_argument,        0,    'v'},
    {"help",      no_argument,        0,    'h'},
    {NULL,        0,                  NULL, 0}
  };
  /* if (argc != 2) perror ("Usage: ubw <file>"); exit (EXIT_FAILURE); */
  int nb_optc = 0;
  while ((optc = getopt_long (argc, argv, "vh", long_opts, NULL)) != -1)
  {
    if (nb_optc == 1)
      optc = 0;
    switch(optc)
    {
      case 'v': /* Verbose output */
        verbose = true;
        nb_optc++;
        break;

      case 'h': /* Display this help */
        usage(EXIT_SUCCESS, argv[0]);
        break;

      default:
        usage(EXIT_FAILURE, argv[0]);
    }
  }
  switch (argc) {
    case 4:
      BLOCK_SIZE = atoi (argv[3]);
      break;
    case 3:
      BLOCK_SIZE = atoi (argv[2]);
      break;
  default:
    break;
  }
  /* Retrieve files in the archive */
  int file_fd = uncompress_archive (argv[1]);
  
  node_t *dictionnary = create_dictionnary (ENCODE_HUF);
  decomp_huffman (dictionnary, file_fd, INV_HUFF);
  delete_dictionnary (dictionnary);
  close (file_fd);
  undo_mtf(INV_HUFF, DICTIONNARY_ENC, RETURN_UMTF);
  undo_bw (RETURN_UMTF);

  /* Removing created files */
  unlink (ENCODE_HUF);
  unlink (DICTIONNARY_ENC);
  unlink (SIZE_HUF);
  unlink (INDEX_BW);
  unlink (INV_HUFF);
  unlink (RETURN_UMTF);
  unlink (argv[1]);
  return EXIT_SUCCESS;
}
