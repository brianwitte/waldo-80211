#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2023, Brian Witte

# create the directory structure for the vmlinux header file
mkdir -p ../vmlinux/x86

# set the input file for bpftool. use the first script argument if provided,
# otherwise default to /sys/kernel/btf/vmlinux
INPUT_FILE=${1:-/sys/kernel/btf/vmlinux}

# run bpftool to dump BTF (BPF Type Format) information from the specified input
# file.  the output is formatted in C and redirected to vmlinux.h in the
# specified directory
../build_output/bpftool/bootstrap/bpftool btf dump file \
                                          $INPUT_FILE format c \
                                          >../vmlinux/x86/vmlinux.h

# create a symbolic link to the generated vmlinux.h for easier access from a
# common directory
ln -s ../vmlinux/x86/vmlinux.h ../vmlinux/vmlinux.h
