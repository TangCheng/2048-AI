#include <stdlib.h>
#include "minmax.h"
#include "../models/calculator.h"
#include "tree.h"
#include "board_pool.h"
#include "evaluator.h"

typedef struct _minmax
{
  tree        *bt;
  board_pool  *bp;
  evaluator   *be;
  calculator  *bc;
} minmax;

static bool minmax_change_tree_root(minmax *self, board *b,
  enum direction last_dir);
static void minmax_growth_tree(minmax *self);
static void minmax_new_level(minmax *self, tree_node *node);
static void minmax_new_level_for_player(minmax *self, tree_node *node,
  board_data *bd);
static void minmax_new_level_for_computer(minmax *self, tree_node *node,
  board_data *bd);
static double minmax_search_engine(minmax *self, uint32 depth, tree_node *root,
  uint32 positions, uint32 cutoffs);

static void minmax_data_free_callback(void *owner, void *data)
{
  minmax *self = (minmax *)owner;
  if (self != NULL && data != NULL)
  {
    board_pool_put(self->bp, (board_data *)data);
  }
}

static bool minmax_data_compare_callback(void *data1, void *data2)
{
  board_data *bd1 = (board_data *)data1;
  board_data *bd2 = (board_data *)data2;

  return board_is_equal(bd1->b, bd2->b) && (bd2->r == PLAYER_TURN)/*(bd1->dir == bd2->dir)*/;
}

bool minmax_create(minmax **self)
{
  bool ret = false;

  *self = (minmax *)malloc(sizeof(minmax));
  if (*self != NULL)
  {
    if (tree_create(&(*self)->bt) == true)
    {
      tree_set_data_free_callback((*self)->bt, *self, minmax_data_free_callback);
      tree_set_data_compare_callback((*self)->bt, minmax_data_compare_callback);
    }
    board_pool_create(&(*self)->bp);
    evaluator_create(&(*self)->be);
    calculator_create(&(*self)->bc);
    ret = true;
  }

  return ret;
}

void minmax_destory(minmax **self)
{
  if ((*self != NULL)) {
    evaluator_destory(&(*self)->be);
    tree_destory(&(*self)->bt);
    board_pool_destory(&(*self)->bp);
    calculator_destory(&(*self)->bc);
    free(*self);
    *self = NULL;
  }
}

enum direction minmax_search(minmax *self, board *b, enum direction last_dir,
  uint32 depth)
{
  enum direction best = BOTTOM_OF_DIRECTION;
  double best_value = 0;
  uint32 tree_depth = 0;
  tree_node *root = NULL;

  if (self != NULL && b != NULL && depth != 0)
  {
    if (minmax_change_tree_root(self, b, last_dir) == true)
    {
      tree_depth = tree_get_depth(self->bt);
      while (tree_depth <= depth)
      {
        minmax_growth_tree(self);
        tree_depth = tree_get_depth(self->bt);
      }
      root = tree_get_root(self->bt);
      if (root != NULL)
      {
        best_value = minmax_search_engine(self, depth, root, 0, 0);
        //LOG("best value is %d", best_value);
        tree_node *child_node = tree_get_child(self->bt, root);
        while (child_node != NULL)
        {
          board_data *bd = tree_get_data(self->bt, child_node);
          LOG("bd value is %f, dir is %u", bd->value, bd->dir);
          if (bd->value == best_value)
          {
            best = bd->dir;
            break;
          }
          child_node = tree_get_sibling(self->bt, child_node);
        }
      }
    }
  }

  return best;
}

static bool minmax_change_tree_root(minmax *self, board *b,
  enum direction last_dir)
{
  bool ret = false;
  board_data *bd = NULL;

  bd = board_pool_get(self->bp);
  if (bd != NULL)
  {
    board_clone_data(b, bd->b);
    tree_node *current_root = NULL;
    current_root = tree_get_root(self->bt);
    if (current_root == NULL)
    {
      bd->r = PLAYER_TURN;
      tree_insert(self->bt, NULL, (void *)bd);
      LOG("change root from %p to %p", current_root, tree_get_root(self->bt));
    }
    else
    {
      bd->dir = last_dir;
      tree_node *node = tree_find_node(self->bt, (void *)bd);
      if (node != NULL)
      {
        if (node != current_root)
        {
          tree_set_new_root(self->bt, node);
          LOG("change root from %p to %p", current_root, node);
        }
        board_pool_put(self->bp, bd);
      }
      else
      {
        LOG("could not find any node.");
        bd->r = PLAYER_TURN;
        tree_delete(self->bt, current_root);
        tree_insert(self->bt, NULL, (void *)bd);
        LOG("change root from %p to %p", current_root, tree_get_root(self->bt));
      }
    }
    ret = true;
  }

  return ret;
}

static void minmax_growth_tree(minmax *self)
{
  tree_node *leaf = NULL;

  leaf = tree_get_first_leaf(self->bt);
  while (leaf != NULL)
  {
    minmax_new_level(self, leaf);
    leaf = tree_get_next_leaf(self->bt);
  }
}

static void minmax_new_level(minmax *self, tree_node *node)
{
  board_data *bd = NULL;

  bd = tree_get_data(self->bt, node);
  if (bd != NULL)
  {
    switch (bd->r)
    {
      case PLAYER_TURN:
        minmax_new_level_for_player(self, node, bd);
        break;
      case COMPUTER_TURN:
        minmax_new_level_for_computer(self, node, bd);
        break;
      default:
        break;
    }
  }
}

static void minmax_new_level_for_player(minmax *self, tree_node *node,
  board_data *bd)
{
  enum direction dir = BOTTOM_OF_DIRECTION;
  board_data *new_bd = NULL;

  for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++)
  {
    new_bd = board_pool_get(self->bp);
    if (new_bd != NULL)
    {
      if (calculator_move(self->bc, bd->b, new_bd->b, dir) == true)
      {
        new_bd->dir = dir;
        new_bd->r = COMPUTER_TURN;
        //new_bd->value = evaluator_get_value(self->be, new_bd->b);
        tree_insert(self->bt, node, (void *)new_bd);
      }
      else
      {
        board_pool_put(self->bp, new_bd);
      }
    }
  }
}

static void minmax_new_level_for_computer(minmax *self, tree_node *node,
  board_data *bd)
{
  uint64 *pos_array = NULL;
  uint32 len = 0;
  uint32 i = 0, j = 0;
  board_data *new_bd = NULL;
  uint32 value[] = GAME_NUBMER_ELEMENTS;
  uint32 smoothness = 0, islands = 0, worst_score = 0;
  uint32 worst_pos = 0, worst_val = value[0];

  board_get_empty(bd->b, &pos_array, &len);
  new_bd = board_pool_get(self->bp);
  if (new_bd != NULL)
  {
    for (i = 0; i < len; i++)
    {
      for (j = 0; j < ARRAY_SIZE(value); j++)
      {
        board_clone_data(bd->b, new_bd->b);
        board_set_value_by_pos(new_bd->b, pos_array[i], value[j]);
        smoothness = evaluator_smoothness(self->be, new_bd->b);
        islands = evaluator_islands(self->be, new_bd->b);
        if (worst_score < (-smoothness + islands))
        {
          worst_score = -smoothness + islands;
          worst_pos = pos_array[i];
          worst_val = value[j];
        }
      }
    }
    new_bd->r = PLAYER_TURN;
    new_bd->dir = bd->dir;
    board_clone_data(bd->b, new_bd->b);
    board_set_value_by_pos(new_bd->b, worst_pos, worst_val);
    //new_bd->value = evaluator_get_value(self->be, new_bd->b);
    tree_insert(self->bt, node, (void *)new_bd);
  }
  free(pos_array);
}

static double minmax_search_engine(minmax *self, uint32 depth, tree_node *root,
  uint32 positions, uint32 cutoffs)
{
  double value = 0.0;
  double score = 0.0;
  board_data *bd = NULL;
  tree_node *child_node = NULL;

  bd = tree_get_data(self->bt, root);
  if (bd != NULL)
  {
    if (depth == 0)
    {
      value = evaluator_get_value(self->be, bd->b);
      return value;
    }
    if (bd->r == PLAYER_TURN)
    {
      bd->value = -10000.0;
    }
    else
    {
      bd->value = 10000.0;
    }
    child_node = tree_get_child(self->bt, root);
    while (child_node != NULL)
    {
      score = minmax_search_engine(self, depth - 1, child_node, positions, cutoffs);
      if (bd->r == PLAYER_TURN)
      {
        if (score > bd->value)
        {
          bd->value = score;
          value = bd->value;
        }
      }
      else
      {
        if (score < bd->value)
        {
          bd->value = score;
          value = bd->value;
        }
      }
      child_node = tree_get_sibling(self->bt, child_node);
    }
  }

  return value;
}
