#include <stdio.h>
#include <stdlib.h>
#include "../ai/tree.h"
#include "../ai/evaluator.h"
#include "../views/output.h"

void data_free(void *owner, void *data)
{
  printf("%u\t", *(uint32 *)owner);
  printf("%u\n", *(uint32 *)data);
}

bool data_compare(void *data1, void *data2)
{
  return *(uint32 *)data1 == *(uint32 *)data2;
}

int main(int argc, char *argv[])
{
  tree *t = NULL;
  uint32 owner = 0;
  uint32 data[5] = {0, 1, 2, 3, 4};
  tree_create(&t);
  tree_set_data_free_callback(t, &owner, data_free);
  tree_set_data_compare_callback(t, data_compare);
  tree_insert(t, NULL, &data[0]);
  tree_node *node = tree_get_root(t);
  for (int i = 1; i < 5; i++)
  {
    tree_insert(t, node, &data[i]);
  }
  node = tree_get_child(t, node);
  tree_insert(t, node, &data[2]);
  tree_insert(t, node, &data[4]);
  node = tree_get_sibling(t, node);
  tree_insert(t, node, &data[1]);
  tree_insert(t, node, &data[1]);
  printf("%u\n", tree_get_depth(t));
  printf("%u\n", tree_get_degree(t));
  printf("%u\n", tree_get_node_degree(t, node));
  printf("%u\n", tree_get_node_level(t, node));
  printf("%p\n", tree_find_node(t, &data[2]));
  tree_set_new_root(t, node);
  printf("%u\n", tree_get_depth(t));
  tree_destory(&t);

  board *b = NULL;
  evaluator *eval = NULL;
  cout *o = NULL;
  if (board_create(&b, ROWS_OF_BOARD, COLS_OF_BOARD))
  {
    evaluator_create(&eval);
    cout_create(&o);

    board_set_value(b, 0, 0, 2);
    board_set_value(b, 1, 0, 4);
    board_set_value(b, 2, 0, 64);
    board_set_value(b, 3, 0, 4);
    board_set_value(b, 0, 1, 8);
    board_set_value(b, 1, 1, 128);
    board_set_value(b, 2, 1, 32);
    board_set_value(b, 3, 1, 4);
    board_set_value(b, 0, 2, 256);
    board_set_value(b, 1, 2, 64);
    board_set_value(b, 2, 2, 16);
    board_set_value(b, 3, 2, 16);
    board_set_value(b, 0, 3, 2);
    board_set_value(b, 1, 3, 4);
    board_set_value(b, 2, 3, 4);
    board_set_value(b, 3, 3, 4);

    uint32 len = 0;
    cout_display_board(o, b);
    len = board_count_availables(b);
    printf("empty count is %u\n", len);
    printf("monotonicity is %d\n", evaluator_monotonicity(eval, b));
    printf("smoothness is %d\n", evaluator_smoothness(eval, b));
    printf("empty is %.13f\n", evaluator_empty(eval, b));
    printf("max value is %u\n", evaluator_max_value(eval, b));
    printf("islands is %u\n", evaluator_islands(eval, b));
    printf("value is %.13f\n", evaluator_get_value(eval, b));

    evaluator_destory(&eval);
    cout_destory(&o);
    board_destory(&b);
  }

  return 0;
}
