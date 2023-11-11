/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2023 Brian Witte
 */

#ifndef METRICS_H
#define METRICS_H

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

static inline void kmh_get_start_time(const char *ps_name, char *buffer,
				     size_t buffer_size)
{
	pid_t pid = getpid();
	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);
	char time_str[30];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
	snprintf(buffer, buffer_size, "%s (pid: %d) started at %s\n", ps_name,
		 pid, time_str);
}
static inline double kmh_calculate_running_time()
{
	struct sysinfo info;
	sysinfo(&info);
	double uptime_seconds = (double)info.uptime;

	pid_t pid = getpid();
	char path[40];
	snprintf(path, sizeof(path), "/proc/%d/stat", pid);
	FILE *fp = fopen(path, "r");
	if (fp) {
		long long start_time;
		// Skip the first 21 words in /proc/[pid]/stat
		for (int i = 0; i < 21; i++) {
			fscanf(fp, "%*s");
		}
		fscanf(fp, "%lld", &start_time);
		fclose(fp);

		double ticks_per_second = sysconf(_SC_CLK_TCK);
		double start_time_seconds =
			(double)start_time / ticks_per_second;
		return uptime_seconds - start_time_seconds;
	}
	return 0.0;
}

static inline void kmh_get_process_info(char *buffer, size_t buffer_size)
{
	pid_t pid = getpid();
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);

	double running_time = kmh_calculate_running_time();

	snprintf(
		buffer, buffer_size,
		"pid: %d, cpu utime = %ld.%06ld sec, mem maxrss = %ld kB, running time: %.2f seconds\n",
		pid, usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
		usage.ru_maxrss, running_time);
}

typedef struct {
	void *data;
	void (*update)(void *data);
	void *(*get_value)(void *data);
} km_point;

static inline km_point *create_point(void *(*get_value_func)(void *),
				    void (*update_func)(void *), void *data)
{
	km_point *point = (km_point *)malloc(sizeof(km_point));
	if (point) {
		point->data = data;
		point->get_value = get_value_func;
		point->update = update_func;
	}
	return point;
}

static inline void *point_get_value(km_point *point)
{
	if (point && point->get_value) {
		return point->get_value(point->data);
	}
	return NULL;
}

static inline void point_update(km_point *point)
{
	if (point && point->update) {
		point->update(point->data);
	}
}

static inline void destroy_point(km_point *point)
{
	if (point) {
		free(point);
	}
}

typedef struct {
	int64_t count;
} km_counter;

static inline km_counter *create_counter()
{
	km_counter *counter = (km_counter *)malloc(sizeof(km_counter));
	if (counter) {
		counter->count = 0;
	}
	return counter;
}

static inline void counter_inc(km_counter *counter, int64_t value)
{
	if (counter) {
		counter->count += value;
	}
}

static inline void counter_dec(km_counter *counter, int64_t value)
{
	if (counter) {
		counter->count -= value;
	}
}

static inline int64_t counter_get_value(km_counter *counter)
{
	return counter ? counter->count : 0;
}

static inline void destroy_counter(km_counter *counter)
{
	if (counter) {
		free(counter);
	}
}

typedef struct {
	long long count;
	double totalTime;
} km_rangemap;

static inline km_rangemap *create_rangemap()
{
	km_rangemap *rangemap = (km_rangemap *)malloc(sizeof(km_rangemap));
	if (rangemap) {
		rangemap->count = 0;
		rangemap->totalTime = 0;
	}
	return rangemap;
}

static inline void rangemap_update(km_rangemap *rangemap, double duration)
{
	if (rangemap) {
		rangemap->count++;
		rangemap->totalTime += duration;
	}
}

static inline double rangemap_mean(const km_rangemap *rangemap)
{
	if (!rangemap || rangemap->count == 0)
		return 0;
	return rangemap->totalTime / rangemap->count;
}

static inline void destroy_rangemap(km_rangemap *rangemap)
{
	if (rangemap) {
		free(rangemap);
	}
}

typedef struct {
	long long eventCount;
	time_t startTime;
} km_meter;

static inline km_meter *create_meter()
{
	km_meter *meter = (km_meter *)malloc(sizeof(km_meter));
	if (meter) {
		meter->eventCount = 0;
		meter->startTime = time(NULL);
	}
	return meter;
}

static inline void meter_mark(km_meter *meter, long long count)
{
	if (meter) {
		meter->eventCount += count;
	}
}

static inline double meter_mean_rate(const km_meter *meter)
{
	if (!meter || meter->eventCount == 0)
		return 0;

	time_t now = time(NULL);
	double elapsedSeconds = difftime(now, meter->startTime);

	if (elapsedSeconds == 0)
		return 0;
	return (double)meter->eventCount / elapsedSeconds;
}

static inline void destroy_meter(km_meter *meter)
{
	if (meter) {
		free(meter);
	}
}

typedef struct {
	km_meter meter;
	km_rangemap rangemap;
	time_t startTime;
} km_timer;

static inline km_timer *create_timer()
{
	km_timer *timer = (km_timer *)malloc(sizeof(km_timer));
	if (timer) {
		timer->meter = *create_meter();
		timer->rangemap = *create_rangemap();
		timer->startTime = time(NULL);
	}
	return timer;
}

static inline void timer_start(km_timer *timer)
{
	if (timer) {
		timer->startTime = clock();
	}
}

static inline void timer_stop(km_timer *timer)
{
	if (timer) {
		clock_t endTime = clock();
		double duration =
			(double)(endTime - timer->startTime) / CLOCKS_PER_SEC;
		rangemap_update(&timer->rangemap, duration);
		meter_mark(&timer->meter, 1);
	}
}

static inline void destroy_timer(km_timer *timer)
{
	if (timer) {
		free(timer);
	}
}

#endif // METRICS_H
