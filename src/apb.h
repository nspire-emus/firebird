#ifndef _H_APB
#define _H_APB

struct apb_map_entry {
u32 (*read)(u32 addr);
void (*write)(u32 addr, u32 value);
};

#endif
