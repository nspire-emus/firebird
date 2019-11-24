#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "emu.h"
#include "flash.h"
#include "mem.h"
#include "cpu.h"
#include "os/os.h"

nand_state nand;
static uint8_t *nand_data = NULL;

static const struct nand_metrics chips[] = {
    { 0x20, 0x35, 0x210, 5, 0x10000 }, // ST Micro NAND256R3A
    { 0xEC, 0xA1, 0x840, 6, 0x10000 }, // Samsung 1 GBit
};

bool nand_initialize(bool large, const char *filename) {
    if(nand_data)
        nand_deinitialize();

    memcpy(&nand.metrics, &chips[large], sizeof(nand_metrics));
    nand.state = 0xFF;

    nand_data = (uint8_t*) os_map_cow(filename, large ? 132*1024*1024 : 33*1024*1024);
    if(!nand_data)
        nand_deinitialize();

    return nand_data != nullptr;
}

void nand_deinitialize()
{
    if(nand_data)
        os_unmap_cow(nand_data, (nand.metrics.num_pages == 0x840) ? 132*1024*1024 : 33*1024*1024);

    nand_data = nullptr;
}

void nand_write_command_byte(uint8_t command) {
    //printf("\n[%08X] Command %02X", arm.reg[15], command);
    switch (command) {
        case 0x01: case 0x50: // READ0, READOOB
            if (nand.metrics.page_size >= 0x800)
                goto unknown;
            // Fallthrough
        case 0x00: // READ0
            nand.nand_area_pointer = (command == 0x50) ? 2 : command;
            nand.nand_addr_state = 0;
            nand.state = 0x00;
            break;
        case 0x10: // PAGEPROG
            if (nand.state == 0x80) {
                if (!nand.nand_writable)
                    error("program with write protect on");
                uint8_t *pagedata = &nand_data[nand.nand_row * nand.metrics.page_size + nand.nand_col];
                int i;
                for (i = 0; i < nand.nand_buffer_pos; i++)
                    pagedata[i] &= nand.nand_buffer[i];
                nand.nand_block_modified[nand.nand_row >> nand.metrics.log2_pages_per_block] = true;
                nand.state = 0xFF;
            }
            break;
        case 0x30: // READSTART
            break;
        case 0x60: // ERASE1
            nand.nand_addr_state = 2;
            nand.state = command;
            break;
        case 0x80: // SEQIN
            nand.nand_buffer_pos = 0;
            nand.nand_addr_state = 0;
            nand.state = command;
            break;
        case 0xD0: // ERASE2
            if (nand.state == 0x60) {
                uint32_t block_bits = (1 << nand.metrics.log2_pages_per_block) - 1;
                if (!nand.nand_writable)
                    error("erase with write protect on");
                if (nand.nand_row & block_bits) {
                    warn("NAND flash: erase nonexistent block %x", nand.nand_row);
                    nand.nand_row &= ~block_bits; // Assume extra bits ignored like read
                }
                memset(&nand_data[nand.nand_row * nand.metrics.page_size], 0xFF,
                        nand.metrics.page_size << nand.metrics.log2_pages_per_block);
                nand.nand_block_modified[nand.nand_row >> nand.metrics.log2_pages_per_block] = true;
                nand.state = 0xFF;
            }
            break;
        case 0xFF: // RESET
            nand.nand_row = 0;
            nand.nand_col = 0;
            nand.nand_area_pointer = 0;
            // fallthrough
        case 0x70: // STATUS
        case 0x90: // READID
            nand.nand_addr_state = 6;
            nand.state = command;
            break;
        default:
unknown:
            warn("Unknown NAND command %02X", command);
    }
    //printf("State %02X row %08X column %04X\n", nand.state, nand.nand_row, nand.nand_col);
}

void nand_write_address_byte(uint8_t byte) {
    //printf(" Addr(%d)=%02X", state.nand_addr_state, byte);
    if (nand.nand_addr_state >= 6)
        return;
    switch (nand.nand_addr_state++) {
        case 0:
            if (nand.metrics.page_size < 0x800) {
                // High bits of column come from whether 00, 01, or 50 command was used
                nand.nand_col = nand.nand_area_pointer << 8;
                nand.nand_addr_state = 2;
                // Docs imply that an 01 command is only effective once
                nand.nand_area_pointer &= ~1;
            }
            nand.nand_col = (nand.nand_col & ~0xFF) | byte;
            break;
        case 1:
            nand.nand_col = (nand.nand_col & 0xFF) | (byte << 8);
            break;
        default: {
            int bit = (nand.nand_addr_state - 3) * 8;
            nand.nand_row = (nand.nand_row & ~(0xFF << bit)) | byte << bit;
            nand.nand_row &= nand.metrics.num_pages - 1;
            break;
        }
    }
}

uint8_t nand_read_data_byte() {
    //putchar('r');
    switch (nand.state) {
        case 0x00:
            if (nand.nand_col >= nand.metrics.page_size) {
                //warn("NAND read past end of page");
                return 0;
            }
            return nand_data[nand.nand_row * nand.metrics.page_size + nand.nand_col++];
        case 0x70: return 0x40 | (nand.nand_writable << 7); // Status register
        case 0x90: nand.state++; return nand.metrics.chip_manuf;
        case 0x90+1: if(nand.metrics.chip_model == 0xA1) nand.state++; else nand.state = 0xFF; return nand.metrics.chip_model;
        case 0x90+2: nand.state++; return 1; // bits per cell: SLC
        case 0x90+3: nand.state++; return 0x15; // extid: erase size: 128 KiB, page size: 2048, OOB size: 64, 8-bit
        case 0x90+4: nand.state = 0xFF; return 0;
        default:
            //warn("NAND read byte in state %02X", nand.state);
            return 0;
    }
}
uint32_t nand_read_data_word() {
    //putchar('R');
    switch (nand.state) {
        case 0x00:
            if (nand.nand_col + 4 > nand.metrics.page_size) {
                //warn("NAND read past end of page");
                return 0;
            }
            return *(uint32_t *)&nand_data[nand.nand_row * nand.metrics.page_size + (nand.nand_col += 4) - 4];
        case 0x70: return 0x40 | (nand.nand_writable << 7); // Status register
        case 0x90: nand.state = 0xFF; return nand.metrics.chip_model << 8 | nand.metrics.chip_manuf;
        default:
            //warn("NAND read word in state %02X", nand.state);
            return 0;
    }
}
void nand_write_data_byte(uint8_t value) {
    //putchar('w');
    switch (nand.state) {
        case 0x80:
            if (nand.nand_buffer_pos + nand.nand_col >= nand.metrics.page_size)
                warn("NAND write past end of page");
            else
                nand.nand_buffer[nand.nand_buffer_pos++] = value;
            return;
        default:
            warn("NAND write in state %02X", nand.state);
            return;
    }
}
void nand_write_data_word(uint32_t value) {
    //putchar('W');
    switch (nand.state) {
        case 0x80:
            if (nand.nand_buffer_pos + nand.nand_col + 4 > nand.metrics.page_size)
                warn("NAND write past end of page");
            else {
                //*(uint32_t *)&nand.nand_buffer[nand.nand_buffer_pos] = value;
                memcpy(&nand.nand_buffer[nand.nand_buffer_pos], &value, sizeof(value));
                nand.nand_buffer_pos += 4;
            }
            break;
        default:
            warn("NAND write in state %02X", nand.state);
            return;
    }
}


static uint32_t parity(uint32_t word) {
    word ^= word >> 16;
    word ^= word >> 8;
    word ^= word >> 4;
    return 0x6996 >> (word & 15) & 1;
}
static uint32_t ecc_calculate(uint8_t page[512]) {
    uint32_t ecc = 0;
    uint32_t *in = (uint32_t *)page;
    uint32_t temp[64];
    int i, j;
    uint32_t words;

    for (j = 64; j != 0; j >>= 1) {
        words = 0;
        for (i = 0; i < j; i++) {
            words ^= in[i];
            temp[i] = in[i] ^ in[i + j];
        }
        ecc = ecc << 2 | parity(words);
        in = temp;
    }

    words = temp[0];
    ecc = ecc << 2 | parity(words & 0x0000FFFF);
    ecc = ecc << 2 | parity(words & 0x00FF00FF);
    ecc = ecc << 2 | parity(words & 0x0F0F0F0F);
    ecc = ecc << 2 | parity(words & 0x33333333);
    ecc = ecc << 2 | parity(words & 0x55555555);
    return (ecc | ecc << 1) ^ (parity(words) ? 0x555555 : 0xFFFFFF);
}

void nand_phx_reset(void) {
    memset(&nand.phx, 0, sizeof nand.phx);
    nand.nand_writable = 1;
}
uint32_t nand_phx_read_word(uint32_t addr) {
    switch (addr & 0x3FFFFFF) {
        case 0x00: return 0; /* ??? */
        case 0x08: return 0; /* "Operation in progress" register */
        case 0x34: return 0x40; /* Status register (bit 0 = error, bit 6 = ready, bit 7 = writeprot */
        case 0x40: return 1; /* ??? */
        case 0x44: return nand.phx.ecc; /* Calculate page ECC */
    }
    return bad_read_word(addr);
}
void nand_phx_write_word(uint32_t addr, uint32_t value) {
    switch (addr & 0x3FFFFFF) {
        case 0x00: return;
        case 0x04: nand.nand_writable = value; return;
        case 0x08: { // Begin operation
            if (value != 1)
                error("NAND controller: wrote something other than 1 to reg 8");

            uint32_t *addr = (uint32_t *)nand.phx.address;
            logprintf(LOG_FLASH, "NAND controller: op=%06x addr=%08x size=%08x raddr=%08x\n", nand.phx.operation, *addr, nand.phx.op_size, nand.phx.ram_address);

            nand_write_command_byte(nand.phx.operation);

            uint32_t i;
            for (i = 0; i < (nand.phx.operation >> 8 & 7); i++)
                nand_write_address_byte(nand.phx.address[i]);

            if (nand.phx.operation & 0x400800) {
                uint8_t *ptr = (uint8_t*) phys_mem_ptr(nand.phx.ram_address, nand.phx.op_size);
                if (!ptr)
                    error("NAND controller: address %x is not in RAM\n", addr);

                if (nand.phx.operation & 0x000800) {
                    for (i = 0; i < nand.phx.op_size; i++)
                        nand_write_data_byte(ptr[i]);
                } else {
                    for (i = 0; i < nand.phx.op_size; i++)
                        ptr[i] = nand_read_data_byte();
                }

                if (nand.phx.op_size >= 0x200) { // XXX: what really triggers ECC?
                    // Set ECC register
                    if (!memcmp(&nand_data[0x206], "\xFF\xFF\xFF", 3))
                        nand.phx.ecc = 0xFFFFFF; // flash image created by old version of nspire_emu
                    else
                        nand.phx.ecc = ecc_calculate(ptr);
                }
            }

            if (nand.phx.operation & 0x100000) // Confirm code
                nand_write_command_byte(nand.phx.operation >> 12);
            return;
        }
        case 0x0C: nand.phx.operation = value; return;
        case 0x10: nand.phx.address[0] = value; return;
        case 0x14: nand.phx.address[1] = value; return;
        case 0x18: nand.phx.address[2] = value; return;
        case 0x1C: nand.phx.address[3] = value; return;
        case 0x20: return;
        case 0x24: nand.phx.op_size = value; return;
        case 0x28: nand.phx.ram_address = value; return;
        case 0x2C: return; /* AHB speed / 2500000 */
        case 0x30: return; /* APB speed / 250000  */
        case 0x40: return;
        case 0x44: return;
        case 0x48: return;
        case 0x4C: return;
        case 0x50: return;
        case 0x54: return;
    }
    bad_write_word(addr, value);
}

// "U-Boot" diagnostics expects to access the NAND chip directly at 0x08000000
uint8_t nand_phx_raw_read_byte(uint32_t addr) {
    if (addr == 0x08000000) return nand_read_data_byte();
    return bad_read_byte(addr);
}
void nand_phx_raw_write_byte(uint32_t addr, uint8_t value) {
    if (addr == 0x08000000) return nand_write_data_byte(value);
    if (addr == 0x08040000) return nand_write_command_byte(value);
    if (addr == 0x08080000) return nand_write_address_byte(value);
    bad_write_byte(addr, value);
}

uint8_t nand_cx_read_byte(uint32_t addr) {
    if ((addr & 0xFF180000) == 0x81080000)
        return nand_read_data_byte();
    return bad_read_byte(addr);
}
uint32_t nand_cx_read_word(uint32_t addr) {
    if ((addr & 0xFF180000) == 0x81080000)
        return nand_read_data_word();
    return bad_read_word(addr);
}
void nand_cx_write_byte(uint32_t addr, uint8_t value) {
    if ((addr & 0xFF080000) == 0x81080000) {
        nand_write_data_byte(value);

        if (addr & 0x100000)
            nand_write_command_byte(addr >> 11);
        return;
    }
    bad_write_byte(addr, value);
}
void nand_cx_write_word(uint32_t addr, uint32_t value) {
    static int addr_bytes_remaining = 0;
    if (addr >= 0x81000000 && addr < 0x82000000) {
        if (addr & 0x080000) {
            if(!(addr & (1 << 21)))
                warn("Doesn't work on HW");
            nand_write_data_word(value);
        } else {
            int addr_bytes = addr >> 21 & 7;
            if(addr_bytes_remaining)
            {
                addr_bytes = addr_bytes_remaining;
                addr_bytes_remaining = 0;
            }
            if(addr_bytes > 4)
            {
                addr_bytes_remaining = addr_bytes - 4;
                addr_bytes = 4;
            }
            nand_write_command_byte(addr >> 3);
            for (; addr_bytes != 0; addr_bytes--) {
                nand_write_address_byte(value);
                value >>= 8;
            }
        }

        if (addr & 0x100000)
            nand_write_command_byte(addr >> 11);
        return;
    }
    bad_write_word(addr, value);
}

FILE *flash_file = NULL;

typedef enum Partition {
    PartitionManuf=0,
    PartitionBoot2,
    PartitionBootdata,
    PartitionDiags,
    PartitionFilesystem
} Partition;

// Returns offset into nand_data
size_t flash_partition_offset(Partition p, struct nand_metrics *nand_metrics, uint8_t *nand_data)
{
    static size_t offset_classic[] = { 0, 0x4200, 0x15a800, 0x16b000, 0x210000 };

    if(nand_metrics->page_size < 0x800)
        return offset_classic[p];

    static size_t parttable_cx[] = { 0x870, 0x874, 0x86c, 0x878 };
    if(p == PartitionManuf)
        return 0;

    return (*(uint32_t*)(nand_data + parttable_cx[p-1]))/0x800 * 0x840;
}

bool flash_open(const char *filename) {
    bool large = false;
    if(flash_file)
        fclose(flash_file);

    flash_file = fopen_utf8(filename, "r+b");

    if (!flash_file) {
        gui_perror(filename);
        return false;
    }
    fseek(flash_file, 0, SEEK_END);
    uint32_t size = ftell(flash_file);

    if (size == 33*1024*1024)
        large = false;
    else if (size == 132*1024*1024)
        large = true;
    else {
        emuprintf("%s not a flash image (wrong size)\n", filename);
        return false;
    }

    if(!nand_initialize(large, filename))
    {
        fclose(flash_file);
        flash_file = NULL;
        emuprintf("Could not read flash image from %s\n", filename);
        return false;
    }

    return true;
}

bool flash_save_changes() {
    if (flash_file == NULL) {
        gui_status_printf("No flash loaded!");
        return false;
    }
    uint32_t block, count = 0;
    uint32_t block_size = nand.metrics.page_size << nand.metrics.log2_pages_per_block;
    for (block = 0; block < nand.metrics.num_pages; block += 1 << nand.metrics.log2_pages_per_block) {
        if (nand.nand_block_modified[block >> nand.metrics.log2_pages_per_block]) {
            fseek(flash_file, block * nand.metrics.page_size, SEEK_SET);
            fwrite(&nand_data[block * nand.metrics.page_size], block_size, 1, flash_file);
            nand.nand_block_modified[block >> nand.metrics.log2_pages_per_block] = false;
            count++;
        }
    }
    fflush(flash_file);
    gui_status_printf("Flash: Saved %d modified blocks", count);
    return true;
}

int flash_save_as(const char *filename) {
    FILE *f = fopen_utf8(filename, "wb");
    if (!f) {
        emuprintf("NAND flash: could not open ");
        gui_perror(filename);
        return 1;
    }
    emuprintf("Saving flash image %s...", filename);
    if (!fwrite(nand_data, nand.metrics.page_size * nand.metrics.num_pages, 1, f) || fflush(f)) {
        fclose(f);
        f = NULL;
        remove(filename);
        printf("\n could not write to ");
        gui_perror(filename);
        return 1;
    }
    memset(nand.nand_block_modified, 0, nand.metrics.num_pages >> nand.metrics.log2_pages_per_block);
    if (flash_file)
        fclose(flash_file);

    flash_file = f;
    printf("done\n");
    return 0;
}

static void ecc_fix(uint8_t *nand_data, struct nand_metrics nand_metrics, int page) {
    uint8_t *data = &nand_data[page * nand_metrics.page_size];
    if (nand_metrics.page_size < 0x800) {
        uint32_t ecc = ecc_calculate(data);
        data[0x206] = ecc >> 6;
        data[0x207] = ecc >> 14;
        data[0x208] = ecc >> 22 | ecc << 2;
    } else {
        int i;
        for (i = 0; i < 4; i++) {
            uint32_t ecc = ecc_calculate(&data[i * 0x200]);
            data[0x808 + i * 0x10] = ecc >> 6;
            data[0x809 + i * 0x10] = ecc >> 14;
            data[0x80A + i * 0x10] = ecc >> 22 | ecc << 2;
        }
    }
}

static uint32_t load_file_part(uint8_t *nand_data, struct nand_metrics nand_metrics, uint32_t offset, FILE *f, uint32_t length) {
    uint32_t start = offset;
    uint32_t page_data_size = (nand_metrics.page_size & ~0x7F);
    while (length > 0) {
        uint32_t page = offset / page_data_size;
        uint32_t pageoff = offset % page_data_size;
        if (page >= nand_metrics.num_pages) {
            printf("Preload image(s) too large\n");
            return 0;
        }

        uint32_t readsize = page_data_size - pageoff;
        if (readsize > length) readsize = length;

        int ret = fread(&nand_data[page * nand_metrics.page_size + pageoff], 1, readsize, f);
        if (ret <= 0)
            break;
        readsize = ret;
        ecc_fix(nand_data, nand_metrics, page);
        offset += readsize;
        length -= readsize;
    }
    return offset - start;
}

static uint32_t load_file(uint8_t *nand_data, struct nand_metrics nand_metrics, Partition p, const char *filename, size_t off) {
    FILE *f = fopen_utf8(filename, "rb");
    if (!f) {
        gui_perror(filename);
        return 0;
    }
    size_t offset = flash_partition_offset(p, &nand_metrics, nand_data);
    offset /= nand_metrics.page_size;
    offset *= nand_metrics.page_size & ~0x7F; // Convert offset into offset without spare bytes
    offset += off;
    uint32_t size = load_file_part(nand_data, nand_metrics, offset, f, -1);
    fclose(f);
    return size;
}

// Broken.
/*static uint32_t load_zip_entry(uint8_t *nand_data, struct nand_metrics nand_metrics, uint32_t offset, FILE *f, char *name) {
    struct __attribute__((packed)) {
        uint32_t sig;
        uint16_t version;
        uint16_t flags;
        uint16_t method;
        uint32_t datetime;
        uint32_t crc;
        uint32_t comp_size, uncomp_size;
        uint16_t name_length, extra_length;
    } zip_entry;

    fseek(f, 0, SEEK_SET);
    while (fread(&zip_entry, sizeof(zip_entry), 1, f) == 1 && zip_entry.sig == 0x04034B50) {
        char namebuf[16];
        if (zip_entry.name_length >= sizeof namebuf) {
            fseek(f, zip_entry.name_length + zip_entry.extra_length + zip_entry.comp_size, SEEK_CUR);
            continue;
        }
        if (fread(namebuf, zip_entry.name_length, 1, f) != 1)
            break;
        namebuf[zip_entry.name_length] = '\0';
        //TODO: if (!stricmp(namebuf, name)) {
        if (strcasecmp(namebuf, name)) {
            fseek(f, zip_entry.extra_length, SEEK_CUR);
            return load_file_part(nand_data, nand_metrics, offset, f, zip_entry.comp_size);
        }
        fseek(f, zip_entry.extra_length + zip_entry.comp_size, SEEK_CUR);
    }
    fprintf(stderr, "Could not locate %s in CAS+ OS file\n", name);
    return 0;
}*/

static void preload(uint8_t *nand_data, struct nand_metrics nand_metrics, Partition p, const char *name, const char *filename) {
    uint32_t page = flash_partition_offset(p, &nand_metrics, nand_data) / nand_metrics.page_size;
    uint32_t manifest_size, image_size;

    if (emulate_casplus && strcmp(name, "IMAGE") == 0) {
        assert(false); // load_zip_entry doesn't have "offset" anymore
        __builtin_unreachable();
        /*FILE *f = fopen(filename, "rb");
        if (!f) {
            gui_perror(filename);
            return false;
        }
        manifest_size = load_zip_entry(nand_data, nand_metrics, offset+32, f, "manifest_img");
        offset += manifest_size;
        image_size = load_zip_entry(nand_data, nand_metrics, offset+32, f, "phoenix.img");
        offset += image_size;
        fclose(f);
        if(!manifest_size || !image_size)
            return false;*/
    } else {
        manifest_size = 0;
        image_size = load_file(nand_data, nand_metrics, p, filename, 32);
        if(!image_size)
            return;
    }

    uint8_t *pagep = &nand_data[page * nand_metrics.page_size];
    sprintf((char *)&pagep[0], "***PRELOAD_%s***", name);
    *(uint32_t *)&pagep[20] = BSWAP32(0x55F00155);
    *(uint32_t *)&pagep[24] = BSWAP32(manifest_size);
    *(uint32_t *)&pagep[28] = BSWAP32(image_size);
    ecc_fix(nand_data, nand_metrics, page);
}

struct manuf_data_804 {
    uint16_t product;
    uint16_t revision;
    char locale[8];
    char _unknown_810[8];
    struct manuf_data_ext {
        uint32_t signature;
        uint32_t features;
        uint32_t default_keypad;
        uint16_t lcd_width;
        uint16_t lcd_height;
        uint16_t lcd_bpp;
        uint16_t lcd_color;
        uint32_t offset_diags;
        uint32_t offset_boot2;
        uint32_t offset_bootdata;
        uint32_t offset_filesys;
        uint32_t config_clocks;
        uint32_t config_sdram;
        uint32_t lcd_spi_count;
        uint32_t lcd_spi_data[8][2];
        uint16_t lcd_light_min;
        uint16_t lcd_light_max;
        uint16_t lcd_light_default;
        uint16_t lcd_light_incr;
    } ext;
    uint8_t bootgfx_count;
    uint8_t bootgfx_iscompressed;
    uint16_t bootgfx_unknown;
    struct {
        uint16_t pos_y;
        uint16_t pos_x;
        uint16_t width;
        uint16_t height;
        uint32_t offset;
    } bootgfx_images[12];
    uint32_t bootgfx_compsize;
    uint32_t bootgfx_rawsize;
    uint32_t bootgfx_certsize;
};

static uint8_t bootdata[] = {
    0xAA, 0xC6, 0x8C, 0x92, // Signature
    0x00, 0x00, 0x00, 0x00, // Minimum OS version
    0x00, 0x00, 0x00, 0x00, // PTT status 1
    0x00, 0x00, 0x00, 0x00, // PTT status 2
    0x00, 0x00, 0x00, 0x00, // Boot diags?
    // Stuff we don't care about
};

bool flash_create_new(bool flag_large_nand, const char **preload_file, unsigned int product, unsigned int features, bool large_sdram, uint8_t **nand_data_ptr, size_t *size) {
    assert(nand_data_ptr);
    assert(size);

    struct nand_metrics nand_metrics;
    memcpy(&nand_metrics, &chips[flag_large_nand], sizeof(nand_metrics));

    *size = nand_metrics.page_size * nand_metrics.num_pages;
    uint8_t *nand_data = *nand_data_ptr = (uint8_t*) malloc(*size);
    if(!nand_data)
        return false;

    memset(nand_data, 0xFF, *size);

    if (preload_file[0]) {
        load_file(nand_data, nand_metrics, PartitionManuf, preload_file[0], 0);

        // Overwrite some values to match the configuration
        struct manuf_data_804 *manuf = (struct manuf_data_804 *)&nand_data[0x844];
        manuf->product = product >> 4;
        manuf->revision = product & 0xF;
        if (product >= 0x0F)
            manuf->ext.features = features;
        ecc_fix(nand_data, nand_metrics, nand_metrics.page_size < 0x800 ? 4 : 1);
    } else if (!emulate_casplus) {
        *(uint32_t *)&nand_data[0] = 0x796EB03C;
        ecc_fix(nand_data, nand_metrics, 0);

        struct manuf_data_804 *manuf = (struct manuf_data_804 *)&nand_data[0x844];
        manuf->product = product >> 4;
        manuf->revision = product & 0xF;
        if (manuf->product >= 0x0F) {
            manuf->ext.signature = 0x4C9E5F91;
            manuf->ext.features = features;
            manuf->ext.default_keypad = 76; // Touchpad
            manuf->ext.lcd_width = 320;
            manuf->ext.lcd_height = 240;
            manuf->ext.lcd_bpp = 16;
            manuf->ext.lcd_color = 1;
            if (nand_metrics.page_size < 0x800) {
                manuf->ext.offset_diags    = 0x160000;
                manuf->ext.offset_boot2    = 0x004000;
                manuf->ext.offset_bootdata = 0x150000;
                manuf->ext.offset_filesys  = 0x200000;
            } else {
                manuf->ext.offset_diags    = 0x320000;
                manuf->ext.offset_boot2    = 0x020000;
                manuf->ext.offset_bootdata = 0x2C0000;
                manuf->ext.offset_filesys  = 0x400000;
            }
            manuf->ext.config_clocks = 0x561002; // 132 MHz
            manuf->ext.config_sdram = large_sdram ? 0xFC018012 : 0xFE018011;
            manuf->ext.lcd_spi_count = 0;
            manuf->ext.lcd_light_min = 0x11A;
            manuf->ext.lcd_light_max = 0x1CE;
            manuf->ext.lcd_light_default = 0x16A;
            manuf->ext.lcd_light_incr = 0x14;
            manuf->bootgfx_count = 0;
        }
        ecc_fix(nand_data, nand_metrics, nand_metrics.page_size < 0x800 ? 4 : 1);
    }

    if (preload_file[1]) load_file(nand_data, nand_metrics, PartitionBoot2, preload_file[1], 0); // Boot2 area
    size_t bootdata_offset = flash_partition_offset(PartitionBootdata, &nand_metrics, nand_data); // Bootdata
    memset(nand_data + bootdata_offset, 0xFF, nand_metrics.page_size);
    memset(nand_data + bootdata_offset + 0x62, 0, 414);
    memcpy(nand_data + bootdata_offset, bootdata, sizeof(bootdata));
    if (preload_file[2]) load_file(nand_data, nand_metrics, PartitionDiags, preload_file[2], 0); // Diags area
    if (preload_file[3]) preload(nand_data, nand_metrics, PartitionFilesystem, "IMAGE", preload_file[3]); // Filesystem/OS

    return true;
}

bool flash_read_settings(uint32_t *sdram_size, uint32_t *product, uint32_t *features, uint32_t *asic_user_flags) {
    assert(nand_data);

    *sdram_size = 32 * 1024 * 1024;
    *features = 0;
    *asic_user_flags = 0;

    if (*(uint32_t *)&nand_data[0] == 0xFFFFFFFF) {
        // No manuf data = CAS+
        *product = 0x0C0;
        return true;
    }

    if((*(uint16_t *)&nand_data[0] & 0xF0FF) == 0x0050) {
        // TODO: Parse new manuf
        // CX II CAS
        *sdram_size = 64 * 1024 * 1024;
        *features = 0;
        *asic_user_flags = 0;
        *product = 0x1C0;
        return true;
    }

    struct manuf_data_804 *manuf = (struct manuf_data_804 *)&nand_data[0x844];
    *product = manuf->product << 4 | manuf->revision;

    static const unsigned char flags[] = { 1, 0, 0, 1, 0, 3, 2 };
    if (manuf->product >= 0x0C && manuf->product <= 0x12)
        *asic_user_flags = flags[manuf->product - 0x0C];

    if (*product >= 0x0F0 && manuf->ext.signature == 0x4C9E5F91) {
        uint32_t cfg = manuf->ext.config_sdram;
        int logsize = (cfg & 7) + (cfg >> 3 & 7);
        if (logsize > 4) {
            emuprintf("Invalid SDRAM size in flash\n");
            return false;
        }
        *features = manuf->ext.features;
        *sdram_size = (4 * 1024 * 1024) << logsize;
    }

    return true;
}

std::string flash_read_type(FILE *flash)
{
    uint32_t i;
    struct manuf_data_804 manuf;
    if(fread(&i, sizeof(i), 1, flash) != 1)
        return "";

    if(i == 0xFFFFFFFF)
        return "CAS+";

    if((i & 0xF0FF) == 0x0050)
        return "CX II something"; // TODO: Parse new manuf

    if((fseek(flash, 0x844 - sizeof(i), SEEK_CUR) != 0)
    || (fread(&manuf, sizeof(manuf), 1, flash) != 1))
        return "";

    std::string ret;
    switch(manuf.product)
    {
    case 0x0C:
        if(manuf.revision < 2)
            ret = "Clickpad CAS";
        else
            ret = "Touchpad CAS";

        break;
    case 0x0D:
        ret = "Lab Cradle";
        break;
    case 0x0E:
        ret = "Touchpad";
        break;
    case 0x0F:
        ret = "CX CAS";
        break;
    case 0x10:
        ret = "CX";
        break;
    case 0x11:
        ret = "CM CAS";
        break;
    case 0x12:
        ret = "CM";
        break;
    default:
        ret = "???";
        break;
    }

    if(manuf.product >= 0x0F)
    {
        ret += " (HW ";
        switch(manuf.ext.features)
        {
        case 0x05:
            ret += "A)";
            break;
        case 0x85:
            ret += "J)";
            break;
        case 0x185:
            ret += "W)";
            break;
        default:
            ret += "?)";
            break;
        }
    }

    return ret;
}

size_t flash_suspend_flexsize()
{
    const size_t num_blocks = nand.metrics.num_pages >> nand.metrics.log2_pages_per_block,
            block_size = nand.metrics.page_size << nand.metrics.log2_pages_per_block;

    size_t count_modified_blocks = std::count_if(nand.nand_block_modified, nand.nand_block_modified + num_blocks, [](uint8_t i) { return i; });

    return block_size * count_modified_blocks;
}

bool flash_suspend(emu_snapshot *snapshot)
{
    flash_snapshot *flash = &snapshot->flash;

    flash->state = nand;

    uint8_t *cur_modified_block = flash->nand_modified_blocks;
    const size_t num_blocks = nand.metrics.num_pages >> nand.metrics.log2_pages_per_block,
            block_size = nand.metrics.page_size << nand.metrics.log2_pages_per_block;

    for(unsigned int cur_modified_block_nr = 0; cur_modified_block_nr < num_blocks; ++cur_modified_block_nr)
    {
        if(!nand.nand_block_modified[cur_modified_block_nr])
            continue;

        memcpy(cur_modified_block, nand_data + block_size * cur_modified_block_nr, block_size);
        cur_modified_block += block_size;
    }

    return true;
}

bool flash_resume(const emu_snapshot *snapshot)
{
    const flash_snapshot *flash = &snapshot->flash;

    flash_close();

    if(!flash_open(snapshot->path_flash))
        return false;

    nand = flash->state;

    const uint8_t *cur_modified_block = flash->nand_modified_blocks;
    const size_t num_blocks = nand.metrics.num_pages >> nand.metrics.log2_pages_per_block,
            block_size = nand.metrics.page_size << nand.metrics.log2_pages_per_block;

    for(unsigned int cur_modified_block_nr = 0; cur_modified_block_nr < num_blocks; ++cur_modified_block_nr)
    {
        if(!nand.nand_block_modified[cur_modified_block_nr])
            continue;

        memcpy(nand_data + block_size * cur_modified_block_nr, cur_modified_block, block_size);
        cur_modified_block += block_size;
    }

    return true;
}

void flash_close()
{
    if(flash_file)
    {
        fclose(flash_file);
        flash_file = NULL;
    }

    nand_deinitialize();
}

void flash_set_bootorder(BootOrder order)
{
    assert(nand_data);

    if(order == ORDER_DEFAULT)
        return;

    // CX II
    if((*(uint16_t *)&nand_data[0] & 0xF0FF) == 0x0050)
        return;

    size_t bootdata_offset = flash_partition_offset(PartitionBootdata, &nand.metrics, nand_data);

    if(*(uint32_t*)(nand_data + bootdata_offset) != 0x928cc6aa)
    {
        // No bootdata yet
        memset(nand_data + bootdata_offset, 0xFF, nand.metrics.page_size);
        memset(nand_data + bootdata_offset + 0x62, 0, 414);
        memcpy(nand_data + bootdata_offset, bootdata, sizeof(bootdata));
    }

    // Loop through all bootdata pages with signature
    while(*(uint32_t*)(nand_data + bootdata_offset) == 0x928cc6aa)
    {
        *(uint32_t*)(nand_data + bootdata_offset + 0x10) = order;
        unsigned int page = bootdata_offset / nand.metrics.page_size;
        nand.nand_block_modified[page >> nand.metrics.log2_pages_per_block] = true;
        ecc_fix(nand_data, nand.metrics, page);
        bootdata_offset += nand.metrics.page_size;
    }
}

enum class FlashSPICmd : uint8_t {
    GET_FEATURES        = 0x0F,
    SET_FEATURES        = 0x1F,
    READ_FROM_CACHE     = 0x0B,
    READ_FROM_CACHE_x4  = 0x6B,
    PROGRAM_EXECUTE     = 0x10,
    READ_PAGE           = 0x13,
    BLOCK_ERASE         = 0xD8,
    PROGRAM_LOAD        = 0x02,
    PROGRAM_LOAD_x4     = 0x32,
    PROGRAM_LOAD_RANDOM_DATA = 0x84,
    PROGRAM_LOAD_RANDOM_DATA_x4 = 0x34,
    WRITE_DISABLE       = 0x04,
    WRITE_ENABLE        = 0x06,
};

struct flash_param_page_struct {
    flash_param_page_struct() :
        signature{'O', 'N', 'F', 'I'},
        optional_commands{6},
        manufacturer{'M','I','C','R','O','N',' ',' ',' ',' ',' ',' '},
        model{'M','T','2','9','F','1','G','0','1','A','A','A','D','D','H','4',' ',' ',' ',' '},
        manuf_id{0x2C},
        page_data_size{2048},
        page_spare_size{64},
        partial_page_data_size{512},
        partial_page_spare_size{16},
        pages_per_block{64},
        blocks_per_unit{1024},
        count_logical_units{1},
        bits_per_cell{1},
        bad_blocks_per_unit_max{20},
        block_endurance{0x501},
        programs_per_page{4},
        pin_capacitance{10},
        time_max_prog{900},
        time_max_erase{10000},
        time_max_read{100},
        rev_vendor{1}
    {}

    char        signature[4];
    uint16_t    revision;
    uint16_t    features;
    uint16_t    optional_commands;
    char        reserved0[22];
    char        manufacturer[12];
    char        model[20];
    uint8_t     manuf_id;
    uint16_t    date_code;
    uint8_t     reserved1[13];
    uint32_t    page_data_size;
    uint16_t    page_spare_size;
    uint32_t    partial_page_data_size;
    uint16_t    partial_page_spare_size;
    uint32_t    pages_per_block;
    uint32_t    blocks_per_unit;
    uint8_t     count_logical_units;
    uint8_t     address_cycles;
    uint8_t     bits_per_cell;
    uint16_t    bad_blocks_per_unit_max;
    uint16_t    block_endurance;
    uint8_t     guaranteed_valid_blocks;
    uint8_t     programs_per_page;
    uint8_t     toolazytotype[17];
    uint8_t     pin_capacitance;
    uint16_t    timing[2];
    uint16_t    time_max_prog;
    uint16_t    time_max_erase;
    uint16_t    time_max_read;
    uint8_t     toolazy[27];
    uint16_t    rev_vendor;
    uint8_t     vendor_data[88];
    uint16_t    crc;
} __attribute__((packed));

static const flash_param_page_struct param_page_micron{};

void flash_spi_reset()
{
    memset(&nand.spi, 0, sizeof(nand.spi));
    memset(nand.nand_buffer, 0xFF, sizeof(nand.nand_buffer));
    assert(sizeof(param_page_micron) == 256);
}

static uint8_t flash_spi_transceive(uint8_t data = 0x00)
{
    uint8_t ret = 0;
    switch(nand.state)
    {
    case SPI_COMMAND: // Command cycle
        nand.spi.command = data;

        nand.spi.address = 0;
        nand.nand_addr_state = 0;
        nand.spi.address_cycles_total = 0;
        nand.spi.dummy_cycles_remaining = 0;

        switch(FlashSPICmd(nand.spi.command))
        {
        case FlashSPICmd::GET_FEATURES:
        case FlashSPICmd::SET_FEATURES:
            nand.spi.address_cycles_total = 1;
            break;
        case FlashSPICmd::READ_FROM_CACHE:
        case FlashSPICmd::READ_FROM_CACHE_x4:
            nand.spi.address_cycles_total = 2;
            nand.spi.dummy_cycles_remaining = 1;
            break;
        case FlashSPICmd::PROGRAM_EXECUTE:
        case FlashSPICmd::READ_PAGE:
        case FlashSPICmd::BLOCK_ERASE:
            nand.spi.address_cycles_total = 3;
            break;
        case FlashSPICmd::PROGRAM_LOAD:
        case FlashSPICmd::PROGRAM_LOAD_x4:
            memset(nand.nand_buffer, 0xFF, sizeof(nand.nand_buffer));
            /* fallthrough */
        case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA:
        case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA_x4:
            nand.spi.address_cycles_total = 2;
            break;
        case FlashSPICmd::WRITE_DISABLE:
            nand.nand_writable = false;
            break;
        case FlashSPICmd::WRITE_ENABLE:
            nand.nand_writable = true;
            break;
        default:
            warn("Unknown flash SPI command %x", data);
        }

        nand.state = SPI_ADDRESS;
        break;
    case SPI_ADDRESS: // Address cycles
        nand.spi.address |= data << (nand.nand_addr_state * 8);
        nand.nand_addr_state += 1;

        if(nand.nand_addr_state == nand.spi.address_cycles_total)
        {
            nand.state = nand.spi.dummy_cycles_remaining ? SPI_DUMMY : SPI_DATA;
            switch(FlashSPICmd(nand.spi.command))
            {
            case FlashSPICmd::READ_PAGE:
            {
                auto page_size = param_page_micron.page_data_size + param_page_micron.page_spare_size;
                memcpy(nand.nand_buffer, &nand_data[nand.spi.address * page_size], page_size);
                break;
            }
            case FlashSPICmd::BLOCK_ERASE:
            {
                if(!nand.nand_writable)
                    break;

                auto page_size = param_page_micron.page_data_size + param_page_micron.page_spare_size;
                auto block_size = param_page_micron.pages_per_block * page_size;
                auto block_number = nand.spi.address / param_page_micron.pages_per_block;
                memset(nand_data + (block_number * block_size), 0xFF, block_size);
                nand.nand_block_modified[block_number] = true;
                break;
            }
            case FlashSPICmd::PROGRAM_EXECUTE:
            {
                if(!nand.nand_writable)
                    break;

                auto page_size = param_page_micron.page_data_size + param_page_micron.page_spare_size;
                auto page_pointer = &nand_data[page_size * nand.spi.address];
                for(size_t i = 0; i < page_size; ++i)
                    page_pointer[i] &= nand.nand_buffer[i];

                nand.nand_block_modified[nand.spi.address / param_page_micron.pages_per_block] = true;
                break;
            }
            case FlashSPICmd::READ_FROM_CACHE:
            case FlashSPICmd::READ_FROM_CACHE_x4:
            case FlashSPICmd::PROGRAM_LOAD:
            case FlashSPICmd::PROGRAM_LOAD_x4:
            case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA:
            case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA_x4:
                nand.spi.address &= ~0x1000; // Ignore the plane bit
                break;
            case FlashSPICmd::GET_FEATURES:
            case FlashSPICmd::SET_FEATURES:
                break;
            default:
                warn("Unhandled?");
            }
        }

        break;
    case SPI_DUMMY: // Dummy cycles
        if(--nand.spi.dummy_cycles_remaining == 0)
            nand.state = SPI_DATA;
        break;
    case SPI_DATA: // Data cycles
        switch(FlashSPICmd(nand.spi.command))
        {
        case FlashSPICmd::PROGRAM_LOAD:
        case FlashSPICmd::PROGRAM_LOAD_x4:
        case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA:
        case FlashSPICmd::PROGRAM_LOAD_RANDOM_DATA_x4:
            if(nand.spi.address < sizeof(nand.nand_buffer))
                nand.nand_buffer[nand.spi.address] = data;
            /* The OS does that all the time - writing 128 bytes into a 64 OOB area :-/
             * They are never read back though, so ignoring should be fine.
             * else
             *  warn("Write past end of page\n");
             */

            break;
        case FlashSPICmd::GET_FEATURES:
            switch(nand.spi.address)
            {
            case 0xA0: // Block lock
            case 0xB0: // OTP
                ret = 0;
                break;
            case 0xC0: // Status
                ret = nand.nand_writable << 1;
                break;
            default:
                warn("Unknown status register %x\n", nand.spi.address);
            }
            break;
        case FlashSPICmd::READ_FROM_CACHE:
        case FlashSPICmd::READ_FROM_CACHE_x4:
            if(nand.spi.param_page_active)
            {
                auto param_page_raw = reinterpret_cast<const uint8_t*>(&param_page_micron);
                ret = param_page_raw[nand.spi.address & 0xFF];
            }
            else
            {
                if(nand.spi.address < sizeof(nand.nand_buffer))
                    ret = nand.nand_buffer[nand.spi.address];
                else
                    warn("Read past end of page\n");
            }
            break;
        case FlashSPICmd::SET_FEATURES:
            if(nand.spi.address == 0xb0 && data == 0x0)
                nand.spi.param_page_active = false;
            else if(nand.spi.address == 0xb0 && data == 0x40)
                nand.spi.param_page_active = true;
            else
                warn("Unknown SET FEATURE request %x at %x", data, nand.spi.address);
            break;
        default:
            warn("Data cycle with unknown command");
            break;
        }

        nand.spi.address += 1;
        break;
    }

    return ret;
}

static void flash_spi_cs(bool enabled)
{
    if(enabled)
        return;

    if(nand.nand_addr_state < nand.spi.address_cycles_total
       || nand.spi.dummy_cycles_remaining)
        warn("CS disabled before command complete");

    nand.state = SPI_COMMAND;
}

struct nand_cx2_state nand_cx2_state;

/* SPI multiplexing - depending on the currently active chip select line,
 * the SPI transmissions end up in different places. */
static uint8_t spinand_cx2_transceive(uint8_t data=0x00)
{
    switch(nand_cx2_state.active_cs)
    {
    case 1: // NAND
        return flash_spi_transceive(data);
    case 0: // Not connected
    case 2:
    case 3:
        return 0;
    default:
        warn("Unknown chip select %d\n", nand_cx2_state.active_cs);
        return 0;
    case 0xFF:
        warn("Transmission without chip select active\n");
        return 0;
    }
}

static void spinand_cx2_set_cs(uint8_t cs, bool state)
{
    nand_cx2_state.active_cs = state ? cs : 0xFF;

    switch(cs)
    {
    case 1: // NAND
        return flash_spi_cs(state);
    case 0: // Not connected
    case 2:
    case 3:
        return;
    default:
        warn("Unknown chip select %d\n", cs);
        return;
    }
}

uint32_t spinand_cx2_read_word(uint32_t addr)
{
	switch(addr & 0xFFFF)
	{
    case 0x000: // REG_CMD0: Address
        return nand_cx2_state.addr;
    case 0x004: // REG_CMD1: No. of cmd, addr and dummy cycles
        return nand_cx2_state.cycl;
    case 0x008: // REG_CMD2: No. of data bytes
        return nand_cx2_state.len;
    case 0x00c: // REG_CMD3: Command
        return nand_cx2_state.cmd;
    case 0x010: // REG_CTRL: Control
        return nand_cx2_state.ctrl;
    case 0x018: // REG_STR: Status
        return 0b11; // Data available, FIFO not full
    case 0x020: // REG_ICR: Interrupt control
        return nand_cx2_state.icr;
    case 0x024: // REG_ISR: Interrupt status
        return nand_cx2_state.isr;
    case 0x028: // REG_RDST: Status register
        return nand_cx2_state.rdsr;
    case 0x054: // REG_FEA: Features
        return 0x02022020;
    case 0x100: // REG_DATA: Data words
    {
        if(nand_cx2_state.cmd & 0x2)
            return 0; // Write only command

        uint32_t data = 0;
        for(int i = 0; i < 4 && nand_cx2_state.len_cur; i++, nand_cx2_state.len_cur--)
            data |= spinand_cx2_transceive(0) << (i << 3);

        if(!nand_cx2_state.len_cur)
        {
            spinand_cx2_set_cs(nand_cx2_state.active_cs, false);
            nand_cx2_state.isr |= 1;
            //int_set(INT_SPINAND, nand_cx2_state.isr & nand_cx2_state.icr);
        }
        return data;
    }
	}

    return bad_read_word(addr);
}

uint8_t spinand_cx2_read_byte(uint32_t addr)
{
    return spinand_cx2_read_word(addr);
}

void spinand_cx2_write_word(uint32_t addr, uint32_t value)
{
    switch(addr & 0xFFFF)
    {
    case 0x000: // REG_CMD0: Address
        nand_cx2_state.addr = value;
        return;
    case 0x004: // REG_CMD1: No. of cmd, addr and dummy cycles
        nand_cx2_state.cycl = value;
        return;
    case 0x008: // REG_CMD2: No. of data bytes
        nand_cx2_state.len_cur = nand_cx2_state.len = value;
        return;
    case 0x00c: // REG_CMD3: Command
    {
        nand_cx2_state.cmd = value;
        uint8_t cs = (nand_cx2_state.cmd >> 8) & 0x3;

        // Toggle CS
        spinand_cx2_set_cs(cs, false);
        spinand_cx2_set_cs(cs, true);

        uint8_t cmd = nand_cx2_state.cmd >> 24;

        unsigned int cycl;
        for(cycl = 0; cycl < std::min((nand_cx2_state.cycl >> 24) & 3, 2u); cycl++)
            spinand_cx2_transceive(cmd); // Command cycle

        for(cycl = 0; cycl < std::min(nand_cx2_state.cycl & 7, 4u); cycl++)
            spinand_cx2_transceive(nand_cx2_state.addr >> (cycl << 3)); // Address cycle

        for(cycl = 0; cycl < ((nand_cx2_state.cycl >> 19) & 0x1F); cycl++)
            spinand_cx2_transceive(0); // Dummy cycle

        if((nand_cx2_state.cmd & 0x6) == 0x4)
        {
            // Read status into rdsr
            do {
                nand_cx2_state.rdsr = spinand_cx2_transceive();
                if(nand_cx2_state.cmd & 0x8)
                    break; // Only once
            }
            while(nand_cx2_state.rdsr & (1 << nand_cx2_state.wip));
        }

        if(nand_cx2_state.len_cur == 0)
            spinand_cx2_set_cs(cs, false);

        //if(cmd & 0x1)
            nand_cx2_state.isr |= 1;

        //int_set(INT_SPINAND, nand_cx2_state.isr & nand_cx2_state.icr);
        return;
    }
    case 0x010: // REG_CTRL: Control
        nand_cx2_state.ctrl = value & 0x70013;
        nand_cx2_state.wip = (value >> 16) & 0x7;
        return;
    case 0x020: //  REG_ICR: Interrupt control
        nand_cx2_state.icr = value;
        return;
    case 0x024: //  REG_ISR: Interrupt status
        nand_cx2_state.isr &= ~value; // Clear bits
        return;
    case 0x100: // REG_DATA: Data words
        if((nand_cx2_state.cmd & 0x2) == 0)
            return; // Read only command

        for(int i = 0; i < 4 && nand_cx2_state.len_cur > 0; i++, nand_cx2_state.len_cur--)
            spinand_cx2_transceive(value >> (i << 3));

        if(nand_cx2_state.len_cur == 0)
        {
            spinand_cx2_set_cs(nand_cx2_state.active_cs, false);
            nand_cx2_state.isr |= 1;
            //int_set(INT_SPINAND, nand_cx2_state.isr & nand_cx2_state.icr);
        }

        return;
    }

    return bad_write_word(addr, value);
}

void spinand_cx2_write_byte(uint32_t addr, uint8_t value)
{
    return spinand_cx2_write_word(addr, value);
}
