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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

/* Size of each block passed through Burrows Wheeler */
/* !!!!! Faire gaffe si on depasse la taille d'un uint (8 ou 16) pour index */
#define BLOCK_SIZE 20
/* Size of data scanned (2 bytes because we use uint16_t) */
#define DATA_SIZE 2
/* Alphabet used in Huffman's Algorithm has for size 2**LETTER_SIZE */
#define LETTER_SIZE 8
#define BYTES_SIZE 8
/* Should be 2**LETTER_SIZE */
#define ALPHABET_SIZE 256
/* Amount of data scanned */
#define DATA_READ 50000
/* Name of in betwwen files for storage */
#define RETURN_BW           "result_bw"
#define INDEX_BW            ".index_bw"
#define RETURN_ENC          "result_encode"
#define DICTIONNARY_ENC     ".dico_enc"
#define RETURN_HUF          "result_huffman"
#define ENCODE_HUF          ".code_huff"
/* DEBUG: Fichier ou se trouve le resultat bw inverse */
#define INV_BW              ".bw_toto"

#define INV_HUFF            "inv_huff"
#define SIZE_HUF            ".size_huff"

/* Huffman Structs */
typedef struct node {
  uint16_t data;
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


void print_array (uint16_t a) {
  int i;
  for (i = 16 - 1; i >= 0; i--) {
//    if (i == 11 || i == 7 || i == 3)
//      printf(" ");
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  }
//  printf(" ");
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

void print_sort_tab (uint16_t **tab, unsigned block_size) {
  unsigned i, j;
  for (i = 0; i < block_size; i++) {
    for (j = 0; j < block_size; j++) {
      //      printf("%c", tab[i][j]);
      print_array (tab[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}


/* DECOMPRESSION BW */
int compare (const void *a, const void *b, unsigned block_size) {
  uint16_t *_a = (uint16_t *) a;
  uint16_t *_b = (uint16_t *) b;
  unsigned i;

  /* Little endian storage */
  for (i = 0; i < block_size; i++) {
    if (_a[i] != _b[i]) {
      return _a[i] - _b[i];
    }
  }
  return 0;
}

void merge (uint16_t **tab, uint16_t **tab2,
            unsigned tab_size1, unsigned tab_size2, unsigned block_size) {
  uint16_t *tmp1[tab_size1];
  uint16_t *tmp2[tab_size2];
  unsigned i;
  for (i = 0; i < tab_size1; i++) {
    tmp1[i] = tab[i];
  }
  for (i = 0; i < tab_size2; i++) {
    tmp2[i] = tab2[i];
  }
  unsigned i_tab2 = 0;
  unsigned i_tab1 = 0;
  unsigned c = 0;
  while (i_tab1 < tab_size1 && i_tab2 < tab_size2) {
    if (compare(tmp1[i_tab1], tmp2[i_tab2], block_size) > 0) {
      tab[c] = tmp2[i_tab2];
      c++;
      i_tab2++;
    }
    else {
      tab[c] = tmp1[i_tab1];
      c++;
      i_tab1++;
    }
  }
  if (i_tab1 == tab_size1)
    for (i = i_tab2; i < tab_size2; i++) {
      tab[c] = tmp2[i];
      c++;
    }
  else
    for (i = i_tab1; i < tab_size1; i++) {
      tab[c] = tmp1[i];
      c++;
    }
}

void merge_sort (uint16_t **tab, unsigned tab_size, unsigned block_size) {
  if (tab_size > 1){
    unsigned tab_size1 = tab_size / 2;
    unsigned tab_size2 = tab_size - tab_size1;
    merge_sort (tab, tab_size1, block_size);
    merge_sort (tab + tab_size1, tab_size2, block_size);
    merge (tab, tab + tab_size1, tab_size1, tab_size2, block_size);
  }
}

void reverse_bw (uint16_t *array, uint16_t index, int n) {
  uint16_t **strings =  malloc (n * sizeof (*strings));
    int i, j;
    for (i = 0; i < n; i++) {
      strings[i] = malloc (n * sizeof(uint16_t));
      for (j = 0; j < n ; j++)
        strings[i][j] = 0;
    }
    /* Take the string, add a letter from array and then sort them */
    for (i = n - 1; i >= 0 ; i--) {
      for (j = 0; j < n; j++)
        strings[j][i] = array[j];
      // DEBUG
      /* printf ("Avant \n"); */
      /* print_sort_tab (strings, n); */

      /////////////////
      //
      // Erreur merge_sort, mettre les print
      // Le sort part de gauche a droite, l'algo part de droite
      // a gauche, donc le sort compare "0" à "0" et pas ce que je
      // veux.
      // Au contraire si je place de gauche a droite, sort bien
      // mais pas la bonne chose, essaye de voir si tu peux
      //
      /////////////////

      merge_sort (strings, n, n);
      /* printf ("Apres \n"); */
      /* print_sort_tab (strings, n); */
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
void undo_bw (char *file, char *index_file) {
  int file_fd = open (file, O_RDONLY),
    index_fd = open(index_file, O_RDONLY),
    return_ubw = open ("toto", O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file_fd == -1 || index_fd == -1) {
    fprintf (stderr, "BW: couldn't open file\n");
    exit (EXIT_FAILURE);
  }

  /* Retrieving byte size and the block size */
  int byte_read = 0, block_size = 0;
  read (index_fd, &byte_read, 1);
  read (index_fd, &block_size, 2);
  uint16_t *array = malloc (sizeof (uint16_t) * (block_size));
  int data_read, data;
  uint16_t index = 0;
  bool flag = false;
  while (!flag) {
    data_read = 0;
    //   data_read = read (file_fd, array, byte_read * block_size);
    for (int k = 0; k < block_size && !flag; k++) {
      data = read (file_fd, array + k, byte_read);
      data_read += data;
      if (data < 1)
        flag = true;
    }
    read (index_fd, &index, byte_read);
    /* Applying the opposite of Burrows Wheeler to each block */
    reverse_bw (array, index, data_read);
    int a;
    for (a = 0; a < data_read; a++)
      write (return_ubw, array + a, byte_read);
    ////////////////////////////////
    //
    // A voir, si ecriture dans un fichier,
    // ou on écrit sur le fichier d'entrée
    // pour éviter de bourrer
    //
    ////////////////////////////////
  }

  free (array);
  close (file_fd);
  close (index_fd);
}

/* DECOMPRESSION MTF */



/* DECOMPRESSSION HUFFMAN */

int cpy_data (uint64_t *array, int size_of_array, int nbaw_array,
              uint64_t data, int size_of_data, int nbaw_data) {
  int i;
  if (size_of_data > 8)
    printf("");
  int cpy_bits = 0;
  for (i = size_of_data - nbaw_data - 1;
      i >= 0 && (nbaw_array + cpy_bits) != size_of_array; i--) {
    if ((data & (1 << i)) != 0)
      *array = *array | (1 << (size_of_array - (nbaw_array + cpy_bits) - 1));
    cpy_bits++;
  }
  return (cpy_bits);
}

void print_encode (uint16_t a, int size) {
  int i;
  for (i = size - 1; i >= 0; i--)
    (a & (1 << i)) == 0 ?printf("0"): printf("1");
  printf(" ");
}

node_t *create_node () {
  node_t *node = malloc (sizeof (node_t));
  node->left = NULL;
  node->right = NULL;
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

node_t *create_dictionnary () {
  uint16_t data_read;
  unsigned i_data_read;
  data_read = data_read ^ data_read;

  /* Use to explain what we search */
  unsigned mode = 1;

  // nb de bits déjà copier
  int nb_bits_cpy = 0;

  /* Use to take the word */
  uint16_t word_read = 0;

  /* Use to take the size of encode */
  uint8_t size_read = 0;

  /* Use to take the size of encode */
  uint64_t encode_read = 0;

  unsigned i_cpy = 0;

  /* Create a tree to store the dictionnary of data and encode */
  node_t *tree = create_node();

  /* Save the encode_huffman */
  int huff_code_file = open (ENCODE_HUF, O_RDONLY);
  if (huff_code_file == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open to file of encode rules\n");
    exit (EXIT_FAILURE);
  }
  // Tant que l'on a des truc à lire; on lit
  while (read (huff_code_file, &data_read, sizeof (uint16_t)) > 0) {
/*//    printf("read : ");
    print_array(data_read);
//    printf("\n");
  }{*/
    bool need_more_data = false;
    i_data_read = 0;
    while (!need_more_data) {
      switch (mode) {
        // recherche du word
        case 1:
          i_cpy = cpy_data (&word_read, 8, nb_bits_cpy,
                            data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
//          printf("!!!!!!!MOD 1!!!!!!!\ni_cpy = %d\nnb bits cpy = %d\ndata read = %d\n", i_cpy, nb_bits_cpy, i_data_read);
          // S'il ne reste plus de bits à copier pour le word
          if (nb_bits_cpy == 8) {
            nb_bits_cpy = 0;
            mode = 2;
          }
          break;

        // recherche de la size de l'encode
        case 2:
          i_cpy = cpy_data (&size_read, 8, nb_bits_cpy,
                            data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
//          printf("!!!!!!!MOD 2!!!!!!!\ni_cpy = %d\nnb bits cpy = %d\ndata read = %d\n", i_cpy, nb_bits_cpy, i_data_read);
          if (nb_bits_cpy == 8) {
            nb_bits_cpy = 0;
            mode = 3;
          }
          break;

        // recherche de l'encode
        case 3:
          i_cpy = cpy_data (&encode_read, (int)size_read, nb_bits_cpy,
                            data_read, 16, i_data_read);
          nb_bits_cpy += i_cpy;
          i_data_read += i_cpy;
//          printf("!!!!!!!MOD 3!!!!!!!\ni_cpy = %d\nnb bits cpy = %d\ndata read = %d\n", i_cpy, nb_bits_cpy, i_data_read);
          if (nb_bits_cpy == size_read) {
            nb_bits_cpy = 0;
            mode = 4;
          }
          break;

        /* Add all data in dictionnary */
        case 4:
          add_data_on_tree (tree, size_read, encode_read, word_read);
/*
          printf("\nword read :");
          print_array(word_read);
          printf("\nsize read :");
          print_array(size_read);
          printf("\nencode read :");
          print_array(encode_read);
          printf("\n");
*/

          printf ("%u : ", word_read);
          print_encode (word_read, 8);
          printf (" to ");
          print_encode (encode_read, size_read);
          printf ("    , depth = %d", size_read);
          printf ("\n");

/*
          add_on_list (dictionnary, new_data, size_read);
*/          mode = 1;
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


int cpy_data2 (uint64_t *array, int size_of_array, int nbaw_array,
              uint64_t data, int size_of_data, int nbaw_data) {
  int i;
  int cpy_bits = 0;
  for (i = size_of_data - nbaw_data - 1;
      i >= 0 && (nbaw_array + cpy_bits) != size_of_array; i--) {
    if ((data & (1ULL << i)) != 0)
      *array = (*array << 1) + 1;
    else
      *array = *array << 1;

//    if ((data & (1ULL << i)) != 0)
//      *array = *array | (1ULL << (size_of_array - (nbaw_array + cpy_bits) - 1));
//    else
//      *array = *array ^ (1ULL << (size_of_array - (nbaw_array + cpy_bits) - 1));

    cpy_bits++;
  }
  return (cpy_bits);
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

void decomp_huffman (node_t *tree) {
  uint64_t nb_bits_on_file = get_nb_writing_bits ();
  printf("nb_bits_on_file = %llu\n", nb_bits_on_file);
  int huff_prev_result = open (RETURN_HUF, O_RDONLY);
  if (huff_prev_result == -1) {
    fprintf (stderr, "Décomp_Huffman: couldn't open file of previous result\n");
    exit (EXIT_FAILURE);
  }
  /* Save the decode_huffman */
  int huff_decode_file = open (INV_HUFF, O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
    cpy_data2 (&encode_read, 64, i_encode_read, data_read, 8, 0);
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

int main (int arc, char **argv) {
  /* Ouverture fichier compressé
   * + Huffman a l'eners : récupère le fichier post mft
   * donc mtf dans le meme sens, copier coller fonction
   * bw decompression
   *
   * Retourner, ecrire dans meme fichier/ailleurs decompression
   */
//  undo_bw (RETURN_BW, INDEX_BW);
  node_t *dictionnary = create_dictionnary ();
  decomp_huffman (dictionnary);
  delete_dictionnary (dictionnary);
  return EXIT_SUCCESS;
}
