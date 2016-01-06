#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "hash.h"

#define VLEN    8
#define TNLEN   32

typedef struct _bucket {
  uint32          h;            /* hash value of key, keyvalue if key is a uint
                                   or ulong */
  char            *key;         /* the point to key , if key is a string */
  char            value[VLEN];  /* store a var of builtin type in a 8bit buffer */
  struct _bucket  *list_next;
  struct _bucket  *list_last;
  struct _bucket  *next;
  struct _bucket  *last;
} bucket;

typedef struct _hash
{
  int32           table_size;
  int32           table_mask;
  int32           num_of_elements;
  char            key_type[TNLEN];
  char            value_type[TNLEN];
  bucket          *internal_pointer;
  bucket          *list_head;
  bucket          *list_tail;
  bucket          **buckets;
} hash;

#define CONNECT_TO_BUCKET_DLLIST(element, list_head) do {      \
  (element)->next = (list_head);                               \
  (element)->last = NULL;                                      \
  if ((element)->next) {                                       \
    (element)->next->last = (element);                         \
  }                                                            \
} while(0);

#define DECONNECT_FROM_BUCKET_DLLIST(element, list_head) do {  \
  if ((element)->last) {                                       \
    (element)->last->next = (element)->next;                   \
  } else {                                                     \
    (list_head) = (element)->next;                             \
  }                                                            \
  if ((element)->next) {                                       \
    (element)->next->last = (element)->last;                   \
  }                                                            \
} while(0);

#define CONNECT_TO_GLOBAL_DLLIST(element, h) do {              \
  (element)->list_last = (h)->list_tail;                       \
  (h)->list_tail = (element);                                  \
  (element)->list_next = NULL;                                 \
  if ((element)->list_last != NULL) {                          \
    (element)->list_last->list_next = (element);               \
  }                                                            \
  if (!(h)->list_head) {                                       \
    (h)->list_head = (element);                                \
  }                                                            \
  if ((h)->internal_pointer == NULL) {                         \
    (h)->internal_pointer = (element);                         \
  }                                                            \
} while(0);

#define DECONNECT_FROM_GLOBAL_DLLIST(element, h) do {          \
  if ((element)->list_next) {                                  \
    (element)->list_next->list_last = (element)->list_last;    \
  } else {                                                     \
    (h)->list_tail = (element)->list_last;                     \
  }                                                            \
  if ((element)->list_last) {                                  \
    (element)->list_last->list_next = (element)->list_next;    \
  } else {                                                     \
    (h)->list_head = (element)->list_next;                     \
    (h)->internal_pointer = (element)->list_next;              \
  }                                                            \
} while(0);

static inline int php_charmask(unsigned char *input, int len, char *mask)
{
  unsigned char *end;
  unsigned char c;
  int result = 0;

  memset(mask, 0, 256);
  for (end = input + len; input < end; input++) {
    c = *input;
    if ((input + 3 < end) && input[1] == '.' &&
         input[2] == '.' && input[3] >= c) {
      memset(mask + c, 1, input[3] - c + 1);
      input += 3;
    } else if ((input + 1 < end) && input[0] == '.' && input[1] == '.') {
      if (end - len >= input) {
        result = -1;
        continue;
      }
      if (input + 2 >= end) {
        result = -1;
        continue;
      }
      if (input[-1] > input[2]) {
        result = -1;
        continue;
      }
      result = -1;
      continue;
    } else {
      mask[c] = 1;
    }
  }
  return result;
}

static inline char *trim(char *c, int mode)
{
  if (!c) {
    return NULL;
  }
  register int i;
  int len = strlen(c) + 1;
  int trimmed = 0;
  char mask[256];
  php_charmask((unsigned char*)" \n\r\t\v\0", 6, mask);
  if (mode & 1) {
    for (i = 0; i < len; i++) {
      if (mask[(unsigned char)c[i]]) {
        trimmed++;
      } else {
        break;
      }
    }
    len -= trimmed;
    c += trimmed;
  }
  if (mode & 2) {
    for (i = len - 1; i >= 0; i--) {
      if (mask[(unsigned char)c[i]]) {
        len--;
      } else {
        break;
      }
    }
  }
  c[len] = '\0';
  return c;
}

static inline char **split(const char* string, char delim, uint32* count)
{
  if (!string) {
    return 0;
  }
  int i, j, c;
  i = 0; j = c = 1;
  int length = strlen(string);
  char *copy_str = (char *)malloc(length + 1);
  memmove(copy_str, string, length);
  copy_str[length] = '\0';
  for (; i<length; i++) {
    if (copy_str[i] == delim) {
      c += 1;
    }
  }
  (*count) = c;
  char **str_array = malloc(sizeof(char *) * c);
  str_array[0] = copy_str;
  for (i = 0; i < length; i++) {
    if (copy_str[i] == delim) {
      copy_str[i] = '\0';
      str_array[j++] = copy_str + i + 1;
    }
  }
  return str_array;
}

static inline uint32 hash_func(hash *self, char *key)
{
  register uint32 hash_value = 5381;
  int32 key_length = strlen(key);

  for (; key_length >= 8; key_length -= 8) {
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
      hash_value = ((hash_value << 5) + hash_value) + *key++;
  }
  switch (key_length) {
      case 7: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 6: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 5: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 4: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 3: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 2: hash_value = ((hash_value << 5) + hash_value) + *key++; /* fallthrough... */
      case 1: hash_value = ((hash_value << 5) + hash_value) + *key++; break;
      case 0: break;
      default:
              break;
  }
  return hash_value;
}

bool _hash_create(hash **self, uint32 size, const char *type_name)
{
  bool ret = false;
  uint32 count = 0;
  char types[TNLEN] = {0};

  if (type_name == NULL ||
      strlen(type_name) == 0 ||
      strlen(type_name) >= TNLEN) {
    return ret;
  }

  strcpy(types, type_name);
  char **str_array = split(trim(types, 3), ',', &count);
  if (count != 2) {
      goto failed_point;
  }

  if (strcmp(trim(str_array[0], 3), "int")  &&
      strcmp(trim(str_array[0], 3), "long") &&
      strcmp(trim(str_array[0], 3), "char*")) {
    goto failed_point;
  }

  if (strcmp(trim(str_array[1], 3), "int"   )   &&
      strcmp(trim(str_array[1], 3), "long"  )   &&
      strcmp(trim(str_array[1], 3), "double")   &&
      strcmp(trim(str_array[1], 3), "float" )   &&
      strcmp(trim(str_array[1], 3), "short" )   &&
      strcmp(trim(str_array[1], 3), "char*" )   &&
      strcmp(trim(str_array[1], 3), "char")) {
    goto failed_point;
  }

  *self = (hash *)malloc(sizeof(hash));
  if (*self != NULL) {
    strcpy((*self)->key_type, trim(str_array[0], 3));
    strcpy((*self)->value_type, trim(str_array[1], 3));

    uint32 i = 3;
    if (size >= 0x80000000) {
      (*self)->table_size = 0x80000000;
    } else {
      while ((1U << i) < size) {
            i++;
      }
      (*self)->table_size = 1 << i;
    }
    (*self)->buckets = (bucket **)malloc((*self)->table_size * sizeof(bucket *));
    if ((*self)->buckets != NULL) {
      memset((*self)->buckets, 0, (*self)->table_size * sizeof(bucket *));
      (*self)->table_mask = (*self)->table_size - 1;
      (*self)->list_head = NULL;
      (*self)->list_tail = NULL;
      (*self)->internal_pointer = NULL;
      (*self)->num_of_elements = 0;
      ret = true;
    } else {
      free(*self);
      *self = NULL;
    }
  }

failed_point:
  free(str_array[0]);
  free(str_array);
  return ret;
}

void hash_destory(hash **self)
{
  if (*self != NULL) {
    bucket *p, *q;
    p = (*self)->list_head;
    while (p != NULL) {
      q = p;
      p = p->list_next;
      if (strcmp((*self)->key_type, "char*") == 0 && q->key) {
        free(q->key);
        q->key = NULL;
      }
      if (strcmp((*self)->value_type, "char*") == 0) {
        free(*(char**)q->value);
      }
      free(q);
      q = NULL;
    }

    if ((*self)->buckets) {
      free((*self)->buckets);
      (*self)->buckets = NULL;
    }
    free(*self);
    *self = NULL;
  }
}

bool _hash_add(hash *self, ...)
{
  bool ret = false;

  if (self != NULL) {
    uint32 h;
    char *key = NULL;
    int key_len = 0;
    char value[VLEN];
    uint32 index;
    bucket *p;

    va_list vlist;
    va_start(vlist, self);
    if (strcmp(self->key_type, "int") == 0) {
      int k = va_arg(vlist, int);
      h = k;
    } else if (strcmp(self->key_type, "long") == 0) {
      long k = va_arg(vlist, long);
      h = k;
    } else if (strcmp(self->key_type, "char*") == 0) {
      char* k = va_arg(vlist, char*);
      h = hash_func(self, k);
      key = k;
      key_len = strlen(key);
    } else {
      return ret;
    }

    if (strcmp(self->value_type, "char") == 0) {
      (*value) = (char)va_arg(vlist, int);
    } else if (strcmp(self->value_type, "short") == 0) {
      (*(short*)value) = (short)va_arg(vlist, int);
    } else if(strcmp(self->value_type, "int") == 0) {
      (*(int*)value) = va_arg(vlist, int);
    } else if (strcmp(self->value_type, "long") == 0) {
      (*(long*)value) = va_arg(vlist, long);
    } else if (strcmp(self->value_type, "float") == 0) {
      (*(float*)value) = (float)va_arg(vlist, double);
    } else if (strcmp(self->value_type, "double") == 0) {
      (*(double*)value) = va_arg(vlist, double);
    } else if (strcmp(self->value_type, "char*") == 0) {
      char *tmp_str = va_arg(vlist, char*);
      char *new_str = (char *)malloc(strlen(tmp_str) + 1);
      strcpy(new_str, tmp_str);
      new_str[strlen(tmp_str)] = '\0';
      (*(char **)value) = new_str;
    } else {
      return ret;
    }

    va_end(vlist);

    index = h & self->table_mask;
    p = self->buckets[index];

    while (p!= NULL) {
      if (p->h == h) {
        if ((strcmp(self->key_type, "char*") != 0)) {
          memcpy(p->value, value, VLEN);
          ret = true;
          break;
        } else if(strcmp(p->key, key) == 0) {
          free(*(char **)p->value);
          memcpy(p->value, value, VLEN);
          ret = true;
          break;
        }
      }
      p = p->next;
    }
    if (ret == false) {
      p = (bucket *)malloc(sizeof(bucket));
      if (p == NULL) {
        if (strcmp(self->value_type, "char*") == 0) {
          free(*(char **)value);
        }
        return ret;
      }

      p->h = h;
      p->key = NULL;
      memcpy(p->value, value, VLEN);
      if (0 == strcmp(self->key_type, "char*")) {
        p->key = (char *)malloc(key_len + 1);
        memcpy(p->key, key, key_len);
        p->key[key_len] = '\0';
      }

      CONNECT_TO_BUCKET_DLLIST(p, self->buckets[index]);
      CONNECT_TO_GLOBAL_DLLIST(p, self);
      self->buckets[index] = p;
      self->num_of_elements += 1;
      ret = true;
    }
  }

  return ret;
}

bool _hash_find(hash *self, ...)
{
  bool ret = false;

  if (self != NULL) {
    uint32 h;
    char *key = NULL;
    int key_len = 0;
    uint32 index = 0;
    bucket *p = NULL;

    va_list vlist;
    va_start(vlist,self);
    if (strcmp(self->key_type, "int") == 0) {
      int k = va_arg(vlist, int);
      h = k;
    } else if (strcmp(self->key_type, "long") == 0) {
      long k = va_arg(vlist, long);
      h = k;
    } else if (strcmp(self->key_type, "char*") == 0) {
      char *k = va_arg(vlist, char*);
      h = hash_func(self, k);
      key = k;
      key_len = strlen(key);
    }
    else{
      return ret;
    }

    index = h & self->table_mask;
    p = self->buckets[index];
    while (NULL != p) {
      if (p->h == h){
        if ((strcmp(self->key_type, "char*") != 0) ||
            (strcmp(self->key_type, "char*") == 0 &&
            strcmp(p->key, key) == 0)) {
          if (strcmp(self->value_type, "char") == 0) {
            char *value = va_arg(vlist, char *);
            *value = (char)(*(p->value));
          } else if (strcmp(self->value_type, "short") == 0) {
            short *value = va_arg(vlist, short *);
            *value = *((short *)p->value);
          } else if (strcmp(self->value_type, "int") == 0) {
            int *value = va_arg(vlist, int *);
            *value = *((int *)p->value);
          } else if (strcmp(self->value_type, "long") == 0) {
            long *value = va_arg(vlist, long *);
            *value = *((long *)p->value);
          } else if (strcmp(self->value_type, "float") == 0) {
            float *value = va_arg(vlist, float *);
            *value = *((float *)p->value);
          } else if (strcmp(self->value_type, "double") == 0) {
            double *value = va_arg(vlist, double *);
            *value = *((double *)p->value);
          } else if (strcmp(self->value_type, "char*") == 0) {
            char **value = va_arg(vlist, char **);
            *value = *((char **)p->value);
          } else {
            break;
          }
          ret = true;
          break;
        }
      }
      p = p->next;
    }
    va_end(vlist);
  }
  return ret;
}

bool _hash_del(hash *self, ...)
{
  bool ret = false;

  if (self != NULL) {
    uint32 h;
    char * key = NULL;
    int key_len = 0;
    uint32 index = 0;
    bucket *p = NULL;

    va_list vlist;
    va_start(vlist, self);
    if (strcmp(self->key_type, "int") == 0) {
      int k = va_arg(vlist, int);
      h = k;
    } else if(strcmp(self->key_type, "long") == 0) {
      long k = va_arg(vlist, long);
      h = k;
    } else if (strcmp(self->key_type, "char*") == 0) {
      char *k = va_arg(vlist, char *);
      h = hash_func(self, k);
      key = k;
      key_len = strlen(key);
    } else {
      return ret;
    }
    va_end(vlist);

    index = h & self->table_mask;
    p = self->buckets[index];
    while (NULL != p) {
      if (p->h == h){
        if ((strcmp(self->key_type, "char*") != 0) ||
            (strcmp(self->key_type, "char*") == 0 &&
            strcmp(p->key, key) == 0)) {
          DECONNECT_FROM_BUCKET_DLLIST(p, self->buckets[index]);
          DECONNECT_FROM_GLOBAL_DLLIST(p, self);
          if (p->key) {
            free(p->key);
            p->key = NULL;
          }
          if (strcmp(self->value_type, "char*") == 0) {
            free(*(char **)p->value);
          }
          free(p);
          p = NULL;
          self->num_of_elements -= 1;
          ret = true;
          break;
        }
      }
      p = p->next;
    }
  }
  return ret;
}

bool _hash_exists(hash *self, ...)
{
  bool ret = false;

  if (self != NULL) {
    uint32 h;
    char * key = NULL;
    int key_len = 0;
    uint32 index = 0;
    bucket *p = NULL;

    va_list vlist;
    va_start(vlist,self);
    if (strcmp(self->key_type,"int") == 0) {
      int k = va_arg(vlist, int);
      h = k;
    } else if(strcmp(self->key_type, "long") == 0) {
      long k = va_arg(vlist, long);
      h = k;
    } else if (strcmp(self->key_type, "char*") == 0) {
      char *k = va_arg(vlist, char *);
      h = hash_func(self, k);
      key = k;
      key_len = strlen(key);
    } else {
      return ret;
    }
    va_end(vlist);

    index = h & self->table_mask;
    p = self->buckets[index];
    while (NULL != p) {
      if (p->h == h) {
        if ((strcmp(self->key_type, "char*") != 0) ||
            (strcmp(self->key_type, "char*") == 0 &&
            strcmp(p->key,key) == 0)) {
          ret = true;
          break;
        }
      }
      p = p->next;
    }
  }

  return ret;
}

uint32  hash_num_elements(hash *self)
{
  if (self != NULL) {
    return self->num_of_elements;
  } else {
    return 0;
  }

}
