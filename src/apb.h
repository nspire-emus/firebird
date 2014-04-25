struct apb_map_entry {
u32 (*read)(u32 addr);
void (*write)(u32 addr, u32 value);
};
