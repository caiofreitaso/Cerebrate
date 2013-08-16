#pragma once
#include "Economy.h"

void Cerebrate::Economy::Economist::update(Player player, int frame, unsigned droneCount) {
	int i;
	if (frame - states[0].frame >= 20) {

		i = SIZE-1;
		for (; i > 0; i--) {
			states[i].mineral = states[i-1].mineral;
			states[i].gas = states[i-1].gas;
			states[i].frame = states[i-1].frame;
			states[i].droneCount = states[i-1].droneCount;
		}

		states[0].mineral = player->gatheredMinerals() - 50;
		states[0].gas = player->gatheredGas();
		states[0].frame = frame;
		states[0].droneCount = droneCount;

		for (i = SIZE-1; i > 0; i--) {
			income[i].mineral = income[i-1].mineral;
			income[i].gas = income[i-1].gas;
			income[i].frame = income[i-1].frame;
			income[i].droneCount = income[i-1].droneCount;
		}

		income[0] = Resource();
		for (i = 0; i < SIZE-1; i++)
			if (frame - states[i].frame > 500)
				break;

		income[0].mineral = (states[0].mineral - states[i].mineral) / (frame - states[i].frame);
		income[0].gas = (states[0].gas - states[i].gas) / (frame - states[i].frame);
		income[0].frame = frame;
		income[0].droneCount = droneCount;

		incomeGrowth = Resource();
		incomeGrowth.mineral = (income[0].mineral - income[1].mineral) / (frame - income[1].frame);
		incomeGrowth.gas = (income[0].gas - income[1].gas) / (frame - income[1].frame);
		incomeGrowth.frame = frame;
	}
}
bool Cerebrate::Economy::Economist::support(BWAPI::UnitType unit, int quantity) {
	return	states[0].mineral >= (unit.mineralPrice()*quantity) &&
			states[0].gas >= (unit.gasPrice()*quantity);
}
Cerebrate::Economy::Resource Cerebrate::Economy::Economist::projectResources(int frames) {
	Cerebrate::Economy::Resource ret;
	ret.frame	=	-1;
	ret.mineral	=	states[0].mineral + income[0].mineral * frames;
	ret.gas		=	states[0].gas + income[0].gas * frames;
	return ret;
}