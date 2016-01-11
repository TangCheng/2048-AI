#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "input.h"

typedef struct _input
{
  struct termios new_settings;
  struct termios stored_settings;
  enum direction dir;
} input;

bool input_create(input **self)
{
  bool ret = false;

  *self = (input *)malloc(sizeof(input));
  if (*self != NULL)
  {
    tcgetattr(STDIN_FILENO, &(*self)->stored_settings);
    (*self)->new_settings = (*self)->stored_settings;
    (*self)->new_settings.c_lflag &= ~(ICANON | ECHO);
    (*self)->new_settings.c_cc[VTIME] = 0;
    (*self)->new_settings.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &(*self)->new_settings);
    (*self)->dir = BOTTOM_OF_DIRECTION;
    ret = true;
  }
  return ret;
}

void input_destory(input **self)
{
  if (*self != NULL)
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &(*self)->stored_settings);
    free(*self);
    *self = NULL;
  }
}

bool input_parser_alphabet_key(input *self, char in)
{
  bool got = true;
  switch (in)
  {
    case 'w':
    case 'W':
      self->dir = UP;
      break;
    case 'a':
    case 'A':
      self->dir = LEFT;
      break;
    case 's':
    case 'S':
      self->dir = DOWN;
      break;
    case 'd':
    case 'D':
      self->dir = RIGHT;
      break;
    case 0x1B:
      self->dir = BOTTOM_OF_DIRECTION;
    default:
      got = false;
      break;
  }
  return got;
}

bool input_parser_direction_key(input *self, char in)
{
  bool got = true;
  switch (in)
  {
    case 'A':
      self->dir = UP;
      break;
    case 'B':
      self->dir = DOWN;
      break;
    case 'C':
      self->dir = RIGHT;
      break;
    case 'D':
      self->dir = LEFT;
      break;
    default:
      got = false;
      break;
  }
  return got;
}

bool input_parser(input *self, char in, bool direction_key)
{
  bool got = false;
  if (direction_key)
  {
    got = input_parser_direction_key(self, in);
  }
  else
  {
    got = input_parser_alphabet_key(self, in);
  }

  return got;
}

enum direction input_get(input *self)
{
  int8 buffer[3];
  ssize_t bytes_read;
  bool got = false;

  if (self != NULL)
  {
    do
    {
      bytes_read = read(STDIN_FILENO, &buffer, 3);
      switch (bytes_read)
      {
        case 1:
        case 3:
          got = input_parser(self, buffer[bytes_read - 1], bytes_read == 3);
          break;
        case -1:
        default:
          return BOTTOM_OF_DIRECTION;
          break;
      }
    } while(got == false);
    return self->dir;
  }
  return BOTTOM_OF_DIRECTION;
}
