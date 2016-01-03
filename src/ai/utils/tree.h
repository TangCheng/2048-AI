#ifndef __TREE_H__
#define __TREE_H__

#include "constants.h"

typedef struct _tree tree;
typedef struct _tree_node tree_node;
typedef void (*callback_data_free)(void *owner, void *data);
typedef bool (*callback_data_compare)(void *user_data, void *node_data);

bool tree_create(tree **self);
void tree_destory(tree **self);
void tree_set_data_free_callback(tree *self, void *owner, callback_data_free func);
void tree_set_data_compare_callback(tree *self, callback_data_compare func);
tree_node *tree_get_root(tree *self);
void tree_set_new_root(tree *self, tree_node *node);
bool tree_insert(tree *self, tree_node *parent, void *data);
bool tree_delete(tree *self, tree_node *node);
tree_node *tree_get_child(tree *self, tree_node *node);
tree_node *tree_get_parent(tree *self, tree_node *node);
tree_node *tree_get_sibling(tree *self, tree_node *node);
void *tree_get_data(tree *self, tree_node *node);
uint32 tree_get_depth(tree *self);
uint32 tree_get_degree(tree *self);
uint32 tree_get_node_degree(tree *self, tree_node *node);
uint32 tree_get_node_level(tree *self, tree_node *node);
tree_node *tree_find_node(tree *self, void *data);
tree_node *tree_get_first_leaf(tree *self);
tree_node *tree_get_next_leaf(tree *self);

#endif /* __TREE_H__ */
