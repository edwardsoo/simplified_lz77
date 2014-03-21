#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "prefix_tree.h"
#include "queue.h"

#define WHERE() printf("%u\n", __LINE__)

void test_compress_file (FILE *in, FILE *out) {
  bit_out_stream_t *out_stream;
  out_stream = bit_out_stream_new (out);

  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'm');
  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'a');
  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'h');
  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'i');
  write_1bit (out_stream, 0);
  write_8bits (out_stream, ' ');
  write_1bit (out_stream, 1);
  write_12bits (out_stream, 4);
  write_4bits (out_stream, 4);

  bit_out_stream_destroy (&out_stream);
  fclose (in);
}

void test_compress_file_2 (FILE *in, FILE *out) {
  bit_out_stream_t *out_stream;
  out_stream = bit_out_stream_new (out);

  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'a');
  write_1bit (out_stream, 0);
  write_8bits (out_stream, 'b');
  write_1bit (out_stream, 1);
  write_12bits (out_stream, 1);
  write_4bits (out_stream, 0xF);

  bit_out_stream_destroy (&out_stream);
  fclose (in);
}

void print_tree_inorder (prefix_tree_t *tree, int depth) {
  int i;

  if (tree) {
    printf ("%*sprefix = [", depth, "");
    for (i = 0; i < tree->prefix_len; i++) {
      printf ("0x%x ", tree->prefix[i]);
    }
    if (tree->has_key) {
      printf("] key = %ld\n", tree->key);
    } else {
      printf("] key = NULL\n");
    }
    for (i = 0; i < 0x100; i++) {
      print_tree_inorder (tree->child[i], depth + 1);
    }
  }
}

void test_prefix_tree_1 () {
  prefix_tree_t *tree = prefix_tree_new (NULL, 0, 0, 0);
  prefix_tree_insert (&tree, (uint8_t*) "ab", 2, 1);
  prefix_tree_insert (&tree, (uint8_t*) "abc", 3, 2);
  print_tree_inorder (tree, 0);
  prefix_tree_destroy (&tree);
}

void test_prefix_tree_2 () {
  prefix_tree_t *tree = prefix_tree_new (NULL, 0, 0, 0);
  prefix_tree_insert (&tree, (uint8_t*) "abc", 3, 1);
  prefix_tree_insert (&tree, (uint8_t*) "ab", 2, 2);
  print_tree_inorder (tree, 0);
  prefix_tree_destroy (&tree);
}

void test_prefix_tree_3 () {
  prefix_tree_t *tree = prefix_tree_new (NULL, 0, 0, 0);
  prefix_tree_insert (&tree, (uint8_t*) "abc", 3, 1);
  prefix_tree_insert (&tree, (uint8_t*) "abc", 3, 2);
  print_tree_inorder (tree, 0);
  prefix_tree_destroy (&tree);
}

void test_prefix_tree_4 () {
  prefix_tree_t *tree = prefix_tree_new (NULL, 0, 0, 0);
  prefix_tree_insert (&tree, (uint8_t*) "abc", 3, 1);
  prefix_tree_insert (&tree, (uint8_t*) "abd", 3, 2);
  print_tree_inorder (tree, 0);
  prefix_tree_destroy (&tree);
}

void print_uint4_bits (uint8_t value) {
  int mask = 0x8;
  int i;
  for (i = 0; i < 4; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_uint8_bits (uint8_t value) {
  int mask = 0x80;
  int i;
  for (i = 0; i < 8; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_uint12_bits (uint16_t value) {
  int mask = 0x800;
  int i;
  for (i = 0; i < 12; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_bytes(FILE *in) {
  int c;
  fseek (in, 0, SEEK_SET);
  printf("file in bytes\n");
  while ((c = fgetc (in)) != EOF) {
    print_uint8_bits (c);
  }
  printf("\n");
}

void print_bits(FILE *in) {
  uint8_t value;
  int i;
  fseek (in, 0, SEEK_SET);
  bit_in_stream_t* stream = bit_in_stream_new (in);
  for (i = 1; read_1bit (stream, &value) == 0; i++) {
    printf("%u", !!(0x1 & value));
    if (i % 8 == 0) {
      printf("\n");
    }
  }
}

void print_4bits(FILE *in) {
  uint8_t value;
  fseek (in, 0, SEEK_SET);
  bit_in_stream_t* stream = bit_in_stream_new (in);
  while (read_4bits (stream, &value) == 0) {
    print_uint4_bits (value);
  }
}

void print_12bits(FILE *in) {
  uint16_t value;
  fseek (in, 0, SEEK_SET);
  bit_in_stream_t* stream = bit_in_stream_new (in);
  printf ("bit stream: file_size %lu\n", stream->file_size);
  while (read_12bits (stream, &value) == 0) {
    print_uint12_bits (value);
  }
}

int main (int argc, char* argv[]) {
  test_prefix_tree_1 ();
  test_prefix_tree_2 ();
  test_prefix_tree_3 ();
  test_prefix_tree_4 ();
  return 0;
}
