//
//  BWT.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//
// DESCRIPTION
// -----------
//
//  This program performs a Burrows-Wheeler transform on an input
//  file or stream, and sends the result to an output file or stream.
//
//  While this program can be compiled in 16 bit mode, it will suffer
//  greatly by virtue of the fact that it will need to drop its
//  block size tremendously.
//
//  This program takes two arguments: an input file and an output
//  file.  You can leave off one argument and send your output to
//  stdout.  Leave off two arguments and read your input from stdin
//  as well.
//
//  The output consists of a series of blocks that look like this:
//
//  long byte_count |  ...data... | long first | long last
//
//  The byte_count refers to the number of data bytes.  The data
//  itself is the "L" column from the sorted data.  "first" is the
//  index where the first character from the buffer appears in the
//  sorted output.  "last" is where the end-of-buffer special byte
//  appears in the output buffer.  These blocks are repeated until
//  I'm out of data.
//
//  This program accompanies my article "Data Compression with the
//  Burrows-Wheeler Transform."  There is one major deviation from
//  the text of the article in this implementation.  To simplify the
//  sorting, I append a special end-of-buffer character to the end
//  of the input buffer.  The end-of-buffer character isn't found
//  in the buffer, which means I no longer have to wrap around to
//  the start of the buffer when performing comparisons.  Instead,
//  I'm guaranteed that a memcmp() will terminate at or before the
//  last character in the buffer.
//
//  One problem, though.  Since I can handle any kind of binary input,
//  what character is guaranteed to never appear in the buffer?  None,
//  so instead I do a special hack and make sure I never *really*
//  look at that last position when comparing.  Instead, I only compare
//  until one or the other string gets to the end, then award the
//  comparison to whoever hit the end first.
//
//  This special character means the output has N+1 characters.  I just
//  output a '?' when I hit that special end-of-buffer character, but
//  I also have to pass along the information about the end-of-buffer
//  character's position to the decoder, so I append it to the end
//  of each data block.
//
//  The sorting for this routine is done via conventional qsort().
//  The STL capable version in BWT.CPP is neater, and in theory
//  should run faster.  The comparison function here is nearly
//  the same as that from BWT.CPP, but it isn't a template fn.  Also,
//  instead of passing pointers into the buffer, the qsort() compare
//  function gets indices.
//
// Build Instructions
// ------------------
//
//  Define the constant unix for UNIX or UNIX-like systems.  The
//  use of this constant turns off the code used to force the MS-DOS
//  file system into binary mode.  g++ already does this, your UNIX
//  C++ compiler might also.
//
//  Microsoft Visual C++ 2.x  : cl /W4 bwta.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w bwt.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 bwt.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 bwt.cpp
//  g++                       : g++ -o bwt bwt.cpp
//
// Typical Use
// -----------
//
//  rle < raw-file | bwt | mtf | rle | ari > compressed-file
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif
#include <limits.h>

#if ( INT_MAX == 32767 )
#define BLOCK_SIZE 20000
#else
#define BLOCK_SIZE 200000
#endif

//
// length has the number of bytes presently read into the buffer,
// buffer contains the data itself.  indices[] is an array of
// indices into the buffer.  indices[] is what gets sorted in
// order to sort all the strings in the buffer.
//
long length;
unsigned char buffer[ BLOCK_SIZE ];
int indices[ BLOCK_SIZE + 1 ];

//
// The logic in unbwt.cpp depends upon the strings having been
// sorted as unsigned characters.  Some versions of memcmp() sort
// using *signed* characters.  When this is the case, I compare
// using this special replacement version of memcmp().
//
int memcmp_signed;

int unsigned_memcmp( void *p1, void *p2, unsigned int i )
{
    unsigned char *pc1 = (unsigned char *) p1;
    unsigned char *pc2 = (unsigned char *) p2;
    while ( i-- ) {
        if ( *pc1 < *pc2 )
            return -1;
        else if ( *pc1++ > *pc2++ )
            return 1;
    }
    return 0;
}

//
// This is the special comparison function used when calling
// qsort() to sort the array of indices into the buffer. Remember that
// the character at buffer+length doesn't really exist, but it is assumed
// to be the special end-of-buffer character, which is bigger than
// any character found in the input buffer.  So I terminate the
// comparison at the end of the buffer.
//

int
#if defined( _MSC_VER )
_cdecl
#endif
bounded_compare( const unsigned int *i1,
                 const unsigned int *i2 )
{
    static int ticker = 0;
    if ( ( ticker++ % 4096 ) == 0 )
        fprintf( stderr, "." );
    unsigned int l1 = (unsigned int) ( length - *i1 );
    unsigned int l2 = (unsigned int) ( length - *i2 );
    int result;
    if ( memcmp_signed )
        result = unsigned_memcmp( buffer + *i1,
                                  buffer + *i2,
                                  l1 < l2 ? l1 : l2 );
    else
        result = memcmp( buffer + *i1,
                         buffer + *i2,
                         l1 < l2 ? l1 : l2 );
    if ( result == 0 )
        return l2 - l1;
    else
        return result;
};


main( int argc, char *argv[] )
{
    int debug = 0;
    if ( argc > 1 && strcmp( argv[ 1 ], "-d" ) == 0 ) {
        debug = 1;
        argv++;
        argc--;
    }
    fprintf( stderr, "Performing BWT on " );
    if ( argc > 1 ) {
        freopen( argv[ 1 ], "rb", stdin );
        fprintf( stderr, "%s", argv[ 1 ] );
    } else
        fprintf( stderr, "stdin" );
    fprintf( stderr, " to " );
    if ( argc > 2 ) {
        freopen( argv[ 2 ], "wb", stdout );
        fprintf( stderr, "%s", argv[ 2 ] );
    } else
        fprintf( stderr, "stdout" );
    fprintf( stderr, "\n" );
#if !defined( unix )
    setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
#endif
    if ( memcmp( "\x070", "\x080", 1 ) < 0 ) {
        memcmp_signed = 0;
        fprintf( stderr, "memcmp() treats character data as unsigned\n" );
    } else {
        memcmp_signed = 1;
        fprintf( stderr, "memcmp() treats character data as signed\n" );
    }
//
// This is the start of the giant outer loop.  Each pass
// through the loop compresses up to BLOCK_SIZE characters.
// When an fread() operation finally reads in 0 characters,
// we break out of the loop and are done.
//
    for ( ; ; ) {
//
// After reading in the data into the buffer, I do some
// UI stuff, then write the length out to the output
// stream.
//
        length = fread( (char *) buffer, 1, BLOCK_SIZE, stdin );
        if ( length == 0 )
            break;
        fprintf( stderr, "Performing BWT on %ld bytes\n", length );
        long l = length + 1;
        fwrite( (char *) &l, 1, sizeof( long ), stdout );
//
// Sorting the input strings is simply a matter of inserting
// the indices into the array, then calling qsort() with the
// special bounded_compare() function I wrote to work with
// these string types.  Note that I sort N+1 indices. The last index
// points one past the end of the buffer, which is where the
// imaginary end-of-buffer character resides.  Sort of.
//
        int i;
        for ( i = 0 ; i <= length ; i++ )
            indices[ i ] = i;
        qsort( indices,
               (int)( length + 1 ),
               sizeof( int ),
               ( int (*)(const void *, const void *) ) bounded_compare );
        fprintf( stderr, "\n" );
//
// If the debug flag was turned on, I print out the sorted
// strings, along with their prefix characters.  This is
// not a very good idea when you are compressing a giant
// binary file, but it can be real helpful when debugging.
//
        if ( debug ) {
            for ( i = 0 ; i <= length ; i++ ) {
                fprintf( stderr, "%d : ", i );
                unsigned char prefix;
                if ( indices[ i ] == 0 )
                    prefix = '?';
                else
                    prefix = (unsigned char) buffer[ indices[ i ] - 1 ];
                if ( isprint( prefix ) )
                    fprintf( stderr, "%c", prefix );
                else
                    fprintf( stderr, "<%d>", prefix );
                fprintf( stderr, ": " );
                int stop = (int)( length - indices[ i ] );
                if ( stop > 30 )
                    stop = 30;
                for ( int j = 0 ; j < stop ; j++ ) {
                    if ( isprint( buffer[ indices[ i ] + j ] ) )
                        fprintf( stderr, "%c", buffer[ indices[ i ] + j ] );
                    else
                        fprintf( stderr, "<%d>", buffer[ indices[ i ] + j ] );
                }
                fprintf( stderr, "\n" );
            }
        }
//
// Finally, I write out column L.  Column L consists of all
// the prefix characters to the sorted strings, in order.
// It's easy to get the prefix character, but I have to
// handle S0 with care, since its prefix character is the
// imaginary end-of-buffer character.  I also have to spot
// the positions in L of the end-of-buffer character and
// the first character, so I can write them out at the end
// for transmission to the output stream.
//
        long first;
        long last;
        for ( i = 0 ; i <= length ; i++ ) {
            if ( indices[ i ] == 1 )
                first = i;
            if ( indices[ i ] == 0 ) {
                last = i;
                fputc( '?', stdout );
            } else
                fputc( buffer[ indices[ i ] - 1 ], stdout );
        }
        fprintf( stderr,
                 "first = %ld"
                 "  last = %ld\n",
                 first,
                 last );
        fwrite( (char *) &first, 1, sizeof( long ), stdout );
        fwrite( (char *) &last, 1, sizeof( long ), stdout );
    }
    return 0;
}

