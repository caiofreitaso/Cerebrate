#pragma once
#include "Infrastructure.h"

Cerebrate::Unitset Cerebrate::Infrastructure::Builder::getLarva() const {
	Unitset ret;
	for (Unitset::const_iterator hatch = hatcheries.begin(); hatch != hatcheries.end(); hatch++) {
		std::set<Unit> larvae = (*hatch)->getLarva();
		for (std::set<Unit>::iterator larva = larvae.begin(); larva != larvae.end(); larva++)
			ret.push_back(*larva);
	}
	return ret;
}
Cerebrate::Unit Cerebrate::Infrastructure::Builder::getNearestHatch(BWAPI::Position where) const {
	Unitset::const_iterator hatch = hatcheries.begin();

	double distance = where.getDistance((*hatch)->getPosition());
	Unit ret = *hatch;

	hatch++;


	for (; hatch != hatcheries.end(); hatch++)
		if (where.getDistance((*hatch)->getPosition()) < distance) {
			distance = where.getDistance((*hatch)->getPosition());
			ret = *hatch;
		}

	return ret;
}
unsigned Cerebrate::Infrastructure::Builder::getNearestHatchIndex(BWAPI::Position where) const {
	double distance = where.getDistance(hatcheries[0]->getPosition());
	unsigned ret = 0, i = 0;

	i++;

	for (; i < hatcheries.size(); i++)
		if (where.getDistance(hatcheries[i]->getPosition()) < distance) {
			distance = where.getDistance(hatcheries[i]->getPosition());
			ret = i;
		}

	return ret;
}

void Cerebrate::Infrastructure::Builder::addHatch(Unit hatch) {
	hatcheries.insert(hatcheries.begin(),hatch);
}