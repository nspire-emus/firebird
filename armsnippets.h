/* Declarations for armsnippets.S */

#ifndef _H_ARMSNIPPETS
#define _H_ARMSNIPPETS

enum SNIPPETS {
        SNIPPET_ndls_debug_alloc, SNIPPET_ndls_debug_free
};
extern char binary_snippets_bin_start[] __asm__("_binary_snippets_bin_start");
extern char binary_snippets_bin_end[] __asm__("_binary_snippets_bin_end");
enum ARMLOADER_PARAM_TYPE {ARMLOADER_PARAM_VAL, ARMLOADER_PARAM_PTR};
struct armloader_load_params {
        enum ARMLOADER_PARAM_TYPE t;
        union {
                struct p {
                        void *ptr;
                        unsigned int size;
                } p;
                uint32_t v; // simple value
        };
};
void armloader_cb(void);
int armloader_load_snippet(enum SNIPPETS snippet, struct armloader_load_params params[], unsigned params_num, void (*callback)(struct arm_state *));

#endif
