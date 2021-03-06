/* Declarations for armsnippets */

#ifndef _H_ARMSNIPPETS
#define _H_ARMSNIPPETS

#include <stdbool.h>
#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

enum SNIPPETS {
    SNIPPET_ndls_debug_alloc, SNIPPET_ndls_debug_free, SNIPPET_ndls_exec
};
enum ARMLOADER_PARAM_TYPE {ARMLOADER_PARAM_VAL, ARMLOADER_PARAM_PTR};
struct armloader_load_params {
    enum ARMLOADER_PARAM_TYPE t;
    union {
        struct {
            void *ptr;
            uint32_t size;
        } p;
        uint32_t v; // simple value
    };
};
void armloader_cb(void);
bool armloader_load_snippet(enum SNIPPETS snippet, struct armloader_load_params params[], unsigned params_num, void (*callback)(struct arm_state *));

#ifdef __cplusplus
}
#endif

#endif
