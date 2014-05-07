#pragma once
#include "Intelligence.h"

//// BaseInfo

int Cerebrate::Intelligence::BaseInfo::minerals() const {
	int r = 0;
	for (unsigned i = 0; i < patches.size(); i++)
		r += patches[i].ammount;
	return r;
}
int Cerebrate::Intelligence::BaseInfo::gas() const {
	int r = 0;
	for (unsigned i = 0; i < geysers.size(); i++)
		r += geysers[i].ammount;
	return r;
}

void Cerebrate::Intelligence::BaseInfo::draw() const {
	BWAPI::Broodwar->setTextSize(0);
	BWAPI::Position position = base->getPosition();
	std::set<Choke> chokes = base->getRegion()->getChokepoints();

	
	BWAPI::Color color;
	switch(owner) {
		case Cerebrate::Intelligence::None:
			color = BWAPI::Colors::Brown;
			break;
		case Cerebrate::Intelligence::Mine:
			color = BWAPI::Colors::Blue;
			break;
		case Cerebrate::Intelligence::His:
			color = BWAPI::Colors::Red;
			break;
	}
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,color);
	if (base->isStartLocation()) {
		BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Green);
		BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),72,BWAPI::Colors::Green);
	}

	BWAPI::Broodwar->setTextSize(1);
	for (std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++) {
		BWAPI::Broodwar->drawLineMap((*choke)->getSides().first.x(), (*choke)->getSides().first.y(), (*choke)->getSides().second.x(),(*choke)->getSides().second.y(), BWAPI::Colors::Grey);
		BWAPI::Broodwar->drawTextMap((*choke)->getCenter().x(), (*choke)->getCenter().y(), "\x1D%.0f",(*choke)->getWidth());
	}

	for (unsigned i = 0; i < patches.size(); i++)
		if (patches[i].position.first || patches[i].position.second) {
			BWAPI::Broodwar->drawLineMap(patches[i].position.first, patches[i].position.second, position.x(), position.y(), BWAPI::Colors::Yellow);
			BWAPI::Broodwar->drawCircleMap(patches[i].position.first, patches[i].position.second, 3, BWAPI::Colors::Yellow);
		}

	BWTA::Polygon poly = base->getRegion()->getPolygon();
	unsigned k = 0;
	for (; k < poly.size()-1; k++)
		BWAPI::Broodwar->drawLineMap(poly[k].x(),poly[k].y(), poly[k+1].x(),poly[k+1].y(), BWAPI::Colors::Orange);
	BWAPI::Broodwar->drawLineMap(poly[k].x(),poly[k].y(), poly[0].x(),poly[0].y(), BWAPI::Colors::Orange);
}

//// Location

void Cerebrate::Intelligence::Location::addBase(Cerebrate::Intelligence::BaseInfo* b) {
	double distance = b->base->getGroundDistance(info->base);
	unsigned i = 0;
	for (; i < ground.size(); i++)
		if (ground[i] > distance)
			break;

	bases.insert(bases.begin()+i, b);
	ground.insert(ground.begin()+i, distance);
	potential.push_back(1);
}
bool Cerebrate::Intelligence::Location::compare(unsigned i, unsigned j) {
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
void Cerebrate::Intelligence::Location::sort() {
	for (unsigned i = 1; i < bases.size(); i++)
		for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
			std::swap(bases[j], bases[j-1]);
			std::swap(ground[j], ground[j-1]);
			std::swap(potential[j], potential[j-1]);
		}
}

//// StartLocation

void Cerebrate::Intelligence::StartLocation::addBase(Cerebrate::Intelligence::BaseInfo* b) {
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
void Cerebrate::Intelligence::StartLocation::sort() {
	for (unsigned i = 1; i < bases.size(); i++)
		for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
			std::swap(bases[j], bases[j-1]);
			std::swap(ground[j], ground[j-1]);
			std::swap(potential[j], potential[j-1]);
			std::swap(air[j], air[j-1]);
		}
}

////BaseGraph

Cerebrate::Base Cerebrate::Intelligence::BaseGraph::main() const { return startLocations[selfIndex].info->base; }
Cerebrate::Intelligence::StartLocation const& Cerebrate::Intelligence::BaseGraph::self() const { return startLocations[selfIndex]; }
Cerebrate::Base Cerebrate::Intelligence::BaseGraph::enemyMain() const { return startLocations[enemyIndex].info->base; }
Cerebrate::Intelligence::StartLocation const& Cerebrate::Intelligence::BaseGraph::enemy() const { return startLocations[enemyIndex]; }
bool Cerebrate::Intelligence::BaseGraph::enemyKnown() const { return enemyIndex < startLocations.size(); }
unsigned Cerebrate::Intelligence::BaseGraph::expanded(BWAPI::Position where) {
	unsigned i = 0;
	for (; i < bases.size(); i++)
		if (where == bases[i].base->getPosition()) {
			bases[i].owner = Mine;
			break;
		}
	return i;
}
void Cerebrate::Intelligence::BaseGraph::enemySighted(BWAPI::Position where) {
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
void Cerebrate::Intelligence::BaseGraph::populate() {
	bases.reserve(BWTA::getBaseLocations().size());
	for (std::set<Base>::const_iterator base = BWTA::getBaseLocations().begin(); base != BWTA::getBaseLocations().end(); base++) {
		bases.push_back(BaseInfo());
		bases[bases.size()-1].base = *base;
		bases[bases.size()-1].region = TerrainAnalysis::convert((*base)->getRegion()->getPolygon());
		for (std::set<Unit>::const_iterator i = (*base)->getStaticMinerals().begin(); i != (*base)->getStaticMinerals().end(); i++) {
			bases[bases.size()-1].patches.push_back(Resource(*i,1500));
			if ((*i)->isVisible()) {
				bases[bases.size()-1].patches[bases[bases.size()-1].patches.size()-1].position.first = (*i)->getPosition().x();
				bases[bases.size()-1].patches[bases[bases.size()-1].patches.size()-1].position.second = (*i)->getPosition().y();
			}
		}
		bases[bases.size()-1].owner = None;
		for (std::set<Unit>::const_iterator i = (*base)->getGeysers().begin(); i != (*base)->getGeysers().end(); i++)
			bases[bases.size()-1].geysers.push_back(Resource(*i,5000));
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
void Cerebrate::Intelligence::BaseGraph::update() {
	for (unsigned j = 0; j < self().info->patches.size(); j++)
		if (self().info->patches[j].patch->isVisible()) {
			self().info->patches[j].ammount = self().info->patches[j].patch->getResources();
			self().info->patches[j].position.first = self().info->patches[j].patch->getPosition().x();
			self().info->patches[j].position.second = self().info->patches[j].patch->getPosition().y();
		}
	for (unsigned j = 0; j < self().info->geysers.size(); j++)
		if (self().info->geysers[j].patch->isVisible()) {
			self().info->geysers[j].ammount = self().info->geysers[j].patch->getResources();
			self().info->geysers[j].position.first = self().info->geysers[j].patch->getPosition().x();
			self().info->geysers[j].position.second = self().info->geysers[j].patch->getPosition().y();
		}

	for (unsigned j = 0; j < self().natural.info->patches.size(); j++)
		if (self().natural.info->patches[j].patch->isVisible()) {
			self().natural.info->patches[j].ammount = self().natural.info->patches[j].patch->getResources();
			self().natural.info->patches[j].position.first = self().natural.info->patches[j].patch->getPosition().x();
			self().natural.info->patches[j].position.second = self().natural.info->patches[j].patch->getPosition().y();
		}
	for (unsigned j = 0; j < self().natural.info->geysers.size(); j++)
		if (self().natural.info->geysers[j].patch->isVisible()) {
			self().natural.info->geysers[j].ammount = self().natural.info->geysers[j].patch->getResources();
			self().natural.info->geysers[j].position.first = self().natural.info->geysers[j].patch->getPosition().x();
			self().natural.info->geysers[j].position.second = self().natural.info->geysers[j].patch->getPosition().y();
		}
	for (unsigned i = 0; i < self().natural.bases.size(); i++) {
		double nearMe, farFromHim;
		double minerals, patches;
		double gas;

		for (unsigned j = 0; j < self().natural.bases[i]->patches.size(); j++)
			if (self().natural.bases[i]->patches[j].patch->isVisible()) {
				self().natural.bases[i]->patches[j].ammount = self().natural.bases[i]->patches[j].patch->getResources();
				self().natural.bases[i]->patches[j].position.first = self().natural.bases[i]->patches[j].patch->getPosition().x();
				self().natural.bases[i]->patches[j].position.second = self().natural.bases[i]->patches[j].patch->getPosition().y();
			}

		for (unsigned j = 0; j < self().natural.bases[i]->geysers.size(); j++)
			if (self().natural.bases[i]->geysers[j].patch->isVisible()) {
				self().natural.bases[i]->geysers[j].ammount = self().natural.bases[i]->geysers[j].patch->getResources();
				self().natural.bases[i]->geysers[j].position.first = self().natural.bases[i]->geysers[j].patch->getPosition().x();
				self().natural.bases[i]->geysers[j].position.second = self().natural.bases[i]->geysers[j].patch->getPosition().y();
			}

		if (startLocations[selfIndex].natural.bases[i]->base->isIsland()) {
			nearMe = 0;
			farFromHim = 1;
		} else {
			//nearMe = 1-((1-exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2)))/(1+exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2))) + 1)/2; //min(1,max(0,-(startLocations[selfIndex].natural.ground[i]/1000)+3));
			nearMe = 1-(tanh(1.5*(startLocations[selfIndex].natural.ground[i]/1000-2)) + 1)/2;
			farFromHim = 1;
			if (enemyKnown()) {
				unsigned j = 0;
				for (; j < startLocations[enemyIndex].natural.bases.size(); j++)
					if (startLocations[enemyIndex].natural.bases[j] == startLocations[selfIndex].natural.bases[i])
						break;

				if (j == startLocations[enemyIndex].natural.bases.size())
					farFromHim = 0;
				else
					farFromHim = (tanh(1.5*(startLocations[enemyIndex].natural.ground[j]/1000-2))+1)/2;//((1-exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2)))/(1+exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2))) + 1)/2;//1-min(1,max(0,-(startLocations[enemyIndex].natural.ground[j]/1000)+3));
			}
		}


		double aux = 0;

		aux = startLocations[selfIndex].natural.bases[i]->base->getStaticMinerals().size();
		patches = (tanh((aux-5)/2) + 1)/2;
		//patches = ((1-exp(-aux+5))/(1+exp(-aux+5)) + 1)/2;

		aux = startLocations[selfIndex].natural.bases[i]->minerals();
		aux /= 1000;
		minerals = (tanh(0.3*(aux-6)) + 1)/2;
		//minerals = ((1-exp(.6*(-aux+6)))/(1+exp(.6*(-aux+6))) + 1)/2;

		aux = startLocations[selfIndex].natural.bases[i]->gas();
		aux /= 10000;
		gas = aux + 0.5;

		startLocations[selfIndex].natural.potential[i] = (nearMe * farFromHim) * (minerals * patches*patches*patches * gas);
	}

	startLocations[selfIndex].natural.sort();
}

void Cerebrate::Intelligence::BaseGraph::draw() const {
	unsigned i;
	BWAPI::Position position = self().info->base->getPosition();
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Blue);
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,BWAPI::Colors::Green);
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),72,BWAPI::Colors::Blue);

	BWTA::Polygon poly = self().info->base->getRegion()->getPolygon();
	for (unsigned k = 0; k < poly.size()-1; k++)
		BWAPI::Broodwar->drawLineMap(poly[k].x(),poly[k].y(), poly[k+1].x(),poly[k+1].y(), BWAPI::Colors::Orange);

	for (i = 0; i < self().bases.size(); i++) {
		position = self().bases[i]->base->getPosition();
		BWAPI::Broodwar->setTextSize(0);
		BWAPI::Broodwar->drawTextMap(position.x()-45, position.y()+27, "M.G:%.0f\nM.A:%.0f",self().ground[i],self().air[i]);
	}
	self().natural.info->draw();

	for (i = 0; i < self().natural.bases.size(); i++) {
		self().natural.bases[i]->draw();
		BWAPI::Broodwar->setTextSize(0);
		BWAPI::Broodwar->drawTextMap(self().natural.bases[i]->base->getPosition().x(), self().natural.bases[i]->base->getPosition().y()+16,
									"%d (%.3f)\n\x0F%d\x02\nN.G:%.0f",i,self().natural.potential[i],self().natural.bases[i]->patches.size(),self().natural.ground[i]);
	}

	if (enemyKnown())
		for (i = 0; i < enemy().natural.bases.size(); i++) {
			position = enemy().natural.bases[i]->base->getPosition();
			BWAPI::Broodwar->drawTextMap(position.x(), position.y()+49, "E.G:%.0f",enemy().natural.ground[i]);
		}
}
BWAPI::TilePosition Cerebrate::Intelligence::BaseGraph::nextBasePosition() const {
	if (self().natural.info->owner == None)
		return self().natural.info->base->getTilePosition();
	else
		for (unsigned i = 0; i < self().natural.bases.size(); i++)
			if (self().natural.bases[i]->owner == None)
				return self().natural.bases[i]->base->getTilePosition();
}
Cerebrate::Intelligence::BaseInfo* Cerebrate::Intelligence::BaseGraph::nextBase() const {
	if (self().natural.info->owner == None)
		return self().natural.info;
	else
		for (unsigned i = 0; i < self().natural.bases.size(); i++)
			if (self().natural.bases[i]->owner == None)
				return self().natural.bases[i];
}
