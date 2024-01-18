/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2023 Qais Yousef */
#include <stdlib.h>
#include <string.h>

#include "parse_argp.h"

const char *argp_program_version = "sched-analyzer 0.1";
const char *argp_program_bug_address = "<qyousef@layalina.io>";

static char doc[] =
"Extract scheduer data using BPF and emit them into perfetto as track events";

struct sa_opts sa_opts = {
	/* perfetto opts */
	.system = true,
	.app = false,
	/* controls */
	.output = "sched-analyzer.perfetto-trace",
	.output_path = NULL,
	.max_size = 250 * 1024 * 1024, /* 250MiB */
	/* events */
	.util_avg_cpu = false,
	.util_avg_task = false,
	.util_avg_rt = false,
	.util_avg_dl = false,
	.util_avg_irq = false,
	.util_avg_thermal = false,
	.util_est_cpu = false,
	.util_est_task = false,
	.cpu_nr_running = false,
	.cpu_freq = false,
	.cpu_idle = false,
	.softirq = false,
	.sched_switch = false,
	.load_balance = false,
	/* filters */
	.pid = 0,
	.comm = { 0 },
};

enum sa_opts_flags {
	OPT_DUMMY_START = 0x80,

	/* perfetto opts */
	OPT_SYSTEM,
	OPT_APP,

	/* controls */
	OPT_OUTPUT,
	OPT_OUTPUT_PATH,
	OPT_MAX_SIZE,

	/* events */
	OPT_LOAD_AVG,
	OPT_RUNNABLE_AVG,
	OPT_UTIL_AVG,
	OPT_LOAD_AVG_CPU,
	OPT_RUNNABLE_AVG_CPU,
	OPT_UTIL_AVG_CPU,
	OPT_LOAD_AVG_TASK,
	OPT_RUNNABLE_AVG_TASK,
	OPT_UTIL_AVG_TASK,
	OPT_UTIL_AVG_RT,
	OPT_UTIL_AVG_DL,
	OPT_UTIL_AVG_IRQ,
	OPT_UTIL_AVG_THERMAL,
	OPT_UTIL_EST,
	OPT_UTIL_EST_CPU,
	OPT_UTIL_EST_TASK,
	OPT_CPU_NR_RUNNING,
	OPT_LOAD_BALANCE,

	/* filters */
	OPT_FILTER_PID,
	OPT_FILTER_COMM,
};

static const struct argp_option options[] = {
	/* perfetto opts */
	{ "system", OPT_SYSTEM, 0, 0, "Collect system wide data, requires traced and traced_probes to be running (default)." },
	{ "app", OPT_APP, 0, 0, "Collect only data generated by this app. Runs standalone without external dependencies on traced." },
	/* controls */
	{ "output", OPT_OUTPUT, "FILE", 0, "Filename of the perfetto-trace file to produce." },
	{ "output_path", OPT_OUTPUT_PATH, "PATH", 0, "Path to store perfetto-trace. PWD by default for perfetto." },
	{ "max_size", OPT_MAX_SIZE, "SIZE(KiB)", 0, "Maximum size of perfetto file to produce, 250MiB by default." },
	/* events */
	{ "load_avg", OPT_LOAD_AVG, 0, 0, "Collect load_avg for CPU and tasks." },
	{ "runnable_avg", OPT_RUNNABLE_AVG, 0, 0, "Collect runnable_avg for CPU and tasks." },
	{ "util_avg", OPT_UTIL_AVG, 0, 0, "Collect util_avg for CPU, tasks, irq, dl and rt." },
	{ "load_avg_cpu", OPT_LOAD_AVG_CPU, 0, 0, "Collect load_avg for CPU." },
	{ "runnable_avg_cpu", OPT_RUNNABLE_AVG_CPU, 0, 0, "Collect runnable_avg for CPU." },
	{ "util_avg_cpu", OPT_UTIL_AVG_CPU, 0, 0, "Collect util_avg for CPU." },
	{ "load_avg_task", OPT_LOAD_AVG_TASK, 0, 0, "Collect load_avg for tasks." },
	{ "runnable_avg_task", OPT_RUNNABLE_AVG_TASK, 0, 0, "Collect runnable_avg for tasks." },
	{ "util_avg_task", OPT_UTIL_AVG_TASK, 0, 0, "Collect util_avg for tasks." },
	{ "util_avg_rt", OPT_UTIL_AVG_RT, 0, 0, "Collect util_avg for rt." },
	{ "util_avg_dl", OPT_UTIL_AVG_DL, 0, 0, "Collect util_avg for dl." },
	{ "util_avg_irq", OPT_UTIL_AVG_IRQ, 0, 0, "Collect util_avg for irq." },
	{ "util_avg_thermal", OPT_UTIL_AVG_THERMAL, 0, 0, "Collect util_avg for thermal pressure." },
	{ "util_est", OPT_UTIL_EST, 0, 0, "Collect util_est for CPU and tasks." },
	{ "util_est_cpu", OPT_UTIL_EST_CPU, 0, 0, "Collect util_est for CPU." },
	{ "util_est_task", OPT_UTIL_EST_TASK, 0, 0, "Collect util_est for tasks." },
	{ "cpu_nr_running", OPT_CPU_NR_RUNNING, 0, 0, "Collect nr_running tasks for each CPU." },
	{ "load_balance", OPT_LOAD_BALANCE, 0, 0, "Collect load balance related info." },
	{ "pid", OPT_FILTER_PID, "PID", 0, "Collect data for task match pid only." },
	{ "comm", OPT_FILTER_COMM, "COMM", 0, "Collect data for tasks that contain comm only." },
	{ 0 },
};

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	char *end_ptr;

	switch (key) {
	/* perfetto opts */
	case OPT_SYSTEM:
		sa_opts.system = true;
		sa_opts.app = false;
		break;
	case OPT_APP:
		sa_opts.system = false;
		sa_opts.app = true;
		break;
	/* controls */
	case OPT_OUTPUT:
		sa_opts.output = arg;
		break;
	case OPT_OUTPUT_PATH:
		sa_opts.output_path = arg;
		break;
	case OPT_MAX_SIZE:
		errno = 0;
		sa_opts.max_size = strtol(arg, &end_ptr, 0) * 1024;
		if (errno != 0) {
			perror("Unsupported max_size value\n");
			return errno;
		}
		if (end_ptr == arg) {
			fprintf(stderr, "max_size: no digits were found\n");
			argp_usage(state);
			return -EINVAL;
		}
		break;
	/* events */
	case OPT_LOAD_AVG:
		sa_opts.load_avg_cpu = true;
		sa_opts.load_avg_task = true;
		break;
	case OPT_RUNNABLE_AVG:
		sa_opts.load_avg_cpu = true;
		sa_opts.runnable_avg_task = true;
		break;
	case OPT_UTIL_AVG:
		sa_opts.util_avg_cpu = true;
		sa_opts.util_avg_task = true;
		sa_opts.util_avg_rt = true;
		sa_opts.util_avg_dl = true;
		sa_opts.util_avg_irq = true;
		sa_opts.util_avg_thermal = true;
		break;
	case OPT_LOAD_AVG_CPU:
		sa_opts.load_avg_cpu = true;
		break;
	case OPT_RUNNABLE_AVG_CPU:
		sa_opts.runnable_avg_cpu = true;
		break;
	case OPT_UTIL_AVG_CPU:
		sa_opts.util_avg_cpu = true;
		break;
	case OPT_LOAD_AVG_TASK:
		sa_opts.load_avg_task = true;
		break;
	case OPT_RUNNABLE_AVG_TASK:
		sa_opts.runnable_avg_task = true;
		break;
	case OPT_UTIL_AVG_TASK:
		sa_opts.util_avg_task = true;
		break;
	case OPT_UTIL_AVG_RT:
		sa_opts.util_avg_rt = true;
		break;
	case OPT_UTIL_AVG_DL:
		sa_opts.util_avg_dl = true;
		break;
	case OPT_UTIL_AVG_IRQ:
		sa_opts.util_avg_irq = true;
		break;
	case OPT_UTIL_AVG_THERMAL:
		sa_opts.util_avg_thermal = true;
		break;
	case OPT_UTIL_EST:
		sa_opts.util_est_cpu = true;
		sa_opts.util_est_task = true;
		break;
	case OPT_UTIL_EST_CPU:
		sa_opts.util_est_cpu = true;
		break;
	case OPT_UTIL_EST_TASK:
		sa_opts.util_est_task = true;
		break;
	case OPT_CPU_NR_RUNNING:
		sa_opts.cpu_nr_running = true;
		break;
	case OPT_LOAD_BALANCE:
		sa_opts.load_balance = true;
		break;
	case OPT_FILTER_PID:
		errno = 0;
		sa_opts.pid = strtol(arg, &end_ptr, 0);
		if (errno != 0) {
			perror("Unsupported pid value\n");
			return errno;
		}
		if (end_ptr == arg) {
			fprintf(stderr, "pid: no digits were found\n");
			argp_usage(state);
			return -EINVAL;
		}
		break;
	case OPT_FILTER_COMM:
		strncpy(sa_opts.comm, arg, TASK_COMM_LEN-1);
		sa_opts.comm[TASK_COMM_LEN-1] = 0;
		break;
	case ARGP_KEY_ARG:
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

const struct argp argp = {
	.options = options,
	.parser = parse_arg,
	.doc = doc,
};
