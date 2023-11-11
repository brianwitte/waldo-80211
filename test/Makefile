# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright(c) 2023 Brian Witte

CC = gcc
CFLAGS = -Wall -Wextra -g

BUILD_DIR = build

TEST_TARGET = $(BUILD_DIR)/test
METRICS_TARGET = $(BUILD_DIR)/metrics

TEST_SRCS = test.c
METRICS_SRCS = metrics.c
TEST_OBJS = $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
METRICS_OBJS = $(METRICS_SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(TEST_TARGET) $(METRICS_TARGET) run

$(TEST_TARGET): $(TEST_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(METRICS_TARGET): $(METRICS_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TEST_TARGET)
	./$(TEST_TARGET)

metrics: $(METRICS_TARGET)
	./$(METRICS_TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run metrics
