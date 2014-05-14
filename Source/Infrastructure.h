#pragma once
#include "Resources.h"
#include "Economy.h"
#include "Intelligence.h"
#include <algorithm>

namespace Cerebrate {
	namespace Industry {
		struct Manager;
	}
	namespace Resources {
		struct Miner;
	}
	namespace Economy {
		struct Economist;
	}
	
	namespace Infrastructure {
		struct Builder;
	
		namespace PotentialField {
			inline unsigned walkTiles(BWAPI::TilePosition tile) {
				unsigned tiles = 0;
				for (unsigned i = 0; i < 4; i++)
					for (unsigned j = 0; j < 4; j++)
						if (BWAPI::Broodwar->isWalkable(tile.x()*4 +i, tile.y()*4+j))
							tiles++;
				return tiles;
			}
			inline bool isWalkable(BWAPI::TilePosition tile) { return walkTiles(tile) > 0; }
		
			extern double wpatch;
			extern double wchoke;
			extern double wpoly;
			
			double value(Intelligence::Agent const&, const Builder, BWAPI::TilePosition, bool creep, bool occupied = true);
			double value(Intelligence::Agent const&, const Builder, int x, int y, BWAPI::TilePosition, bool creep, bool occupied = true);
			
			double valueForTarget(BWAPI::Position, Intelligence::Agent const&, const Builder, BWAPI::TilePosition, bool creep, bool occupied = true);
			double valueForTarget(BWAPI::Position, Intelligence::Agent const&, const Builder, int x, int y, BWAPI::TilePosition, bool creep, bool occupied = true);
			
			bool onCreep(int x, int y, BWAPI::TilePosition);
		}
	
		enum BuilderStates {
			Moving,
			Building,
			Fleeing,
			Done
		};

		struct BuilderDrone {
			Unit drone;
			BWAPI::UnitType building;
			BuilderStates state;
			
			BWAPI::TilePosition target;
			BWAPI::Position center;
			
			unsigned budgetID;
			
			BuilderDrone(Unit, BWAPI::UnitType, BWAPI::TilePosition, unsigned);
		};
		
		struct BuilderSet {
			std::vector<BuilderDrone> builders;
			
			bool in(Unit) const;
			void act(Economy::Economist&, Industry::Manager&, Resources::Miner&, Intelligence::Agent&, Builder&);
			void draw() const;
		};

		struct BuildingSlot {
			int x;
			int y;
			BWAPI::TilePosition position;
			
			bool isOccupied(BWAPI::TilePosition pos) const {
				return pos.x() >= position.x() && pos.x() < position.x() + x &&
						pos.y() >= position.y() && pos.y() < position.y() + y;
			}
			bool isHatch() const { return x == 4 && y == 3; }
		};

		struct Hatchery {
			BWAPI::TilePosition hatch;
			Intelligence::BaseInfo* base;
			
			std::vector<BuildingSlot> wall;
			std::vector<BuildingSlot> sunkens;
			std::vector<BuildingSlot> spores;
			
			Hatchery(Unit h) : hatch(h->getTilePosition()), base(0) { }
			Hatchery(Unit h, Intelligence::BaseInfo* b) : hatch(h->getTilePosition()), base(b) { }
			
			bool isMacro() const { return !base; }
			bool isOccupied(BWAPI::TilePosition) const;
			bool canWall() const;
			bool adjacent(BuildingSlot) const;
		};

		struct Builder {
			std::vector<Hatchery> hatcheries;
			BuilderSet builders;
			


			bool build(Resources::Miner&, Intelligence::Agent const&, BWAPI::UnitType, unsigned);
			
			void addHatch(Unit, Intelligence::Agent&);
			
			bool isOccupied(BWAPI::TilePosition) const;
			BWAPI::Position spiral(Intelligence::Agent const&, BWAPI::Position, BuildingSlot&, BWAPI::TilePosition, bool creep) const;
			
			void act(Economy::Economist&, Industry::Manager&, Resources::Miner&, Intelligence::Agent&);
			void update(Industry::Manager&, Intelligence::Agent&);
			void draw(Intelligence::Agent const&) const;
		};
	};
};