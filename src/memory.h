/* Declarations for memory.c */

#define MEM_MAXSIZE (65*1024*1024) // also defined as RAM_FLAGS in asmcode.S

// Must be allocated below 2GB (see comments for mmu.c)
extern u8 *mem_and_flags;
struct mem_area_desc {
	u32 base, size;
	u8 *ptr;
};
extern struct mem_area_desc mem_areas[4];
void *phys_mem_ptr(u32 addr, u32 size);

/* Each word of memory has a flag word associated with it. For fast access,
 * flags are located at a constant offset from the memory data itself.
 *
 * These can't be per-byte because a translation index wouldn't fit then.
 * This does mean byte/halfword accesses have to mask off the low bits to
 * check flags, but the alternative would be another 32MB of memory overhead. */
#define RAM_FLAGS(memptr) (*(u32 *)((u8 *)(memptr) + MEM_MAXSIZE))

#define RF_READ_BREAKPOINT   1
#define RF_WRITE_BREAKPOINT  2
#define RF_EXEC_BREAKPOINT   4
#define RF_EXEC_DEBUG_NEXT   8
#define RF_EXEC_HACK         16
#define RF_CODE_TRANSLATED   32
#define RF_CODE_NO_TRANSLATE 64
#define RF_READ_ONLY         128
#define RF_ARMLOADER_CB      256
#define RFS_TRANSLATION_INDEX 9

u8 bad_read_byte(u32 addr);
u16 bad_read_half(u32 addr);
u32 bad_read_word(u32 addr);
void bad_write_byte(u32 addr, u8 value);
void bad_write_half(u32 addr, u16 value);
void bad_write_word(u32 addr, u32 value);

u32 __attribute__((fastcall)) mmio_read_byte(u32 addr) __asm__("mmio_read_byte");
u32 __attribute__((fastcall)) mmio_read_half(u32 addr) __asm__("mmio_read_half");
u32 __attribute__((fastcall)) mmio_read_word(u32 addr) __asm__("mmio_read_word");
void __attribute__((fastcall)) mmio_write_byte(u32 addr, u32 value) __asm__("mmio_write_byte");
void __attribute__((fastcall)) mmio_write_half(u32 addr, u32 value) __asm__("mmio_write_half");
void __attribute__((fastcall)) mmio_write_word(u32 addr, u32 value) __asm__("mmio_write_word");

void memory_initialize();
void *memory_save_state(size_t *size);
void memory_reload_state(void *state);
