Algorithme de Compression

3 parties :

1/ Transformation de Burrows Wheeler

2/ Codage Differentiel (Delta Encoding)

3/ Codage de Huffman


Problèmes :
  - Le BW a un problème lorsque les charactères sont spéciaux, la lecture du
  charactère est eronnée. 

  - Problème lors du free des strings. surement dus à la non/mauvaise utilisation du strcpy
