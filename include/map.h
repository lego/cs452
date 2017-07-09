#pragma once

typedef struct {
  char *key;
  void *val;
} map_val_t;

typedef struct {
  int size;
  int max_size;
  map_val_t *values;
} map_t;

void map_init(map_t *map, map_val_t *buf, int buf_size);

int map_insert(map_t *map, char *key, void *val);

void *map_get(map_t *map, char *key);
