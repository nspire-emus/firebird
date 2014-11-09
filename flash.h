/* Declarations for flash.c */

#ifndef _H_FLASH
#define _H_FLASH

extern bool nand_writable;
void nand_initialize(bool large);
void nand_write_command_byte(u8 command);
void nand_write_address_byte(u8 byte);
u8 nand_read_data_byte(void);
u32 nand_read_data_word(void);
void nand_write_data_byte(u8 value);
void nand_write_data_word(u32 value);

void nand_phx_reset(void);
u32 nand_phx_read_word(u32 addr);
void nand_phx_write_word(u32 addr, u32 value);
u8 nand_phx_raw_read_byte(u32 addr);
void nand_phx_raw_write_byte(u32 addr, u8 value);
u8 nand_cx_read_byte(u32 addr);
u32 nand_cx_read_word(u32 addr);
void nand_cx_write_byte(u32 addr, u8 value);
void nand_cx_write_word(u32 addr, u32 value);

void flash_open(const char *filename);
void flash_save_changes();
int flash_save_as(const char *filename);
void flash_create_new(const char **preload, int product, bool large_sdram);
void flash_read_settings(u32 *sdram_size);
void *flash_save_state(size_t *size);
void flash_reload_state(void *state);

#endif
