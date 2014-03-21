#ifndef PREFIX_TREE_H
#define PREFIX_TREE_H

/* Prefix tree to help find matching consecutive bytes
 */
typedef struct prefix_tree {
  struct prefix_tree* child[0x100];
  uint64_t key;
  uint8_t *prefix, has_key;
  int prefix_len, num_child;
} prefix_tree_t;

prefix_tree_t *prefix_tree_new (
    uint8_t *prefix, int len, uint64_t key, uint8_t has_key
    );
void prefix_tree_destroy (prefix_tree_t **tree_p);
void prefix_tree_insert (
    prefix_tree_t **tree_p, uint8_t *prefix, int len, uint64_t key
    );
int num_prefix_match (uint8_t *prefix1, uint8_t *prefix2, int len1, int len2);

#endif
