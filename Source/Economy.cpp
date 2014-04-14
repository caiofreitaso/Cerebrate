#pragma once
#include "Economy.h"

unsigned Cerebrate::Economy::Budget::start = 0;

void Cerebrate::Economy::Economist::add(Cerebrate::Industry::Production type) {
	Cerebrate::Economy::Budget b;
	b.id = Cerebrate::Economy::Budget::start;
	
	Cerebrate::Economy::Budget::start++;
	
	if (type.unit) {
		b.minerals = type.type.mineralPrice();
		b.gas = type.type.gasPrice();
	} else {
		b.minerals = type.tech.mineralPrice();
		b.gas = type.tech.gasPrice();
	}
	
	budgets.push_back(b);
}

int Cerebrate::Economy::Economist::minerals() const {
	int ret = BWAPI::Broodwar->self()->minerals();
	for (unsigned i = 0; i < budgets.size(); i++)
		ret -= budgets[i].minerals;
	return ret;
}
int Cerebrate::Economy::Economist::gas() const {
	int ret = BWAPI::Broodwar->self()->gas();
	for (unsigned i = 0; i < budgets.size(); i++)
		ret -= budgets[i].gas;
	return ret;
}