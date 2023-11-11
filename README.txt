# waldo - debug/test/trace tool for linux 80211 stack

༼つಠ益ಠ༽つ ─=≡ΣO))) W A L D O
Wireless Analyzer - Log, Debug, Observe

## SUMMARY

A tool for Linux wireless leveraging the various native 80211 interfaces and
select APIs along with libbpf/kprobes/tracepoints to understand and validate
various wireless interactions between userspace, kernelspace, drivers, etc.

On one hand, it can do simple connection testing and one-off "internet speed
tests", and on the other, it can coordinate tracepoints, kprobes, commands,
and system logs for deeper understanding of wireless subsystem operation.

It can also adaptively test based on signal quality, track driver registration
and scans via 80211 interfaces, perform spectral analysis for non-wifi signal
interference, analyze channel utilization and latency metrics, etc.

In daemon mode, `waldo` will support multiple backends, e.g., SQLite and various
log/trace forwarding schemes for structuring analysis results.

## SETUP

### Requirements:
- kernel version 5.15.x or higher
- bpf enabled kernel
- make
- gcc
- clang
- git
- curl

#### Optional:
- clang-format
- bear

### Installation:
1. Clone the repository (choose one):
   `git clone git@github.com:brianwitte/waldo-80211.git`
   `git clone https://github.com/brianwitte/waldo-80211.git`
2. Fetch libbpf and bpftool standalone deps:
   `cd waldo-80211`
   `./scripts/fetch_libraries.sh`
3. Build `waldo` from source:
   `make`

If compilation succeeds, `waldo` binary will be in project root.

## USAGE

### Command:
Run `sudo ./waldo` to start...
Follow output from there.

## LICENSE
`waldo` is licensed under the GPL-2.0 license.

All source files within this project are GPL-2.0-or-later unless
specified otherwise at top of file via SPDX-License-Identifier.

See COPYING file in project root accompanying this README.
