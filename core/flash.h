/* Declarations for flash.c */

#ifndef _H_FLASH
#define _H_FLASH

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

bool nand_initialize(bool large, const char *filename);
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

typedef enum BootOrder {
    ORDER_BOOT2=0,
    ORDER_DIAGS,
    ORDER_DEFAULT
} BootOrder;

struct nand_metrics {
    uint8_t chip_manuf, chip_model;
    uint16_t page_size;
    uint8_t log2_pages_per_block;
    uint32_t num_pages;
};

struct nand_phx{
    uint32_t operation;
    uint8_t address[7];
    uint32_t op_size;
    uint32_t ram_address;
    uint32_t ecc;
};

typedef struct nand_state {
    struct nand_metrics metrics;
    struct nand_phx phx;
    bool nand_writable, nand_block_modified[2048];
    uint32_t nand_row, nand_col;
    uint8_t nand_addr_state, nand_area_pointer;
    uint8_t nand_buffer[0x840];
    int nand_buffer_pos, state;
} nand_state;

extern nand_state nand;

typedef struct flash_snapshot {
    nand_state state;
    uint8_t nand_modified_blocks[]; // First modified block comes first, then the 2nd modified one etc.
} flash_snapshot;

bool flash_open(const char *filename);
void flash_close();
size_t flash_suspend_flexsize();

struct emu_snapshot;
bool flash_suspend(struct emu_snapshot *snapshot);
bool flash_resume(const struct emu_snapshot *snapshot);
bool flash_save_changes();
int flash_save_as(const char *filename);
bool flash_create_new(bool flag_large_nand, const char **preload_file, unsigned int product, unsigned int features, bool large_sdram, uint8_t **nand_data_ptr, size_t *size);
bool flash_read_settings(uint32_t *sdram_size, uint32_t *product, uint32_t *features, uint32_t *asic_user_flags);
void flash_set_bootorder(BootOrder order);
bool flash_create_virtual(bool flag_large_nand, const char **preload_file, unsigned int product, unsigned int features, bool large_sdram);

#ifdef __cplusplus
}

/* Give it a FILE pointing to a flash image and it'll return a string
 * that describes the HW type, such as "CX (HW J)". If reading fails,
 * it returns an empty string. */
std::string flash_read_type(FILE *flash);
#endif

#endif
