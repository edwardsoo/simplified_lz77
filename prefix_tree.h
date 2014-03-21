#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

/* Prefix tree to help find matching consecutive bytes
 */
typedef struct prefix_tree {
  struct prefix_tree* child[0x100];
  uint64_t value;
  uint8_t *key, has_value;
  int key_len, num_child;
} prefix_tree_t;

prefix_tree_t *prefix_tree_new (
    uint8_t *key, int len, uint64_t value, uint8_t has_value
    );
void prefix_tree_destroy (prefix_tree_t **tree_p);
void prefix_tree_insert (
    prefix_tree_t **tree_p, uint8_t *key, int len, uint64_t value
    );
int prefix_tree_lookup (
    prefix_tree_t *tree, uint8_t *key, int len, uint64_t *value
    );
int prefix_tree_delete (
    prefix_tree_t **tree_p, uint8_t *key, int len, int (*fn(void*)), void* arg
    );
int num_prefix_match (uint8_t *key1, uint8_t *key2, int len1, int len2);

#endif
