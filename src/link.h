/* Declarations for link.c */



void send_file(char *filename);



void ti84_io_link_reset(void);

u32 ti84_io_link_read(u32 addr);

void ti84_io_link_write(u32 addr, u32 value);

void *link_save_state(size_t *size);

void link_reload_state(void *state);
