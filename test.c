#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "queue.h"
#include "hash.h"

#ifndef __WHERE__
#define __WHERE__
#define WHERE() printf("%u\n", __LINE__)
#endif

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

void print_key_bytes (uint8_t *key, int len) {
  int i;
  for (i = 0; i < len; i++) {
    printf ("0x%x ", key[i]);
  }
}

void print_key_chars (uint8_t *key, int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (key[i] != '\n') printf ("%c", key[i]);
    else printf ("\\n");
  }
}

void print_hash_table (hash_t *hash) {
  int i;
  list_t* list;

  for (i = 0; i < hash->size; i++) {
    if (!hash->array[i]) continue;
    printf ("Entry[%4d] => ", i);
    list = hash->array[i];
    while (list) {
      printf ("[");
      print_key_chars (list->key, list->key_len);
      printf ("]:%ld => ", list->value);
      list = list->next;
    }
    printf ("\n");
  }
  printf ("\n");
}

void test_hash_insert () {
  hash_t *hash = hash_new (0x1000);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);
  hash_destroy (&hash);
}

void test_hash_insert_2 () {
  hash_t *hash = hash_new (0x1);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);
  hash_destroy (&hash);
}

int test_value_leq (uint64_t value, uint64_t arg) {
  return (value <= arg);
}

void test_hash_delete () {
  hash_t *hash = hash_new (0x1000);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);

  hash_delete (hash, (uint8_t*) "abcd", 4, NULL, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "bcde", 4, test_value_leq, 8);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "zZZZ", 4, NULL, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "abc", 3, NULL, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "a", 1, test_value_leq, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "a", 1, test_value_leq, 10);
  print_hash_table (hash);
  hash_destroy (&hash);
}

void test_hash_delete_2 () {
  hash_t *hash = hash_new (0x1);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);

  hash_delete (hash, (uint8_t*) "abcd", 4, NULL, 5);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "bcde", 4, test_value_leq, 8);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "zZZZ", 4, test_value_leq, 100);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "abc", 3, NULL, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "a", 1, test_value_leq, 0);
  print_hash_table (hash);
  hash_delete (hash, (uint8_t*) "a", 1, test_value_leq, 10);
  print_hash_table (hash);
  hash_destroy (&hash);
}

void test_hash_lookup () {
  uint64_t value;
  hash_t *hash = hash_new (0x100);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);

  hash_lookup (hash, (uint8_t*) "a", 1, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "ab", 2, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "abc", 3, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "abcd", 4, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "b", 1, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bc", 2, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bcd", 3, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bcde", 4, &value);
  printf ("value %ld\n", value);
  hash_destroy (&hash);
}

void test_hash_lookup_2 () {
  uint64_t value;
  hash_t *hash = hash_new (0x1);
  hash_insert (hash, (uint8_t*) "a", 1, 1);
  hash_insert (hash, (uint8_t*) "ab", 2, 2);
  hash_insert (hash, (uint8_t*) "abc", 3, 3);
  hash_insert (hash, (uint8_t*) "abcd", 4, 4);
  hash_insert (hash, (uint8_t*) "b", 1, 5);
  hash_insert (hash, (uint8_t*) "bc", 2, 6);
  hash_insert (hash, (uint8_t*) "bcd", 3, 7);
  hash_insert (hash, (uint8_t*) "bcde", 4, 8);
  print_hash_table (hash);

  hash_lookup (hash, (uint8_t*) "a", 1, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "ab", 2, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "abc", 3, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "abcd", 4, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "b", 1, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bc", 2, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bcd", 3, &value);
  printf ("value %ld\n", value);
  hash_lookup (hash, (uint8_t*) "bcde", 4, &value);
  printf ("value %ld\n", value);
  hash_destroy (&hash);
}

int main (int argc, char* argv[]) {
  test_hash_lookup ();
  test_hash_lookup_2 ();
  return 0;
}
