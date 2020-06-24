/*
 * TODO:
 * - Explicitely support the endianness (set/get_registers). Currently the host must be little-endian
 *   as ARM is.
 * - fix vFile commands, currently broken because of the armsnippets
 */

/*
 * Some parts derive from GDB's sparc-stub.c.
 * Refer to Appendix D - GDB Remote Serial Protocol in GDB's documentation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <poll.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#endif

#include "emu.h"
#include "debug.h"
#include "mem.h"
#include "cpu.h"
#include "armsnippets.h"
#include "gdbstub.h"
#include "translate.h"

static void gdbstub_disconnect(void);

bool ndls_is_installed(void) {
    uint32_t *vectors = virt_mem_ptr(0x20, 0x20);
    if (vectors) {
        // The Ndless marker is 8 bytes before the SWI handler
        uint32_t *sig = virt_mem_ptr(vectors[EX_SWI] - 8, 4);
        if (sig) return *sig == 0x4E455854 /* 'NEXT' */;
    }
    return false;
}

static int listen_socket_fd = -1;
static int socket_fd = -1;
static bool gdb_handshake_complete = false;

static void log_socket_error(const char *msg) {
#ifdef __MINGW32__
    int errCode = WSAGetLastError();
    LPSTR errString = NULL;  // will be allocated and filled by FormatMessage
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  0, errCode, 0, (LPSTR)&errString, 0, 0);
    gui_debug_printf("%s: %s (%i)\n", msg, errString, errCode);
    LocalFree( errString );
#else
    gui_perror(msg);
#endif
}

static char sockbuf[4096];
static char *sockbufptr = sockbuf;

static bool flush_out_buffer(void) {
#ifndef MSG_NOSIGNAL
    #ifdef __APPLE__
        #define MSG_NOSIGNAL SO_NOSIGPIPE
    #else
        #define MSG_NOSIGNAL 0
    #endif
#endif
    char *p = sockbuf;
    while (p != sockbufptr) {
        int n = send(socket_fd, p, sockbufptr-p, MSG_NOSIGNAL);
        if (n == -1) {
#ifdef __MINGW32__
            if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
            if (errno == EAGAIN)
#endif
                continue; // not ready to send
            else {
                log_socket_error("Failed to send to GDB stub socket");
                return false;
            }
        }
        p += n;
    }
    sockbufptr = sockbuf;
    return true;
}

static bool put_debug_char(char c) {
    if (log_enabled[LOG_GDB]) {
        logprintf(LOG_GDB, "%c", c);
        fflush(stdout);
        if (c == '+' || c == '-')
            logprintf(LOG_GDB, "\t");
    }
    if (sockbufptr == sockbuf + sizeof sockbuf)
        if(!flush_out_buffer())
            return false;

    *sockbufptr++ = c;
    return true;
}

// returns 1 if at least one instruction translated in the range
static int range_translated(uint32_t range_start, uint32_t range_end) {
    uint32_t pc;
    int translated = 0;
    for (pc = range_start; pc < range_end;  pc += 4) {
        void *pc_ram_ptr = virt_mem_ptr(pc, 4);
        if (!pc_ram_ptr)
            break;
        translated |= RAM_FLAGS(pc_ram_ptr) & RF_CODE_TRANSLATED;
    }
    return translated;
}

/* Returns -1 on disconnection */
static char get_debug_char(void) {
    char c;

    #ifndef WIN32
        while(true)
        {
            struct pollfd pfd;
            pfd.fd = socket_fd;
            pfd.events = POLLIN;
            int p = poll(&pfd, 1, 100);
            if(p == -1) // Disconnected
                return -1;

            if(p) // Data available
                break;

            else // No data available
            {
                if(exiting)
                    return -1;

                gui_do_stuff(false);
            }
        }
    #endif

    int r = recv(socket_fd, &c, 1, 0);
    if (r == -1) {
        // only for debugging - log_socket_error("Failed to recv from GDB stub socket");
        return -1;
    }
    if (r == 0)
        return -1; // disconnected
    if (log_enabled[LOG_GDB]) {
        logprintf(LOG_GDB, "%c", c);
        fflush(stdout);
        if (c == '+' || c == '-')
            logprintf(LOG_GDB, "\n");
    }
    return c;
}

static void set_nonblocking(int socket, bool nonblocking) {
#ifdef __MINGW32__
    u_long mode = nonblocking;
    ioctlsocket(socket, FIONBIO, &mode);
#else
    int ret = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, nonblocking ? (ret | O_NONBLOCK) : (ret & ~O_NONBLOCK));
#endif
}

bool gdbstub_init(unsigned int port) {
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
        log_socket_error("Failed to create GDB stub socket");
        return false;
    }
    set_nonblocking(listen_socket_fd, true);

    memset (&sockaddr, '\000', sizeof sockaddr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    r = bind(listen_socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (r == -1) {
        log_socket_error("Failed to bind GDB stub socket. Check that Firebird is not already running");
        return false;
    }
    r = listen(listen_socket_fd, 0);
    if (r == -1) {
        log_socket_error("Failed to listen on GDB stub socket");
    }

    return true;
}

// program block pre-allocated by Ndless, used for vOffsets queries
static uint32_t ndls_debug_alloc_block = 0;
static bool ndls_debug_received = false;

static void gdb_connect_ndls_cb(struct arm_state *state) {
    ndls_debug_alloc_block = state->reg[0]; // can be 0
    ndls_debug_received = true;
}

/* BUFMAX defines the maximum number of characters in inbound/outbound buffers */
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 2048

static const char hexchars[]="0123456789abcdef";

enum regnames {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC,
               F0H, F0M, F0L,
               F1H, F1M, F1L,
               F2H, F2M, F2L,
               F3H, F3M, F3L,
               F4H, F4M, F4L,
               F5H, F5M, F5L,
               F6H, F6M, F6L,
               F7H, F7M, F7L, FPS, CPSR, NUMREGS};

/* Number of bytes of registers. */
#define NUMREGBYTES (NUMREGS * 4)

// see GDB's include/gdb/signals.h
enum target_signal {SIGNAL_ILL_INSTR = 4, SIGNAL_TRAP = 5};

/* Convert ch from a hex digit to an int */
static int hex(char ch) {
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    return -1;
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>. # will be replaced with \0.
 * Returns NULL on disconnection. */
char *getpacket(void) {
    char *buffer = &remcomInBuffer[0];
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while (1) {
        /* wait around for the start character, ignore all other characters */
        do {
            ch = get_debug_char();
            if (ch == (char)-1) // disconnected
                return NULL;
        } while (ch != '$');

retry:
        checksum = 0;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while (count < BUFMAX - 1) {
            ch = get_debug_char();
            if (ch == '$')
                goto retry;
            buffer[count] = ch;
            if (ch == '#')
                break;
            count = count + 1;
            checksum = checksum + ch;
        }

        if (ch == '#') {
            buffer[count] = 0;
            ch = get_debug_char();
            xmitcsum = hex(ch) << 4;
            ch = get_debug_char();
            xmitcsum += hex(ch);

            if (checksum != xmitcsum) {
                if(!put_debug_char('-')	/* failed checksum */
                   || !flush_out_buffer())
                    return NULL;
            } else {
                put_debug_char('+');	/* successful transfer */

                /* if a sequence char is present, reply the sequence ID */
                if(buffer[2] == ':') {
                    if(!put_debug_char(buffer[0])
                       || !put_debug_char(buffer[1])
                       || !flush_out_buffer())
                        return NULL;

                    return &buffer[3];
                }
                if(!flush_out_buffer())
                    return NULL;

                return &buffer[0];
            }
        }
    }
}

/* send the packet in buffer.  */
static bool putpacket(char *buffer) {
    unsigned char checksum;
    int count;
    char ch;

    /*  $<packet info>#<checksum> */
    do {
        if(!put_debug_char('$'))
            return false;

        checksum = 0;
        count = 0;

        while ((ch = buffer[count])) {
            put_debug_char(ch);
            checksum += ch;
            count += 1;
        }

        if(!put_debug_char('#')
           || !put_debug_char(hexchars[checksum >> 4])
           || !put_debug_char(hexchars[checksum & 0xf])
           || !flush_out_buffer())
            return false;

        ch = get_debug_char();
    } while (ch != '+' && ch != (char) -1);

    return true;
}

/* Indicate to caller of mem2hex or hex2mem that there has been an
 * error.  */
static int mem_err = 0;

/* Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 * If MAY_FAULT is non-zero, then we will handle memory faults by returning
 * a 0, else treat a fault like any other fault in the stub.
 */
static char *mem2hex(void *mem, char *buf, int count) {
    unsigned char ch;
    unsigned char *memptr = mem;

    while (count-- > 0) {
        ch = *memptr++;
        if (mem_err)
            return 0;
        *buf++ = hexchars[ch >> 4];
        *buf++ = hexchars[ch & 0xf];
    }
    *buf = 0;
    return buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem
 * return a pointer to the character AFTER the last byte written.
 * If count is null stops at the first non hex digit */
static void *hex2mem(char *buf, void *mem, int count) {
    int i;
    int ch;
    uint8_t *memb = mem;

    for (i = 0; i < count || !count; i++) {
        ch = hex(*buf++);
        if (ch == -1)
            return memb;
        ch <<= 4;
        ch |= hex(*buf++);
        *memb++ = (uint8_t) ch;
        if (mem_err)
            return 0;
    }
    return memb;
}

/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */
static int hexToInt(char **ptr, int *intValue) {
    int numChars = 0;
    int hexValue;

    *intValue = 0;
    while (**ptr) {
        hexValue = hex(**ptr);
        if (hexValue < 0)
            break;
        *intValue = (*intValue << 4) | hexValue;
        numChars ++;
        (*ptr)++;
    }

    return (numChars);
}

/* See Appendix D - GDB Remote Serial Protocol - Overview.
 * A null character is appended. */
__attribute__((unused)) static void binary_escape(char *in, int insize, char *out, int outsize) {
    while (insize-- > 0 && outsize > 1) {
        if (*in == '#' || *in == '$' || *in == '}' || *in == 0x2A) {
            if (outsize < 3)
                break;
            *out++ = '}';
            *out++ = (0x20 ^ *in++);
            outsize -= 2;
        }
        else {
            *out++ = *in++;
            outsize--;
        }
    }
    *out = '\0';
}

/* From emu to GDB. Returns regbuf. */
static uint32_t *get_registers(uint32_t regbuf[NUMREGS]) {
    // GDB's format in arm-tdep.c/arm_register_names
    memset(regbuf, 0, sizeof(uint32_t) * NUMREGS);
    memcpy(regbuf, arm.reg, sizeof(uint32_t) * 16);
    regbuf[NUMREGS-1] = (uint32_t)get_cpsr();
    return regbuf;
}

/* From GDB to emu */
static void set_registers(const uint32_t regbuf[NUMREGS]) {
    memcpy(arm.reg, regbuf, sizeof(uint32_t) * 16);
    set_cpsr_full(regbuf[NUMREGS-1]);
}

/* GDB Host I/O */

#define append_hex_char(ptr,ch) do {*ptr++ = hexchars[(ch) >> 4]; *ptr++ = hexchars[(ch) & 0xf];} while (0)

/* See GDB's documentation: D.3 Stop Reply Packets
 * stop reason and r can be null. */
static bool send_stop_reply(int signal, const char *stop_reason, const char *r) {
    char *ptr = remcomOutBuffer;
    ptr += sprintf(ptr, "T%02xthread:1;", signal);
    if (stop_reason) {
        strcpy(ptr, stop_reason);
        ptr += strlen(stop_reason);
        *ptr++ = ':';
        strcpy(ptr, r);
        ptr += strlen(ptr);
        *ptr++ = ';';
    }
    append_hex_char(ptr, 13);
    *ptr++ = ':';
    ptr = mem2hex(&arm.reg[13], ptr, sizeof(uint32_t));
    *ptr++ = ';';
    append_hex_char(ptr, 15);
    *ptr++ = ':';
    ptr = mem2hex(&arm.reg[15], ptr, sizeof(uint32_t));
    *ptr++ = ';';
    *ptr = 0;
    return putpacket(remcomOutBuffer);
}

void gdbstub_loop(void) {
    int addr;
    int length;
    char *ptr, *ptr1;
    void *ramaddr;
    uint32_t regbuf[NUMREGS];
    bool reply, set;

    gui_debugger_entered_or_left(in_debugger = true);

    while (1) {
        remcomOutBuffer[0] = 0;

        ptr = getpacket();
        if (!ptr) {
            gdbstub_disconnect();
            gui_debugger_entered_or_left(in_debugger = false);
            return;
        }

        reply = true;
        switch (*ptr++) {
            case '?':
                if(!send_stop_reply(SIGNAL_TRAP, NULL, 0))
                    goto disconnect;

                reply = false; // already done
                break;

            case 'g':  /* return the value of the CPU registers */
                get_registers(regbuf);
                ptr = remcomOutBuffer;
                ptr = mem2hex(regbuf, ptr, NUMREGS * sizeof(uint32_t));
                break;

            case 'G':  /* set the value of the CPU registers - return OK */
                hex2mem(ptr, regbuf, NUMREGS * sizeof(uint32_t));
                set_registers(regbuf);
                strcpy(remcomOutBuffer,"OK");
                break;

            case 'H':
                if(ptr[1] == '1')
                    strcpy(remcomOutBuffer, "OK");
                break;

            case 'p': /* pn Read the value of register n */
                if (hexToInt(&ptr, &addr) && (size_t)addr < sizeof(regbuf)/sizeof(uint32_t)) {
                    mem2hex(get_registers(regbuf) + addr, remcomOutBuffer, sizeof(uint32_t));
                } else {
                    strcpy(remcomOutBuffer,"E01");
                }
                break;

            case 'P': /* Pn=r Write register n with value r */
                ptr = strtok(ptr, "=");
                if (hexToInt(&ptr, &addr)
                        && (ptr=strtok(NULL, ""))
                        && (size_t)addr < sizeof(regbuf)/sizeof(uint32_t)
                        // TODO hex2mem doesn't check the format
                        && hex2mem((char*)ptr, &get_registers(regbuf)[addr], sizeof(uint32_t))
                        ) {
                    set_registers(regbuf);
                    strcpy(remcomOutBuffer, "OK");
                } else {
                    strcpy(remcomOutBuffer,"E01");
                }
                break;

            case 'm':  /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
                /* Try to read %x,%x */
                if (hexToInt(&ptr, &addr)
                        && *ptr++ == ','
                        && hexToInt(&ptr, &length)
                        && (size_t)length < (sizeof(remcomOutBuffer) - 1) / 2)
                {
                    ramaddr = virt_mem_ptr(addr, length);
                    if (ramaddr && mem2hex(ramaddr, remcomOutBuffer, length))
                        break;
                    strcpy(remcomOutBuffer, "E03");
                } else
                    strcpy(remcomOutBuffer,"E01");
                break;

            case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA..AA  */
                /* Try to read '%x,%x:' */
                if (hexToInt(&ptr, &addr)
                        && *ptr++ == ','
                        && hexToInt(&ptr, &length)
                        && *ptr++ == ':')
                {
                    ramaddr = virt_mem_ptr(addr, length);
                    if (!ramaddr) {
                        strcpy(remcomOutBuffer, "E03");
                        break;
                    }
                    if (range_translated((uintptr_t)ramaddr, (uintptr_t)((char *)ramaddr + length)))
                        flush_translations();
                    if (hex2mem(ptr, ramaddr, length))
                        strcpy(remcomOutBuffer, "OK");
                    else
                        strcpy(remcomOutBuffer, "E03");
                } else
                    strcpy(remcomOutBuffer, "E02");
                break;

            case 'S': /* Ssig[;AA..AA] Step with signal at address AA..AA(optional). Same as 's' for us. */
                ptr = strchr(ptr, ';'); /* skip the signal */
                if (ptr)
                    ptr++;
                // fallthrough
            case 's': /* s[AA..AA]  Step at address AA..AA(optional) */
                cpu_events |= EVENT_DEBUG_STEP;
                goto parse_new_pc;
            case 'C': /* Csig[;AA..AA] Continue with signal at address AA..AA(optional). Same as 'c' for us. */
                ptr = strchr(ptr, ';'); /* skip the signal */
                if (ptr)
                    ptr++;
                // fallthrough
            case 'c':    /* c[AA..AA]    Continue at address AA..AA(optional) */
parse_new_pc:
                if (ptr && hexToInt(&ptr, &addr)) {
                    arm.reg[15] = addr;
                }

                gui_debugger_entered_or_left(in_debugger = false);
                return;
            case 'q': /* qString Get value of String */
                if (!strcmp("Offsets", ptr))
                {
                    /* Offsets of sections */
                    sprintf(remcomOutBuffer, "Text=%x;Data=%x;Bss=%x",
                            ndls_debug_alloc_block, ndls_debug_alloc_block,	ndls_debug_alloc_block);
                }
                else if(!strcmp("C", ptr))
                {
                    /* Current thread id */
                    strcpy(remcomOutBuffer, "QC1"); // First and only thread
                }
                else if(!strcmp("fThreadInfo", ptr))
                {
                    /* First thread id */
                    strcpy(remcomOutBuffer, "m1"); // First and only thread
                }
                else if(!strcmp("sThreadInfo", ptr))
                {
                    /* Next thread id */
                    strcpy(remcomOutBuffer, "l"); // No more threads
                }
                else if(!strcmp("HostInfo", ptr))
                {
                    /* Host information */
                    strcpy(remcomOutBuffer, "cputype:12;cpusubtype:7;endian:little;ptrsize:4;");
                }
                else if(!strcmp("Supported", ptr))
                {
                    /* Feature query */
                    strcpy(remcomOutBuffer, "vContSupported");
                }
                else if(!strcmp("Symbol::", ptr))
                {
                    /* Symbols can be queried */
                    strcpy(remcomOutBuffer, "OK");
                }
                else
                    gui_debug_printf("Unsupported GDB cmd '%s'\n", ptr - 1);

                break;
            case 'v':
                if(!strcmp("Cont?", ptr))
                    strcpy(remcomOutBuffer, "");
                else
                    gui_debug_printf("Unsupported GDB cmd '%s'\n", ptr - 1);

                break;
            case 'Z': /* 0|1|2|3|4,addr,kind  */
            case 'z': /* 0|1|2|3|4,addr,kind  */
                set = *(ptr - 1) == 'Z';
                // kinds other than 4 aren't supported
                ptr1 = ptr++;
                ptr = strtok(ptr, ",");
                if (ptr && hexToInt(&ptr, &addr) && (ramaddr = virt_mem_ptr(addr & ~3, 4))) {
                    uint32_t *flags = &RAM_FLAGS(ramaddr);
                    switch (*ptr1) {
                        case '0': // mem breakpoint
                        case '1': // hw breakpoint
                            if (set) {
                                if (*flags & RF_CODE_TRANSLATED) flush_translations();
                                *flags |= RF_EXEC_BREAKPOINT;
                            } else
                                *flags &= ~RF_EXEC_BREAKPOINT;
                            break;
                        case '2': // write watchpoint
                        case '4': // access watchpoint
                            if (set) *flags |= RF_WRITE_BREAKPOINT;
                            else *flags &= ~RF_WRITE_BREAKPOINT;
                            if (*ptr1 != '4')
                                break;
                            // fallthrough
                        case '3': // read watchpoint, access watchpoint
                            if (set) *flags |= RF_READ_BREAKPOINT;
                            else *flags &= ~RF_READ_BREAKPOINT;
                            break;
                        default:
                            goto reply;
                    }
                    strcpy(remcomOutBuffer, "OK");
                } else
                    strcpy(remcomOutBuffer, "E01");
                break;
        }			/* switch */

reply:
        /* reply to the request */
        if (reply && !putpacket(remcomOutBuffer))
            goto disconnect;
    }

disconnect:
    gdbstub_disconnect();
    gui_debugger_entered_or_left(in_debugger = false);
}

void gdbstub_reset(void) {
    ndls_debug_alloc_block = 0; // the block is obvisouly freed by the OS on reset
}

static void gdbstub_disconnect(void) {
    gui_status_printf("GDB disconnected.");
#ifdef __MINGW32__
    closesocket(socket_fd);
#else
    close(socket_fd);
#endif
    socket_fd = -1;
    gdb_connected = false;
    if (ndls_is_installed())
        armloader_load_snippet(SNIPPET_ndls_debug_free, NULL, 0, NULL);
}

/* Non-blocking poll. Enter the debugger loop if a message is received. */
void gdbstub_recv(void) {
    if(listen_socket_fd == -1)
        return;

    int ret, on;
    if (socket_fd == -1) {
        socket_fd = accept(listen_socket_fd, NULL, NULL);
        if (socket_fd == -1)
            return;
        set_nonblocking(socket_fd, true);
        /* Disable Nagle for low latency */
        on = 1;
#ifdef __MINGW32__
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
#else
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
#endif
        if (ret == -1)
            log_socket_error("setsockopt(TCP_NODELAY) failed for GDB stub socket");

        /* Interface with Ndless */
        if (ndls_is_installed())
        {
            armloader_load_snippet(SNIPPET_ndls_debug_alloc, NULL, 0, gdb_connect_ndls_cb);
            ndls_debug_received = false;
        }
        else
        {
            emuprintf("Ndless not detected or too old. Debugging of applications not available!\n");
            ndls_debug_received = true;
        }

        gdb_connected = true;
        gdb_handshake_complete = false;
        gui_status_printf("GDB connected.");
    }

    // Wait until we know the program location
    if(!ndls_debug_received)
        return;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET((unsigned)socket_fd, &rfds);
    ret = select(socket_fd + 1, &rfds, NULL, NULL, &(struct timeval) {0, 0});
    if (ret == -1 && errno == EBADF) {
        gdbstub_disconnect();
    }
    else if (ret)
    {
        if(!gdb_handshake_complete)
        {
            gdb_handshake_complete = true;
            gdbstub_loop();
        }
        else
            gdbstub_debugger(DBG_USER, 0);
    }
}

/* addr is only required for read/write breakpoints */
void gdbstub_debugger(enum DBG_REASON reason, uint32_t addr) {
    cpu_events &= ~EVENT_DEBUG_STEP;
    char addrstr[9]; // 8 digits
    snprintf(addrstr, sizeof(addrstr), "%x", addr);
    switch (reason) {
        case DBG_WRITE_BREAKPOINT:
            send_stop_reply(SIGNAL_TRAP, "watch", addrstr);
            break;
        case DBG_READ_BREAKPOINT:
            send_stop_reply(SIGNAL_TRAP, "rwatch", addrstr);
            break;
        default:
            send_stop_reply(SIGNAL_TRAP, NULL, 0);
    }
    gdbstub_loop();
}

void gdbstub_quit()
{
    if(listen_socket_fd != -1)
    {
        #ifdef __MINGW32__
            closesocket(listen_socket_fd);
        #else
            close(listen_socket_fd);
        #endif
        listen_socket_fd = -1;
    }

    if(socket_fd != -1)
    {
        #ifdef __MINGW32__
            closesocket(socket_fd);
        #else
            close(socket_fd);
        #endif
        socket_fd = -1;
    }
}
