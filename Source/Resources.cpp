#pragma once
#include "Resources.h"

Cerebrate::Resources::Mineralset::Mineralset(Cerebrate::Unitset const& minerals, BWAPI::Position hatchery) {
	for (unsigned i = 0; i < minerals.size(); i++) {
		patches.push_back(minerals[i]);
		mining.push_back(false);
		miners.push_back(Cerebrate::Resources::Minerset());
	}
	hatch = hatchery;
}
void Cerebrate::Resources::Mineralset::update() {
	std::vector<unsigned> empties;
	for (unsigned i = 0; i < patches.size(); i++)
		if (patches[i]->getResources() == 0)
			empties.push_back(i);
	for (unsigned i = 0; i < empties.size(); i++) {
		patches.erase(patches.begin()+empties[i]-i);
		mining.erase(mining.begin()+empties[i]-i);
		miners.erase(miners.begin()+empties[i]-i);
	}

	for (unsigned i = 0; i < miners.size(); i++)
		for (unsigned j = 0; j < miners[i].size();)
			if (miners[i][j].drone->exists())
				j++;
			else
				miners[i].erase(miners[i].begin()+j);
}
Cerebrate::Unitset Cerebrate::Resources::Mineralset::getMiners() const {
	Cerebrate::Unitset ret;

	for(unsigned i = 0; i < miners.size(); i++)
		for (unsigned j = 0; j < miners[i].size(); j++)
			ret.push_back(miners[i][j].drone);

	return ret;
}
unsigned Cerebrate::Resources::Mineralset::size() const { return patches.size(); }
unsigned Cerebrate::Resources::Mineralset::indexOf(Cerebrate::Unit mineral) const {
	unsigned i = 0;
	for (; i < patches.size(); i++)
		if (patches[i] == mineral)
			break;
	return i;
}
void Cerebrate::Resources::Mineralset::addMiner(unsigned index, Cerebrate::Resources::MinerDrone drone) { miners[index].push_back(drone); }
void Cerebrate::Resources::Mineralset::addMiner(Cerebrate::Unit mineral, Cerebrate::Resources::MinerDrone drone) { miners[indexOf(mineral)].push_back(drone); }
void Cerebrate::Resources::Mineralset::act() {
	for (unsigned i = 0; i < miners.size(); i++)
		for (unsigned j = 0; j < miners[i].size(); j++)
			switch(miners[i][j].state) {
				case Cerebrate::Resources::Idle:
					if (!miners[i][j].drone->isCarryingMinerals())
						miners[i][j].state = Cerebrate::Resources::Waiting;
					break;
				case Cerebrate::Resources::Waiting:
					if (!mining[i]) {
						miners[i][j].drone->gather(patches[i]);
						mining[i] = true;
						miners[i][j].state = Cerebrate::Resources::Mining;
					} else if (miners[i][j].drone->getOrder() != BWAPI::Orders::HoldPosition) {
						miners[i][j].drone->gather(patches[i]);
						miners[i][j].drone->holdPosition(true);
					}
					break;
				case Cerebrate::Resources::Mining:
					if (miners[i][j].drone->isCarryingMinerals()) {
						miners[i][j].state = Cerebrate::Resources::Returning;
						mining[i] = false;
					}
					break;
				case Cerebrate::Resources::Returning:
					if (miners[i][j].drone->getOrder() != BWAPI::Orders::ReturnMinerals) {
						if (miners[i][j].drone->isCarryingMinerals())
							miners[i][j].drone->returnCargo();
						else
							miners[i][j].state = Cerebrate::Resources::Idle;
					}
					break;
			}
}
Cerebrate::Unit Cerebrate::Resources::Mineralset::getBestMineral() {
	Cerebrate::Unitset candidates;
	unsigned minSize = 3;
	for (unsigned i = 0; i < miners.size(); i++)
		if (miners[i].size() < minSize)
			minSize = miners[i].size();

	for (unsigned i = 0; i < miners.size(); i++)
		if (miners[i].size() == minSize)
			candidates.push_back(patches[i]);

	if (candidates.size()) {
		double distance = hatch.getDistance(candidates[0]->getPosition());
		Unit ret = candidates[0];

		for (unsigned i = 1; i < candidates.size(); i++)
			if (hatch.getDistance(candidates[i]->getPosition()) < distance) {
				distance = hatch.getDistance(candidates[i]->getPosition());
				ret = candidates[i];
			}

		return ret;
	} else
		return 0;
}
Cerebrate::Unit Cerebrate::Resources::Mineralset::getDrone() {
	Cerebrate::Resources::Minerset candidates;
	unsigned x,y;

	for (unsigned i = 0; i < miners.size(); i++)
		for (unsigned j = 0; j < miners[i].size(); j++)
			if (!miners[i][j].drone->isCarryingMinerals()) {
				unsigned k = 0;
				for (; k < candidates.size(); k++)
					if ((miners[i][j].state == candidates[k].state && miners[i][j].drone->getPosition().getDistance(hatch) < candidates[k].drone->getPosition().getDistance(hatch)) ||
						miners[i][j].state == Idle ||
						(miners[i][j].state == Waiting && candidates[k].state == Cerebrate::Resources::Mining))
						break;
				candidates.insert(candidates.begin()+k, miners[i][j]);
				if (!k) {
					x = i;
					y = j;
				}
			}
	if (candidates.size()) {
		miners[x].erase(miners[x].begin()+y);
		if (candidates[0].state == Cerebrate::Resources::Mining)
			mining[x] = false;
		return candidates[0].drone;
	} else
		return 0;
}

void Cerebrate::Resources::Miner::add(Cerebrate::Unit hatch) {
	Unitset patches;
	std::set<Unit> units = hatch->getUnitsInRadius(300);
	for (std::set<Unit>::iterator i = units.begin(); i != units.end(); i++)
		if ((*i)->getType().isMineralField())
			patches.push_back(*i);
	minerals.insert(minerals.begin(),Cerebrate::Resources::Mineralset(patches,hatch->getPosition()));
}
void Cerebrate::Resources::Miner::remove(unsigned index) {
	minerals.erase(minerals.begin()+index);
}
void Cerebrate::Resources::Miner::update() {
	for (unsigned i = 0; i < minerals.size(); i++)
		minerals[i].update();

	std::vector<unsigned> empties;
	for (unsigned i = 0; i < minerals.size(); i++)
		if (minerals[i].size() == 0)
			empties.push_back(i);
	for (unsigned i = 0; i < empties.size(); i++)
		minerals.erase(minerals.begin()+empties[i]-i);
}
Cerebrate::Unitset Cerebrate::Resources::Miner::getAllMiners() const {
	Cerebrate::Unitset ret;

	for(unsigned i = 0; i < minerals.size(); i++) {
		Unitset tmp = minerals[i].getMiners();
		ret.insert(ret.begin(), tmp.begin(), tmp.end());
	}

	return ret;
}
void Cerebrate::Resources::Miner::idleWorker(Cerebrate::Unit unit, Cerebrate::Infrastructure::Builder& builder) {
	#ifndef NORMAL_MINING
	if (!has(getAllMiners(),unit))
	#endif
	{
		Unit mineral = 0;
		unsigned i = 0;
		for (; i < minerals.size(); i++)
			if (builder.hatcheries[i]->isCompleted()) {
				mineral = minerals[i].getBestMineral();
				if (mineral)
					break;
			}
		#ifdef NORMAL_MINING
		unit->gather(mineral);
		#else
		minerals[i].addMiner(mineral,Cerebrate::Resources::MinerDrone(unit,Cerebrate::Resources::Idle));
		#endif
	}
}
void Cerebrate::Resources::Miner::act() {
	for (unsigned i = 0; i < minerals.size(); i++)
		minerals[i].act();
}
Cerebrate::Unit Cerebrate::Resources::Miner::getDrone(BWAPI::Position where) {
	double distance = where.getDistance(minerals[0].hatch);
	unsigned index = 0, i = 0;
	
	i++;

	for (; i < minerals.size(); i++)
		if (where.getDistance(minerals[i].hatch) < distance) {
			distance = where.getDistance(minerals[i].hatch);
			index = i;
		}
	return minerals[index].getDrone();
}