#pragma once

#include "as_util.h"
#include "as_val.h"
#include "as_iterator.h"
#include <stdbool.h>
#include <inttypes.h>

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef struct as_list_s as_list;
typedef struct as_list_hooks_s as_list_hooks;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

struct as_list_s {
    as_val                  _;
    void *                  source;
    const as_list_hooks *   hooks;
};

struct as_list_hooks_s {
    int (*free)(as_list *);
    uint32_t (*hash)(as_list *);
    uint32_t (* size)(const as_list *);
    int (* append)(as_list *, as_val *);
    int (* prepend)(as_list *, as_val *);
    as_val * (* get)(const as_list *, const uint32_t);
    int (* set)(as_list *, const uint32_t, as_val *);
    as_val * (* head)(const as_list *);
    as_list * (* tail)(const as_list *);
    as_iterator * (* iterator)(const as_list *);
};

/******************************************************************************
 *
 * FUNCTION DECLARATIONS
 * 
 ******************************************************************************/

as_list * as_list_new(void *, const as_list_hooks *);

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS – VALUES
 * 
 ******************************************************************************/

inline void * as_list_source(const as_list * l) {
    return l->source;
}

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS – HOOKS
 * 
 ******************************************************************************/

inline int as_list_free(as_list * l) {
    return as_util_hook(free, 1, l);
}

inline uint32_t as_list_hash(as_list * l) {
    return as_util_hook(hash, 0, l);
}

inline uint32_t as_list_size(as_list * l) {
    return as_util_hook(size, 0, l);
}

inline int as_list_append(as_list * l, as_val * v) {
    return as_util_hook(append, 1, l, v);
}

inline int as_list_prepend(as_list * l, as_val * v) {
    return as_util_hook(prepend, 1, l, v);
}

inline as_val * as_list_get(const as_list * l, const uint32_t i) {
    return as_util_hook(get, NULL, l, i);
}

inline int as_list_set(as_list * l, const uint32_t i, as_val * v) {
    return as_util_hook(set, 1, l, i, v);
}

inline as_val * as_list_head(const as_list * l) {
    return as_util_hook(head, NULL, l);
}

inline as_list * as_list_tail(const as_list * l) {
    return as_util_hook(tail, NULL, l);
}

inline as_iterator * as_list_iterator(const as_list * l) {
    return as_util_hook(iterator, NULL, l);
}

/******************************************************************************
 *
 * INLINE FUNCTION DEFINITIONS – CONVERSIONS
 * 
 ******************************************************************************/

inline as_val * as_list_toval(const as_list * l) {
    return (as_val *) l;
}

inline as_list * as_list_fromval(const as_val * v) {
    return as_util_fromval(v, AS_LIST, as_list);
}