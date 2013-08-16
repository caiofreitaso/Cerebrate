#pragma once
#include "Util.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace Cerebrate {
	namespace Infrastructure {
		enum Ownership {
			None,
			Mine,
			His
		};
		
		struct BaseInfo {
			Ownership owner;
			std::vector< std::pair<Unit,int> > patches;
			int gas;
			Base base;

			int minerals() const;
		};

		struct Location {
			BaseInfo* info;
			std::vector<BaseInfo*> bases;
			std::vector<double> ground;
			std::vector<double> potential;
			
			virtual void addBase(BaseInfo* b);		
			bool compare(unsigned i, unsigned j);		
			virtual void sort();
		};

		struct StartLocation : Location {
			std::vector<double> air;
			Location natural;
			
			virtual void addBase(BaseInfo* b);
			virtual void sort();
		};

		struct BaseGraph {
			std::vector<BaseInfo> bases;
			std::vector<StartLocation> startLocations;
			unsigned selfIndex;
			unsigned enemyIndex;

			Base main() const;
			StartLocation const& self() const;
			Base enemyMain() const;
			StartLocation const& enemy() const;
			
			bool enemyKnown() const;
			
			void expanded(BWAPI::Position where);
			void enemySighted(BWAPI::Position where);

			void populate();		
			void update();
		};


		struct Builder {
			Unitset hatcheries;
			Unitset techBuildings;

			BaseGraph* allBases;

			Builder() : allBases(0) { }
			
			Unitset getLarva() const;
			Unit getNearestHatch(BWAPI::Position where) const;
			unsigned getNearestHatchIndex(BWAPI::Position where) const;
			void addHatch(Unit hatch);
		};
	};
};