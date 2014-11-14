#include "emu.h"
#include <stdio.h>
#include <string.h>
#include "schedule.h"

uint32_t clock_rates[6] = { 0, 0, 0, 27000000, 12000000, 32768 };

struct sched_item sched_items[SCHED_NUM_ITEMS];

uint32_t next_cputick;
int next_index; // -1 if no more events this second

static inline uint32_t muldiv(uint32_t a, uint32_t b, uint32_t c) {
    #if defined(__i386__) || defined(__x86_64__)
        asm ("mull %1; divl %2" : "+a" (a) : "m" (b), "m" (c) : "edx");
        return a;
    #else
        uint64_t d = a;
        d *= b;
        return d / c;
    #endif
}

void sched_reset(void) {
	memset(sched_items, 0, sizeof sched_items);
}

void event_repeat(int index, uint32_t ticks) {
	struct sched_item *item = &sched_items[index];

	uint32_t prev = item->tick;
	item->second = ticks / clock_rates[item->clock];
	item->tick = ticks % clock_rates[item->clock];
	if (prev >= clock_rates[item->clock] - item->tick) {
		item->second++;
		item->tick -= clock_rates[item->clock];
	}
	item->tick += prev;

	item->cputick = muldiv(item->tick, clock_rates[CLOCK_CPU], clock_rates[item->clock]);
}

void sched_update_next_event(uint32_t cputick) {
	next_cputick = clock_rates[CLOCK_CPU];
	next_index = -1;
	int i;
	for (i = 0; i < SCHED_NUM_ITEMS; i++) {
		struct sched_item *item = &sched_items[i];
		if (item->proc != NULL && item->second == 0 && item->cputick < next_cputick) {
			next_cputick = item->cputick;
			next_index = i;
		}
	}
    //printf("Next event: (%8d,%d)\n", next_cputick, next_index);
	cycle_count_delta = cputick - next_cputick;
}

uint32_t sched_process_pending_events() {
	uint32_t cputick = next_cputick + cycle_count_delta;
	while (cputick >= next_cputick) {
		if (next_index < 0) {
            //printf("[%8d] New second\n", cputick);
			int i;
			for (i = 0; i < SCHED_NUM_ITEMS; i++) {
				if (sched_items[i].second >= 0)
					sched_items[i].second--;
			}
			cputick -= clock_rates[CLOCK_CPU];
		} else {
            //printf("[%8d/%8d] Event %d\n", cputick, next_cputick, next_index);
			sched_items[next_index].second = -1;
			sched_items[next_index].proc(next_index);
		}
		sched_update_next_event(cputick);
	}
	return cputick;
}

void event_clear(int index) {
	uint32_t cputick = sched_process_pending_events();

	sched_items[index].second = -1;

	sched_update_next_event(cputick);
}
void event_set(int index, int ticks) {
	uint32_t cputick = sched_process_pending_events();

	struct sched_item *item = &sched_items[index];
	item->tick = muldiv(cputick, clock_rates[item->clock], clock_rates[CLOCK_CPU]);
	event_repeat(index, ticks);

	sched_update_next_event(cputick);
}

uint32_t event_ticks_remaining(int index) {
	uint32_t cputick = sched_process_pending_events();

	struct sched_item *item = &sched_items[index];
	return item->second * clock_rates[item->clock]
	       + item->tick - muldiv(cputick, clock_rates[item->clock], clock_rates[CLOCK_CPU]);
}

void sched_set_clocks(int count, uint32_t *new_rates) {
	uint32_t cputick = sched_process_pending_events();

	uint32_t remaining[SCHED_NUM_ITEMS];
	int i;
	for (i = 0; i < SCHED_NUM_ITEMS; i++) {
		struct sched_item *item = &sched_items[i];
		if (item->second >= 0)
			remaining[i] = event_ticks_remaining(i);
	}
	cputick = muldiv(cputick, new_rates[CLOCK_CPU], clock_rates[CLOCK_CPU]);
	memcpy(clock_rates, new_rates, sizeof(uint32_t) * count);
	for (i = 0; i < SCHED_NUM_ITEMS; i++) {
		struct sched_item *item = &sched_items[i];
		if (item->second >= 0) {
			item->tick = muldiv(cputick, clock_rates[item->clock], clock_rates[CLOCK_CPU]);
			event_repeat(i, remaining[i]);
		}
	}

	sched_update_next_event(cputick);
}
