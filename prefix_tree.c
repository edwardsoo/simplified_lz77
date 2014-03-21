#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prefix_tree.h"

static const prefix_tree_t *zero_block[0x100] = {0};

prefix_tree_t *prefix_tree_new (
    uint8_t *prefix, int len, uint64_t key, uint8_t has_key
    ) {
  prefix_tree_t *tree = malloc (sizeof (prefix_tree_t));
  if (tree) {
    if (len) {
      tree->prefix = malloc (sizeof (uint8_t) * len);
    } else {
      tree->prefix = NULL;
    }
    tree->prefix_len = len;
    memcpy (tree->prefix, prefix, len);
    tree->key = key;
    tree->has_key = has_key;
    tree->num_child = 0;
    memset (tree->child, 0, 0x100);
  }
  return tree;
}

void prefix_tree_destroy (prefix_tree_t **tree_p) {
  int i;

  prefix_tree_t *tree = *tree_p;
  for (i = 0; i < 0x100; i++) {
    if (tree->child[i]) {
      prefix_tree_destroy (tree->child + i);
    }
  }
  free (tree->prefix);
  *tree_p = NULL;
}

void prefix_tree_insert (
    prefix_tree_t **tree_p, uint8_t *prefix, int len, uint64_t key
    ) {
  int matched;
  prefix_tree_t *tree, *new;
  tree = *tree_p;

  if (!tree) {
    *tree_p = prefix_tree_new (prefix, len, key, 1);

  } else {
    if (tree->prefix_len == 0) {
      prefix_tree_insert (tree->child + prefix[0], prefix, len, key);

    } else {
      matched = num_prefix_match (tree->prefix, prefix, tree->prefix_len, len);
      assert (matched > 0);

      if (tree->prefix_len < len && matched == tree->prefix_len) {
        prefix_tree_insert (tree->child + prefix[matched],
            prefix + matched, len - matched, key);

      } else if (tree->prefix_len > len && matched == len) {
        new = prefix_tree_new (prefix, len, key, 1);
        *tree_p = new;
        
        new->child[tree->prefix[matched]] = tree;
        memmove (tree->prefix, tree->prefix + matched, tree->prefix_len - matched);
        tree->prefix_len -= matched;

      } else if (tree->prefix_len == len && matched == len) {
        tree->key = key;
        tree->has_key = 1;

      } else {
        new = prefix_tree_new (prefix, matched, 0, 0);
        *tree_p = new;

        new->child[tree->prefix[matched]] = tree;
        memmove (tree->prefix, tree->prefix + matched, tree->prefix_len - matched);
        tree->prefix_len -= matched;

        new->child[prefix[matched]] = prefix_tree_new (prefix + matched,
            len - matched, key, 1);
      }
    }
  }
}

inline int min_int (int a, int b) {
  return (a < b ? a : b);
}

/* Returns the number of matching bytes in 2 prefix arrays
 */
int num_prefix_match (uint8_t *prefix1, uint8_t *prefix2, int len1, int len2) {
  int i;
  for (i = 0; i < min_int (len1, len2) && prefix1[i] == prefix2[i]; i++);
  return i;
}
