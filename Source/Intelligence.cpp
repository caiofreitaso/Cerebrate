#pragma once
#include "Intelligence.h"

double Cerebrate::Intelligence::BaseInfo::wchoke = -3;
double Cerebrate::Intelligence::BaseInfo::wpatch = 13;
double Cerebrate::Intelligence::BaseInfo::wpoly = 1;
//BWTA::RectangleArray<double> *Cerebrate::Intelligence::BaseInfo::map = 0;

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

BWAPI::TilePosition Cerebrate::Intelligence::BaseInfo::wallEnd(BWAPI::TilePosition A, BWAPI::TilePosition B) const {
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

double Cerebrate::Intelligence::BaseInfo::tileValue(BWAPI::TilePosition tile) const {
	std::set<Choke> chokes = base->getRegion()->getChokepoints();

	double tileValue = 0, chokeValue, patchValue, polyValue, d;

	unsigned walk = walkTiles(tile);
	if (walk < 16 || !BWAPI::Broodwar->isBuildable(tile))
		tileValue -= 80;

	for (unsigned k = 0; k < patches.size(); k++) {
		d = (tile.getDistance(BWAPI::TilePosition(BWAPI::Position(patches[k].position.first, patches[k].position.second))));
		if (d != 0) {
			patchValue = wpatch/(d*d);
			tileValue -= patchValue;
		} else
			return -80;
	}

	for (unsigned k = 0; k < geysers.size(); k++) {
		d = (tile.getDistance(BWAPI::TilePosition(BWAPI::Position(geysers[k].position.first, geysers[k].position.second))));
		if (d != 0) {
			patchValue = 4*wpatch/(d*d);
			tileValue -= patchValue;
		} else
			return -80;
	}

	d = 5000;
	for (std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++)
		if (d > tile.getDistance(BWAPI::TilePosition((*choke)->getCenter()))) {
			d = tile.getDistance(BWAPI::TilePosition((*choke)->getCenter()));
			chokeValue = (*choke)->getWidth() < 80 ? (*choke)->getWidth() : 80;
		}

	if (d != 0)
		chokeValue *= (wchoke/(d*d));
	else
		return -80;

	tileValue += chokeValue;

	d = 5000;
	for (unsigned i = 0; i < base->getRegion()->getPolygon().size(); i++) {
		if (d > tile.getDistance(BWAPI::TilePosition(base->getRegion()->getPolygon()[i])))
			d = tile.getDistance(BWAPI::TilePosition(base->getRegion()->getPolygon()[i]));
	}

	if (d != 0)
		polyValue = wpoly/(d*d);
	else
		polyValue = wpoly;
	tileValue += polyValue;

	return tileValue;
}

void Cerebrate::Intelligence::BaseInfo::draw(bool heat) const {
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

				int strength = 255*(tileValue(k)+15)/17;
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
		BWAPI::Broodwar->drawDotMap(patches[i].position.first, patches[i].position.second, BWAPI::Colors::Yellow);

	BWTA::Polygon poly = base->getRegion()->getPolygon();
	for (unsigned k = 0; k < poly.size()-1; k++)
		BWAPI::Broodwar->drawLineMap(poly[k].x(),poly[k].y(), poly[k+1].x(),poly[k+1].y(), BWAPI::Colors::Orange);
}


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


Cerebrate::Base Cerebrate::Intelligence::BaseGraph::main() const { return startLocations[selfIndex].info->base; }
Cerebrate::Intelligence::StartLocation const& Cerebrate::Intelligence::BaseGraph::self() const { return startLocations[selfIndex]; }
Cerebrate::Base Cerebrate::Intelligence::BaseGraph::enemyMain() const { return startLocations[enemyIndex].info->base; }
Cerebrate::Intelligence::StartLocation const& Cerebrate::Intelligence::BaseGraph::enemy() const { return startLocations[enemyIndex]; }
bool Cerebrate::Intelligence::BaseGraph::enemyKnown() const { return enemyIndex < startLocations.size(); }
void Cerebrate::Intelligence::BaseGraph::expanded(BWAPI::Position where) {
	for (unsigned i = 0; i < bases.size(); i++)
		if (where == bases[i].base->getPosition()) {
			bases[i].owner = Mine;
			return;
		}
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
		for (std::set<Unit>::const_iterator i = (*base)->getStaticMinerals().begin(); i != (*base)->getStaticMinerals().end(); i++)
			bases[bases.size()-1].patches.push_back(Resource(*i,1500));
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

		startLocations[selfIndex].natural.potential[i] = (nearMe*nearMe * farFromHim) * (minerals * patches*patches*patches * gas);
	}

	startLocations[selfIndex].natural.sort();
}

void Cerebrate::Intelligence::BaseGraph::draw() const {
	unsigned i;
	BWAPI::Position position = self().info->base->getPosition();
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Blue);
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,BWAPI::Colors::Green);
	BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),72,BWAPI::Colors::Blue);

	for (i = 0; i < self().bases.size(); i++) {
		position = self().bases[i]->base->getPosition();
		BWAPI::Broodwar->setTextSize(0);
		BWAPI::Broodwar->drawTextMap(position.x()-45, position.y()+27, "M.G:%.0f\nM.A:%.0f",self().ground[i],self().air[i]);
	}
	self().natural.info->draw();

	for (i = 0; i < self().natural.bases.size(); i++) {
		self().natural.bases[i]->draw(i == 0);
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