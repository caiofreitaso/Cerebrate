#pragma once
#include "Infrastructure.h"
#include <algorithm>
#include <sstream>
#include <cmath>
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

		inline unsigned walkTiles(BWAPI::TilePosition tile) {
			unsigned tiles = 0;
			for (unsigned i = 0; i < 4; i++)
				for (unsigned j = 0; j < 4; j++)
					if (BWAPI::Broodwar->isWalkable(tile.x()*4 +i, tile.y()*4+j))
						tiles++;
			return tiles;
		}
		inline bool isWalkable(BWAPI::TilePosition tile) { return walkTiles(tile) > 0; }

		struct BaseInfo {
			static double wpatch;
			static double wchoke;
			static double wpoly;
//			static Map *map;

			Ownership owner;
			std::vector< Resource > patches;
			std::vector< Resource > geysers;
			Base base;

			int minerals() const;
			int gas() const;
			void draw(bool heat = false) const;
			double tileValue(BWAPI::TilePosition tile) const;

			BWAPI::TilePosition wallEnd(BWAPI::TilePosition A, BWAPI::TilePosition B) const;
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

			void draw() const;
		};

		struct Agent {
			BaseGraph* bases;

			Agent() : bases(0) { }
			void draw() {
				if (bases)
					bases->draw();
			}
			BWAPI::TilePosition nextBase() const {
				if (bases) {
					if (bases->self().natural.info->owner == None)
						return bases->self().natural.info->base->getTilePosition();
					else
						for (unsigned i = 0; i < bases->self().natural.bases.size(); i++)
							if (bases->self().natural.bases[i]->owner == None)
								return bases->self().natural.bases[i]->base->getTilePosition();
				}
			}
		};
	};
};