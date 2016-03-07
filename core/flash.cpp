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
    static size_t offset_classic[] = { 0, 0x2400, 0x15a800, 0x16b000, 0x2100000 };

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
