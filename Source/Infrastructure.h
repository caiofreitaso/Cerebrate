#pragma once
#include "Resources.h"
#include <algorithm>
#include <sstream>
#ifdef SAVE_IMG
#include <fstream>
#endif

namespace Cerebrate {
	struct Resources::Miner;
	namespace Infrastructure {
		inline unsigned walkTiles(BWAPI::TilePosition tile) {
			unsigned tiles = 0;
			for (unsigned i = 0; i < 4; i++)
				for (unsigned j = 0; j < 4; j++)
					if (BWAPI::Broodwar->isWalkable(tile.x()*4 +i, tile.y()*4+j))
						tiles++;
			return tiles;
		}
		inline bool isWalkable(BWAPI::TilePosition tile) { return walkTiles(tile) > 0; }
	
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
			static double wpatch;
			static double wchoke;
			static double wpoly;
//			static Map *map;

			Ownership owner;
			std::vector< Resource > patches;
			std::vector< Resource > geysers;
			Base base;
			TerrainAnalysis::Polygon region;

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

			BWAPI::TilePosition nextBase() const;

			void draw() const;
		};


		struct BuildingSlot {
			int x;
			int y;
			BWAPI::TilePosition position;
			
			bool isOccupied(BWAPI::TilePosition pos) const {
				return pos.x() >= position.x() && pos.x() <= position.x() + x &&
						pos.y() >= position.y() && pos.y() <= position.y() + y;
			}
			bool isHatch() const { return x == 4 && y == 3; }
		};

		struct Hatchery {
			Unit hatch;
			BaseInfo* base;
			
			std::vector<BuildingSlot> wall;
			std::vector<BuildingSlot> sunkens;
			std::vector<BuildingSlot> spores;
			
			Hatchery(Unit h) : hatch(h), base(0) { }
			Hatchery(Unit h, BaseInfo* b) : hatch(h), base(b) { }
			
			bool isMacro() const { return !base; }
			bool isOccupied(BWAPI::TilePosition pos) const;
			bool canWall() const;
		};

		struct Builder {
			BaseGraph* bases;
			std::vector<Hatchery> hatcheries;
			//std::vector<BuildingSlot>
			
			Builder() : bases(0) { }



			BWAPI::TilePosition nextBase() const { return bases->nextBase(); }
			
			Unitset getLarva() const;
			bool drone(BWAPI::Position where);
			bool build(Resources::Miner& miner, BWAPI::UnitType structure);
			
			
			Unit getNearestHatch(BWAPI::Position where) const;
			unsigned getNearestHatchIndex(BWAPI::Position where) const;
			
			
			void addHatch(Unit hatch);

			BWAPI::Position spiral(BWAPI::Position center, BuildingSlot& slot, BWAPI::TilePosition position);
			double getValue(int x, int y, BWAPI::TilePosition position);
			bool onCreep(int x, int y, BWAPI::TilePosition position);
			bool isOccupied(BWAPI::TilePosition pos) const;
			
			void updateHatchs();
			void draw() const;
		};
	};
};