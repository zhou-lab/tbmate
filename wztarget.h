#ifndef _WZTARGET_H
#define _WZTARGET_H

#include <assert.h>
#include "wvec.h"
#include "htslib/htslib/khash.h"
KHASH_DECLARE(s2i, kh_cstr_t, int64_t)

typedef struct targets_t {
  khash_t(s2i) *dict;
} targets_t;

static inline int get_tid(targets_t *targets, const char *ss, int is_add) {
    khint_t k;
    if (targets->dict == 0) targets->dict = kh_init(s2i);
    khash_t(s2i) *d = targets->dict;
    if (is_add) {
        int absent;
        k = kh_put(s2i, d, ss, &absent);
        if (absent) {
            kh_key(d, k) = strdup(ss);
            kh_val(d, k) = kh_size(d) - 1;
        }
    } else k = kh_get(s2i, d, ss);
    return k == kh_end(d)? -1 : kh_val(d, k);
}

/* int tbx_name2id(tbx_t *tbx, const char *ss) */
/* { */
/*     return get_tid(tbx, ss, 0); */
/* } */

static inline void destroy_targets(targets_t *targets) {
  kh_destroy(s2i, targets->dict);
}

static inline const char **target_names(targets_t *targets, int *n) {

  khash_t(s2i) *d = targets->dict;
  if (d == NULL) {
    *n = 0;
    return NULL;
  }
  int tid, m = kh_size(d);
  const char **names = (const char**) calloc(m,sizeof(const char*));
  khint_t k;
  for (k=kh_begin(d); k<kh_end(d); k++)
  {
    if ( !kh_exist(d,k) ) continue;
    tid = kh_val(d,k);
    assert( tid<m );
    names[tid] = kh_key(d,k);
  }
  // sanity check: there should be no gaps
  for (tid=0; tid<m; tid++)
    assert(names[tid]);
  *n = m;
  return names;
}

#endif /* _WZTARGET_H */
