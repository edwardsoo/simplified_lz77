#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char* argv[]) {
  char c;
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

  in = fopen (input_filename, "r");
  if (!in) {
    printf ("Failed to open file %s\n", input_filename);
    perror ("fopen");
    return 1;
  }

  out = fopen (argv[optind], "w");
  if (!out) {
    printf("Failed to open file %s\n", argv[optind]);
    perror ("fopen");
    return 1;
  }

  fclose (in);
  fclose (out);
  return 0;
}
