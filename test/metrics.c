/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2023 Brian Witte
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // for sleep function
#include "../kernel_metrics.h"
#include "greatest.h" // single header test framework

static void *point_test_get_value(void *data)
{
	return data;
}

TEST point_creation_and_value_test(void)
{
	int test_value = 42;
	km_point *g = create_point(point_test_get_value, NULL, &test_value);
	ASSERT(g != NULL);
	ASSERT_EQ(test_value, *(int *)point_get_value(g));
	destroy_point(g);
	PASS();
}

TEST counter_increment_decrement_test(void)
{
	km_counter *c = create_counter();
	ASSERT(c != NULL);
	counter_inc(c, 5);
	ASSERT_EQ(5, counter_get_value(c));
	counter_dec(c, 3);
	ASSERT_EQ(2, counter_get_value(c));
	destroy_counter(c);
	PASS();
}

TEST rangemap_update_and_mean_test(void)
{
	km_rangemap *h = create_rangemap();
	ASSERT(h != NULL);
	rangemap_update(h, 10.0);
	rangemap_update(h, 20.0);
	ASSERT_EQ(2, h->count);
	ASSERT_IN_RANGE(15.0, rangemap_mean(h), 0.001);
	destroy_rangemap(h);
	PASS();
}

TEST meter_mark_and_mean_rate_test(void)
{
	km_meter *m = create_meter();
	ASSERT(m != NULL);
	meter_mark(m, 1);
	sleep(1); // to simulate time passing
	ASSERT(m->eventCount > 0);
	double mean_rate = meter_mean_rate(m);
	ASSERT(mean_rate > 0.0);
	destroy_meter(m);
	PASS();
}

TEST timer_start_stop_test(void)
{
	km_timer *t = create_timer();
	ASSERT(t != NULL);
	timer_start(t);
	sleep(1); // to simulate some operation
	timer_stop(t);
	ASSERT(t->rangemap.count > 0);
	ASSERT(t->rangemap.totalTime > 0);
	ASSERT(t->meter.eventCount > 0);
	destroy_timer(t);
	PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
	GREATEST_MAIN_BEGIN();
	RUN_TEST(point_creation_and_value_test);
	RUN_TEST(counter_increment_decrement_test);
	RUN_TEST(rangemap_update_and_mean_test);
	RUN_TEST(meter_mark_and_mean_rate_test);
	RUN_TEST(timer_start_stop_test);
	GREATEST_MAIN_END();
}
