#include <stdio.h>

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

int main(void) {
	int bat_level = get_bat_level();
	int bat_action = get_bat_action();

	printf("%d%% %c\n", bat_level, bat_action);
	return 0;
}
