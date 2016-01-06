#include <stdlib.h>
#include "output.h"

#define STANDARD_OUTPUT(format, ...) do { \
  fprintf(stdout, format, ##__VA_ARGS__); \
  /* fflush(stdout); */                        \
} while(0);

typedef struct _console_output
{
  uint32 count;
} cout;

static char directions[BOTTOM_OF_DIRECTION] = {'U', 'D', 'L', 'R'};
static cout *output = NULL;

bool cout_create(cout **self)
{
  bool ret = false;

  if (output != NULL)
  {
    output->count++;
    *self = output;
    ret = true;
  }
  else
  {
    output = (cout *)malloc(sizeof(cout));
    if (output != NULL)
    {
      output->count = 1;
      *self = output;
      ret = true;
    }
  }

  return ret;
}

void cout_destory(cout **self)
{
  if (output != NULL && *self == output)
  {
    output->count--;
    *self = NULL;
    if (output->count == 0)
    {
      free(output);
      output = NULL;
    }
  }
}

void cout_display_text(cout *self, char *text)
{
  if (self != NULL)
  {
    STANDARD_OUTPUT("%s\n", text);
  }
}

void cout_display_direction(cout *self, enum direction d)
{
  if (self != NULL)
  {
    if (d < BOTTOM_OF_DIRECTION)
    {
      STANDARD_OUTPUT("%c\n", directions[d]);
    }
    else
    {
      LOG("Invalid direction.");
    }
  }
}

void cout_display_board(cout *self, board *b)
{
  if (self != NULL && b != NULL)
  {
    uint32 x = 0, y = 0;
    uint32 rows = 0, cols = 0;

    rows = board_get_rows(b);
    cols = board_get_cols(b);

    for (y = 0; y < rows; y++)
    {
      for (x = 0; x < cols; x++)
      {
        STANDARD_OUTPUT("|%u|", board_get_value(b, x, y));
      }
      STANDARD_OUTPUT("\n");
    }
  }
}

void cout_display_score(cout *self, uint32 score)
{
  if (self != NULL)
  {
    LOG("score: %u", score);
  }
}
