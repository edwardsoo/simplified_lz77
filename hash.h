#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct list {
  struct list *next;
  int key_len;
  uint8_t *key;
  uint64_t value;
} list_t;

typedef struct hash {
  list_t** array;
  int size;
  int count;
} hash_t;

hash_t* hash_new (int size);
void hash_destroy (hash_t **hash_p);
void hash_insert (hash_t *hash, uint8_t *key, int key_len, uint64_t value);
void hash_delete (hash_t *hash, uint8_t *key, int key_len,
    int (*fn)(uint64_t value, uint64_t arg), uint64_t arg);
int hash_lookup (hash_t *hash, uint8_t *key, int key_len, uint64_t *value);
list_t* list_new (uint8_t *key, int key_len, uint64_t value, list_t *next);
void list_destroy (list_t **list_p);

#endif
