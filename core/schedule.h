/* Declarations for schedule.c */

#ifndef _H_SCHEDULE
#define _H_SCHEDULE

#ifdef __cplusplus
extern "C" {
#endif

enum clock_id { CLOCK_CPU, CLOCK_AHB, CLOCK_APB, CLOCK_27M, CLOCK_12M, CLOCK_32K };

enum sched_item_index {
        SCHED_THROTTLE,
        SCHED_KEYPAD,
        SCHED_LCD,
        SCHED_TIMERS,
        SCHED_WATCHDOG,
        SCHED_NUM_ITEMS
};

#define SCHED_CASPLUS_TIMER1 SCHED_KEYPAD
#define SCHED_CASPLUS_TIMER2 SCHED_LCD
#define SCHED_CASPLUS_TIMER3 SCHED_TIMERS

struct sched_item {
        enum clock_id clock;
        int second; // -1 = disabled
        uint32_t tick;
        uint32_t cputick;
        void (*proc)(int index);
};

typedef struct sched_state {
    struct sched_item items[SCHED_NUM_ITEMS];
    uint32_t clock_rates[6];
    uint32_t next_cputick;
    int next_index; // -1 if no more events this second
} sched_state;

extern sched_state sched;

void sched_reset(void);
typedef struct emu_snapshot emu_snapshot;
bool sched_resume(const emu_snapshot *snapshot);
bool sched_suspend(emu_snapshot *snapshot);
void event_repeat(int index, uint32_t ticks);
void sched_update_next_event(uint32_t cputick);
uint32_t sched_process_pending_events();
void event_clear(int index);
void event_set(int index, int ticks);
uint32_t event_ticks_remaining(int index);
void sched_set_clocks(int count, uint32_t *new_rates);

#ifdef __cplusplus
}
#endif

#endif
