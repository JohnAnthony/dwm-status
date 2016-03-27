#include <stdio.h>
#include <stdbool.h>

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
	int new_down;
	int new_up;
	char buf[9];
	buf[8] = '\0';

	f = fopen("/proc/net/dev", "r");
	if (!f) {
		ret.success = false;
		return ret;
	}

	// Find our network interface
	while (true) {
		r = fgets(buf, 8, f);
		if (r != buf)
			goto exit;
		if (!strcmp(buf, "wlp1s0:"))
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

	// Get download amount
	r = fgets(buf, 9, f);
	if (r != buf)
		goto exit;
	new_down = atoi(buf);

	// Seek forwards to upload amount
	fseek(f, 51, SEEK_CUR);
	// TODO: This can fail

	// Get upload amount
	r = fgets(buf, 9, f);
	if (r != buf)
		goto exit;
	new_up = atoi(buf);

	close(f);

	// Now read the previous values and modify by the offsets
	f = fopen("/tmp/status.net", "r"); // Race condition?
	if (!f)
		goto exit;

	// Offset download amount
	r = fgets(buf, 9, f);
	if (r != buf)
		goto exit;
	ret.down = new_down - atoi(buf);

	fgetc(f); // Wind over the space

	// Offset upload amount
	r = fgets(buf, 9, f);
	if (r != buf)
		goto exit;
	ret.up = new_up - atoi(buf);

	close(f);
	
	ret.success = true;
exit:
	close(f);
	write_old_net_stats(new_down, new_up);
	return ret;
}

void to_si(char* buf, unsigned int n) {
	snprintf(buf, 5, "%d", n);
	buf[5] = '\0';
}

void print_separator() {
	printf(" | ");
}

int main(void) {
	int bat_level = get_bat_level();
	int bat_action = get_bat_action();

	struct net_pair network = get_network_pair();
	char up[6];
	char down[6];
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
	else 
		printf("%d%% %c", bat_level, bat_action);

	puts("");

	// All done

	return 0;
}
