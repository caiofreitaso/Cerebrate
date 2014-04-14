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

void Cerebrate::Industry::Manager::morph(Cerebrate::Infrastructure::Builder& builder) {
	Unitset larvae = builder.getLarva();

	for (Unitset::iterator larva = larvae.begin(); larva != larvae.end(); larva++) {
		if ((*larva)->morph(queue[0].type))
			queue.pop();
		break;
	}
}

void Cerebrate::Industry::Manager::build(Cerebrate::Infrastructure::Builder& builder, Cerebrate::Resources::Miner& mines) {
	if (builder.build(mines,queue[0].type))
		queue.pop();
}

void Cerebrate::Industry::Manager::pop(Cerebrate::Infrastructure::Builder& builder, Cerebrate::Resources::Miner& mines, Cerebrate::Economy::Economist& eco) {
	if (queue.size())
		if (eco.minerals() >= queue[0].minerals() && eco.gas() >= queue[0].gas()) {
			if (queue[0].isBuilding())
				build(builder,mines);
			else if (queue[0].unit)
				morph(builder);
		}
}

void Cerebrate::Industry::Manager::update(double threshold, Infrastructure::Builder& builder, Resources::Miner& mines, Economy::Economist& eco) {
	pop(builder,mines,eco);
	queue.update(threshold);
}