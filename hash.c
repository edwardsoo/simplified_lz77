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

uint32_t hash_code (uint8_t *key, int key_len) {
  uint32_t h, i;
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

/* Insert key-value into hash table.
 * If key is alreay in table, update value
 */
void hash_insert (hash_t *hash, uint8_t *key, int key_len, uint64_t value) {
  uint32_t h;
  int diff;
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

/* Delete key-value from hash table.
 * If key not found on table then the table is not modified
 */
void hash_delete (hash_t *hash, uint8_t *key, int key_len,
    int (*fn)(uint64_t value, uint64_t arg), uint64_t arg) {
  uint32_t h;
  int diff;
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

/* Lookup the value mapped by a key in the hash table.
 * The value is place in address supplied,
 * return 0 if key-value is found, -1 otherwise
 */
int hash_lookup (hash_t *hash, uint8_t *key, int key_len, uint64_t *value) {
  uint32_t h;
  int diff;
  list_t **list_p, *list;

  h = hash_code (key, key_len) % hash->size;
  list_p = hash->array + h;

  while (*list_p) {
    list = *list_p;

    if (list->key_len > key_len) {
      // Key not in table
      return -1;

    } else if (list->key_len == key_len) {
      diff = memcmp (key, list->key, key_len);

      if (diff == 0) {
        // Found key
        *value = list->value;
        return 0;

      } else if (diff < 0) {
        // Key not in table
        return -1;
      }
    }
    list_p = &(list->next);
  }
  return -1;
}
