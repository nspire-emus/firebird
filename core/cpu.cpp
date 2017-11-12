#include <algorithm>
#include <cassert>
#include <mutex>

// Uncomment the following line to measure the time until the OS is loaded
// #define BENCHMARK
#ifdef BENCHMARK
    #include <ctime>
#endif

#include "armsnippets.h"
#include "asmcode.h"
#include "cpu.h"
#include "cpudefs.h"
#include "debug.h"
#include "emu.h"
#include "mem.h"
#include "mmu.h"
#include "translate.h"

// Global CPU state
struct arm_state arm;

void cpu_arm_loop()
{
    while (!exiting && cycle_count_delta < 0 && current_instr_size == 4)
    {
        arm.reg[15] &= ~0x3; // Align PC
        Instruction *p = static_cast<Instruction*>(read_instruction(arm.reg[15]));

        #ifdef BENCHMARK
            static clock_t start = 0;
            if(arm.reg[15] == 0)
            {
                start = clock();
                turbo_mode = true;
            }
            else if(arm.reg[15] == 0x10000000 || arm.reg[15] == 0x11800000)
            {
                clock_t diff = clock() - start;
                printf("%ld ms\n", diff / 1000);
            }
        #endif

        uint32_t *flags_ptr = &RAM_FLAGS(p);

        // Check for pending events
        if(cpu_events)
        {
            // Events other than DEBUG_STEP are handled outside
            if(cpu_events & ~EVENT_DEBUG_STEP)
                break;
            goto enter_debugger;
        }

#ifndef NO_TRANSLATION
        // If the instruction is translated, use the translation
        if(*flags_ptr & RF_CODE_TRANSLATED)
        {
            translation_enter();
            continue;
        }
#endif

        // TODO: Other flags
        if(*flags_ptr & (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_ARMLOADER_CB))
        {
            if(*flags_ptr & RF_ARMLOADER_CB)
            {
                *flags_ptr &= ~RF_ARMLOADER_CB;
                armloader_cb();
            }
            else
            {
                if(*flags_ptr & RF_EXEC_BREAKPOINT)
                    gui_debug_printf("Breakpoint at 0x%08x\n", arm.reg[15]);
                enter_debugger:
                uint32_t pc = arm.reg[15];
                debugger(DBG_EXEC_BREAKPOINT, 0);
                if(arm.reg[15] != pc)
                    continue; // Debugger changed PC
            }
        }
#ifndef NO_TRANSLATION
        else if(do_translate && !(*flags_ptr & RF_CODE_NO_TRANSLATE))
        {
            translate(arm.reg[15], &p->raw);
            continue;
        }
#endif

        arm.reg[15] += 4; // Increment now to account for the pipeline
        ++cycle_count_delta;
        do_arm_instruction(*p);
    }
}

// Makes arm.reg[15] point to the current instruction
void fix_pc_for_fault()
{
#ifndef NO_TRANSLATION
    translate_fix_pc();
#endif

    arm.reg[15] -= current_instr_size;
}

void prefetch_abort(uint32_t mva, uint8_t status)
{
    warn("Prefetch abort: address=%08x status=%02x\n", mva, status);
    arm.reg[15] += 4;
    // Fault address register not changed
    arm.instruction_fault_status = status;
    cpu_exception(EX_PREFETCH_ABORT);
    if (mva == arm.reg[15])
        error("Abort occurred with exception vectors unmapped");
    #ifndef NO_SETJMP
        __builtin_longjmp(restart_after_exception, 1);
    #else
        assert(false);
    #endif
}

void data_abort(uint32_t mva, uint8_t status)
{
    fix_pc_for_fault();
    warn("Data abort: address=%08x status=%02x instruction at %08x\n", mva, status, arm.reg[15]);
    arm.reg[15] += 8;
    arm.fault_address = mva;
    arm.data_fault_status = status;
    cpu_exception(EX_DATA_ABORT);
    #ifndef NO_SETJMP
        __builtin_longjmp(restart_after_exception, 1);
    #else
        assert(false);
    #endif
}

void undefined_instruction()
{
    fix_pc_for_fault();
    warn("Undefined instruction at %08x\n", arm.reg[15]);
    arm.reg[15] += current_instr_size;
    cpu_exception(EX_UNDEFINED);
    #ifndef NO_SETJMP
        __builtin_longjmp(restart_after_exception, 1);
    #else
        assert(false);
    #endif
}

void *try_ptr(uint32_t addr)
{
    //There are two different addr_cache formats...
#ifdef AC_FLAGS
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    if(unlikely(entry & AC_FLAGS))
    {
        if(entry & AC_INVALID)
        {
            addr_cache_miss(addr, false, nullptr);
            return try_ptr(addr);
        }
        else // MMIO stuff
            return nullptr;
    }

    entry += addr;
    return (void*)entry;
#else
    void *ptr = &addr_cache[(addr >> 10) << 1][addr];
    if(unlikely((uintptr_t)ptr & AC_NOT_PTR))
        ptr = addr_cache_miss(addr, false, nullptr);

    return ptr;
#endif
}

void * FASTCALL read_instruction(uint32_t addr)
{
    //There are two different addr_cache formats...
#ifdef AC_FLAGS
    uintptr_t entry = *(uintptr_t*)(addr_cache + ((addr >> 10) << 1));

    if(unlikely(entry & AC_FLAGS))
    {
        if(entry & AC_INVALID)
        {
            addr_cache_miss(addr, false, prefetch_abort);
            return read_instruction(addr);
        }
        else // Executing MMIO stuff
        {
            error("PC in MMIO range: 0x%x\n", addr);
        }
    }

    entry += addr;
    return (void*)entry;
#else
    void *ptr = &addr_cache[(addr >> 10) << 1][addr];
    if(unlikely((uintptr_t)ptr & AC_NOT_PTR))
    {
        ptr = addr_cache_miss(addr, false, prefetch_abort);
        if (!ptr)
            error("Bad PC: %08x\n", addr);
    }
    return ptr;
#endif
}

/*void cpu_thumb_loop()
{
    //TODO
    assert(false);
}*/

// Update cpu_events
void cpu_int_check()
{
    static std::mutex mut;
    std::lock_guard<std::mutex> lg(mut);

    if (arm.interrupts & ~arm.cpsr_low28 & 0x80)
        cpu_events |= EVENT_IRQ;
    else
        cpu_events &= ~EVENT_IRQ;

    if (arm.interrupts & ~arm.cpsr_low28 & 0x40)
        cpu_events |= EVENT_FIQ;
    else
        cpu_events &= ~EVENT_FIQ;
}

static const constexpr uint8_t exc_flags[] = {
    MODE_SVC | 0xC0, /* Reset */
    MODE_UND | 0x80, /* Undefined instruction */
    MODE_SVC | 0x80, /* Software interrupt */
    MODE_ABT | 0x80, /* Prefetch abort */
    MODE_ABT | 0x80, /* Data abort */
    0,               /* Reserved */
    MODE_IRQ | 0x80, /* IRQ */
    MODE_FIQ | 0xC0, /* FIQ */
};

void cpu_exception(int type)
{
    /* Switch mode, disable interrupts */
    uint32_t old_cpsr = get_cpsr();
    set_cpsr_full((old_cpsr & ~0x3F) | exc_flags[type]);
    *ptr_spsr() = old_cpsr;

    /* Branch-and-link to exception handler */
    arm.reg[14] = arm.reg[15];
    arm.reg[15] = type << 2;
    if (arm.control & 0x2000) /* High vectors */
        arm.reg[15] += 0xFFFF0000;
}

uint32_t get_cpsr_flags()
{
    return arm.cpsr_n << 31
         | arm.cpsr_z << 30
         | arm.cpsr_c << 29
         | arm.cpsr_v << 28;
}

void set_cpsr_flags(uint32_t flags)
{
    arm.cpsr_n = (flags >> 31) & 1;
    arm.cpsr_z = (flags >> 30) & 1;
    arm.cpsr_c = (flags >> 29) & 1;
    arm.cpsr_v = (flags >> 28) & 1;
}

// Get full CPSR register
uint32_t FASTCALL get_cpsr()
{
    return arm.cpsr_n << 31
         | arm.cpsr_z << 30
         | arm.cpsr_c << 29
         | arm.cpsr_v << 28
         | arm.cpsr_low28;
}

void set_cpsr_full(uint32_t cpsr)
{
    uint8_t old_mode = arm.cpsr_low28 & 0x1F,
            new_mode = cpsr & 0x1F;

    if(old_mode == new_mode)
        goto same_mode;

    // Only FIQ mode has more than 2 regs banked
    if(old_mode == MODE_FIQ)
        std::copy(arm.reg + 8, arm.reg + 13, arm.r8_fiq);
    else
        std::copy(arm.reg + 8, arm.reg + 13, arm.r8_usr);

    switch(old_mode)
    {
    case MODE_USR: case MODE_SYS:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_usr);
        break;
    case MODE_FIQ:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_fiq);
        break;
    case MODE_IRQ:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_irq);
        break;
    case MODE_SVC:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_svc);
        break;
    case MODE_ABT:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_abt);
        break;
    case MODE_UND:
        std::copy(arm.reg + 13, arm.reg + 15, arm.r13_und);
        break;
    default: assert(false);
    }

    if(new_mode == MODE_FIQ)
        std::copy(arm.r8_fiq, arm.r8_fiq + 5, arm.reg + 8);
    else
        std::copy(arm.r8_usr, arm.r8_usr + 5, arm.reg + 8);

    switch(new_mode)
    {
    case MODE_USR: case MODE_SYS:
        std::copy(arm.r13_usr, arm.r13_usr + 2, arm.reg + 13);
        break;
    case MODE_FIQ:
        std::copy(arm.r13_fiq, arm.r13_fiq + 2, arm.reg + 13);
        break;
    case MODE_IRQ:
        std::copy(arm.r13_irq, arm.r13_irq + 2, arm.reg + 13);
        break;
    case MODE_SVC:
        std::copy(arm.r13_svc, arm.r13_svc + 2, arm.reg + 13);
        break;
    case MODE_ABT:
        std::copy(arm.r13_abt, arm.r13_abt + 2, arm.reg + 13);
        break;
    case MODE_UND:
        std::copy(arm.r13_und, arm.r13_und + 2, arm.reg + 13);
        break;
    default: error("Invalid mode 0x%x\n", new_mode);
    }

    // Access permissions are different
    if((old_mode == MODE_USR) ^ (new_mode == MODE_USR))
        addr_cache_flush();

    same_mode:
    if(cpsr & 0x01000000)
        error("Jazelle not implemented!");

    arm.cpsr_n = (cpsr >> 31) & 1;
    arm.cpsr_z = (cpsr >> 30) & 1;
    arm.cpsr_c = (cpsr >> 29) & 1;
    arm.cpsr_v = (cpsr >> 28) & 1;
    arm.cpsr_low28 = cpsr & 0x090000FF; // Mask off reserved bits
    cpu_int_check();
}

void FASTCALL set_cpsr(uint32_t cpsr, uint32_t mask) {
    if (!(arm.cpsr_low28 & 0x0F)) {
        /* User mode. Don't change privileged or execution state bits */
        mask &= ~0x010000FF;
    }
    cpsr = (cpsr & mask) | (get_cpsr() & ~mask);
    if (cpsr & 0x20)
        error("Cannot set T bit with MSR instruction");
    set_cpsr_full(cpsr);
}

uint32_t *ptr_spsr()
{
    switch (arm.cpsr_low28 & 0x1F)
    {
        case MODE_FIQ: return &arm.spsr_fiq;
        case MODE_IRQ: return &arm.spsr_irq;
        case MODE_SVC: return &arm.spsr_svc;
        case MODE_ABT: return &arm.spsr_abt;
        case MODE_UND: return &arm.spsr_und;
    }
    error("Attempted to access SPSR from user or system mode");
}

uint32_t FASTCALL get_spsr() {
    return *ptr_spsr();
}

void FASTCALL set_spsr(uint32_t spsr, uint32_t mask) {
    *ptr_spsr() ^= (*ptr_spsr() ^ spsr) & mask;
}

uint32_t reg(uint8_t i)
{
    if(unlikely(i == 15))
        error("PC invalid in this context!\n");
    return arm.reg[i];
}

uint32_t reg_pc(uint8_t i)
{
    if(unlikely(i == 15))
        return arm.reg[15] + 4;
    return arm.reg[i];
}

uint32_t reg_pc_mem(uint8_t i)
{
    if(unlikely(i == 15))
        return arm.reg[15] + 8;
    return arm.reg[i];
}

void set_reg(uint8_t i, uint32_t value)
{
    if(unlikely(i == 15))
        error("PC invalid in this context!\n");
    arm.reg[i] = value;
}

void set_reg_pc(uint8_t i, uint32_t value)
{
    arm.reg[i] = value;
}

void set_reg_bx(uint8_t i, uint32_t value)
{
    arm.reg[i] = value;

    if(i == 15 && (value & 1))
    {
        arm.cpsr_low28 |= 0x20; // Enter Thumb mode
        arm.reg[15] -= 1;
    }
}

bool cpu_resume(const emu_snapshot *s)
{
    arm = s->cpu_state;
    cpu_events = s->cpu_state.cpu_events_state;
    return true;
}

bool cpu_suspend(emu_snapshot *s)
{
    s->cpu_state = arm;
    s->cpu_state.cpu_events_state = cpu_events;
    return true;
}
