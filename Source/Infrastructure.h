#pragma once
#include "Resources.h"
#include "Production.h"
#include <algorithm>
#include <sstream>
#ifdef SAVE_IMG
#include <fstream>
#endif

namespace Cerebrate {
	namespace Industry {
		struct Manager;
	}
	namespace Resources {
		struct Miner;
	}
	
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

			void expanded(BWAPI::Position);
			void enemySighted(BWAPI::Position);

			void populate();
			void update();

			BaseInfo* nextBase() const;
			BWAPI::TilePosition nextBasePosition() const;

			void draw() const;
		};


		enum BuilderStates {
			Moving,
			Building,
			Fleeing
		};

		struct BuilderDrone {
			Unit drone;
			BWAPI::UnitType building;
			BuilderStates state;
			
			BWAPI::TilePosition target;
			BWAPI::Position center;
			
			BuilderDrone(Unit, BWAPI::UnitType, BWAPI::TilePosition);
		};
		
		struct BuilderSet {
			std::vector<BuilderDrone> builders;
			
			bool in(Unit) const;
			void act();
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
			bool isOccupied(BWAPI::TilePosition) const;
			bool canWall() const;
		};

		struct Builder {
			BaseGraph* bases;
			std::vector<Hatchery> hatcheries;
			BuilderSet builders;
			
			Builder() : bases(0) { }



			BWAPI::TilePosition nextBase() const { return bases->nextBasePosition(); }
			
			Unitset getLarva() const;
			bool drone(BWAPI::Position);
			bool build(Resources::Miner&, BWAPI::UnitType);
			
			
			Unit getNearestHatch(BWAPI::Position) const;
			unsigned getNearestHatchIndex(BWAPI::Position) const;
			
			
			void addHatch(Unit);

			BWAPI::Position spiral(BWAPI::Position, BuildingSlot&, BWAPI::TilePosition, bool creep) const;
			double getValue(BWAPI::TilePosition, bool creep) const;
			double getValue(int x, int y, BWAPI::TilePosition, bool creep) const;
			bool onCreep(int x, int y, BWAPI::TilePosition) const;
			bool isOccupied(BWAPI::TilePosition) const;
			
			void act();
			void update(Industry::Manager&);
			void draw() const;
		};
	};
};