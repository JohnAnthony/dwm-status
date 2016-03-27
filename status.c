#include <stdio.h>
#include <stdbool.h>

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#include "config.h"

static char SI_UNITS[] = {
	' ',
	'k',
	'M',
	'G',
	'T'
};

struct net_pair {
	int down;
	int up;
	bool success;
};

int get_bat_level() {
	int ret;
	FILE* f;

	f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f)
		return -1;

	fscanf(f, "%d", &ret);
	close(f);
	return ret;
}

char get_bat_action() {
	FILE* f;
	char buf[8];
	char ret;

	f = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (!f)
		return 'X';

	fread(&buf, sizeof(char), 7, f);
	buf[7] = '\0';
	if (!strcmp(buf, "Dischar"))
		ret = '-';
	else if (!strcmp(buf, "Chargin"))
		ret = '+';
	else if (!strcmp(buf, "Full"))
		ret = '^';
	else
		ret = '?';

	close(f);
	return ret;
}

void write_old_net_stats(int down, int up) {
	FILE* f = fopen("/tmp/status.net", "w");
	if (!f)
		return;

	fprintf(f, "%8d %8d", down, up);
	close(f);
}

struct net_pair get_network_pair() {
	struct net_pair ret;
	FILE* f;
	char* r; // Used for detecting read errors
	int rr; // User for ^
	long unsigned int old_down;
	long unsigned int old_up;
	long unsigned int new_down;
	long unsigned int new_up;
	long unsigned int junk;
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
	rr = fscanf(f, "%ld", &new_down);
	if (rr <= 0)
		goto exit;

	// Seek forwards to upload amount
	for (int i = 0; i < 7; i++) {
		rr = fscanf(f, "%ld", &junk);
		if (rr <= 0) {
			puts("derp");
			goto exit;
		}
	}

	// Get upload amount
	rr = fscanf(f, "%ld", &new_up);
	if (rr <= 0)
		goto exit;

	close(f);

	//////////

	// Now read the previous values and modify by the offsets
	f = fopen("/tmp/status.net", "r"); // Race condition?
	if (!f)
		goto exit;

	rr = fscanf(f, "%ld %ld", &old_down, &old_up);
	if (!f)
		goto exit;
	ret.down = new_down - old_down;
	ret.up = new_up - old_up;

	close(f);

	//////////
	//
	ret.success = true;
exit:
	close(f);
	write_old_net_stats(new_down, new_up);
	return ret;
}

void to_si(char* buf, unsigned int n) {
	size_t unit = 0;

	while (n > 999) {
		unit++;
		n /= 1024;
	}

	if (unit >= LENGTH(SI_UNITS)) {
		snprintf(buf, 5, "^^^^^");
		return;
	}

	snprintf(buf, 5, "%03d%c", n, SI_UNITS[unit]);
	buf[5] = '\0';
}

void print_separator() {
	printf(" | ");
}

int main(void) {
	int bat_level = get_bat_level();
	int bat_action = get_bat_action();

	struct net_pair network = get_network_pair();
	char up[5];
	char down[5];
	to_si(up, network.up);
	to_si(down, network.down);

	// Do all the printing
	
	if (!network.success)
		printf("NETERR");
	else
		printf("U:%s D:%s", up, down);
	
	print_separator();

	if (bat_level == -1)
		printf("BATERR");
	else if (bat_level >= 100)
		printf("FULL");
	else 
		printf("%02d%% %c", bat_level, bat_action);

	puts("");

	// All done

	return 0;
}
