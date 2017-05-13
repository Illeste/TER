Algorithme de Compression

3 parties :

1/ Transformation de Burrows Wheeler

2/ Codage Differentiel (Delta Encoding)

3/ Codage de Huffman

Fichiers générés par MAKEFILE :

bw : pour compression de fichier
ubw : pour décompression


Fichiers utilitaires : 

transform_file.c : pour copier un fichier de test en lui changeant son "letter_size" (recopiage des caractères de 8bits en bloc de 4bits)
script_test.sh : script executable pour réaliser quelques tests
