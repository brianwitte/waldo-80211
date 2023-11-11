#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright(c) 2023 Brian Witte

FMT_SRC_FILES="waldo.c waldo.bpf.c test/test.c test/metrics.c"
FMT_HDR_FILES="metrics.h"

format_file() {
	local FILE="$1"
	local TMP_FILE=$(mktemp)

	clang-format "$FILE" >"$TMP_FILE"

	echo "Showing diff for $FILE..."
	diff -u "$FILE" "$TMP_FILE"

	read -p "Apply changes to $FILE? (y/n): " -r
	echo

	if [[ $REPLY =~ ^[Yy]$ ]]; then
		mv "$TMP_FILE" "$FILE"
		echo "File $FILE updated."
	else
		echo "Changes to $FILE not applied."
	fi

	[ -f "$TMP_FILE" ] && rm "$TMP_FILE"
}

for file in $FMT_SRC_FILES; do
	if [ ! -f "$file" ]; then
		echo "File not found: $file"
		continue
	fi

	format_file "$file"
done

for file in $FMT_HDR_FILES; do
	if [ ! -f "$file" ]; then
		echo "File not found: $file"
		continue
	fi

	format_file "$file"
done
