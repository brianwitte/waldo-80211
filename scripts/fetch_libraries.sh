#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright(c) 2023 Brian Witte

# define urls of the release tarballs
LIBBPF_URL="https://github.com/libbpf/libbpf/archive/refs/tags/v1.2.2.tar.gz"
BPFTOOL_URL="https://github.com/libbpf/bpftool/releases/download/v7.2.0/bpftool-libbpf-v7.2.0-sources.tar.gz"

# define where to extract the libraries
LIBBPF_DIR="libbpf"
BPFTOOL_DIR="bpftool"

# create directories
mkdir -p "$LIBBPF_DIR" "$BPFTOOL_DIR"

# download and extract libraries
echo "Fetching and extracting libbpf..."
curl -Ls "$LIBBPF_URL" | tar -xz -C "$LIBBPF_DIR" --strip-components=1

echo "Fetching and extracting bpftool..."
curl -Ls "$BPFTOOL_URL" | tar -xz -C "$BPFTOOL_DIR" --strip-components=1

echo "Libraries fetched and extracted successfully."
