# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright(c) 2023 Brian Witte

# check for clang
CLANG ?= $(shell command -v clang 2> /dev/null)
ifndef CLANG
$(error "clang not found. Please install clang or set CLANG env var.")
endif

# build artifact dir
BUILD_OUTPUT := build_output

# libbpf dir in project root
LIBBPF_SRC := $(abspath libbpf/src)
LIBBPF_OBJ := $(abspath $(BUILD_OUTPUT)/libbpf.a)

# bpftool dir in project root
BPFTOOL_SRC := $(abspath bpftool/src)
BPFTOOL_OUTPUT ?= $(abspath $(BUILD_OUTPUT)/bpftool)
BPFTOOL ?= $(BPFTOOL_OUTPUT)/bootstrap/bpftool

# determine system architecture
ARCH ?= $(shell uname -m | awk '{ \
    gsub("aarch64", "arm64"); \
    gsub("arm.*", "arm"); \
    gsub("loongarch64", "loongarch"); \
    gsub("mips.*", "mips"); \
    gsub("ppc64le", "powerpc"); \
    gsub("riscv64", "riscv"); \
    gsub("x86_64", "x86"); \
    print; \
}')

VMLINUX := vmlinux/$(ARCH)/vmlinux.h
INCLUDES := -I$(BUILD_OUTPUT) -Ilibbpf/include/uapi -I$(dir $(VMLINUX))
BPF_CFLAGS += -D__TARGET_ARCH_$(ARCH)
CFLAGS := -g -Wall
LDFLAGS := -lelf -lz

.PHONY: all clean waldo test

# build everything
all: waldo

# clean build artifacts
clean:
	@echo -n "Cleaning up... "
	@rm -rf $(BUILD_OUTPUT) waldo
	@echo "done."

# create necessary directories
$(BUILD_OUTPUT) $(BUILD_OUTPUT)/libbpf $(BPFTOOL_OUTPUT):
	@echo "Creating directory $@"
	@mkdir -p $@

#################################################################################
# MAIN COMPILATION TARGETS - start;
#################################################################################

#--------------------------------------------------------------------------------
# **build libbpf** (`$(LIBBPF_OBJ)`):
#
# - compiler: gcc (implied by `$(MAKE)` without specifying a compiler)
#
# - why gcc: `libbpf` is a user-space library providing an api for loading
#   and managing bpf programs and maps. gcc is well-suited for compiling
#   standard c libraries meant for userspace.
#
# - function: compiles the `libbpf` library from its source code. this
#   library is essential for the user-space application to interface with bpf
#   programs in the kernel. it provides functions to load bpf programs, attach
#   them to hooks in the kernel, and manipulate bpf maps.
#
$(LIBBPF_OBJ): $(wildcard $(LIBBPF_SRC)/*.[ch] $(LIBBPF_SRC)/Makefile) | $(BUILD_OUTPUT)/libbpf
	@echo "Building libbpf..."
	@$(MAKE) -C $(LIBBPF_SRC) \
		BUILD_STATIC_ONLY=1 \
		OBJDIR=$(dir $@)/libbpf \
		DESTDIR=$(dir $@) \
		INCLUDEDIR= LIBDIR= UAPIDIR= \
		install

#--------------------------------------------------------------------------------
# **build bpftool** (`$(BPFTOOL)`):
#
# - compiler: gcc (implied by `$(MAKE)` without specifying a compiler)
#
# - why gcc: `bpftool` is a user-space utility for bpf-related tasks, and
#   gcc is sufficient for compiling standard c code for user-space
#   applications.
#
# - function: compiles the `bpftool` utility, which is used for various
#   bpf-related operations, including debugging and inspecting bpf programs
#   and maps.
#
$(BPFTOOL): | $(BPFTOOL_OUTPUT)
	@echo "Building bpftool..."
	@$(MAKE) ARCH= CROSS_COMPILE= OUTPUT=$(BPFTOOL_OUTPUT)/ -C $(BPFTOOL_SRC) bootstrap

#--------------------------------------------------------------------------------
# **build bpf code** (`$(BUILD_OUTPUT)/waldo.bpf.o`):
#
# - compiler: clang (required)
#
# - why clang: clang is required for compiling bpf code due to its ability
#   to target the bpf bytecode format, which gcc does not support.
#
# - function: compiles bpf program code (`waldo.bpf.c`) into bpf bytecode
#   (`waldo.bpf.o`), which can be loaded into the linux kernel.
#
$(BUILD_OUTPUT)/waldo.bpf.o: waldo.bpf.c $(LIBBPF_OBJ) | $(BUILD_OUTPUT) $(BPFTOOL)
	@echo -n "Compiling BPF code... "
	@$(CLANG) -g -O2 -target bpf $(BPF_CFLAGS) $(INCLUDES) -c $< -o $(@:.o=.tmp.o)
	@$(BPFTOOL) gen object $@ $(@:.o=.tmp.o)
	@echo "done."

#--------------------------------------------------------------------------------
# **generate bpf skeletons** (`$(BUILD_OUTPUT)/waldo.skel.h`):
#
# - tool used: `bpftool`
#
# - function: generates bpf skeleton headers from the compiled bpf
#   bytecode. these headers provide an api for the user-space application to
#   interact with the bpf program.
#
$(BUILD_OUTPUT)/waldo.skel.h: $(BUILD_OUTPUT)/waldo.bpf.o | $(BUILD_OUTPUT) $(BPFTOOL)
	@echo -n "Generating BPF skeletons... "
	@$(BPFTOOL) gen skeleton $< > $@
	@echo "done."

#--------------------------------------------------------------------------------
# **build userspace code** (`$(BUILD_OUTPUT)/waldo.o`):
#
# - compiler: gcc (implied using `$(CC)`)
#
# - why gcc: the user-space part of the bpf application (like `waldo.c`) can
#   be compiled using gcc since it is standard c code not requiring
#   bpf-specific bytecode generation.
#
# - function: compiles the user-space part of the bpf application, which
#   interacts with the kernel via the bpf skeleton api.
#
$(BUILD_OUTPUT)/waldo.o: waldo.c $(BUILD_OUTPUT)/waldo.skel.h | $(BUILD_OUTPUT) $(BPFTOOL)
	@echo -n "Compiling userspace code... "
	@$(CC) $(CFLAGS) $(INCLUDES) -I$(BUILD_OUTPUT) -c $< -o $@
	@echo "done."

#--------------------------------------------------------------------------------
# **build application binary** (`waldo`):
#
# - compiler: gcc (implied using `$(CC)`)
#
# - why gcc: links the userspace object files (including those interacting
#   with the bpf skeleton) into a single executable binary. gcc is preferred
#   for this task as it involves standard linking of userspace code.
#
# - function: creates the final executable (`waldo`) comprising both the
#   userspace application and the bpf bytecode logic.
#
waldo: $(BUILD_OUTPUT)/waldo.o $(LIBBPF_OBJ)
	@echo -n "Linking waldo binary... "
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "done."

#################################################################################
# MAIN COMPILATION TARGETS - end;
#################################################################################

# run tests
test:
	$(MAKE) -C test

# useful for ensuring a clean build state and avoiding subtle bugs due to
# incomplete builds
.DELETE_ON_ERROR:

# prevents make from automatically deleting any intermediate files created
# during the build
.SECONDARY:
