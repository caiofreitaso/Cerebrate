#pragma once
#include "Util.h"
#include "Infrastructure.h"
#include "Economy.h"
#include <string>

namespace Cerebrate {
	namespace Economy {
		struct Economist;
	}
	
	namespace Infrastructure {
		struct Builder;
	}
	
	namespace Industry {
		struct Production {
			BWAPI::UnitType type;
			BWAPI::TechType tech;
			bool unit;
			double priority;

			Production(BWAPI::UnitType t, double p = 0.9) : type(t),unit(true),priority(p) { }
			Production(BWAPI::TechType t, double p = 0.9) : tech(t),unit(false),priority(p) { }
			
			int minerals() const {
				if (unit)
					return type.mineralPrice();
				else
					return tech.mineralPrice();
			}
			int gas() const {
				if (unit)
					return type.gasPrice();
				else
					return tech.gasPrice();
			}
			
			bool isBuilding() const {
				if (unit)
					return type.isBuilding();
				return false;
			}
			std::string name() const {
				if (unit)
					return type.getName();
				else
					return tech.getName();
			}
			
			bool operator<(Production p) const { return priority < p.priority; }
		};

		class ProductionQueue {
			std::vector<Production> _data;
			public:
				void add(Production p);

				Production& top();
				Production const& top() const;

				Production& operator[](unsigned i);
				Production const& operator[](unsigned i) const;

				void pop();

				unsigned size() const;

				void update(double priorityThreshold);
		};
		
		struct Manager {
			ProductionQueue queue;

			void add(Production p);
			void morph(Infrastructure::Builder& builder);
			void build(Infrastructure::Builder& builder, Resources::Miner& mines);
			
			void pop(Infrastructure::Builder& builder, Resources::Miner& mines, Economy::Economist& eco);
			
			void update(double threshold, Infrastructure::Builder& builder, Resources::Miner& mines, Economy::Economist& eco);
		};
	}
}