#pragma once
#include "Infrastructure.h"

double Cerebrate::Infrastructure::BaseInfo::wchoke = 1000;
double Cerebrate::Infrastructure::BaseInfo::wpatch = 13000;
double Cerebrate::Infrastructure::BaseInfo::wpoly = 1000;

//// BaseInfo

int Cerebrate::Infrastructure::BaseInfo::minerals() const {
	int r = 0;
	for (unsigned i = 0; i < patches.size(); i++)
		r += patches[i].ammount;
	return r;
}
int Cerebrate::Infrastructure::BaseInfo::gas() const {
	int r = 0;
	for (unsigned i = 0; i < geysers.size(); i++)
		r += geysers[i].ammount;
	return r;
}

BWAPI::TilePosition Cerebrate::Infrastructure::BaseInfo::wallEnd(BWAPI::TilePosition A, BWAPI::TilePosition B) const {
	BWAPI::TilePosition C(base->getTilePosition());

	double m, n, x, y;
	if (A.x() == B.x()) {
		x = A.x();
		y = C.y();
	} else if (A.y() == B.y()) {
		x = C.x();
		y = A.y();
	} else {
		m = (A.y() - B.y())/(A.x() - B.x());
		n = -1/m;
		x = (A.y() - C.y() - m*A.x() + n*C.x())/(n-m);
		y = n*x + C.y() - n*C.x();
	}

	return BWAPI::TilePosition(x,y);
}

double Cerebrate::Infrastructure::BaseInfo::tileValue(BWAPI::TilePosition tile) const {
	std::set<Choke> chokes = base->getRegion()->getChokepoints();

	double tileValue = 0, chokeValue, patchValue, polyValue, d;

	unsigned walk = walkTiles(tile);
	
	TerrainAnalysis::Point set[4];
	set[0] = TerrainAnalysis::Point(tile);
	set[1] = set[0];
	set[1].x += 31;
	set[2] = set[0];
	set[2].y += 31;
	set[3] = set[2];
	set[3].x += 31;
	
	if (walk < 16 || !BWAPI::Broodwar->isBuildable(tile))
		tileValue -= 80;

	for (unsigned k = 0; k < patches.size(); k++) {
		d = 0;
		for (unsigned j = 0; j < 4; j++)
			d += set[j].distance(BWAPI::Position(patches[k].position.first, patches[k].position.second));
		d /= 4;

		patchValue = wpatch/(d*d);
		tileValue -= patchValue;
	}

	for (unsigned k = 0; k < geysers.size(); k++) {
		d = 0;
		for (unsigned j = 0; j < 4; j++)
			d += set[j].distance(BWAPI::Position(geysers[k].position.first, geysers[k].position.second));
		d /= 4;

		patchValue = 4*wpatch/(d*d);
		tileValue -= patchValue;
	}

	for (std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++) {
		d = 0;
		for (unsigned j = 0; j < 4; j++)
			d += set[j].distance((*choke)->getCenter());
		d /= 4;

		chokeValue = (*choke)->getWidth() < 80 ? (*choke)->getWidth() : 80;
		chokeValue *= (wchoke/(d*d));
		tileValue -= chokeValue;
	}

	d = 1e37;
	for (unsigned i = 0; i < region.size(); i++) {
		double dist = 0;
		for (unsigned j = 0; j < 4; j++)
			dist += region[i].distance(set[j]);
		dist /= 4;
		
		if (d > dist)
			d = dist;
	}

	if (d == 0)
		return 80;
	
	polyValue = wpoly/(d*d);
	tileValue += polyValue;

	if (tileValue > 80)
		return 80;
	if (tileValue < -80)
		return -80;
	
	return tileValue;
}

void Cerebrate::Infrastructure::BaseInfo::draw(bool heat) const {
	BWAPI::Broodwar->setTextSize(0);
	BWAPI::Position position = base->getPosition();
	std::set<Choke> chokes = base->getRegion()->getChokepoints();

	if (heat) {
		BWAPI::TilePosition tile(base->getTilePosition());

		#ifdef SAVE_IMG
		std::ofstream image;
		image.open("heat.ppm");
		image << "P3\n21 21\n255\n";
		#endif

		for (int j = -10; j < 11; j++)
			 for (int i = -10; i < 11; i++) {
				BWAPI::TilePosition k(tile.x()+i,tile.y()+j);

				unsigned walk = walkTiles(k);

				int strength = 255*(tileValue(k)+40)/40;
				strength = strength > 255 ? 255 : strength;
				strength = strength < 0 ? 0 : strength;

				int wstr = walk ? 64 + 8*walk : 0;
				if (walk == 16) wstr = 255;

				#ifdef SAVE_IMG
				if (k == tile)
					image << strength << " 255 " << wstr;
				else
					image << strength << " 0 " << wstr;

				if (j == 10)
					image << "\r\n";
				else
					image << "\t";
				#endif


				BWAPI::Broodwar->drawCircleMap(BWAPI::Position(k).x()+16,BWAPI::Position(k).y()+16, 8, BWAPI::Color(strength,0,wstr), true);
				BWAPI::Broodwar->drawTextMap(BWAPI::Position(k).x()+16,BWAPI::Position(k).y()+16, "%.2f", tileValue(k));
			}

		#ifdef SAVE_IMG
		image.close();
		#endif

		std::vector<BWAPI::TilePosition> path;
		bool validAdjacents = true;
		while (validAdjacents) {
			std::vector<BWAPI::TilePosition> adjacents;
			adjacents.push_back(BWAPI::TilePosition(tile.x(), tile.y()+1));
			adjacents.push_back(BWAPI::TilePosition(tile.x(), tile.y()-1));
			adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()));
			adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()));

			adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()+1));
			adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()-1));
			adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()+1));
			adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()-1));

			for (unsigned i = 0; i < 8; i++)
				if (!BWAPI::Broodwar->isBuildable(adjacents[i]) && walkTiles(adjacents[i]) < 16) {
					validAdjacents = false;
					break;
				}
			if (!validAdjacents)
				break;

			double value = -100;
			unsigned index = -1;
			for (unsigned i = 0; i < 8; i++)
				if (!has(path,adjacents[i]))
					if (tileValue(adjacents[i]) > value) {
						value = tileValue(adjacents[i]);
						index = i;
					}

			if (index == -1)
				break;

			tile.y() = adjacents[index].y();
			tile.x() = adjacents[index].x();
			path.push_back(tile);
		}

		for(unsigned i = 0; i < path.size(); i++) {
			//int strength = 255*(tileValue(path[i])+15)/20;
			//strength = strength > 255 ? 255 : strength;
			//strength = strength < 0 ? 0 : strength;
			//BWAPI::Broodwar->drawCircleMap(BWAPI::Position(path[i]).x()+16,BWAPI::Position(path[i]).y()+16, 16, BWAPI::Color(0,0,strength), true);
			BWAPI::Broodwar->drawTextMap(BWAPI::Position(path[i]).x()+16,BWAPI::Position(path[i]).y()+16, "%.2f", tileValue(path[i]));
			BWAPI::Broodwar->drawTextMap(BWAPI::Position(path[i]).x(),BWAPI::Position(path[i]).y(), "\x1F%d", i);
		}

		BWAPI::Broodwar->drawLineMap(BWAPI::Position(base->getTilePosition()).x(),BWAPI::Position(base->getTilePosition()).y(),BWAPI::Position(path[path.size()-1]).x(),BWAPI::Position(path[path.size()-1]).y(),BWAPI::Colors::Green);

	}

	/*if (chokes.size() > 1) {
		BWAPI::Position a, b;
		if (chokes.size() > 2) {
			Chokeset danger;
			Choke biggest;

			double value = 5000;
			for (Choke_it choke = chokes.begin(); choke != chokes.end(); choke++)
				if (value > (*choke)->getWidth())
					if (
		} else {
			Choke_it choke = chokes.begin();
			a.x() = (*choke)->getCenter().x();
			a.y() = (*choke)->getCenter().y();

			choke++;
			b.x() = (*choke)->getCenter().x();
			b.y() = (*choke)->getCenter().y();

			BWAPI::TilePosition A(a),B(b),C(base->getTilePosition());

			BWAPI::TilePosition D(wallEnd(A,B));
			BWAPI::Broodwar->drawLineMap(BWAPI::Position(A).x(),BWAPI::Position(A).y(),BWAPI::Position(B).x(),BWAPI::Position(B).y(),BWAPI::Colors::Green);
			BWAPI::Broodwar->drawLineMap(BWAPI::Position(C).x(),BWAPI::Position(C).y(),BWAPI::Position(D).x(),BWAPI::Position(D).y(),BWAPI::Colors::Green);
		}
	}*/

	BWAPI::Color color;
	switch(owner) {
		case Cerebrate::Infrastructure::None:
			color = BWAPI::Colors::Brown;
			break;
		case Cerebrate::Infrastructure::Mine:
			color = BWAPI::Colors::Blue;
			break;
		case Cerebrate::Infrastructure::His:
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

//// StartLocation

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

////BaseGraph

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
void Cerebrate::Infrastructure::BaseGraph::update() {
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

void Cerebrate::Infrastructure::BaseGraph::draw() const {
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
BWAPI::TilePosition Cerebrate::Infrastructure::BaseGraph::nextBasePosition() const {
	if (self().natural.info->owner == None)
		return self().natural.info->base->getTilePosition();
	else
		for (unsigned i = 0; i < self().natural.bases.size(); i++)
			if (self().natural.bases[i]->owner == None)
				return self().natural.bases[i]->base->getTilePosition();
}
Cerebrate::Infrastructure::BaseInfo* Cerebrate::Infrastructure::BaseGraph::nextBase() const {
	if (self().natural.info->owner == None)
		return self().natural.info;
	else
		for (unsigned i = 0; i < self().natural.bases.size(); i++)
			if (self().natural.bases[i]->owner == None)
				return self().natural.bases[i];
}

//// BuilderDrone
Cerebrate::Infrastructure::BuilderDrone::BuilderDrone(Unit d, BWAPI::UnitType b, BWAPI::TilePosition t)
: drone(d), building(b), state(Cerebrate::Infrastructure::Moving),target(t) {
	center.x() = target.x()*32 + building.tileWidth()*16;
	center.y() = target.y()*32 + building.tileHeight()*16;
}

//// BuilderSet
void Cerebrate::Infrastructure::BuilderSet::act() {
	for (unsigned i = 0; i < builders.size(); i++)
		switch(builders[i].state) {
			case Cerebrate::Infrastructure::Moving:
				if (builders[i].drone->getPosition() != builders[i].center)
					builders[i].drone->move(builders[i].center);
				else
					builders[i].state = Cerebrate::Infrastructure::Building;
				break;
			case Cerebrate::Infrastructure::Building:
				if (builders[i].drone->isUnderAttack()) {
					if (builders[i].drone->isMorphing())
						builders[i].drone->cancelMorph();
					builders[i].state = Cerebrate::Infrastructure::Fleeing;
				} else if (!builders[i].drone->isConstructing())
					builders[i].drone->build(builders[i].target,builders[i].building);
				break;
			case Cerebrate::Infrastructure::Fleeing:
				if (!builders[i].drone->isUnderAttack())
					builders[i].state = Cerebrate::Infrastructure::Moving;
				break;
		}
}

void Cerebrate::Infrastructure::BuilderSet::draw() const {
	for (unsigned i = 0; i < builders.size(); i++) {
		BWAPI::Color color;
		char* str;
		switch(builders[i].state) {
			case Cerebrate::Infrastructure::Moving:
				str = "\x11 Moving";
				color = BWAPI::Colors::Orange;
				break;
			case Cerebrate::Infrastructure::Building:
				str = "\x17 Fleeing";
				color = BWAPI::Colors::Purple;
				break;
			case Cerebrate::Infrastructure::Fleeing:
				str = "\x10 Building";
				color = BWAPI::Colors::Yellow;
				break;
		}
		
		BWAPI::Broodwar->drawTextMap(builders[i].drone->getPosition().x()-12, builders[i].drone->getPosition().y()-4, str);
		BWAPI::Broodwar->drawLineMap(builders[i].drone->getPosition().x(), builders[i].drone->getPosition().y(),
									 builders[i].center.x(), builders[i].center.y(), color);
		BWAPI::Broodwar->drawBoxMap(builders[i].target.x()*32, builders[i].target.y()*32,
									builders[i].target.x()*32+builders[i].building.tileWidth()*32,
									builders[i].target.y()*32+builders[i].building.tileHeight()*32, color);
		
	}
}

bool Cerebrate::Infrastructure::BuilderSet::in(Unit d) const {
	for(unsigned i = 0; i < builders.size(); i++)
		if (builders[i].drone == d)
			return true;
	return false;
}

//// Hatchery

bool Cerebrate::Infrastructure::Hatchery::isOccupied(BWAPI::TilePosition pos) const {
	if (!BWAPI::Broodwar->isBuildable(pos,true))
		return false;
	for(unsigned i = 0; i < wall.size(); i++)
		if (wall[i].isOccupied(pos))
			return true;
	for(unsigned i = 0; i < sunkens.size(); i++)
		if (sunkens[i].isOccupied(pos))
			return true;
	for(unsigned i = 0; i < spores.size(); i++)
		if (spores[i].isOccupied(pos))
			return true;
	return false;
}
bool Cerebrate::Infrastructure::Hatchery::canWall() const {
	if (base->base->isStartLocation())
		return false;
	if (base->base->getRegion()->getChokepoints().size() < 2)
		return false;
	
	std::set<Choke> chokes = base->base->getRegion()->getChokepoints();
	std::vector<BWTA::Region*> adjacents;

	for(std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++) {
		unsigned i = 0;
		if ((*choke)->getRegions().first == base->base->getRegion()) {
			for (; i < adjacents.size(); i++)
				if (adjacents[i] == (*choke)->getRegions().second)
					break;
			if (i == adjacents.size())
				adjacents.push_back((*choke)->getRegions().second);
		} else {
			for (; i < adjacents.size(); i++)
				if (adjacents[i] == (*choke)->getRegions().first)
					break;
			if (i == adjacents.size())
				adjacents.push_back((*choke)->getRegions().first);
		}		
	}
	
	return adjacents.size() < 3;
}

//// Builder

Cerebrate::Unitset Cerebrate::Infrastructure::Builder::getLarva() const {
	Unitset ret;
	for (std::vector<Hatchery>::const_iterator hatch = hatcheries.begin(); hatch != hatcheries.end(); hatch++) {
		std::set<Unit> larvae = (*hatch).hatch->getLarva();
		for (std::set<Unit>::iterator larva = larvae.begin(); larva != larvae.end(); larva++)
			ret.push_back(*larva);
	}
	return ret;
}
bool Cerebrate::Infrastructure::Builder::drone(BWAPI::Position where) {
	std::set<Unit> larvas = getNearestHatch(where)->getLarva();
	if (!larvas.empty()) {
		(*larvas.begin())->morph(Drone);
		return true;
	}
	return false;
}
Cerebrate::Unit Cerebrate::Infrastructure::Builder::getNearestHatch(BWAPI::Position where) const {
	std::vector<Hatchery>::const_iterator hatch = hatcheries.begin();

	double distance = where.getDistance(hatch->hatch->getPosition());
	Unit ret = hatch->hatch;

	hatch++;


	for (; hatch != hatcheries.end(); hatch++)
		if (where.getDistance(hatch->hatch->getPosition()) < distance) {
			distance = where.getDistance(hatch->hatch->getPosition());
			ret = hatch->hatch;
		}

	return ret;
}
unsigned Cerebrate::Infrastructure::Builder::getNearestHatchIndex(BWAPI::Position where) const {
	double distance = where.getDistance(hatcheries[0].hatch->getPosition());
	unsigned ret = 0, i = 0;

	i++;

	for (; i < hatcheries.size(); i++)
		if (where.getDistance(hatcheries[i].hatch->getPosition()) < distance) {
			distance = where.getDistance(hatcheries[i].hatch->getPosition());
			ret = i;
		}

	return ret;
}

void Cerebrate::Infrastructure::Builder::addHatch(Unit hatch) {
	BaseInfo* base = 0;
	
	if (bases)
		for (unsigned i = 0; i < bases->bases.size(); i++)
			if (bases->bases[i].base->getPosition() == hatch->getPosition()) {
				base = &(bases->bases[i]);
				break;
			}
	
	hatcheries.insert(hatcheries.begin(),Hatchery(hatch,base));
}
void Cerebrate::Infrastructure::Builder::draw() const {
	for (unsigned i = 0; i < hatcheries.size(); i++)
		if (hatcheries[i].isMacro())
			BWAPI::Broodwar->drawTextMap(hatcheries[i].hatch->getPosition().x() - 10,
										 hatcheries[i].hatch->getPosition().y(),
										 "MACRO");
		else {
			if (hatcheries[i].base == bases->self().info) {
				BWAPI::Broodwar->drawTextMap(hatcheries[i].hatch->getPosition().x() - 10,
											 hatcheries[i].hatch->getPosition().y(),
											 "MAIN");
			 	for (int k = -5; k < 6; k++)
					for (int j = -5; j < 6; j++) {
						BWAPI::TilePosition newp(bases->self().info->base->getTilePosition());
						newp.x() += k;
						newp.y() += j;
						
						BWAPI::Position newpp(newp);
						newpp.x() += 15;
						newpp.y() += 15;
						
						double v = getValue(3,2,newp,true);
						
						BWAPI::Broodwar->drawTextMap(newpp.x(), newpp.y(), "%.3f", v);
					}
			} else if (hatcheries[i].base == bases->self().natural.info)
				BWAPI::Broodwar->drawTextMap(hatcheries[i].hatch->getPosition().x() - 10,
											 hatcheries[i].hatch->getPosition().y(),
											 "NATURAL [%s]", (hatcheries[i].canWall() ? "YES" : "NO"));
			else
				BWAPI::Broodwar->drawTextMap(hatcheries[i].hatch->getPosition().x() - 10,
											 hatcheries[i].hatch->getPosition().y(),
											 "BASE [%s]", (hatcheries[i].canWall() ? "YES" : "NO"));
			
			for (unsigned j = 0; j < hatcheries[i].wall.size(); j++)
				BWAPI::Broodwar->drawBoxMap(hatcheries[i].wall[j].position.x()*32, hatcheries[i].wall[j].position.y()*32,
											hatcheries[i].wall[j].position.x()*32 + hatcheries[i].wall[j].x*32,
											hatcheries[i].wall[j].position.y()*32 + hatcheries[i].wall[j].y*32, BWAPI::Colors::Brown);
		}

	if (bases)
		bases->draw();
	builders.draw();
}
void Cerebrate::Infrastructure::Builder::act() {
	builders.act();
}
void Cerebrate::Infrastructure::Builder::update(Industry::Manager& man) {
	if (bases)
		for (unsigned i = 0; i < hatcheries.size(); i++)
			if (!hatcheries[i].base)
				for (unsigned j = 0; j < bases->bases.size(); j++)
					if (hatcheries[i].hatch->getPosition() == bases->bases[j].base->getPosition()) {
						hatcheries[i].base = &bases->bases[j];
						bases->bases[j].owner = Mine;
						break;
					}
	for (unsigned i = 0; i < builders.builders.size(); i++)
		if (!builders.builders[i].drone->exists()) {
			man.add(Industry::Production(builders.builders[i].building, 0.91));
			builders.builders.erase(builders.builders.begin()+i);
			i--;
		}
}

bool Cerebrate::Infrastructure::Builder::isOccupied(BWAPI::TilePosition pos) const {
	for (unsigned i = 0; i < hatcheries.size(); i++)
		if (hatcheries[i].isOccupied(pos))
			return true;
	return false;
}

double Cerebrate::Infrastructure::Builder::getValue(BWAPI::TilePosition position, bool creep) const {
	if (bases) {
		unsigned i = 0;
		for (; i < bases->bases.size(); i++)
			if (TerrainAnalysis::in(bases->bases[i].region,position))
				break;

		if (i == bases->bases.size())
			return -80;

		double ret = bases->bases[i].tileValue(position) +
						(isOccupied(position) ? -80 : 0);
		if (creep)
			if (!BWAPI::Broodwar->hasCreep(position))
				ret -= 160;

		return ret;
	}
	
	return 0;
}
double Cerebrate::Infrastructure::Builder::getValue(int x, int y, BWAPI::TilePosition position, bool creep) const {
	double ret = 0;
		for (int i = 0; i < x; i++)
			for (int j = 0; j < y; j++) {
				BWAPI::TilePosition new_pos(position.x()+i, position.y()+j);
				ret += getValue(new_pos,creep);
			}
	
	return ret;
}

bool Cerebrate::Infrastructure::Builder::onCreep(int x, int y, BWAPI::TilePosition position) const {
	for (int i = 0; i < x; i++)
		for (int j = 0; j < y; j++)
			if (BWAPI::Broodwar->hasCreep(position.x()+i,position.y()+j))
				return true;
	return false;
}

BWAPI::Position Cerebrate::Infrastructure::Builder::spiral(BWAPI::Position center, Cerebrate::Infrastructure::BuildingSlot& slot, BWAPI::TilePosition position, bool creep) const {
	struct Position {
		int x;
		int y;
		Position(int i, int j):x(i),y(j) { }
		BWAPI::TilePosition tile() const { return BWAPI::TilePosition(x,y); }
		BWAPI::Position pos() const { return BWAPI::Position(tile()); }
	};
	
	Position current(position.x(),position.y()+center.y()), target(center.x(),center.y()+slot.y);

	struct Candidate {
		BWAPI::TilePosition position;
		double value;
		
		Candidate(BWAPI::TilePosition& pos, double v) : position(pos),value(v) { }
		
		bool operator<(Candidate b) const { return value < b.value; }
	};
	
	std::vector< Candidate > candidates;
	
	if (creep) {
		for (int i = 0; i <= target.x; i++) {
			current.x += i;
			candidates.push_back(Candidate(current.tile(), getValue(slot.x,slot.y,current.tile(),creep)));
		}
		target.x += slot.x;
		
		for (int sign = 1; onCreep(slot.x,slot.y,current.tile()); sign = -sign, target.x++, target.y++) {
			for (int i = 0; i < target.y; i++) {
				current.y += i * sign;
				candidates.push_back(Candidate(current.tile(), getValue(slot.x,slot.y,current.tile(),creep)));
			}

			for (int i = 0; i < target.x; i++) {
				current.x -= i * sign;
				candidates.push_back(Candidate(current.tile(), getValue(slot.x,slot.y,current.tile(),creep)));
			}
		}
		
		std::sort(candidates.begin(), candidates.end());
		slot.position = candidates.back().position;
	} else {
		bool changed = true;
		slot.position = position;
		double current_value = getValue(slot.x,slot.y,position,creep);
		
		while(changed) {
			for (int i = -1; i < 2; i++)
				for (int j = -1; j < 2; j++)
					if (i || j) {
						BWAPI::TilePosition new_pos(slot.position.x()+i, slot.position.y()+j);
						double value = getValue(slot.x,slot.y,new_pos,creep);
						if (value > current_value)
							candidates.push_back(Candidate(new_pos,value));
					}
			std::sort(candidates.begin(),candidates.end());
			
			if (candidates.back().position == slot.position)
				changed = false;
			else {
				slot.position = candidates.back().position;
				current_value = candidates.back().value;
			}
			
			candidates.clear();
		}
	}
	return BWAPI::Position(slot.position);
}

bool Cerebrate::Infrastructure::Builder::build(Cerebrate::Resources::Miner& miner, BWAPI::UnitType structure) {
	if (structure == Sunken)
		;
	else if (structure == Spore)
		;
	else {
		BuildingSlot s;
		s.x = structure.tileWidth();
		s.y = structure.tileHeight();
		s.position = BWAPI::TilePosition(0,0);
		
		for(unsigned i = 0; i < hatcheries.size(); i++)
			if (hatcheries[i].canWall()) {
				for (unsigned j = 0; j < hatcheries[i].wall.size(); j++)
					if (hatcheries[i].wall[j].x == s.x &&
						hatcheries[i].wall[j].y == s.y)
						s.position = hatcheries[i].wall[j].position;
			}
		
		bool expand = false;
		BuildingSlot macro;
		macro.x = 4;
		macro.y = 3;
		
		if (!s.position.x() && !s.position.y()) {
			if (structure.isResourceDepot()) {
				expand = true;
				
				s.position = bases->nextBasePosition();
				
				//spiral(BWAPI::Position(4,3),macro,s.position,false);
			} else
				spiral(BWAPI::Position(4,3),s,hatcheries[0].hatch->getTilePosition(), true);
		}
		
		Unit target = miner.getDrone(BWAPI::Position(s.position));
		if (target) {
			builders.builders.push_back(BuilderDrone(target,structure,s.position));
			/*if (expand) {
				hatcheries.push_back(Hatchery(target));
				//hatcheries.back().wall.push_back(macro);
			}*/
			return true;
		}
		return false;
	}
}