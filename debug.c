#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#endif
#include "debug.h"
#include "interrupt.h"
#include "emu.h"
#include "cpu.h"
#include "mem.h"
#include "disasm.h"
#include "mmu.h"
#include "translate.h"
#include "usblink.h"
#include "gdbstub.h"

char target_folder[256];

void *virt_mem_ptr(uint32_t addr, uint32_t size) {
	// Note: this is not guaranteed to be correct when range crosses page boundary
	return (void *)(intptr_t)phys_mem_ptr(mmu_translate(addr, false, NULL), size);
}

void backtrace(uint32_t fp) {
	uint32_t *frame;
	gui_debug_printf("Frame     PrvFrame Self     Return   Start\n");
	do {
		gui_debug_printf("%08X:", fp);
		frame = virt_mem_ptr(fp - 12, 16);
		if (!frame) {
			gui_debug_printf(" invalid address\n");
			break;
		}
        //vgui_debug_printf(" %08X %08X %08X %08X\n", (void *)frame);
		if (frame[0] <= fp) /* don't get stuck in infinite loop :) */
			break;
		fp = frame[0];
	} while (frame[2] != 0);
}

static void dump(uint32_t addr) {
	uint32_t start = addr;
	uint32_t end = addr + 0x7F;

	uint32_t row, col;
	for (row = start & ~0xF; row <= end; row += 0x10) {
		uint8_t *ptr = virt_mem_ptr(row, 16);
		if (!ptr) {
            gui_debug_printf("Address %08X is not in RAM.\n", row);
			break;
		}
		gui_debug_printf("%08X  ", row);
		for (col = 0; col < 0x10; col++) {
			addr = row + col;
			if (addr < start || addr > end)
				gui_debug_printf("  ");
			else
				gui_debug_printf("%02X", ptr[col]);
			putchar(col == 7 && addr >= start && addr < end ? '-' : ' ');
		}
		gui_debug_printf("  ");
		for (col = 0; col < 0x10; col++) {
			addr = row + col;
			if (addr < start || addr > end)
				putchar(' ');
			else if (ptr[col] < 0x20)
				putchar('.');
			else
				putchar(ptr[col]);
		}
		putchar('\n');
	}
}

static uint32_t parse_expr(char *str) {
	uint32_t sum = 0;
	int sign = 1;
	if (str == NULL)
		return 0;
	while (*str) {
		int reg;
		if (isxdigit(*str)) {
			sum += sign * strtoul(str, &str, 16);
			sign = 1;
		} else if (*str == '+') {
			str++;
		} else if (*str == '-') {
			sign = -1;
			str++;
		} else if (*str == 'v') {
			sum += sign * mmu_translate(strtoul(str + 1, &str, 16), false, NULL);
			sign = 1;
		} else if (*str == 'r') {
			reg = strtoul(str + 1, &str, 10);
            if(reg > 15)
            {
                gui_debug_printf("Reg number out of range!\n");
                return 0;
            }
			sum += sign * arm.reg[reg];
			sign = 1;
		} else if (isxdigit(*str)) {
			sum += sign * strtoul(str, &str, 16);
			sign = 1;
		} else {
			for (reg = 13; reg < 16; reg++) {
				if (!memcmp(str, reg_name[reg], 2)) {
					str += 2;
					sum += sign * arm.reg[reg];
					sign = 1;
					goto ok;
				}
			}
			gui_debug_printf("syntax error\n");
			return 0;
			ok:;
		}
	}
	return sum;
}

uint32_t disasm_insn(uint32_t pc) {
	return arm.cpsr_low28 & 0x20 ? disasm_thumb_insn(pc) : disasm_arm_insn(pc);
}

static void disasm(uint32_t (*dis_func)(uint32_t pc)) {
	char *arg = strtok(NULL, " \n");
	uint32_t addr = arg ? parse_expr(arg) : arm.reg[15];
	int i;
	for (i = 0; i < 16; i++) {
		uint32_t len = dis_func(addr);
		if (!len) {
			gui_debug_printf("Address %08X is not in RAM.\n", addr);
			break;
		}
		addr += len;
	}
}

uint32_t *debug_next;
static void set_debug_next(uint32_t *next) {
	if (debug_next != NULL)
		RAM_FLAGS(debug_next) &= ~RF_EXEC_DEBUG_NEXT;
	if (next != NULL) {
		if (RAM_FLAGS(next) & RF_CODE_TRANSLATED)
			flush_translations();
		RAM_FLAGS(next) |= RF_EXEC_DEBUG_NEXT;
	}
	debug_next = next;
}

bool gdb_connected = false;
FILE *debugger_input = NULL;

// return 1: break (should stop being feed with debugger commands), 0: continue (can be feed with other debugger commands)
static int process_debug_cmd(char *cmdline) {
	char *cmd = strtok(cmdline, " \n");
	if (!cmd)
		return 0;
	//if (!stricmp(cmd, "?") || !stricmp(cmd, "h")) {
	if (!strcasecmp(cmd, "?") || !strcasecmp(cmd, "h")) {
		gui_debug_printf(
			"Debugger commands:\n"
			"b - stack backtrace\n"
			"c - continue\n"
			"d <address> - dump memory\n"
			"k <address> <+r|+w|+x|-r|-w|-x> - add/remove breakpoint\n"
			"k - show breakpoints\n"
			"ln c - connect\n"
			"ln s <file> - send a file\n"
			"ln st <dir> - set target directory\n"
			"n - continue until next instruction\n"
			"pr <address> - port or memory read\n"
			"pw <address> <value> - port or memory write\n"
			"q - quit\n"
			"r - show registers\n"
			"rs <regnum> <value> - change register value\n"
			"ss <address> <length> <string> - search a string\n"
			"s - step instruction\n"
			"t+ - enable instruction translation\n"
			"t- - disable instruction translation\n"
			"u[a|t] [address] - disassemble memory\n"
			"wm <file> <start> <size> - write memory to file\n"
			"wf <file> <start> [size] - write file to memory\n");
	//} else if (!stricmp(cmd, "b")) {
	} else if (!strcasecmp(cmd, "b")) {
		char *fp = strtok(NULL, " \n");
		backtrace(fp ? parse_expr(fp) : arm.reg[11]);
	//} else if (!stricmp(cmd, "r")) {
	} else if (!strcasecmp(cmd, "r")) {
		int i, show_spsr;
		uint32_t cpsr = get_cpsr();
		char *mode;
		for (i = 0; i < 16; i++) {
			int newline = ((1 << 5) | (1 << 11) | (1 << 15)) & (1 << i);
			gui_debug_printf("%3s=%08x%c", reg_name[i], arm.reg[i], newline ? '\n' : ' ');
		}
		switch (cpsr & 0x1F) {
			case MODE_USR: mode = "usr"; show_spsr = 0; break;
			case MODE_SYS: mode = "sys"; show_spsr = 0; break;
			case MODE_FIQ: mode = "fiq"; show_spsr = 1; break;
			case MODE_IRQ: mode = "irq"; show_spsr = 1; break;
			case MODE_SVC: mode = "svc"; show_spsr = 1; break;
			case MODE_ABT: mode = "abt"; show_spsr = 1; break;
			case MODE_UND: mode = "und"; show_spsr = 1; break;
			default:       mode = "???"; show_spsr = 0; break;
		}
		gui_debug_printf("cpsr=%08x (N=%d Z=%d C=%d V=%d Q=%d IRQ=%s FIQ=%s T=%d Mode=%s)",
			cpsr,
			arm.cpsr_n, arm.cpsr_z, arm.cpsr_c, arm.cpsr_v,
			cpsr >> 27 & 1,
			(cpsr & 0x80) ? "off" : "on ",
			(cpsr & 0x40) ? "off" : "on ",
			cpsr >> 5 & 1,
			mode);
		if (show_spsr)
			gui_debug_printf(" spsr=%08x", get_spsr());
		gui_debug_printf("\n");
	//} else if (!stricmp(cmd, "rs")) {
	} else if (!strcasecmp(cmd, "rs")) {
		char *reg = strtok(NULL, " \n");
		if (!reg) {
			gui_debug_printf("Parameters are missing.\n");
		} else {
			char *value = strtok(NULL, " \n");
			if (!value) {
				gui_debug_printf("Missing value parameter.\n");
			} else {
				int regi = atoi(reg);
				int valuei = parse_expr(value);
				if (regi >= 0 && regi < 15)
					arm.reg[regi] = valuei;
				else
					gui_debug_printf("Invalid register.\n");
			}
		}
	//} else if (!stricmp(cmd, "k")) {
	} else if (!strcasecmp(cmd, "k")) {
		char *addr_str = strtok(NULL, " \n");
		char *flag_str = strtok(NULL, " \n");
		if (!flag_str)
			flag_str = "+x";
		if (addr_str) {
			uint32_t addr = parse_expr(addr_str);
			void *ptr = phys_mem_ptr(addr & ~3, 4);
			if (ptr) {
				uint32_t *flags = &RAM_FLAGS(ptr);
				bool on = true;
				for (; *flag_str; flag_str++) {
					switch (tolower(*flag_str)) {
						case '+': on = true; break;
						case '-': on = false; break;
						case 'r':
							if (on) *flags |= RF_READ_BREAKPOINT;
							else *flags &= ~RF_READ_BREAKPOINT;
							break;
						case 'w':
							if (on) *flags |= RF_WRITE_BREAKPOINT;
							else *flags &= ~RF_WRITE_BREAKPOINT;
							break;
						case 'x':
							if (on) {
								if (*flags & RF_CODE_TRANSLATED) flush_translations();
								*flags |= RF_EXEC_BREAKPOINT;
							} else
								*flags &= ~RF_EXEC_BREAKPOINT;
							break;
					}
				}
			} else {
				gui_debug_printf("Address %08X is not in RAM.\n", addr);
			}
		} else {
			unsigned int area;
			for (area = 0; area < sizeof(mem_areas)/sizeof(*mem_areas); area++) {
				uint32_t *flags;
				uint32_t *flags_start = &RAM_FLAGS(mem_areas[area].ptr);
				uint32_t *flags_end = &RAM_FLAGS(mem_areas[area].ptr + mem_areas[area].size);
				for (flags = flags_start; flags != flags_end; flags++) {
                    uint32_t addr = mem_areas[area].base + ((uint8_t *)flags - (uint8_t *)flags_start);
					if (*flags & (RF_READ_BREAKPOINT | RF_WRITE_BREAKPOINT | RF_EXEC_BREAKPOINT)) {
                        gui_debug_printf("%08x %c%c%c\n",
                            addr,
							(*flags & RF_READ_BREAKPOINT)  ? 'r' : ' ',
							(*flags & RF_WRITE_BREAKPOINT) ? 'w' : ' ',
							(*flags & RF_EXEC_BREAKPOINT)  ? 'x' : ' ');
					}
				}
			}
		}
	//} else if (!stricmp(cmd, "c")) {
	} else if (!strcasecmp(cmd, "c")) {
		return 1;
	//} else if (!stricmp(cmd, "s")) {
	} else if (!strcasecmp(cmd, "s")) {
		cpu_events |= EVENT_DEBUG_STEP;
		return 1;
	//} else if (!stricmp(cmd, "n")) {
	} else if (!strcasecmp(cmd, "n")) {
		set_debug_next(virt_mem_ptr(arm.reg[15] & ~3, 4) + 1);
		return 1;
	//} else if (!stricmp(cmd, "d")) {
	} else if (!strcasecmp(cmd, "d")) {
		char *arg = strtok(NULL, " \n");
		if (!arg) {
			gui_debug_printf("Missing address parameter.\n");
		} else {
			uint32_t addr = parse_expr(arg);
			dump(addr);
		}
	//} else if (!stricmp(cmd, "u")) {
	} else if (!strcasecmp(cmd, "u")) {
		disasm(disasm_insn);
	//} else if (!stricmp(cmd, "ua")) {
	} else if (!strcasecmp(cmd, "ua")) {
		disasm(disasm_arm_insn);
	//} else if (!stricmp(cmd, "ut")) {
	} else if (!strcasecmp(cmd, "ut")) {
		disasm(disasm_thumb_insn);
	//} else if (!stricmp(cmd, "ln")) {
	} else if (!strcasecmp(cmd, "ln")) {
		char *ln_cmd = strtok(NULL, " \n");
		if (!ln_cmd) return 0;
		//if (!stricmp(ln_cmd, "c")) {
		if (!strcasecmp(ln_cmd, "c")) {
			usblink_connect();
			return 1; // and continue, ARM code needs to be run
		//} else if (!stricmp(ln_cmd, "s")) {
		} else if (!strcasecmp(ln_cmd, "s")) {
			char *file = strtok(NULL, "\n");
			if (!file) {
				gui_debug_printf("Missing file parameter.\n");
			} else {
				// remove optional surrounding quotes
				if (*file == '"') file++;
				size_t len = strlen(file);
				if (*(file + len - 1) == '"')
					*(file + len - 1) = '\0';
				if (usblink_put_file(file, target_folder))
					return 1; // and continue
			}
		//} else if (!stricmp(ln_cmd, "st")) {
		} else if (!strcasecmp(ln_cmd, "st")) {
			char *dir = strtok(NULL, " \n");
			if (dir)
				strncpy(target_folder, dir, sizeof(target_folder));
			else
				gui_debug_printf("Missing directory parameter.\n");
		}
	//} else if (!stricmp(cmd, "taskinfo")) {
	} else if (!strcasecmp(cmd, "taskinfo")) {
		uint32_t task = parse_expr(strtok(NULL, " \n"));
		uint8_t *p = virt_mem_ptr(task, 52);
		if (p) {
			gui_debug_printf("Previous:	%08x\n", *(uint32_t *)&p[0]);
			gui_debug_printf("Next:		%08x\n", *(uint32_t *)&p[4]);
			gui_debug_printf("ID:		%c%c%c%c\n", p[15], p[14], p[13], p[12]);
			gui_debug_printf("Name:		%.8s\n", &p[16]);
			gui_debug_printf("Status:		%02x\n", p[24]);
			gui_debug_printf("Delayed suspend:%d\n", p[25]);
			gui_debug_printf("Priority:	%02x\n", p[26]);
			gui_debug_printf("Preemption:	%d\n", p[27]);
			gui_debug_printf("Stack start:	%08x\n", *(uint32_t *)&p[36]);
			gui_debug_printf("Stack end:	%08x\n", *(uint32_t *)&p[40]);
			gui_debug_printf("Stack pointer:	%08x\n", *(uint32_t *)&p[44]);
			gui_debug_printf("Stack size:	%08x\n", *(uint32_t *)&p[48]);
			uint32_t sp = *(uint32_t *)&p[44];
			uint32_t *psp = virt_mem_ptr(sp, 18 * 4);
            if (psp) {
                #ifdef __i386__
				gui_debug_printf("Stack type:	%d (%s)\n", psp[0], psp[0] ? "Interrupt" : "Normal");
				if (psp[0]) {
                    gui_debug_vprintf("cpsr=%08x  r0=%08x r1=%08x r2=%08x r3=%08x  r4=%08x\n"
							"  r5=%08x  r6=%08x r7=%08x r8=%08x r9=%08x r10=%08x\n"
							" r11=%08x r12=%08x sp=%08x lr=%08x pc=%08x\n",
                        (va_list)&psp[1]);
				} else {
                    gui_debug_vprintf("cpsr=%08x  r4=%08x  r5=%08x  r6=%08x r7=%08x r8=%08x\n"
							"  r9=%08x r10=%08x r11=%08x r12=%08x pc=%08x\n",
                        (va_list)&psp[1]);
				}
                #endif
            }
		}
	//} else if (!stricmp(cmd, "tasklist")) {
	} else if (!strcasecmp(cmd, "tasklist")) {
		uint32_t tasklist = parse_expr(strtok(NULL, " \n"));
		uint8_t *p = virt_mem_ptr(tasklist, 4);
		if (p) {
			uint32_t first = *(uint32_t *)p;
			uint32_t task = first;
			gui_debug_printf("Task      ID   Name     St D Pr P | StkStart StkEnd   StkPtr   StkSize\n");
			do {
				p = virt_mem_ptr(task, 52);
				if (!p)
					return 1;
				gui_debug_printf("%08X: %c%c%c%c %-8.8s %02x %d %02x %d | %08x %08x %08x %08x\n",
					task, p[15], p[14], p[13], p[12],
					&p[16], /* name */
					p[24],  /* status */
					p[25],  /* delayed suspend */
					p[26],  /* priority */
					p[27],  /* preemption */
					*(uint32_t *)&p[36], /* stack start */
					*(uint32_t *)&p[40], /* stack end */
					*(uint32_t *)&p[44], /* stack pointer */
					*(uint32_t *)&p[48]  /* stack size */
					);
				task = *(uint32_t *)&p[4]; /* next */
			} while (task != first);
		}
	//} else if (!stricmp(cmd, "t+")) {
	} else if (!strcasecmp(cmd, "t+")) {
		do_translate = 1;
	//} else if (!stricmp(cmd, "t-")) {
	} else if (!strcasecmp(cmd, "t-")) {
		flush_translations();
		do_translate = 0;
	//} else if (!stricmp(cmd, "wm") || !stricmp(cmd, "wf")) {
	} else if (!strcasecmp(cmd, "wm") || !strcasecmp(cmd, "wf")) {
		bool frommem = cmd[1] != 'f';
		char *filename = strtok(NULL, " \n");
		char *start_str = strtok(NULL, " \n");
		char *size_str = strtok(NULL, " \n");
		if (!start_str) {
			gui_debug_printf("Parameters are missing.\n");
			return 0;
		}
		uint32_t start = parse_expr(start_str);
		uint32_t size = 0;
		if (size_str)
			size = parse_expr(size_str);
		void *ram = phys_mem_ptr(start, size);
		if (!ram) {
			gui_debug_printf("Address range %08x-%08x is not in RAM.\n", start, start + size - 1);
			return 0;
		}
		FILE *f = fopen(filename, frommem ? "wb" : "rb");
		if (!f) {
			gui_perror(filename);
			return 0;
		}
		if (!size && !frommem) {
			fseek (f, 0, SEEK_END);
			size = ftell(f);
			rewind(f);
		}

        if (!(frommem ? fwrite(ram, size, 1, f) : fread(ram, size, 1, f))) {
            fclose(f);
			gui_perror(filename);
			return 0;
		}
        fclose(f);
        return 0;
	//} else if (!stricmp(cmd, "ss")) {
	} else if (!strcasecmp(cmd, "ss")) {
		char *addr_str = strtok(NULL, " \n");
		char *len_str = strtok(NULL, " \n");
		char *string = strtok(NULL, " \n");
		if (!addr_str || !len_str || !string) {
			gui_debug_printf("Missing parameters.\n");
		} else {
			uint32_t addr = parse_expr(addr_str);
			uint32_t len = parse_expr(len_str);
			char *strptr = phys_mem_ptr(addr, len);
			char *ptr = strptr;
			char *endptr = strptr + len;
			if (ptr) {
				size_t slen = strlen(string);
				while (1) {
					ptr = memchr(ptr, *string, endptr - ptr);
					if (!ptr) {
						gui_debug_printf("String not found.\n");
                        return 0;
					}
					if (!memcmp(ptr, string, slen)) {
                        uint32_t found_addr = ptr - strptr + addr;
                        gui_debug_printf("Found at address %08x.\n", found_addr);
                        return 0;
					}
					if (ptr < endptr)
						ptr++;
				}
            } else {
				gui_debug_printf("Address range %08x-%08x is not in RAM.\n", addr, addr + len - 1);
            }
		}
        return 0;
	//} else if (!stricmp(cmd, "int")) {
	} else if (!strcasecmp(cmd, "int")) {
		gui_debug_printf("active		= %08x\n", intr.active);
		gui_debug_printf("status		= %08x\n", intr.status);
		gui_debug_printf("mask		= %08x %08x\n", intr.mask[0], intr.mask[1]);
		gui_debug_printf("priority_limit	= %02x       %02x\n", intr.priority_limit[0], intr.priority_limit[1]);
		gui_debug_printf("noninverted	= %08x\n", intr.noninverted);
		gui_debug_printf("sticky		= %08x\n", intr.sticky);
		gui_debug_printf("priority:\n");
		int i, j;
		for (i = 0; i < 32; i += 16) {
			gui_debug_printf("\t");
			for (j = 0; j < 16; j++)
				gui_debug_printf("%02x ", intr.priority[i+j]);
			gui_debug_printf("\n");
		}
	//} else if (!stricmp(cmd, "int+")) {
	} else if (!strcasecmp(cmd, "int+")) {
		int_set(atoi(strtok(NULL, " \n")), 1);
	//} else if (!stricmp(cmd, "int-")) {
	} else if (!strcasecmp(cmd, "int-")) {
		int_set(atoi(strtok(NULL, " \n")), 0);
	//} else if (!stricmp(cmd, "pr")) {
	} else if (!strcasecmp(cmd, "pr")) {
		// TODO: need to avoid entering debugger recursively
		// also, where should error() go?
		uint32_t addr = parse_expr(strtok(NULL, " \n"));
		gui_debug_printf("%08x\n", mmio_read_word(addr));
	//} else if (!stricmp(cmd, "pw")) {
	} else if (!strcasecmp(cmd, "pw")) {
		// TODO: ditto
		uint32_t addr = parse_expr(strtok(NULL, " \n"));
		uint32_t value = parse_expr(strtok(NULL, " \n"));
		mmio_write_word(addr, value);
	} else {
		gui_debug_printf("Unknown command %s\n", cmd);
	}
	return 0;
}

#define MAX_CMD_LEN 300

static void native_debugger(void) {
	char line[MAX_CMD_LEN];
	uint32_t *cur_insn = virt_mem_ptr(arm.reg[15] & ~3, 4);

	// Did we hit the "next" breakpoint?
	if (cur_insn == debug_next) {
		set_debug_next(NULL);
		disasm_insn(arm.reg[15]);
	}

	if (cpu_events & EVENT_DEBUG_STEP) {
		cpu_events &= ~EVENT_DEBUG_STEP;
		disasm_insn(arm.reg[15]);
	}

	throttle_timer_off();
	while (1) {
        if(debugger_input == NULL)
        {
            char *cmd = gui_debug_prompt();
            if(process_debug_cmd(cmd))
            {
                //free(cmd);
                break;
            }
            //free(cmd);
            continue;
        }
		fflush(stdout);
		fflush(stderr);
        while (!fgets(line, sizeof line, debugger_input)) {
            // switch to GUI
			fclose(debugger_input);
            debugger_input = NULL;
            break;
		}
        if(!debugger_input)
            continue;

        gui_debug_printf("Remote debug cmd: %s", line);

		if (process_debug_cmd(line))
			break;
		else
			continue;
	}
	throttle_timer_on();
}

static int listen_socket_fd = 0;
static int socket_fd = 0;

static void log_socket_error(const char *msg) {
#ifdef __MINGW32__
	int errCode = WSAGetLastError();
	LPSTR errString = NULL;  // will be allocated and filled by FormatMessage
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, errCode, 0, (LPSTR)&errString, 0, 0);
	gui_debug_printf( "%s: %s (%i)\n", msg, errString, errCode);
	LocalFree( errString );
#else
	gui_perror(msg);
#endif
}

static void set_nonblocking(int socket, bool nonblocking) {
#ifdef __MINGW32__
	u_long mode = nonblocking;
	ioctlsocket(socket, FIONBIO, &mode);
#else
	int ret = fcntl(socket, F_GETFL, 0);
	fcntl(socket, F_SETFL, nonblocking ? ret | O_NONBLOCK : ret & ~O_NONBLOCK);
#endif
}

bool rdebug_bind(int port) {
	struct sockaddr_in sockaddr;
	int r;
	
#ifdef __MINGW32__
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData)) {
		log_socket_error("WSAStartup failed");
        return false;
	}
#endif

	listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket_fd == -1) {
		log_socket_error("Remote debug: Failed to create socket");
        return false;
	}
	set_nonblocking(listen_socket_fd, true);

	memset(&sockaddr, '\000', sizeof sockaddr);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	r = bind(listen_socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (r == -1) {
		log_socket_error("Remote debug: failed to bind socket. Check that nspire_emu is not already running!");
        return false;
	}
	r = listen(listen_socket_fd, 0);
	if (r == -1) {
		log_socket_error("Remote debug: failed to listen on socket");
        return false;
	}

    return true;
}

static char rdebug_inbuf[MAX_CMD_LEN];
size_t rdebug_inbuf_used = 0;

void rdebug_recv(void) {
	int ret, on;
	if (!socket_fd) {
		ret = accept(listen_socket_fd, NULL, NULL);
		if (ret == -1)
			return;
		socket_fd = ret;
		set_nonblocking(socket_fd, false);
		/* Disable Nagle for low latency */
		on = 1;
#ifdef __MINGW32__
		ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
#else
		ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
#endif
		if (ret == -1)
			log_socket_error("Remote debug: setsockopt(TCP_NODELAY) failed for socket");
        gui_debug_printf("Remote debug: connected.\n");
		return;
	}
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET((unsigned)socket_fd, &rfds);
	ret = select(socket_fd + 1, &rfds, NULL, NULL, &(struct timeval) {0, 0});
	if (ret == -1 && errno == EBADF) {
        gui_debug_printf("Remote debug: connection closed.\n");
#ifdef __MINGW32__
		closesocket(socket_fd);
#else
		close(socket_fd);
#endif
		socket_fd = 0;
	}
	else if (!ret)
		return; // nothing receivable

	size_t buf_remain = sizeof(rdebug_inbuf) - rdebug_inbuf_used;
	if (!buf_remain) {
        gui_debug_printf("Remote debug: command is too long\n");
		return;
	}

	ssize_t rv = recv(socket_fd, (void*)&rdebug_inbuf[rdebug_inbuf_used], buf_remain, 0);
	if (!rv) {
        gui_debug_printf("Remote debug: connection closed.\n");
#ifdef __MINGW32__
		closesocket(socket_fd);
#else
		close(socket_fd);
#endif
		socket_fd = 0;
		return;
	}
	if (rv < 0 && errno == EAGAIN) {
		/* no data for now, call back when the socket is readable */
		return;
	}
	if (rv < 0) {
        log_socket_error("Remote debug: connection error");
		return;
	}
	rdebug_inbuf_used += rv;

	char *line_start = rdebug_inbuf;
	char *line_end;
	while ( (line_end = (char*)memchr((void*)line_start, '\n', rdebug_inbuf_used - (line_start - rdebug_inbuf)))) {
		*line_end = 0;
		process_debug_cmd(line_start);
		line_start = line_end + 1;
	}
	/* Shift buffer down so the unprocessed data is at the start */
	rdebug_inbuf_used -= (line_start - rdebug_inbuf);
	memmove(rdebug_inbuf, line_start, rdebug_inbuf_used);
}

void debugger(enum DBG_REASON reason, uint32_t addr) {
	if (gdb_connected)
		gdbstub_debugger(reason, addr);
	else
		native_debugger();
}

void rdebug_quit()
{
    if(socket_fd)
    {
        close(socket_fd);
        socket_fd = 0;
    }

    if(listen_socket_fd)
    {
        close(listen_socket_fd);
        listen_socket_fd = 0;
    }
}
