/* Declarations for flash.c */

#ifndef _H_FLASH
#define _H_FLASH

extern bool nand_writable;
void nand_initialize(bool large);
void nand_deinitialize();
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

bool flash_open(const char *filename);
void flash_close();
void flash_save_changes();
int flash_save_as(const char *filename);
bool flash_create_new(bool large, const char **preload, int product, bool large_sdram);
void flash_read_settings(uint32_t *sdram_size);

#endif
