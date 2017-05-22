#include <basic.h>
#include <map.h>

void map_init(map_t *map, map_val_t *buf, int buf_size) {
  int i;

  map->size = 0;
  map->max_size = buf_size;
  for (i = 0; i < buf_size; i++) {
    map->values[i].key = NULL;
    map->values[i].val = NULL;
  }
}

int map_insert(map_t *map, char *key, void *val) {
  // FIXME: return status if map is full

  map->size++;
  unsigned long key_hash = hash((unsigned char *) key);
  int idx = key_hash % map->max_size;
  while (map->values[idx].key != key && map->values[idx].key != NULL) {
    idx = (idx + 1) % map->max_size;
  }
  map->values[idx].key = key;
  map->values[idx].val = val;
  return 0;
}

void *map_get(map_t *map, char *key) {
  unsigned long key_hash = hash((unsigned char *) key);
  int idx = key_hash % map->max_size;
  while (map->values[idx].key != key && map->values[idx].key != NULL) {
    idx = (idx + 1) % map->max_size;
  }
  // if the key is NULL, so is the val, so we good
  return map->values[idx].val;
}
