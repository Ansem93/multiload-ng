/*
 * Copyright (C) 2016 Mario Cianciolo <mr.udda@gmail.com>
 *
 * This file is part of multiload-ng.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include <config.h>

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include "graph-data.h"
#include "preferences.h"
#include "util.h"


enum {
	CPU_USER	= 0,
	CPU_NICE	= 1,
	CPU_SYS		= 2,
	CPU_IOWAIT	= 3,
	CPU_IDLE	= 4,

	CPU_MAX		= 5
};


void
multiload_graph_cpu_get_data (int Maximum, int data [4], LoadGraph *g, CpuData *xd)
{
	FILE *f;
	guint64 irq, softirq, total;
	gboolean first_call = FALSE;
	guint i;

	guint64 time[CPU_MAX];
	guint64 diff[CPU_MAX];

	char *buf;
	size_t n;

	if (xd->num_cpu == 0) {
		// CPU name and number of CPUs
		buf = NULL; n = 0;
		f = cached_fopen_r("/proc/cpuinfo", TRUE);
		while(getline(&buf, &n, f) >= 0) {
			if (strncmp(buf, "model name", 10) == 0) {
				strcpy(xd->cpu0_name, strchr(buf, ':')+2);
				xd->cpu0_name[strlen(xd->cpu0_name)-1] = '\0'; //remove newline
			} else if (strncmp(buf, "processor", 9) == 0) {
				xd->num_cpu++;
			}
		}
		free(buf);
		first_call = TRUE;
	}

	// MHz
	buf = NULL; n = 0;
	f = cached_fopen_r("/proc/cpuinfo", TRUE);
	while(getline(&buf, &n, f) >= 0) {
		if (strncmp(buf, "cpu MHz", 7) == 0) {
			xd->cpu0_mhz = g_ascii_strtod(strchr(buf, ':')+2, NULL);
			if (errno != 0)
				g_warning("[graph-cpu] Parsing of cpu0_mhz failed");
			break;
		}
	}
	free(buf);

	// governor
	f = cached_fopen_r("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", TRUE);
	if(1 != fscanf(f, "%s", xd->cpu0_governor));

	// uptime
	f = cached_fopen_r("/proc/uptime", TRUE);
	buf = (char*)malloc(12);
	if (1 != fscanf(f, "%s", buf))
		g_warning("[graph-cpu] Parsing of uptime failed (s)");
	else {
		xd->uptime = g_ascii_strtod(buf, NULL);
		if (errno != 0)
			g_warning("[graph-cpu] Parsing of uptime failed (d)");
	}
	free(buf);

	// CPU stats
	f = cached_fopen_r("/proc/stat", TRUE);
	g_assert(7 == fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld", time+CPU_USER, time+CPU_NICE, time+CPU_SYS, time+CPU_IDLE, time+CPU_IOWAIT, &irq, &softirq));
	time[CPU_IOWAIT] += irq+softirq;

	if (!first_call) {
		for (i=0, total=0; i<CPU_MAX; i++) {
			diff[i] = time[i] - xd->last[i];
			total += diff[i];
		}

		xd->user			= (float)(diff[CPU_USER]) / total;
		xd->nice			= (float)(diff[CPU_NICE]) / total;
		xd->system			= (float)(diff[CPU_SYS]) / total;
		xd->iowait			= (float)(diff[CPU_IOWAIT]) / total;
		xd->total_use		= (float)(total-diff[CPU_IDLE]) / total;

		for (i=0; i<CPU_MAX; i++)
			data[i] = rint (Maximum * (float)diff[i] / total);
	}

	memcpy(xd->last, time, sizeof xd->last);
}


void
multiload_graph_cpu_tooltip_update (char **title, char **text, LoadGraph *g, CpuData *xd)
{
	if (g->config->tooltip_style == TOOLTIP_STYLE_DETAILS) {
		gchar *uptime = format_time_duration(xd->uptime);
		*title = g_strdup(xd->cpu0_name);
		*text = g_strdup_printf(_(	"%lu processors  -  %.2f GHz  -  Governor: %s\n"
									"%.1f%% in use by programs\n"
									"%.1f%% in use by low priority programs\n"
									"%.1f%% in use by the kernel\n"
									"%.1f%% in wait for I/O\n"
									"%.1f%% total CPU use\n"
									"\n"
									"Uptime: %s"),
									xd->num_cpu, xd->cpu0_mhz/1000.0, xd->cpu0_governor,
									(xd->user*100),
									(xd->nice*100),
									(xd->system*100),
									(xd->iowait*100),
									(xd->total_use*100),
									uptime);
		g_free(uptime);
	} else {
		*text = g_strdup_printf("%.1f%%", xd->total_use*100);
	}
}
