# Safe Array

## Goals

Provide an array interface with bounds checks for safety. This should be done using macros in order to seamlessly abstract the array interface in a way such that the bounds checks can be optimized out.

Bounds checks.

Initialization checks.

## Interface
```c
DEFARRAY(int, speeds, 20);

void main() {
  int i = ARR_GET(speeds, 2);
  ARR_SET(speeds, 2) = i;
}
```
## Implementation

### Problems

You can't initialize static variables. This means every safe array must first be initialized in a function, or only exist in function scope.

A possible solution here would be to fix non-null static variable initialization.

### Explicit variable expansion

```c
// interface as a macro
DEFARRAY(int, speeds, 20);
// becomes
int speeds[20];
bool speeds_initialized[20];
int speeds_bounds;

// what a GET would look like (except using a macro to prevent function type problems)
static inline void *ARR_GET(NAME, index) {
  if (index < 0 || index > NAME_bounds) ERROR;
  if (!NAME_initialized[index]) ERROR;
  return NAME[index];
}

void main() {
  int i = ARR_GET(speeds, 2);
  ARR_SET(speeds, 2) = i;
}
```

### Struct expansion

```c
typedef struct {
  void *arr;
  int bounds;
  bool initialized;
} array_t;

// interface as a macro
DEFARRAY(int, speeds, 20);
// becomes
int speeds_arr_tmp[20];
array_t speeds = {arr = speeds_arr_tmp, bounds = 20, initialized = false};

// what a GET would look like (except using a macro to prevent function type problems)
static inline void *ARR_GET(array_t *arr, index) {
  if (index < 0 || index > arr->bounds) ERROR;
  if (!arr->initialized) ERROR;
  return arr[index];
}

void main() {
  int i = ARR_GET(speeds, 2);
  ARR_SET(speeds, 2) = i;
}
```
