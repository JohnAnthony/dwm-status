#include <stdio.h>
#include <linux/power_supply.h>

int main(void) {
	char name[]= "BAT0";
	int result = 0;
	struct power_supply *psy = power_supply_get_by_name(name);
	union power_supply_propval chargenow, chargefull;
	result = psy->get_property(psy,POWER_SUPPLY_PROP_CHARGE_NOW,&chargenow);
	if(!result)
		printf(KERN_INFO "The charge level is %d\n",chargenow.intval);

	result = psy->get_property(psy,POWER_SUPPLY_PROP_CHARGE_FULL,&chargefull);
	if(!result)
		printf(KERN_INFO "The charge level is %d\n",chargefull.intval);

	return 0;
}
