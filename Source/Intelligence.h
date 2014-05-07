#pragma once
#include "Util.h"
#include <sstream>
#ifdef SAVE_IMG
#include <fstream>
#endif

namespace Cerebrate {
	namespace Intelligence {
		enum Ownership {
			None,
			Mine,
			His
		};

		struct Resource {
			Unit patch;
			int ammount;
			std::pair<int,int> position;

			Resource(Unit u, int a) : patch(u), ammount(a), position(0,0) { }
		};

		struct BaseInfo {
			Ownership owner;
			std::vector< Resource > patches;
			std::vector< Resource > geysers;
			Base base;
			TerrainAnalysis::Polygon region;

			int minerals() const;
			int gas() const;
			void draw() const;
		};

		struct Location {
			BaseInfo* info;
			std::vector<BaseInfo*> bases;
			std::vector<double> ground;
			std::vector<double> potential;

			virtual void addBase(BaseInfo*);
			bool compare(unsigned i, unsigned j);
			virtual void sort();
		};

		struct StartLocation : Location {
			std::vector<double> air;
			Location natural;

			virtual void addBase(BaseInfo*);
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

			unsigned expanded(BWAPI::Position);
			void enemySighted(BWAPI::Position);

			void populate();
			void update();

			BaseInfo* nextBase() const;
			BWAPI::TilePosition nextBasePosition() const;

			void draw() const;
		};
		
		struct Agent {
			BaseGraph* graph;
			
			Agent() : graph(0) {}
			
			void expanded(BWAPI::Position position) { if (graph) graph->expanded(position); }
			void draw() const { if (graph) graph->draw(); }
			void update() { if (graph) graph->update(); }
			BWAPI::TilePosition nextBase() const { return graph->nextBasePosition(); }
		};
	};
};