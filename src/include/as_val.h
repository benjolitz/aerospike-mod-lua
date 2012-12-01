#pragma once

#include <stdlib.h>
#include <inttypes.h>

/******************************************************************************
 *
 * TYPE DECLARATIONS
 * 
 ******************************************************************************/

typedef enum as_val_t as_val_t;

typedef struct as_val_s as_val;

/******************************************************************************
 *
 * TYPE DEFINITIONS
 * 
 ******************************************************************************/

enum as_val_t {
    AS_UNKNOWN = 0,
    AS_EMPTY,
    AS_BOOLEAN,
    AS_INTEGER,
    AS_STRING,
    AS_LIST,
    AS_MAP,
    AS_REC,
    AS_PAIR
};

struct as_val_s {
    as_val_t type;
    size_t size;
    int (*free)(as_val * v);
    uint32_t (*hash)(as_val * v);
    char * (*tostring)(as_val * v);
};

/******************************************************************************
 *
 * MACROS
 * 
 ******************************************************************************/

#define as_val_free(v) \
    (v && ((as_val *)v)->free ? ((as_val *)v)->free((as_val *)v) : 1)

#define as_val_type(v) \
    (v && ((as_val *)v)->free ? ((as_val *)v)->type : AS_UNKNOWN)

#define as_val_hash(v) \
    (v && ((as_val *)v)->hash ? ((as_val *)v)->hash((as_val *)v) : 0)

#define as_val_tostring(v) \
    (v && ((as_val *)v)->tostring ? ((as_val *)v)->tostring((as_val *)v) : NULL)

#define as_val_size(v) \
    (v ? ((as_val *)v)->size : sizeof(as_val))
