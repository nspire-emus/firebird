/* Declarations for flash.c */

#ifndef _H_FLASH
#define _H_FLASH

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool nand_writable;
bool nand_initialize(bool large);
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
bool flash_save_changes();
int flash_save_as(const char *filename);
bool flash_create_new(bool flag_large_nand, const char **preload_file, int product, bool large_sdram, uint8_t **nand_data_ptr, size_t *size);
bool flash_read_settings(uint32_t *sdram_size);

typedef enum BootOrder {
    ORDER_BOOT2=0,
    ORDER_DIAGS,
    ORDER_DEFAULT
} BootOrder;

void flash_set_bootorder(BootOrder order);

#ifdef __cplusplus
}
#endif

#endif
