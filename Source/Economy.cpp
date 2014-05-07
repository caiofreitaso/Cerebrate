#pragma once
#include "Economy.h"

unsigned Cerebrate::Economy::Economist::start = 0;

int Cerebrate::Economy::Economist::add(Cerebrate::Industry::Production type) {
	Cerebrate::Economy::Budget b;
	
	unsigned id = Cerebrate::Economy::Economist::start;
	
	Cerebrate::Economy::Economist::start++;
	
	if (type.unit) {
		b.minerals = type.type.mineralPrice();
		b.gas = type.type.gasPrice();
	} else {
		b.minerals = type.tech.mineralPrice();
		b.gas = type.tech.gasPrice();
	}
	
	budgets[id] = b;
	
	return id;
}

int Cerebrate::Economy::Economist::add(Cerebrate::Economy::Budget budget) {
	unsigned id = Cerebrate::Economy::Economist::start;
	Cerebrate::Economy::Economist::start++;
	
	budgets[id] = budget;
	
	return id;
}

void Cerebrate::Economy::Economist::remove(unsigned id) {
	budget_it i = budgets.find(id);
	if (i != budgets.end())
		budgets.erase(i);
}

int Cerebrate::Economy::Economist::minerals() const {
	int ret = BWAPI::Broodwar->self()->minerals();
	for (budget_const i = budgets.begin(); i != budgets.end(); i++)
		ret -= i->second.minerals;
	return ret;
}
int Cerebrate::Economy::Economist::gas() const {
	int ret = BWAPI::Broodwar->self()->gas();
	for (budget_const i = budgets.begin(); i != budgets.end(); i++)
		ret -= i->second.gas;
	return ret;
}