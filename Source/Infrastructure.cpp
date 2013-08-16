#pragma once
#include "Infrastructure.h"

int Cerebrate::Infrastructure::BaseInfo::minerals() const {
	int r = 0;
	for (unsigned i = 0; i < patches.size(); i++)
		r += patches[i].second;
	return r;
}

void Cerebrate::Infrastructure::Location::addBase(Cerebrate::Infrastructure::BaseInfo* b) {
	double distance = b->base->getGroundDistance(info->base);
	unsigned i = 0;
	for (; i < ground.size(); i++)
		if (ground[i] > distance)
			break;
	
	bases.insert(bases.begin()+i, b);
	ground.insert(ground.begin()+i, distance);
	potential.push_back(1);
}
bool Cerebrate::Infrastructure::Location::compare(unsigned i, unsigned j) {
	if (bases[i]->owner != bases[j]->owner)
		if (bases[i]->owner == None)
			return true;
		else if (bases[j]->owner == None)
			return false;
		else if (bases[j]->owner == Mine)
			return true;
		else
			return false;
	else
		return potential[i] > potential[j];
}
void Cerebrate::Infrastructure::Location::sort() {
	for (unsigned i = 1; i < bases.size(); i++)
		for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
			std::swap(bases[j], bases[j-1]);
			std::swap(ground[j], ground[j-1]);
			std::swap(potential[j], potential[j-1]);
		}
}

void Cerebrate::Infrastructure::StartLocation::addBase(Cerebrate::Infrastructure::BaseInfo* b) {
	double distance = b->base->getGroundDistance(info->base);
	unsigned i = 0;
	for (; i < ground.size(); i++)
		if (ground[i] > distance)
			break;
	
	bases.insert(bases.begin()+i, b);
	ground.insert(ground.begin()+i, distance);
	air.insert(air.begin()+i, b->base->getAirDistance(info->base));
	potential.push_back(1);					
}
void Cerebrate::Infrastructure::StartLocation::sort() {
	for (unsigned i = 1; i < bases.size(); i++)
		for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
			std::swap(bases[j], bases[j-1]);
			std::swap(ground[j], ground[j-1]);
			std::swap(potential[j], potential[j-1]);
			std::swap(air[j], air[j-1]);
		}
}

Cerebrate::Base Cerebrate::Infrastructure::BaseGraph::main() const { return startLocations[selfIndex].info->base; }
Cerebrate::Infrastructure::StartLocation const& Cerebrate::Infrastructure::BaseGraph::self() const { return startLocations[selfIndex]; }
Cerebrate::Base Cerebrate::Infrastructure::BaseGraph::enemyMain() const { return startLocations[enemyIndex].info->base; }
Cerebrate::Infrastructure::StartLocation const& Cerebrate::Infrastructure::BaseGraph::enemy() const { return startLocations[enemyIndex]; }
bool Cerebrate::Infrastructure::BaseGraph::enemyKnown() const { return enemyIndex < startLocations.size(); }
void Cerebrate::Infrastructure::BaseGraph::expanded(BWAPI::Position where) {
	for (unsigned i = 0; i < bases.size(); i++)
		if (where == bases[i].base->getPosition()) {
			bases[i].owner = Mine;
			return;
		}
}
void Cerebrate::Infrastructure::BaseGraph::enemySighted(BWAPI::Position where) {
	for (unsigned i = 0; i < bases.size(); i++)
		if (where == bases[i].base->getPosition()) {
			bases[i].owner = His;
			break;
		}
	if (!enemyKnown())
		for (unsigned i = 0; i < startLocations.size(); i++)
			if (startLocations[i].info->owner == His) {
				enemyIndex = i;
				return;
			}
}
void Cerebrate::Infrastructure::BaseGraph::populate() {
	bases.reserve(BWTA::getBaseLocations().size());
	for (std::set<Base>::const_iterator base = BWTA::getBaseLocations().begin(); base != BWTA::getBaseLocations().end(); base++) {
		bases.push_back(BaseInfo());
		bases[bases.size()-1].base = *base;
		for (std::set<Unit>::const_iterator i = (*base)->getStaticMinerals().begin(); i != (*base)->getStaticMinerals().end(); i++)
			bases[bases.size()-1].patches.push_back(std::pair<Unit,int>(*i,1500));
		bases[bases.size()-1].owner = None;
		bases[bases.size()-1].gas = (*base)->isMineralOnly() ? 0 : 5000;
	}
	for (unsigned i = 0; i < bases.size(); i++)
		if (bases[i].base->isStartLocation()) {
			startLocations.push_back(StartLocation());
			startLocations[startLocations.size()-1].info = &bases[i];
			
			for (unsigned j = 0; j < bases.size(); j++)
				if (bases[j].base->isStartLocation() && i != j)
					startLocations[startLocations.size()-1].addBase(&bases[j]);
			
			double distance = 1e37;
			for (unsigned j = 0; j < bases.size(); j++)
				if (!bases[j].base->isStartLocation() && !bases[j].base->isIsland()) {
					double thisDistance = bases[j].base->getGroundDistance(bases[i].base);
					if (thisDistance < distance) {
						distance = thisDistance;
						startLocations[startLocations.size()-1].natural.info = &bases[j];
					}
				}
			
			for (unsigned j = 0; j < bases.size(); j++)
				if (j != i && &bases[j] != startLocations[startLocations.size()-1].natural.info)
					startLocations[startLocations.size()-1].natural.addBase(&bases[j]);
		}
	selfIndex = enemyIndex = startLocations.size();
}
void Cerebrate::Infrastructure::BaseGraph::update() {
	std::stringstream tmp;
	tmp.setf(std::ios::fixed);
	tmp.precision(2);
	for (unsigned i = 0; i < self().natural.bases.size(); i++) {						
		double nearMe, farFromHim;
		double minerals, patches;
		double gas;
		
		for (unsigned j = 0; j < self().natural.bases[i]->patches.size(); j++)
			if (self().natural.bases[i]->patches[j].first->isVisible())
				self().natural.bases[i]->patches[j].second = self().natural.bases[i]->patches[j].first->getResources();
			
		for (std::set<Unit>::const_iterator j = self().natural.bases[i]->base->getGeysers().begin(); j != self().natural.bases[i]->base->getGeysers().end(); j++)
			if ((*j)->isVisible())
				self().natural.bases[i]->gas = (*j)->getResources();
		
		if (startLocations[selfIndex].natural.bases[i]->base->isIsland()) {
			nearMe = 0;
			farFromHim = 1;
		} else {
			nearMe = 1-((1-exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2)))/(1+exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2))) + 1)/2; //min(1,max(0,-(startLocations[selfIndex].natural.ground[i]/1000)+3));
			farFromHim = 1;
			if (enemyKnown()) {
				unsigned j = 0;
				for (; j < startLocations[enemyIndex].natural.bases.size(); j++)
					if (startLocations[enemyIndex].natural.bases[j] == startLocations[selfIndex].natural.bases[i])
						break;
				
				if (j == startLocations[enemyIndex].natural.bases.size())
					farFromHim = 0;
				else
					farFromHim = ((1-exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2)))/(1+exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2))) + 1)/2;//1-min(1,max(0,-(startLocations[enemyIndex].natural.ground[j]/1000)+3));
			}
		}
			
		
		double aux = 0;
		
		aux = startLocations[selfIndex].natural.bases[i]->base->getStaticMinerals().size();
		patches = ((1-exp(-aux+5))/(1+exp(-aux+5)) + 1)/2;
			
		aux = startLocations[selfIndex].natural.bases[i]->minerals();
		aux /= 1000;
		minerals = ((1-exp(.6*(-aux+6)))/(1+exp(.6*(-aux+6))) + 1)/2;
		
		aux = startLocations[selfIndex].natural.bases[i]->gas;
		aux /= 10000;
		gas = aux + 0.5;
		
		tmp.str("");
		tmp << (startLocations[selfIndex].natural.bases[i]->owner == His ? "\x08" : "") <<  "[" << i << "]\tnearMe:" << nearMe << "\tfarFromHim:" << farFromHim << "\tminerals:" << minerals << "\tpatches:" << patches;
		BWAPI::Broodwar->drawTextScreen(340,25+i*9,tmp.str().c_str());
		tmp.flush();
		
		startLocations[selfIndex].natural.potential[i] = (nearMe*nearMe * farFromHim) * (minerals * patches*patches*patches * gas);
	}
	
	startLocations[selfIndex].natural.sort();					
}

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
	if (allBases)
		allBases->expanded(hatch->getPosition());
}
