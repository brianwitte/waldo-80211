/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2023 Brian Witte
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <bpf/libbpf.h>
#include "kernel_metrics.h"
#include "waldo.skel.h"

static volatile sig_atomic_t stop;

static void sig_int(int signo)
{
	stop = 1;
}

// forward declaration of the struct
struct waldo_bpf;

// helper function to attach a single kprobe
static long attach_single_kprobe(struct waldo_bpf *obj, struct bpf_link **link,
				 struct bpf_program *prog,
				 const char *func_name)
{
	*link = bpf_program__attach_kprobe(prog, false, func_name);
	long ret = libbpf_get_error(*link);
	if (ret) {
		fprintf(stderr, "Failed to attach kprobe for %s: %s\n",
			func_name, strerror(-ret));
		return -1;
	}
	return 0;
}

// function to attach all kprobes
static long attach_kprobes(struct waldo_bpf *obj)
{
	if (attach_single_kprobe(obj, &obj->links.kprobe_ieee80211_register_hw,
				 obj->progs.kprobe_ieee80211_register_hw,
				 "ieee80211_register_hw") < 0 ||
	    attach_single_kprobe(obj,
				 &obj->links.kprobe_ieee80211_unregister_hw,
				 obj->progs.kprobe_ieee80211_unregister_hw,
				 "ieee80211_unregister_hw") < 0 ||
	    attach_single_kprobe(obj, &obj->links.kprobe_ieee80211_request_scan,
				 obj->progs.kprobe_ieee80211_request_scan,
				 "ieee80211_request_scan") < 0 ||
	    attach_single_kprobe(
		    obj, &obj->links.kretprobe_ieee80211_get_channel_khz,
		    obj->progs.kretprobe_ieee80211_get_channel_khz,
		    "ieee80211_get_channel_khz") < 0 ||
	    attach_single_kprobe(obj,
				 &obj->links.kprobe_ieee80211_scan_completed,
				 obj->progs.kprobe_ieee80211_scan_completed,
				 "ieee80211_scan_completed") < 0) {
		return -1;
	}

	// conditionally attach netdev kprobes based on env var
	char *env_var = getenv("ATTACH_NETDEV_KPROBES");
	if (env_var != NULL && strcmp(env_var, "1") == 0) {
		if (attach_single_kprobe(obj,
					 &obj->links.kprobe_register_netdev,
					 obj->progs.kprobe_register_netdev,
					 "register_netdev") < 0 ||
		    attach_single_kprobe(obj,
					 &obj->links.kprobe_unregister_netdev,
					 obj->progs.kprobe_unregister_netdev,
					 "unregister_netdev") < 0) {
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct waldo_bpf *skel;
	int err;

	// open load and verify bpf application
	skel = waldo_bpf__open_and_load();
	if (!skel) {
		fprintf(stderr, "Failed to open bpf skel\n");
		return 1;
	}

	// attach kprobes
	err = attach_kprobes(skel);
	if (err) {
		goto cleanup;
	}

	if (signal(SIGINT, sig_int) == SIG_ERR) {
		fprintf(stderr, "Cannot set signal handler: %s\n",
			strerror(errno));
		goto cleanup;
	}

	printf("To see bpf traces, run script: `./scripts/trace_pipe_log.sh`\n\n");
	printf("Run a scan with iw: `sudo /usr/sbin/iw dev wlp2s0 scan`\n\n");
	printf("You should see bpf logging scan duration in nanoseconds.\n\n");

	char start_time_str[100];
	char process_info_str[200];

	kmh_get_start_time("waldo", start_time_str, sizeof(start_time_str));
	printf("%s", start_time_str);

	while (!stop) {
		sleep(15);
		kmh_get_process_info(process_info_str, sizeof(process_info_str));
		printf("%s", process_info_str);
	}

cleanup:
	waldo_bpf__destroy(skel);
	return -err;
}
