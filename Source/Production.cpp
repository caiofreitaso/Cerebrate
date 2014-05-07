#include "Production.h"

//// ProductionQueue

void Cerebrate::Industry::ProductionQueue::add(Cerebrate::Industry::Production p) {
	_data.push_back(p);
	std::sort(_data.begin(),_data.end());
}

Cerebrate::Industry::Production& Cerebrate::Industry::ProductionQueue::top() { return _data[0]; }
Cerebrate::Industry::Production const& Cerebrate::Industry::ProductionQueue::top() const { return _data[0]; }

Cerebrate::Industry::Production& Cerebrate::Industry::ProductionQueue::operator[](unsigned i) { return _data[i]; }
Cerebrate::Industry::Production const& Cerebrate::Industry::ProductionQueue::operator[](unsigned i) const { return _data[i]; }

void Cerebrate::Industry::ProductionQueue::pop() { _data.erase(_data.begin()); }

unsigned Cerebrate::Industry::ProductionQueue::size() const { return _data.size(); }

void Cerebrate::Industry::ProductionQueue::update(double priorityThreshold) {
	for (std::vector<Production>::iterator i = _data.begin(); i != _data.end();)
		if (i->priority < priorityThreshold) {
			_data.erase(i);
			i = _data.begin();
		} else
			i++;
	std::sort(_data.begin(),_data.end());
}

//// Manager

void Cerebrate::Industry::Manager::add(Production p) { queue.add(p); }

bool Cerebrate::Industry::Manager::morph() {
	Unitset larvae = getLarva();

	for (Unitset::iterator larva = larvae.begin(); larva != larvae.end(); larva++)
		if ((*larva)->morph(queue[0].type))
			return true;
	return false;
}

bool Cerebrate::Industry::Manager::build(Cerebrate::Infrastructure::Builder& builder, Cerebrate::Resources::Miner& mines, Economy::Economist& eco, Intelligence::Agent& a, unsigned id) {
	return !builder.build(mines,a,queue[0].type,id);
}

void Cerebrate::Industry::Manager::pop(Cerebrate::Infrastructure::Builder& builder, Cerebrate::Resources::Miner& mines, Cerebrate::Economy::Economist& eco, Intelligence::Agent& a) {
	if (queue.size())
		if (eco.minerals() >= queue[0].minerals() && eco.gas() >= queue[0].gas()) {
			unsigned id = eco.add(queue[0]);
			bool remove = true;
			bool removeBudget = true;
			
			if (queue[0].isBuilding()) {
				removeBudget = build(builder,mines,eco,a,id);
				remove = !removeBudget;
			} else if (queue[0].unit)
				remove = morph();
			
			if (remove)
				queue.pop();
			if (removeBudget)
				eco.remove(id);
		}
}

void Cerebrate::Industry::Manager::update(double threshold, Infrastructure::Builder& builder, Resources::Miner& mines, Economy::Economist& eco, Intelligence::Agent& a) {
	pop(builder,mines,eco,a);
	queue.update(threshold);
	
	for (unsigned i = 0; i < hatcheries.size(); i++)
		if (!hatcheries[i]->exists()) {
			unsigned h = 0;
			for (; h < builder.hatcheries.size(); h++)
				if (builder.hatcheries[h].hatch == hatcheries[i]->getTilePosition())
					break;
			if (builder.hatcheries[h].base)
				builder.hatcheries[h].base->owner = Intelligence::None;
			builder.hatcheries.erase(builder.hatcheries.begin()+h);
			
			for (h = 0; h < mines.minerals.size(); h++)
				if (mines.minerals[h].hatch == hatcheries[i]->getPosition())
					break;
			
			if (h < mines.minerals.size()) {
				Unitset drones;
				for (unsigned m = 0; m < mines.minerals[h].miners.size(); m++)
					for (unsigned d = 0; d < mines.minerals[h].miners[m].size(); d++)
						drones.push_back(mines.minerals[h].miners[m][d].drone);
				
				for (unsigned d = 0; d < drones.size(); d++)
					mines.idleWorker(drones[d]);
				
				mines.minerals.erase(mines.minerals.begin()+h);
			}
			
			hatcheries.erase(hatcheries.begin()+i);
			i--;
		}
}

Cerebrate::Unitset Cerebrate::Industry::Manager::getLarva() const {
	Unitset larvae;
	
	for (unsigned i = 0; i < hatcheries.size(); i++) {
		std::set<Unit> l = hatcheries[i]->getLarva();
		larvae.insert(larvae.end(), l.begin(), l.end());
	}
	
	return larvae;
}