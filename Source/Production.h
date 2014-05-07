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
	namespace Intelligence {
		struct Agent;
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
			int supply() const {
				if (unit)
					return type.supplyRequired();
				return 0;
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
				void add(Production);

				Production& top();
				Production const& top() const;

				Production& operator[](unsigned);
				Production const& operator[](unsigned) const;

				void pop();

				unsigned size() const;

				void update(double);
		};
		
		struct Manager {
			ProductionQueue queue;
			
			Unitset hatcheries;

			Unitset getLarva() const;
			void add(Production);
			bool morph();
			bool build(Infrastructure::Builder&, Resources::Miner&, Economy::Economist&, Intelligence::Agent&, unsigned);
			
			void pop(Infrastructure::Builder&, Resources::Miner&, Economy::Economist&, Intelligence::Agent&);
			
			void update(double, Infrastructure::Builder&, Resources::Miner&, Economy::Economist&, Intelligence::Agent&);
		};
	}
}