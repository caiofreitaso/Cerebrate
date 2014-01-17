#pragma once
#include "Util.h"

namespace Cerebrate {
	namespace Resources {
		enum MinerStates {
			Waiting,
			Mining,
			Returning
		};

		struct MinerDrone {
			Unit drone;
			MinerStates state;

			MinerDrone(Unit u, MinerStates s) : drone(u), state(s) { }
		};

		typedef std::vector<MinerDrone> Minerset;

		struct Mineralset {
			std::vector<Unit> patches;
			std::vector<bool> mining;
			std::vector<Minerset> miners;
			BWAPI::Position hatch;


			Mineralset() { }
			Mineralset(Unitset const& minerals, BWAPI::Position hatchery);


			Unitset getMiners() const;
			unsigned size() const;
			unsigned indexOf(Unit mineral) const;

			void addMiner(unsigned index, MinerDrone drone);
			void addMiner(Unit mineral, MinerDrone drone);

			void balance();
			void update();
			void act();

			Unit getBestMineral();
			Unit getDrone();
		};

		struct Miner {
			std::vector<Unitset> dronesPerExtrator;
			std::vector<Mineralset> minerals;


			void add(Unit hatch);
			void remove(unsigned index);
			void update();
			void act();
			void draw();
			void idleWorker(Unit unit);
			Unitset getAllMiners() const;
			Unit getDrone(BWAPI::Position where);
		};
	};
};