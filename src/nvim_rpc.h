#ifndef NVIM_API_H
#define NVIM_API_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  NVAPI_TYPE_NIL,
  NVAPI_TYPE_BOOLEAN,
  NVAPI_TYPE_INTEGER,
  NVAPI_TYPE_FLOAT,
  NVAPI_TYPE_STRING,
  NVAPI_TYPE_ARRAY,
  NVAPI_TYPE_DICTIONARY,
  NVAPI_TYPE_OBJECT,
  NVAPI_TYPE_BUFFER,
  NVAPI_TYPE_WINDOW,
  NVAPI_TYPE_TABPAGE,
} NvapiType;

typedef struct {
    size_t len;
    char *buf;
} NvapiString;

struct NvapiArray;
struct NvapiDictionary;
struct NvapiObject;

typedef struct {
    size_t len;
    struct NvapiObject *buf;
} NvapiArray;

typedef struct {
    size_t len;
    /* stored key1, value1, key2, value2, ... */
    struct NvapiObject *entries;
} NvapiDictionary;

typedef struct NvapiObject {
    NvapiType type;
    union {
        bool boolean;
        int32_t integer;
        double floatdata;
        NvapiString string;
        NvapiArray array;
        NvapiDictionary dictionary;
    } data;
} NvapiObject;

// For the RPC module

// TODO: implement arena for check_response, to allocate a table and link it
// accordingly; it might even be possible to calculate an upper bound on the
// necessary amount of memory from a msgpack to our unpacked struct

bool nvim_rpc_init(void);
void nvim_rpc_uninit(void);

// We just give the data at runtime, rather than making a whole C bindings
// library for neovim (there are 259 api functions!)

// TODO: this api could be simplified based on nvim's reverse response order API contract.
// We would keep a stack of calls and pop responses accordingly

// TODO: in debug mode, we can have the function calls validated against nvim
// --api-info

// TODO fix this api because its bad

/* Manages its own RPC id. This is asynchronous. */
uint32_t nvim_rpc_call(const char *func_name, NvapiArray args);
/* Checks on a response for a particular call. */
bool nvim_rpc_check_response(uint32_t id, NvapiObject *result);

typedef struct {
    char *name;
    NvapiObject args;
} NvapiEvent;

/* Emits an event. */
void nvim_rpc_emit(const char *event_name, NvapiArray args);
/* Checks for events sent by the server.  Returns true when event found. */
bool nvim_rpc_check_event(const char *event_name, NvapiArray args);

#endif // NVIM_API_H
