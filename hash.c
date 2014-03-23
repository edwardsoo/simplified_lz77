#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"

#ifndef __WHERE__
#define __WHERE__
#define WHERE() printf("%u\n", __LINE__)
#endif

hash_t* hash_new (int size) {
  hash_t* hash;
  if (!size)
    return NULL;

  hash = malloc (sizeof (hash_t));
  if (hash) {
    hash->array = malloc (sizeof (list_t*) * size);
    if (!hash->array) {
      free (hash);
      return NULL;
    }
    memset (hash->array, 0, sizeof (list_t*) * size);
    hash->size = size;
    hash->count = 0;
  }
  return hash;
}

void hash_destroy (hash_t **hash_p) {
  int i;
  hash_t *hash = *hash_p;
  for (i = 0; i < hash->size; i++) {
    if (!hash->array[i]) continue;
    list_destroy (hash->array + i);
  }
  free (hash->array);
  free (hash);
  *hash_p = NULL;
}

int hash_code (uint8_t *key, int key_len) {
  int h, i;
  h = 0;
  for (i = 0; i < key_len; i++) {
    h = (31 * h) + key[i];
  }
  return h;
}

list_t* list_new (uint8_t *key, int key_len, uint64_t value, list_t *next) {
  list_t *new;
  new = malloc (sizeof (list_t));
  new->next = next;
  new->key = malloc (sizeof (uint8_t) * key_len);
  memcpy (new->key, key, key_len);
  new->key_len = key_len;
  new->value = value;
  return new;
}

void list_destroy (list_t **list_p) {
  list_t * list;
  list = *list_p;
  free (list->key);
  if (list->next) {
    list_destroy (&(list->next));
  }
  free (list);
  *list_p = NULL;
}

void hash_insert (hash_t *hash, uint8_t *key, int key_len, uint64_t value) {
  int h, diff;
  list_t **list_p, *list;

  h = hash_code (key, key_len) % hash->size;
  list_p = hash->array + h;

  // Keep entries sorted by key_len then key values in collision list
  while (*list_p) {
    list = *list_p;

    if (list->key_len > key_len) {
      *list_p = list_new (key, key_len, value, list);
      hash->count += 1;
      return;

    } else if (list->key_len == key_len) {
      diff = memcmp (key, list->key, key_len);

      if (diff == 0) {
        // Keys are the same, update value
        list->value = value;
        hash->count += 1;
        return;

      } else if (diff < 0) {
        // New key value is less than this node's key
        *list_p = list_new (key, key_len, value, list);
        hash->count += 1;
        return;
      }
    }
    list_p = &(list->next);
  }
  *list_p = list_new (key, key_len, value, NULL);
  hash->count += 1;
}

void hash_delete (hash_t *hash, uint8_t *key, int key_len,
    int (*fn)(uint64_t value, uint64_t arg), uint64_t arg) {
  int h, diff;
  list_t **list_p, *list;

  h = hash_code (key, key_len) % hash->size;
  list_p = hash->array + h;

  while (*list_p) {
    list = *list_p;

    if (list->key_len > key_len) {
      // Key not in table
      return;

    } else if (list->key_len == key_len) {
      diff = memcmp (key, list->key, key_len);

      if (diff == 0) {
        // Found key
        if (!fn || fn (list->value, arg)) {
          *list_p = list->next;
          list->next = NULL;
          list_destroy (&list);
          hash->count -= 1;
          return;
        }

      } else if (diff < 0) {
        // Key not in table
        return;
      }
    }
    list_p = &(list->next);
  }
}

int hash_lookup (hash_t *hash, uint8_t *key, int key_len, uint64_t *value);
