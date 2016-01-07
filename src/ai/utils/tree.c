#include <stdlib.h>
#include "tree.h"
#include "list.h"

typedef struct _tree_node tree_node;

typedef struct _tree
{
  tree_node             *root;
  void                  *data_owner;
  callback_data_free    data_free_func;
  callback_data_compare data_compare_func;
  list                  *unused_nodes;
  list                  *leaf_nodes;
  uint32                depth;
  uint32                degree;
} tree;

struct _tree_node
{
  void        *data;
  tree_node   *parent;
  tree_node   *first_child;
  tree_node   *next_sibling;
};

static void tree_delete_sub_tree(tree *self, tree_node *root);
static tree_node *tree_get_new_node(tree *self);
static void tree_put_unused_node(tree *self, tree_node *node);
static void tree_release_unused_nodes(tree *self);
static void tree_traverse_for_depth(tree *self, tree_node *root);
static void tree_traverse_for_degree(tree *self, tree_node *root);
static tree_node *tree_traverse_for_find(tree *self, tree_node *root, void *data);
static void tree_traverse_for_find_leaf(tree *self, tree_node *root);

bool tree_create(tree **self)
{
  bool ret = false;

  *self = (tree *)malloc(sizeof(tree));
  if (*self != NULL) {
    (*self)->root = NULL;
    (*self)->data_free_func = NULL;
    (*self)->data_compare_func = NULL;
    list_create(&(*self)->unused_nodes);
    list_create(&(*self)->leaf_nodes);
    (*self)->depth = 0;
    (*self)->degree = 0;
    ret = true;
  }

  return ret;
}

void tree_destory(tree **self)
{
  if (*self != NULL)
  {
    tree_delete(*self, (*self)->root);
    tree_release_unused_nodes(*self);
    list_destory(&(*self)->unused_nodes);
    list_destory(&(*self)->leaf_nodes);
    free(*self);
    *self = NULL;
  }
}

void tree_set_data_free_callback(tree *self, void *owner, callback_data_free func)
{
  if (self != NULL)
  {
    self->data_owner = owner;
    self->data_free_func = func;
  }
}

void tree_set_data_compare_callback(tree *self, callback_data_compare func)
{
  if (self != NULL)
  {
    self->data_compare_func = func;
  }
}

tree_node *tree_get_root(tree *self)
{
  tree_node *node = NULL;
  if (self != NULL)
  {
    node = self->root;
  }
  return node;
}

void tree_set_new_root(tree *self, tree_node *node)
{
  if (self != NULL && node != NULL)
  {
    /* first step, release all siblings */
    tree_node *parent = node->parent;
    if (parent != NULL)
    {
      tree_node *sibling = parent->first_child;
      tree_node *temp = NULL;
      while (sibling != NULL)
      {
        temp = sibling->next_sibling;
        if (sibling != node)
        {
          tree_delete(self, sibling);
        }
        sibling = temp;
      }

      /* second step, release all ancestors */
      parent->first_child = NULL;
      tree_delete(self, self->root);

      /* third step, set the new root */
      self->root = node;
      self->root->parent = NULL;
      self->root->next_sibling = NULL;
    }
  }
}

bool tree_insert(tree *self, tree_node *parent, void *data)
{
  bool ret = false;
  tree_node *node = NULL;

  if (self != NULL)
  {
    node = tree_get_new_node(self);
    if (node != NULL)
    {
      node->data = data;
      if (self->root == NULL)
      {
        self->root = node;
        ret = true;
      }
      else if (parent != NULL)
      {
        node->parent = parent;
        tree_node *sibling = parent->first_child;
        if (sibling == NULL)
        {
          parent->first_child = node;
        }
        else
        {
          while (sibling != NULL)
          {
            if (sibling->next_sibling == NULL)
            {
              sibling->next_sibling = node;
              break;
            }
            sibling = sibling->next_sibling;
          }
        }
        ret = true;
      }
      else
      {
        tree_put_unused_node(self, node);
      }
    }
  }

  return ret;
}

bool tree_delete(tree *self, tree_node *node)
{
  bool ret = false;

  if (self != NULL && node != NULL)
  {
    tree_node *child = node->first_child;
    tree_delete_sub_tree(self, child);
    tree_node *sibling = NULL;
    if (node->parent != NULL)
    {
      sibling = node->parent->first_child;
    }
    if (node == sibling)
    {
      node->parent->first_child = node->next_sibling;
    }
    else
    {
      while (sibling != NULL)
      {
        if (sibling->next_sibling == node)
        {
          sibling->next_sibling = node->next_sibling;
          break;
        }
        sibling = sibling->next_sibling;
      }
    }
    if (node == self->root)
    {
      self->root = NULL;
    }
    tree_put_unused_node(self, node);
    ret = true;
  }

  return ret;
}

tree_node *tree_get_child(tree *self, tree_node *node)
{
  tree_node *ret = NULL;
  if (self != NULL && node != NULL)
  {
    ret = node->first_child;
  }
  return ret;
}

tree_node *tree_get_parent(tree *self, tree_node *node)
{
  tree_node *ret = NULL;
  if (self != NULL && node != NULL)
  {
    ret = node->parent;
  }
  return ret;
}

tree_node *tree_get_sibling(tree *self, tree_node *node)
{
  tree_node *ret = NULL;
  if (self != NULL && node != NULL)
  {
    ret = node->next_sibling;
  }
  return ret;
}

void *tree_get_data(tree *self, tree_node *node)
{
  void *data = NULL;

  if (self != NULL && node != NULL)
  {
    data = node->data;
  }

  return data;
}

uint32 tree_get_depth(tree *self)
{
  uint32 depth = 0;

  if (self != NULL)
  {
    self->depth = 0;
    tree_traverse_for_depth(self, self->root);
    depth = self->depth;
  }

  return depth;
}

uint32 tree_get_degree(tree *self)
{
  uint32 degree = 0;

  if (self != NULL)
  {
    self->degree = 0;
    tree_traverse_for_degree(self, self->root);
    degree = self->degree;
  }

  return degree;
}

uint32 tree_get_node_degree(tree *self, tree_node *node)
{
  uint32 degree = 0;

  if (self != NULL && node != NULL)
  {
    tree_node *child_node = node->first_child;
    while (child_node != NULL)
    {
      degree++;
      child_node = child_node->next_sibling;
    }
  }

  return degree;
}

uint32 tree_get_node_level(tree *self, tree_node *node)
{
  uint32 level = 0;

  if (self != NULL && node != NULL)
  {
    if (node == self->root)
    {
      return 1;
    }
    level += tree_get_node_level(self, node->parent) + 1;
  }

  return level;
}

tree_node *tree_find_node(tree *self, void *data)
{
  tree_node *node = NULL;

  if (self != NULL && data != NULL)
  {
    node = tree_traverse_for_find(self, self->root, data);
  }

  return node;
}

tree_node *tree_get_first_leaf(tree *self)
{
  tree_node *leaf = NULL;

  if (self != NULL)
  {
    list_clear(self->leaf_nodes);
    tree_traverse_for_find_leaf(self, self->root);
    leaf = (tree_node *)list_get_from_first(self->leaf_nodes);
  }

  return leaf;
}

tree_node *tree_get_next_leaf(tree *self)
{
  tree_node *leaf = NULL;

  if (self != NULL)
  {
    leaf = (tree_node *)list_get_from_first(self->leaf_nodes);
  }

  return leaf;
}

static void tree_delete_sub_tree(tree *self, tree_node *root)
{
  if (root != NULL)
  {
    tree_delete_sub_tree(self, root->first_child);
    tree_delete_sub_tree(self, root->next_sibling);
    tree_put_unused_node(self, root);
  }
}

static tree_node *tree_get_new_node(tree *self)
{
  tree_node *node = NULL;

  if (list_is_empty(self->unused_nodes) == true)
  {
    node = (tree_node *)malloc(sizeof(tree_node));
  }
  else
  {
    node = (tree_node *)list_get_from_first(self->unused_nodes);
  }

  if (node != NULL)
  {
    node->data = NULL;
    node->parent = NULL;
    node->first_child = NULL;
    node->next_sibling = NULL;
  }

  return node;
}

static void tree_put_unused_node(tree *self, tree_node *node)
{
  if (node->data != NULL)
  {
    if (self->data_owner != NULL && self->data_free_func != NULL)
    {
      self->data_free_func(self->data_owner, node->data);
    }
    else
    {
      free(node->data);
    }
    node->data = NULL;
  }
  list_add_to_last(self->unused_nodes, (void *)node);
}

static void tree_release_unused_nodes(tree *self)
{
  tree_node *node = NULL;
  while (list_is_empty(self->unused_nodes) != true)
  {
    node = (tree_node *)list_get_from_first(self->unused_nodes);
    free(node);
  }
}

static void tree_traverse_for_depth(tree *self, tree_node *root)
{
  uint32 node_level = 0;
  if (root != NULL)
  {
    if (root->first_child == NULL)
    {
      node_level = tree_get_node_level(self, root);
      self->depth = MAX(self->depth, node_level);
    }
    tree_traverse_for_depth(self, root->first_child);
    tree_traverse_for_depth(self, root->next_sibling);
  }
}

static void tree_traverse_for_degree(tree *self, tree_node *root)
{
  uint32 node_degree = 0;
  if (root != NULL)
  {
    node_degree = tree_get_node_degree(self, root);
    self->degree = MAX(self->degree, node_degree);
    tree_traverse_for_depth(self, root->first_child);
    tree_traverse_for_depth(self, root->next_sibling);
  }
}

static tree_node *tree_traverse_for_find(tree *self, tree_node *root, void *data)
{
  tree_node *node = NULL;

  if (root != NULL)
  {
    if (self->data_compare_func(data, root->data) == true)
    {
      node = root;
    }
    if (node == NULL)
    {
      node = tree_traverse_for_find(self, root->first_child, data);
    }
    if (node == NULL)
    {
      node = tree_traverse_for_find(self, root->next_sibling, data);
    }
  }

  return node;
}

static void tree_traverse_for_find_leaf(tree *self, tree_node *root)
{
  if (root != NULL)
  {
    if (root->first_child == NULL)
    {
      list_add_to_last(self->leaf_nodes, (void *)root);
    }
    tree_traverse_for_find_leaf(self, root->first_child);
    tree_traverse_for_find_leaf(self, root->next_sibling);
  }
}
