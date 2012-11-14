#include "as_rec.h"
#include <stdlib.h>
#include <stdio.h>

static const as_val AS_RECORD_VAL;
static int as_rec_freeval(as_val *);

#define LOG(m) \
    // printf("%s:%d  -- %s\n",__FILE__,__LINE__, m);

/**
 * Create a new as_rec backed by source and supported by hooks.
 *
 * @param source the source backing the as_rec.
 * @param hooks the hooks that support the as_rec.
 */
as_rec * as_rec_create(void * source, const as_rec_hooks * hooks) {
    as_rec * r = (as_rec *) malloc(sizeof(as_rec));
    r->_ = AS_RECORD_VAL;
    r->source = source;
    r->hooks = hooks;
    return r;
}

int as_rec_update(as_rec * r, void * source, const as_rec_hooks * hooks) {
    r->_ = AS_RECORD_VAL;
    r->source = source;
    r->hooks = hooks;
    return 0;
}

void * as_rec_source(const as_rec * r) {
    return r->source;
}

/**
 * Get a bin value by name.
 *
 * Proxies to `r->hooks->get(r, name, value)`
 *
 * @param r the as_rec to read the bin value from.
 * @param name the name of the bin.
 * @param a as_val containing the value in the bin.
 */
const as_val * as_rec_get(const as_rec * r, const char * name) {
    return r->hooks->get(r,name);
}

/**
 * Set the value of a bin.
 *
 * Proxies to `r->hooks->set(r, name, value)`
 *
 * @param r the as_rec to write the bin value to.
 * @param name the name of the bin.
 * @param value the value of the bin.
 */
const int as_rec_set(const as_rec * r, const char * name, const as_val * value) {
    return r->hooks->set(r,name,value);
}

/**
 * Free the as_rec.
 * This will free the as_rec object, the source and hooks.
 *
 * Proxies to `r->hooks->free(r)`
 *
 * @param r the as_rec to be freed.
 */
const int as_rec_free(as_rec * r) {
    return r->hooks->free(r);
}

as_val * as_rec_toval(const as_rec * r) {
    return (as_val *) &(r->_);
}

as_rec * as_rec_fromval(const as_val * v) {
    return as_val_type(v) == AS_REC ? (as_rec *) v : NULL;
}

static int as_rec_freeval(as_val * v) {
    return as_val_type(v) == AS_REC ? as_rec_free((as_rec *) v) : 1;
}

static const as_val AS_RECORD_VAL = {AS_REC, as_rec_freeval};
