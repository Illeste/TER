//
//  UNBWT.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//
// DESCRIPTION
// -----------
//
//  This program performs an inverse Burrows-Wheeler transform on an input
//  file or stream, and sends the result to an output file or stream.
//
//  While this program can be compiled in 16 bit mode, it might not
//  be able to invert transforms created by a 32 bit program.  The
//  16 bit program has to drop its block size quite a bit.  However,
//  rewriting the code to use a huge buffer under MS-DOS might solve
//  that problem.
//
//  This program takes two arguments: an input file and an output
//  file.  You can leave off one argument and send your output to
//  stdout.  Leave off two arguments and read your input from stdin
//  as well.  You can also specify "-d" as the first argument to get
//  a debug dump as well.  
//
//  The UNBWT input consists of a series of blocks that look like this:
//
//  long byte_count |  ...data... | long first | long last
//
//  The byte_count refers to the number of data bytes.  The data
//  itself is the "L" column from the sorted data.  "first" is the
//  index where the first character from the buffer appears in the
//  sorted output.  "last" is where the end-of-buffer special byte
//  appears in the output buffer.  These blocks are repeated until
//  an EOF indicates that everything is done.
//
//  This program accompanies my article "Data Compression with the
//  Burrows-Wheeler Transform."  There is one major deviation from
//  the text of the article in this implementation.  The BWT code
//  creates a buffer of N+1 characters out of an input string of
//  N charactes.  The last character is a special end-of-buffer
//  character that makes sorting the input easier.  See BWT.CPP for
//  details on how and why this works.
//
//  The end of buffer character has special implications for UNBWT.CPP,
//  because we have to take note of its appearance in the data stream.
//  The "last" index sent after the block indicates whhich position in
//  the transformed buffer contains the virtual end-of-buffer character.
//  All of the calculations I do in the program have to take into account
//  the possible appearance of this virtual character in the input
//  stream.
//
// Build Instructions
// ------------------
//
//  Define the constant unix for UNIX or UNIX-like systems.  The
//  use of this constant turns off the code used to force the MS-DOS
//  file system into binary mode.  g++ already does this, your UNIX
//  C++ compiler might also.
//
//  Borland C++ 4.5 16 bit    : bcc -w -ml unbwt.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w unbwt.cpp
//  Microsoft Visual C++ 1.5  : cl /W4 /AL unbwt.cpp
//  Microsoft Visual C++ 4.0  : cl /W4 unbwt.cpp
//  g++                       : g++ -o unbwt unbwt.cpp
//
// Typical Use
// -----------
//
//  unari < compressed-file | unrle | unmtf | unbwt | unrle > raw-file
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif
#include <string.h>

#if ( INT_MAX == 32767 )
#define BLOCK_SIZE 20000
#else
#define BLOCK_SIZE 200000
#endif

unsigned char buffer[ BLOCK_SIZE + 1 ];
unsigned int T[ BLOCK_SIZE + 1 ];
long buflen;
unsigned int Count[ 257 ];
unsigned int RunningTotal[ 257 ];

main( int argc, char *argv[] )
{
    int debug = 0;
    if ( argc > 1 && strcmp( argv[ 1 ], "-d" ) == 0 ) {
        debug = 1;
        argv++;
        argc--;
    }
    fprintf( stderr, "Performing Inverse BWT on " );
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

    for ( ; ; ) {
        if ( fread( (char *) &buflen, sizeof( long ), 1, stdin ) == 0 )
            break;
        fprintf( stderr,
                 "Processing %ld bytes\n",
                 buflen );
        if ( buflen > ( BLOCK_SIZE + 1 ) ) {
            fprintf( stderr, "Buffer overflow!\n" );
            abort();
        }
        if ( fread( (char *)buffer, 1, (size_t) buflen, stdin ) != 
                 (size_t) buflen ) {
            fprintf( stderr, "Error reading data\n" );
            abort();
        }
        unsigned long first;
        fread( (char *) &first, sizeof( long ), 1, stdin );
        unsigned long last;
        fread( (char *)&last, sizeof( long ), 1, stdin );
        fprintf( stderr,
                 "first = %lu, "
                 "last = %lu\n",
                 first, last );
//
// To determine a character's position in the output string given
// its position in the input string, we can use the knowledge about
// the fact that the output string is sorted.  Each character 'c' will
// show up in the output stream in in position i, where i is the sum
// total of all characters in the input buffer that precede c in the
// alphabet, plus the count of all occurences of 'c' previously in the
// input stream.
//
// The first part of this code calculates the running totals for all
// the characters in the alphabet.  That satisfies the first part of the
// equation needed to determine where each 'c' will go in the output
// stream.  Remember that the character pointed to by 'last' is a special
// end-of-buffer character that is supposed to be larger than any char
// in the alphabet.
//
        unsigned int i;
        for ( i = 0 ; i < 257 ; i++ )
            Count[ i ] = 0;
        for ( i = 0 ; i < (unsigned int) buflen ; i++ )
            if ( i == last )
                Count[ 256 ]++;
            else
                Count[ buffer[ i ] ]++;
        if ( debug ) {
            fprintf( stderr, "Byte Counts:\n" );
            for ( i = 0 ; i <= 256 ; i++ )
                if ( Count[ i ] ) {
                    if (isprint( i ) )
                        fprintf( stderr, "%c  : ", i );
                    else
                        fprintf( stderr, "%3d: ", i );
                    fprintf( stderr, "%u\n", Count[ i ] );
                }
        }
        int sum = 0;
        if ( debug )
            fprintf( stderr, "Running totals:\n" );
        for ( i = 0 ; i < 257 ; i++ ) {
            RunningTotal[ i ] = sum;
            sum += Count[ i ];
            if ( debug && Count[ i ] ) {
                if (isprint( i ) )
                    fprintf( stderr, "%c  : ", i );
                else
                    fprintf( stderr, "%3d: ", i );
                fprintf( stderr, "%u\n", (long) RunningTotal[ i ] );
            }
            Count[ i ] = 0;
        }
//
// Now that the RunningTotal[] array is filled in, I have half the
// information needed to position each 'c' in the input buffer.  The
// next piece of information is simply the number of characters 'c'
// that appear before this 'c' in the input stream.  I keep track of
// that informatin in the Counts[] array as I go.  By adding those
// two number together, I get the destination of each character in
// the input buffer, and that allows me to fill in all the positions
// in the transformation vector, T[].
//
        for ( i = 0 ; i < (unsigned int) buflen ; i++ ) {
            int index;
            if ( i == last )
                index = 256;
            else
                index = buffer[ i ];
            T[ Count[ index ] + RunningTotal[ index ] ] = i;
            Count[ index ]++;
        }
        if ( debug ) {
            fprintf( stderr, "T = " );
            for ( i = 0 ; i < (unsigned int) buflen ; i++ )
                fprintf( stderr, "%u ", T[ i ] );
            fprintf( stderr, "\n" );
        }
//
// Once the transformation vector is in place, writing the
// output is just a matter of following the indices.  Note
// that I don't have to watch for the end-of-buffer character
// here.  Since I am only outputting N characters, and I know
// it will show up at position N+1, I know it won't pop up
//
        unsigned int j;
        i = (unsigned int) first;
        for ( j = 0 ; j < (unsigned int) ( buflen - 1 ) ; j++ ) {
            putc( buffer[ i ], stdout );
            i = T[ i ];
        }
    }
    return 1;
}

