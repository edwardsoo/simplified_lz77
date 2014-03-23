#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prefix_tree.h"

inline int min_int (int a, int b) {
  return (a < b ? a : b);
}

prefix_tree_t *prefix_tree_new (
    uint8_t *key, int len, uint64_t value, uint8_t has_value
    ) {
  prefix_tree_t *tree = malloc (sizeof (prefix_tree_t));
  if (tree) {
    if (len) {
      tree->key = malloc (sizeof (uint8_t) * len);
    } else {
      tree->key = NULL;
    }
    tree->key_len = len;
    memcpy (tree->key, key, sizeof (uint8_t) * len);
    tree->value = value;
    tree->has_value = has_value;
    tree->num_child = 0;
    memset (tree->child, 0, sizeof (prefix_tree_t *) * CHILD_SIZE);
  }
  return tree;
}

void prefix_tree_destroy (prefix_tree_t **tree_p) {
  int i;

  prefix_tree_t *tree = *tree_p;
  for (i = 0; i < CHILD_SIZE; i++) {
    if (tree->child[i]) {
      prefix_tree_destroy (tree->child + i);
    }
  }
  free (tree->key);
  free (tree);
  *tree_p = NULL;
}

void prefix_tree_insert (
    prefix_tree_t **tree_p, uint8_t *key, int len, uint64_t value
    ) {
  int matched;
  prefix_tree_t *tree, *new;
  tree = *tree_p;

  if (!tree) {
    *tree_p = prefix_tree_new (key, len, value, 1);

  } else {
    if (tree->key_len == 0) { // Root's key is empty
      if (tree->num_child == 0) { // Empty tree
        prefix_tree_destroy (tree_p);
        *tree_p = prefix_tree_new (key, len, value, 1);

      } else { // Non-empty tree with empty root
        if (!tree->child[key[0]]) {
          tree->num_child += 1;
        }
        prefix_tree_insert (tree->child + key[0], key, len, value);
      }

    } else {
      matched = num_prefix_match (tree->key, key, tree->key_len, len);

      if (matched == 0) {
        // Root's key not empty and new key has no prefix match
        new = prefix_tree_new (NULL, 0, 0, 0);
        *tree_p = new;
        
        new->child[tree->key[0]] = tree;
        prefix_tree_insert (new->child + key[0], key, len, value); 
        new->num_child = 2;

      } else if (tree->key_len < len && matched == tree->key_len) {
        // the tree's key is a subarray of the new key
        if (!tree->child[key[matched]]) {
          tree->num_child += 1;
        }
        prefix_tree_insert (tree->child + key[matched],
            key + matched, len - matched, value);

      } else if (tree->key_len > len && matched == len) {
        // the new key is a subarray of the tree's key
        new = prefix_tree_new (key, len, value, 1);
        *tree_p = new;
        
        new->child[tree->key[matched]] = tree;
        new->num_child = 1;
        memmove (tree->key, tree->key + matched, 
            sizeof (uint8_t) * (tree->key_len - matched));
        tree->key_len -= matched;
        tree->key = realloc (tree->key, sizeof (uint8_t) * tree->key_len);


      } else if (tree->key_len == len && matched == len) {
        // exact key match
        tree->value = value;
        tree->has_value = 1;

      } else {
        // no subarray relationship
        new = prefix_tree_new (key, matched, 0, 0);
        *tree_p = new;

        new->child[tree->key[matched]] = tree;
        memmove (tree->key, tree->key + matched,
            sizeof (uint8_t) * (tree->key_len - matched));
        tree->key_len -= matched;
        tree->key = realloc (tree->key, sizeof (uint8_t) * tree->key_len);

        new->child[key[matched]] = prefix_tree_new (key + matched,
            len - matched, value, 1);
        new->num_child = 2;
      }
    }
  }
}

int prefix_tree_longest_match (
    prefix_tree_t *tree, uint8_t *key, int len, uint64_t *value
    ) {
  int matched, rc;
  matched = num_prefix_match (tree->key, key, tree->key_len, len);

  if (
      tree->key_len == matched && tree->has_value && 
      (len == matched || !tree->child[key[matched]])
     ) {
    *value = tree->value;
    return matched;

  } else if (
      len > matched && matched == tree->key_len && tree->child[key[matched]]
      ) {
    rc = prefix_tree_longest_match (tree->child[key[matched]], key + matched,
       len - matched, value);

    if (rc > 0) {
      return rc + matched;

    } else if (tree->has_value) {
      *value = tree->value;
      return matched;

    } else {
      return 0;
    }

  } else {
    return 0;
  }
}

void merge_with_only_child (prefix_tree_t **tree_p) {
  int i;
  prefix_tree_t *tree, *child;
  tree = *tree_p;
  child = NULL;

  for (i = 0; i < CHILD_SIZE; i++) {
    if (tree->child[i]) {
      child = tree->child[i];
      tree->child[i] = NULL;
      break;
    }
  }

  for (i++; i < CHILD_SIZE; i++) {
  }

  child->key = realloc (child->key,
      sizeof (uint8_t) * (tree->key_len + child->key_len));
  memmove (child->key + tree->key_len, child->key,
      sizeof (uint8_t) * child->key_len);
  memmove (child->key, tree->key, tree->key_len * sizeof (uint8_t));
  child->key_len += tree->key_len;
  prefix_tree_destroy (tree_p);
  *tree_p = child;
}

int prefix_tree_delete (
    prefix_tree_t **tree_p, uint8_t *key, int len,
    int (*fn)(prefix_tree_t*, void*), void* arg
    ) {
  int matched, rc;
  prefix_tree_t *tree;
  tree = *tree_p;

  if (!tree) {
    return -1;

  } else {
    matched = num_prefix_match (tree->key, key, tree->key_len, len);

    if (tree->key_len == matched && matched == len) { // keys match
      if (tree->has_value && (!fn || fn (tree, arg))) {
        if (tree->num_child > 1) {
          // Prefix at least 2 sub-trees, turn into internal node
          tree->has_value = 0;

        } else if (tree->num_child == 0) {
          prefix_tree_destroy (tree_p);

        } else {
          // Only has 1 child, merge node & key with child's
          merge_with_only_child (tree_p);
        }
        return 0;

      } else { // No value or value fails evaluation function
        return -1;
      }
    } else if (len > matched && tree->child[key[matched]]) {
      // Try to delete in sub-tree
      rc = prefix_tree_delete (tree->child + key[matched], key + matched,
          len - matched, fn, arg);

      if (rc == 0) {
        if (tree->child[key[matched]] == NULL) {
          // Sub-tree was pruned
          tree->num_child -= 1;
        }
        if (!tree->has_value && tree->num_child == 1) {
          merge_with_only_child (tree_p);
        }
      } else {
      }
      return rc;

    } else {
      return -1;
    }
  }
}


/* Returns the number of matching bytes in 2 key arrays
 */
int num_prefix_match (uint8_t *key1, uint8_t *key2, int len1, int len2) {
  int i, min;
  min = min_int (len1, len2);
  if (min == 0) return 0;
  for (i = 0; i < min && key1[i] == key2[i]; i++);
  return i;
}
