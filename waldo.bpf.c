/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2023 Brian Witte
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "GPL";

static __always_inline void log_event(const char *event_name, void *dev_ptr)
{
	long pid_tgid = bpf_get_current_pid_tgid();
	pid_t pid = pid_tgid >> 32;
	char comm[16];
	bpf_get_current_comm(&comm, sizeof(comm));

	if (dev_ptr) {
		bpf_printk("pid: %d, command: %s, %s called for dev: %p\n", pid,
			   comm, event_name, dev_ptr);
	} else {
		bpf_printk("pid: %d, command: %s, %s called.\n", pid, comm,
			   event_name);
	}
}

SEC("kprobe/ieee80211_register_hw")
int BPF_KPROBE(kprobe_ieee80211_register_hw)
{
	log_event("ieee80211_register_hw", NULL);
	return 0;
}

SEC("kprobe/ieee80211_unregister_hw")
int BPF_KPROBE(kprobe_ieee80211_unregister_hw)
{
	log_event("ieee80211_unregister_hw", NULL);
	return 0;
}

SEC("kprobe/register_netdev")
int BPF_KPROBE(kprobe_register_netdev, struct net_device *dev)
{
	log_event("register_netdev", dev);
	return 0;
}

SEC("kprobe/unregister_netdev")
int BPF_KPROBE(kprobe_unregister_netdev, struct net_device *dev)
{
	log_event("unregister_netdev", dev);
	return 0;
}

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 8192);
	__type(key, u32);
	__type(value, u64);
} map_start_time SEC(".maps");

#define SCAN_ID 15170

SEC("kprobe/ieee80211_request_scan")
int BPF_KPROBE(kprobe_ieee80211_request_scan, struct net_device *dev)
{
	if (!dev) {
		bpf_printk("ieee80211_request_scan: dev is NULL\n");
		return 0;
	}

	u32 hc_scan_id = SCAN_ID;
	bpf_printk("ieee80211_request_scan hc_scan_id var=%d\n", &hc_scan_id);
	u64 ts = bpf_ktime_get_ns();

	int result =
		bpf_map_update_elem(&map_start_time, &hc_scan_id, &ts, BPF_ANY);

	if (result != 0) {
		bpf_printk(
			"ieee80211_request_scan: map update failed, hc_scan_id=%u, error=%d\n",
			hc_scan_id, result);
	}
	u64 *start_ts = bpf_map_lookup_elem(&map_start_time, &hc_scan_id);
	bpf_printk(
		"ieee80211_request_scan: map updated for key->hc_scan_id=%u, value->=%d\n",
		hc_scan_id, start_ts);

	return 0;
}

SEC("kprobe/ieee80211_scan_completed")
int BPF_KPROBE(kprobe_ieee80211_scan_completed, struct net_device *dev)
{
	if (!dev) {
		bpf_printk("ieee80211_scan_completed: dev is NULL\n");
		return 0;
	}

	u32 hc_scan_id = SCAN_ID;

	bpf_printk("ieee80211_scan_completed hc_scan_id var=%d\n", hc_scan_id);
	u64 *start_ts = bpf_map_lookup_elem(&map_start_time, &hc_scan_id);

	if (start_ts) {
		u64 duration = bpf_ktime_get_ns() - *start_ts;
		bpf_printk(
			"scan completed! duration is %llu ns, hc_scan_id=%u\n",
			duration, hc_scan_id);
		int result = bpf_map_delete_elem(&map_start_time, &hc_scan_id);
		if (result != 0) {
			bpf_printk(
				"ieee80211_scan_completed: map delete failed, hc_scan_id=%u, error=%d\n",
				hc_scan_id, result);
		}
	} else {
		bpf_printk(
			"ieee80211_scan_completed: no start time found for hc_scan_id=%u\n",
			hc_scan_id);
	}
	return 0;
}

SEC("kretprobe/ieee80211_get_channel_khz")
int BPF_KRETPROBE(kretprobe_ieee80211_get_channel_khz,
		  struct ieee80211_channel *ch)
{
	// implement channel tracing logic here
	// note: direct dereferencing of ch may not work due to verifier constraints
	return 0;
}
