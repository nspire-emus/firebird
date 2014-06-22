/* Declarations for flash.c */



extern bool nand_writable;

void nand_initialize(bool large);

void nand_write_command_byte(uint8_t command);

void nand_write_address_byte(uint8_t byte);

uint8_t nand_read_data_byte(void);

uint32_t nand_read_data_word(void);

void nand_write_data_byte(uint8_t value);

void nand_write_data_word(uint32_t value);



void nand_phx_reset(void);

uint32_t nand_phx_read_word(uint32_t addr);

void nand_phx_write_word(uint32_t addr, uint32_t value);

uint8_t nand_phx_raw_read_byte(uint32_t addr);

void nand_phx_raw_write_byte(uint32_t addr, uint8_t value);

uint8_t nand_cx_read_byte(uint32_t addr);

uint32_t nand_cx_read_word(uint32_t addr);

void nand_cx_write_byte(uint32_t addr, uint8_t value);

void nand_cx_write_word(uint32_t addr, uint32_t value);



void flash_open(char *filename);

void flash_save_changes();

int flash_save_as(char *filename);

void flash_create_new(char **preload, int product, bool large_sdram);

void flash_read_settings(uint32_t *sdram_size);

void *flash_save_state(size_t *size);

void flash_reload_state(void *state);
