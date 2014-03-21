#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "queue.h"
#include "prefix_tree.h"

int main (int argc, char* argv[]) {
  int c;
  int compress;
  char* input_filename;
  FILE *in, *out;

  compress = -1;
  opterr = 0;
  while ((c = getopt (argc, argv, "c:d:")) != -1) {
    switch (c) {
      case 'c':
        input_filename = optarg;
        compress = 1;
        break;
      case 'd':
        input_filename = optarg;
        compress = 0;
        break;
      default:
        printf ("Usage:\n%s -d FILE OUTPUT to decompress FILE\n"
            "%s -c FILE OUTPUT to compress FILE\n", argv[0], argv[0]);
        return 1;
    }
  }

  if (compress == -1 || optind >= argc) {
    printf ("Usage:\n%s -d FILE OUTPUT to decompress FILE\n"
        "%s -c FILE OUTPUT to compress FILE\n", argv[0], argv[0]);
    return 1;
  }

  in = fopen (input_filename, "rb");
  if (!in) {
    printf ("Failed to open file %s\n", input_filename);
    perror ("fopen");
    return 1;
  }

  out = fopen (argv[optind], "wb");
  if (!out) {
    printf("Failed to open file %s\n", argv[optind]);
    perror ("fopen");
    return 1;
  }

  if (!compress) {
    decompress_file (in, out);
  } else {
    compress_file (in, out);
  }

  return 0;
}
