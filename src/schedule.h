/* Declarations for schedule.c */

#ifndef _H_SCHEDULE
#define _H_SCHEDULE

enum clock_id { CLOCK_CPU, CLOCK_AHB, CLOCK_APB, CLOCK_27M, CLOCK_12M, CLOCK_32K };
extern u32 clock_rates[6];
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
extern struct sched_item {
        enum clock_id clock;
        int second; // -1 = disabled
        u32 tick;
        u32 cputick;
        void (*proc)(int index);
} sched_items[SCHED_NUM_ITEMS];

void sched_reset(void);
void event_repeat(int index, u32 ticks);
void sched_update_next_event(u32 cputick);
u32 sched_process_pending_events();
void event_clear(int index);
void event_set(int index, int ticks);
u32 event_ticks_remaining(int index);
void sched_set_clocks(int count, u32 *new_rates);

#endif
