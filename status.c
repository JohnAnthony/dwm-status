#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#include "config.h"

void print_separator() {
	printf(" | ");
}

#ifdef NETWORK
static char SI_UNITS[] = { 'k',	'M', 'G', 'T' };

struct net_pair {
	long unsigned down;
	long unsigned up;
	bool success;
};

void write_old_net_stats(long unsigned down, long unsigned up) {
	FILE* f = fopen("/tmp/status.net", "w");
	if (!f)
		return;

	fprintf(f, "%8lu %8lu", down, up);
	fclose(f);
}

struct net_pair get_network_pair() {
	struct net_pair ret;
	FILE* f;
	char* r; // Used for detecting read errors
	int rr; // User for ^
	long unsigned old_down;
	long unsigned old_up;
	long unsigned new_down;
	long unsigned new_up;
	char buf[9];
	buf[8] = '\0';

	//////////

	f = fopen("/proc/net/dev", "r");
	if (!f) {
		ret.success = false;
		return ret;
	}

	// Find our network interface
	while (true) {
		r = fgets(buf, 7, f);
		if (r != buf)
			goto exit;
		if (!strcmp(buf, NET_IFACE))
			break;

		// Wind forwards a line
		while (true) {
			int ch = fgetc(f);
			if (ch == EOF)
				goto exit;
			if (ch == '\n')
				break;
		}
	}

	// There's a colon. Ignore it.
	fgetc(f);

	// Get download amount
	rr = fscanf(f, "%lu %*d %*d %*d %*d %*d %*d %*d %lu", &new_down, &new_up);
	if (rr <= 0)
		goto exit;

	fclose(f);

	//////////

	// Now read the previous values and modify by the offsets
	f = fopen("/tmp/status.net", "r"); // Race condition?
	if (!f)
		goto exit;

	rr = fscanf(f, "%lu %lu", &old_down, &old_up);
	if (rr == 0)
		goto exit;
	ret.down = new_down - old_down;
	ret.up = new_up - old_up;

	//////////

	ret.success = true;
exit:
	if (f)
		fclose(f);
	write_old_net_stats(new_down, new_up);
	return ret;
}

void to_si(char* buf /*@out*/, size_t len, long unsigned n) {
	size_t unit = 0;
	double d = n; // HURR HURR

	// We're not even in kB. Just return nothing.
	if (d < 1000) {
		snprintf(buf, len, "----");
		return;
	}

	d /= 1024;
	while (d > 999) {
		unit++;
		d /= 1024;
	}

	// We're way off the scale
	if (unit >= LENGTH(SI_UNITS)) {
		snprintf(buf, len, "^^^^^");
		return;
	}

	// With decimal
	if (d < 10) {
		snprintf(buf, len, "%.1f%c", d, SI_UNITS[unit]);
		return;
	}

	// Without decimal
	snprintf(buf, len, "%03.0f%c", d, SI_UNITS[unit]);
}
#endif // NETWORK

#ifdef BATTERY
int get_bat_level() {
	int ret;
	FILE* f;

	f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f)
		return -1;

	fscanf(f, "%d", &ret);
	fclose(f);
	return ret;
}

char get_bat_action() {
	FILE* f;
	char buf[5];
	char ret;

	f = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (!f)
		return 'X';

	fread(&buf, sizeof(char), LENGTH(buf), f);
	buf[LENGTH(buf) - 1] = '\0';
	if (!strcmp(buf, "Disc"))
		ret = '-';
	else if (!strcmp(buf, "Char"))
		ret = '+';
	else if (!strcmp(buf, "Full"))
		ret = '*';
	else
		ret = '?';

	fclose(f);
	return ret;
}
#endif // BATTERY

int main(void) {
#ifdef NETWORK
	struct net_pair network = get_network_pair();
	char up[5];
	char down[5];
	to_si(up, LENGTH(up), network.up);
	to_si(down, LENGTH(down), network.down);

	if (!network.success)
		printf("NETERR");
	else
		printf("U:%s D:%s", up, down);
	print_separator();
#endif // NETWORK
	
#ifdef BATTERY
	int bat_level = get_bat_level();
	int bat_action = get_bat_action();

	if (bat_level >= 100)
		bat_level = 0;

	if (bat_level == -1)
		printf("BATERR");
	else 
		printf("B:%02d%c", bat_level, bat_action);
	print_separator();
#endif // BATTERY

	char time_str[24];
	time_t t = time(NULL);
	if (t == (time_t) -1) {
		puts("TIMEERR1");
		return 1;
	}

	struct tm* local = localtime(&t); // No ownership (!)
	if (local == NULL) {
		puts("TIMERR2");
		return 2;
	}

	strftime(time_str, LENGTH(time_str), "%Y-%m-%d %T %a", local);
	puts(time_str);

	// All done
	return 0;
}
