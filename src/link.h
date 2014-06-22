/* Declarations for link.c */



void send_file(char *filename);



void ti84_io_link_reset(void);

uint32_t ti84_io_link_read(uint32_t addr);

void ti84_io_link_write(uint32_t addr, uint32_t value);

void *link_save_state(size_t *size);

void link_reload_state(void *state);
