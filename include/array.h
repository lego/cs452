/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <basic.h>

#ifdef DEBUG_MODE
#include <assert.h>
#define ARRAYASSERT assert
#else
#define ARRAYASSERT(x) ((void)(x))
#endif

/*
 * Base array type (resizeable array of void pointers) and operations.
 *
 * create - allocate an array.
 * destroy - destroy an allocated array.
 * init - initialize an array in space externally allocated.
 * cleanup - clean up an array in space exteranlly allocated.
 * num - return number of elements in array.
 * get - return element no. INDEX.
 * set - set element no. INDEX to VAL.
 * setsize - change size to NUM elements; may fail and return error.
 * add - append VAL to end of array; return its index in INDEX_RET if
 *       INDEX_RET isn't null; may fail and return error.
 * remove - excise entry INDEX and slide following entries down to
 *       close the resulting gap.
 *
 * Note that expanding an array with setsize doesn't initialize the new
 * elements. (Usually the caller is about to store into them anyway.)
 */

struct array {
  void **v;
  unsigned num, max;
};

struct array *array_create(void);
void array_destroy(struct array *);
void array_init(struct array *);
void array_cleanup(struct array *);
unsigned array_num(const struct array *);
void *array_get(const struct array *, unsigned index);
void array_set(const struct array *, unsigned index, void *val);
int array_setsize(struct array *, unsigned num);
int array_add(struct array *, void *val, unsigned *index_ret);
void array_remove(struct array *, unsigned index);

/*
 * Inlining for base operations
 */

#ifndef ARRAYINLINE
#define ARRAYINLINE INLINE
#endif

ARRAYINLINE unsigned
array_num(const struct array *a)
{
  return a->num;
}

ARRAYINLINE void *
array_get(const struct array *a, unsigned index)
{
  ARRAYASSERT(index < a->num);
  return a->v[index];
}

ARRAYINLINE void
array_set(const struct array *a, unsigned index, void *val)
{
  ARRAYASSERT(index < a->num);
  a->v[index] = val;
}

ARRAYINLINE int
array_add(struct array *a, void *val, unsigned *index_ret)
{
  unsigned index;
  int ret;

  index = a->num;
  ret = array_setsize(a, index+1);
  if (ret) {
    return ret;
  }
  a->v[index] = val;
  if (index_ret != NULL) {
    *index_ret = index;
  }
  return 0;
}

#endif
