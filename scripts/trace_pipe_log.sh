#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright(c) 2023 Brian Witte

log_file="trace_pipe_$(date '+%Y-%m-%dT%H:%M:%S').log"

sudo cat /sys/kernel/debug/tracing/trace_pipe | awk -v logfile="$log_file" '{
    print $0;
    print $0 >> logfile;
    fflush("");
}'
