#pragma once

#include <BWAPI.h>
#include <BWAPI/Client.h>
#include <BWTA.h>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace Cerebrate {
	
	typedef BWAPI::Unit* Unit;
	typedef BWAPI::Player* Player;
	typedef std::vector<BWAPI::Unit*> Unitset;

	template<typename T>
	bool has(std::vector<T>& vector, T value) {
		for (unsigned i = 0; i < vector.size(); i++)
			if (vector[i] == value)
				return true;
		return false;
	}

	
	#pragma region Units Constants
	const BWAPI::UnitType Egg = BWAPI::UnitType(36);
	const BWAPI::UnitType Drone = BWAPI::UnitType(41);
	const BWAPI::UnitType Overlord = BWAPI::UnitType(42);
	const BWAPI::UnitType Ling = BWAPI::UnitType(37);
	const BWAPI::UnitType Hydra = BWAPI::UnitType(38);
	const BWAPI::UnitType Lurker = BWAPI::UnitType(103);
	const BWAPI::UnitType Muta = BWAPI::UnitType(43);
	const BWAPI::UnitType Scourge = BWAPI::UnitType(47);
	const BWAPI::UnitType Queen = BWAPI::UnitType(45);
	const BWAPI::UnitType Defiler = BWAPI::UnitType(46);
	const BWAPI::UnitType Ultra = BWAPI::UnitType(39);
	#pragma endregion

	enum Role {
		Civil,
		Harass,
		JackOfAllTrades,
		Siege,
		AntiAir,
		AOE,
		Detector,
		Massable,
		CrowdControl,
		Tank
	};
	typedef std::vector<Role> Roles;
	Roles getRoles(BWAPI::UnitType type) {
		Roles ret;
		switch(type.getID()) {
			case 42:
				ret.push_back(Detector);
			case 36:
			case 41:
				ret.push_back(Civil);
				break;
			case 43:
				ret.push_back(Harass);
			case 37:
				ret.push_back(Massable);
				break;
			case 38:
				ret.push_back(JackOfAllTrades);
				ret.push_back(Massable);
				break;
			case 103:
				ret.push_back(Siege);
			case 46:
				ret.push_back(AOE);
				ret.push_back(CrowdControl);
				break;
			case 45:
				ret.push_back(CrowdControl);
				break;
			case 47:
				ret.push_back(AntiAir);
				break;
			case 39:
				ret.push_back(Tank);
				break;
		}
		return ret;
	}


	struct UnitManager {
		Unitset units;

		void add(Unit unit) { units.push_back(unit); }
		void remove(Unit unit) {
			for (Unitset::iterator i = units.begin(); i != units.end(); i++)
				if (*i == unit) {
					units.erase(i);
					return;
				}
		}

		unsigned size() { return units.size(); }
		Unit& operator[](unsigned index) { return units[index]; }
		Unit const& operator[](unsigned index) const { return units[index]; }

		Unitset getType(BWAPI::UnitType type) {
			Unitset ret;
			for (unsigned i = 0; i < units.size(); i++)
				if (units[i]->getType() == type)
					ret.push_back(units[i]);
			return ret;
		}

		unsigned eggsMorphing(BWAPI::UnitType type) {
			Unitset eggs = getType(type);
			unsigned ret = 0;
			for (unsigned i = 0; i < eggs.size(); i++)
				if (eggs[i]->getBuildType() == type)
					ret++;
			return ret;
		}
		unsigned notCompleted(BWAPI::UnitType type) {
			unsigned ret = 0;
			Unitset all = getType(type);
			for (Unitset::iterator unit = all.begin(); unit != all.end(); unit++)
				if (!(*unit)->isCompleted())
					ret++;
			return ret;
		}
	};

	
	namespace Micro {
		bool isntValid(Unit u) {
			return !u->exists() || u->isLockedDown() || u->isMaelstrommed() || u->isStasised() ||
					u->isLoaded() || u->isUnpowered() || u->isStuck() || !u->isCompleted() || u->isConstructing();
		}
	};

	namespace Macro {
		struct Production {
			BWAPI::UnitType type;
			double priority;

			Production(BWAPI::UnitType t, double p) : type(t),priority(p) { }
		};
		bool compare(Production a, Production b) { return a.priority > b.priority; }
		typedef std::vector<Production> ProductionQueue;
		
		//Need for income
		namespace Economy
		{
			struct Resource {
				double mineral;
				double gas;
				
				int frame;

				Resource() : mineral(0), gas(0), frame(0) { }
			};
			struct Economist {
				static const int SIZE = 200;
				Resource states[SIZE];
				Resource income[SIZE];
				Resource incomeGrowth;

				void update(Player player, int frame) {
					int i;
					if (frame - states[0].frame >= 20) {

						i = SIZE-1;
						for (; i > 0; i--) {
							states[i].mineral = states[i-1].mineral;
							states[i].gas = states[i-1].gas;
							states[i].frame = states[i-1].frame;
						}

						states[0].mineral = player->gatheredMinerals() - 50;
						states[0].gas = player->gatheredGas();
						states[0].frame = frame;

						for (i = SIZE-1; i > 0; i--) {
							income[i].mineral = income[i-1].mineral;
							income[i].gas = income[i-1].gas;
							income[i].frame = income[i-1].frame;
						}

						income[0] = Resource();
						for (i = 0; i < SIZE-1; i++)
							if (frame - states[i].frame > 500)
								break;
			
						income[0].mineral = (states[0].mineral - states[i].mineral) / (frame - states[i].frame);
						income[0].gas = (states[0].gas - states[i].gas) / (frame - states[i].frame);
						income[0].frame = frame;
			
						incomeGrowth = Resource();
						incomeGrowth.mineral = (income[0].mineral - income[1].mineral) / (frame - income[1].frame);
						incomeGrowth.gas = (income[0].gas - income[1].gas) / (frame - income[1].frame);
						incomeGrowth.frame = frame;
					}
				}
				bool support(BWAPI::UnitType unit, int quantity = 1) {
					return	states[0].mineral >= (unit.mineralPrice()*quantity) &&
							states[0].gas >= (unit.gasPrice()*quantity);
				}
				Resource projectResources(int frames) {
					Resource ret;
					ret.frame	=	-1;
					ret.mineral	=	states[0].mineral + income[0].mineral * frames;
					ret.gas		=	states[0].gas + income[0].gas * frames;
					return ret;
				}
			};			
		};
		//Need to build
		namespace Infrastructure {
			struct Builder {
				Unitset hatcheries;
				Unitset techBuildings;
				ProductionQueue buildQueue;

				double expand;

				Unitset getLarva() {
					Unitset ret;
					for (Unitset::iterator hatch = hatcheries.begin(); hatch != hatcheries.end(); hatch++) {
						std::set<Unit> larvae = (*hatch)->getLarva();
						for (std::set<Unit>::iterator larva = larvae.begin(); larva != larvae.end(); larva++)
							ret.push_back(*larva);
					}
					return ret;
				}

				Unit getNearestHatch(Unit unit) {
					Unitset::iterator hatch = hatcheries.begin();
					
					double distance = unit->getPosition().getDistance((*hatch)->getPosition());
					Unit ret = *hatch;
					
					hatch++;
					

					for (; hatch != hatcheries.end(); hatch++)
						if (unit->getPosition().getDistance((*hatch)->getPosition()) < distance) {
							distance = unit->getPosition().getDistance((*hatch)->getPosition());
							ret = *hatch;
						}

					return ret;
				}
				unsigned getNearestHatchIndex(Unit unit) {
					double distance = unit->getPosition().getDistance(hatcheries[0]->getPosition());
					unsigned ret = 0, i = 0;
					
					i++;

					for (; i < hatcheries.size(); i++)
						if (unit->getPosition().getDistance(hatcheries[i]->getPosition()) < distance) {
							distance = unit->getPosition().getDistance(hatcheries[i]->getPosition());
							ret = i;
						}

					return ret;
				}
			};
		};
		//Need for resources
		namespace Resources {
			#pragma region Functions
			int remainingMinerals(Unitset minerals) {
				int ret = 0;
				for (Unitset::iterator i = minerals.begin(); i != minerals.end(); i++)
					ret += (*i)->getResources();
				return ret;
			}
			Unitset nearbyMinerals(Unit hatch) {
				Unitset ret;
				std::set<Unit> units = hatch->getUnitsInRadius(300);
				for (std::set<Unit>::iterator i = units.begin(); i != units.end(); i++)
					if ((*i)->getType().isMineralField())
						ret.push_back(*i);
				return ret;
			}

			double miningPotential(Unitset minerals) {
				return 0;
			}
			#pragma endregion

			struct Miner {
				std::vector<Unitset> dronesPerHatchery;
				std::vector<Unitset> dronesPerExtrator;

				std::vector<Unitset> mineralsPerHatchery;
				std::vector<double> miningPotentials;


				void add(Unit hatch) {
					dronesPerHatchery.insert(dronesPerHatchery.begin(),Unitset());
					mineralsPerHatchery.insert(mineralsPerHatchery.begin(),nearbyMinerals(hatch));
					miningPotentials.insert(miningPotentials.begin(),0);
				}

				void remove(unsigned index) {
					dronesPerHatchery.erase(dronesPerHatchery.begin()+index);
					mineralsPerHatchery.erase(mineralsPerHatchery.begin()+index);
					miningPotentials.erase(miningPotentials.begin()+index);
				}

				void update() {
					for (std::vector<Unitset>::iterator hatch = dronesPerHatchery.begin();
						 hatch != dronesPerHatchery.end(); hatch++)
						for (Unitset::iterator drone = hatch->begin(); drone != hatch->end();)
							if (!(*drone)->isGatheringMinerals()) {
								hatch->erase(drone);
								drone = hatch->begin();
							} else
								drone++;
					for (std::vector<Unitset>::iterator hatch = mineralsPerHatchery.begin();
						 hatch != mineralsPerHatchery.end(); hatch++)
						for (Unitset::iterator mineral = hatch->begin(); mineral != hatch->end();)
							if (!(*mineral)->exists()) {
								hatch->erase(mineral);
								mineral = hatch->begin();
							} else
								mineral++;
					/*for (unsigned i = 0; i < miningPotentials.size(); i++)
						miningPotentials[i] = miningPotential(mineralsPerHatchery[i]);*/
				}


				std::pair<Unit,unsigned> getNearestMineral(Unit unit) {
					unsigned hatch = 0;
					Unitset::iterator mineral = mineralsPerHatchery[hatch].begin();
					
					double distance = unit->getPosition().getDistance((*mineral)->getPosition());
					std::pair<Unit,unsigned> ret(*mineral,hatch);

					for (; hatch < mineralsPerHatchery.size(); hatch++)
						for (; mineral != mineralsPerHatchery[hatch].end(); mineral++)
							if (unit->getPosition().getDistance((*mineral)->getPosition()) < distance) {
								distance = unit->getPosition().getDistance((*mineral)->getPosition());
								ret.first = *mineral;
								ret.second = hatch;
							}

					return ret;
				}

				void idleWorker(Unit unit, Infrastructure::Builder& builder) {
					if (unit->isCarryingGas() || unit->isCarryingMinerals())
						unit->returnCargo();
					else {
						std::pair<Unit,unsigned> mineral = getNearestMineral(unit);
						unit->gather(mineral.first);

						dronesPerHatchery[mineral.second].push_back(unit);
					}
				}
			};
		};
		//Need to research
		namespace Technology {
			struct Scientist {
				double armor;
				double ranged;
				double melee;

				double airArmor;
				double airAttack;
				std::vector<BWAPI::UpgradeType> techQueue;
			};
		};
		//Need for units
		namespace Industry {
			#pragma region Functions
			int* remainingTimeForLarva(Infrastructure::Builder& builder) {
				int* ret = new int[builder.hatcheries.size()];
				
				for (unsigned i = 0; i < builder.hatcheries.size(); i++)
					ret[i] = builder.hatcheries[i]->getRemainingTrainTime();
				return ret;
			}
			
			double supply_diff(Player p, double supplyUsed) {
				double supply = p->supplyTotal() - supplyUsed;
				supply = (1 - exp(1.5*supply - 4.5))/(1 + exp(1.5*supply - 4.5));
				supply += 1;
				supply /= 2;
				return supply * supply;
			}
			#pragma endregion

			struct Manager {
				std::vector<Production> unitQueue;
				
				double drones;
				double overlords;

				void needForOverlords(Infrastructure::Builder& builder, UnitManager units, Player p) {
					unsigned i = 0;
					for (; i < unitQueue.size(); i++)
						if (unitQueue[i].type == Overlord)
							break;
					if (i < unitQueue.size())
						overlords = 0;
					else if (units.eggsMorphing(Overlord) != 0 || units.notCompleted(Overlord) != 0)
						overlords = 0;
					else {
						int extraSupply = 0;
						unsigned larvaCount = builder.getLarva().size();

						unsigned maxIndex = larvaCount < unitQueue.size() ? larvaCount : unitQueue.size();

						for (i = 0; i < maxIndex; i++)
							if (!unitQueue[i].type.isWorker())
								extraSupply += unitQueue[i].type.supplyRequired();

						if (extraSupply)
							;
						else
							overlords = supply_diff(p, p->supplyUsed());
					}
				}
				void needForDrones(Infrastructure::Builder& builder, Economy::Economist& economist) {
					double	incomePerHatch = economist.income[0].mineral/builder.hatcheries.size();
					double	nope = (5*incomePerHatch/2)*(5*incomePerHatch/2);
					if (nope > 1)
						nope = 1;
					drones = 1 - nope;
					drones *= drones;
					drones *= drones;
				}
				
				void clean(double priorityThreshold, UnitManager units) {
					if (unitQueue.size())
						for (ProductionQueue::iterator i = unitQueue.begin(); i != unitQueue.end();) {
							if (i->priority < priorityThreshold || i->type == Drone ||
								(i->type == Overlord && (units.eggsMorphing(Overlord) != 0 || units.notCompleted(Overlord) != 0))) {
								unitQueue.erase(i);
								i = unitQueue.begin();
							} else
								i++;
						}
				}
				void queueCivil(double droneThreshold, double overlordThreshold) {
					if (drones >= droneThreshold)
						unitQueue.push_back(Production(Drone,drones));
					if (overlords >= overlordThreshold)
						unitQueue.push_back(Production(Overlord,overlords));
					std::sort(unitQueue.begin(), unitQueue.end(), compare);
				}

				void morph(Macro::Infrastructure::Builder& builder) {
					Unitset larvae = builder.getLarva();
				
					unsigned size = larvae.size();
					size = size < unitQueue.size() ? size : unitQueue.size();
		
					unsigned i = 0;
					for (Unitset::iterator larva = larvae.begin(); larva != larvae.end() && i < size; larva++, i++) {
						if ((*larva)->morph(unitQueue[0].type))
							unitQueue.erase(unitQueue.begin());
					}
				}
			};
		};
		//Need to attack
		namespace Defense {
			struct General {
				double attack;
				double defend;
			
				void update(Player player, UnitManager units) {
					attack = player->supplyTotal() - units.getType(Drone).size();
					attack = (1 - exp(-0.4*attack - 1.2))/(1 + exp(-0.4*attack - 1.2));
					attack += 1;
					attack /= 2;
					attack *= attack;
					attack *= attack;
				}
			};
		};
		//Need to know
		namespace Intelligence {
		};
	};
	
	struct Cerebrate {
		Player player;
		
		UnitManager allUnits;
		
		Macro::Resources::Miner			mines;
		Macro::Economy::Economist		finances;
		Macro::Industry::Manager		industry;
		Macro::Infrastructure::Builder	publicWorks;
		Macro::Technology::Scientist	science;
		Macro::Defense::General			defense;

		bool debug;

		struct Thresholds {
			double drones;
			double overlords;
			
			double priority;

			Thresholds() : drones(.25),overlords(.6),priority(.5) { }
		} thresholds;

		Cerebrate(bool debug = true) : player(0),debug(debug) { }

		void start() {
			BWTA::readMap();
			BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
			BWAPI::Broodwar->setCommandOptimizationLevel(2);
			if (!debug) {
				BWAPI::Broodwar->setLocalSpeed(0);
				BWAPI::Broodwar->setGUI(false);
			}
			player = BWAPI::Broodwar->self();
		}
		void update() {
			finances.update(player, BWAPI::Broodwar->getFrameCount());
			mines.update();
			industry.needForDrones(publicWorks, finances);
			industry.needForOverlords(publicWorks, allUnits, player);
		}
		void act() {
			if (BWAPI::Broodwar->isReplay() || !BWAPI::Broodwar->getFrameCount())
				return;

			if (debug)
				draw();

			if (BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self())
				return;

			update();
		
			if (BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0)
				return;

			industry.clean(thresholds.priority, allUnits);
			industry.queueCivil(thresholds.drones, thresholds.overlords);

			industry.morph(publicWorks);

			for (Unitset::iterator u = allUnits.units.begin(); u != allUnits.units.end(); u++) {
				if (Micro::isntValid(*u))
					continue;

				if ((*u)->isIdle()) {
					if ((*u)->getType().isWorker()) {
						mines.idleWorker(*u, publicWorks);
					} else {
					}
				}
			}
		}
		void draw() {
			#pragma region Resources
			BWAPI::Broodwar->setTextSize(2);
			unsigned i = 0;
			for (; i < publicWorks.hatcheries.size(); i++) {
				BWAPI::Position hatch = publicWorks.hatcheries[i]->getPosition();

				for (unsigned j = 0; j < mines.mineralsPerHatchery[i].size(); j++) {
					BWAPI::Position mineral = mines.mineralsPerHatchery[i][j]->getPosition();
					int resources = mines.mineralsPerHatchery[i][j]->getResources();

					BWAPI::Broodwar->drawTextMap(mineral.x(), mineral.y(), "\x1F%d", resources);
				}
				BWAPI::Broodwar->drawTextMap(hatch.x(), hatch.y(), "\x11Miners: %d", mines.dronesPerHatchery[i].size());
				BWAPI::Broodwar->drawEllipseMap(hatch.x(), hatch.y(), 300, 250, BWAPI::Colors::Cyan);
			}
			#pragma endregion			

			#pragma region Text
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 0, "Income: %f/%f %d", finances.income[0].mineral, finances.income[0].gas, finances.income[0].frame);
			BWAPI::Broodwar->drawTextScreen(5, 10, "Growth: %f/%f", finances.incomeGrowth.mineral, finances.incomeGrowth.gas);
			BWAPI::Broodwar->drawTextScreen(5, 20, "Need for a) drones: %f\tb) overlords: %f", industry.drones, industry.overlords);
			BWAPI::Broodwar->drawTextScreen(5, 30, "Building a) drones: %d [%d]\tb) overlords: %d [%d]",
				allUnits.eggsMorphing(Drone),allUnits.notCompleted(Drone),
				allUnits.eggsMorphing(Overlord),allUnits.notCompleted(Overlord));
			
			std::ostringstream units_decl;
			units_decl << "UNITS = ";
			std::vector<BWAPI::UnitType> alreadyDone;
			BWAPI::UnitType type;
			for (i = 0; i < allUnits.size() - 1; i++) {
				type = allUnits[i]->getType();
				if (!has(alreadyDone, type)) {
					units_decl << "\x04";
					units_decl << &type.c_str()[5];
					units_decl << ":\x02";
					units_decl << allUnits.getType(type).size();
					units_decl << ", ";
					alreadyDone.push_back(type);
				}
			}
			type = allUnits[i]->getType();
			if (!has(alreadyDone, type)) {
				units_decl << "\x04";
				units_decl << &type.c_str()[5];
				units_decl << ":\x02";
				units_decl << allUnits.getType(type).size();
			}
			alreadyDone.clear();
			BWAPI::Broodwar->drawTextScreen(5, 40, units_decl.str().c_str());
			
			if (publicWorks.hatcheries.size()) {
				BWAPI::Broodwar->setTextSize(1);
				BWAPI::Broodwar->drawTextScreen(5, 50, "Queue:");
				BWAPI::Broodwar->setTextSize(0);
				for (i = 0; i < industry.unitQueue.size(); i++)
					BWAPI::Broodwar->drawTextScreen(5,65+10*i,"\x04%s:%.3f",&industry.unitQueue[i].type.getName().c_str()[5],industry.unitQueue[i].priority);
			}
			#pragma endregion

			#pragma region Income Graph
			for (int i = 0; i < 199; i++) {
				BWAPI::Broodwar->drawLineScreen(200-i,300-(int)((10000/42)*finances.income[i].mineral),
												200-i-1,300-(int)((10000/42)*finances.income[i+1].mineral),BWAPI::Colors::Cyan);
				BWAPI::Broodwar->drawLineScreen(200-i,300-(int)((10000/42)*finances.income[i].gas),
												200-i-1,300-(int)((10000/42)*finances.income[i+1].gas),BWAPI::Colors::Green);
			}
			#pragma endregion
			
		}

		void newHatchery(Unit hatch) {
			publicWorks.hatcheries.insert(publicWorks.hatcheries.begin(), hatch);
			mines.add(hatch);
		}
		void removeHatchery(Unit hatch) {
			unsigned index = 0;
			for(Unitset::iterator i = publicWorks.hatcheries.begin(); i != publicWorks.hatcheries.end(); i++, index++)
				if (*i == hatch) {
					publicWorks.hatcheries.erase(i);
					break;
				}
			mines.remove(index);
		}

	};


	class Interface : public BWAPI::AIModule
	{
		private:
			Cerebrate _data;
		public:
		virtual void onStart() {
			if (!BWAPI::Broodwar->isReplay())
				_data.start();
		}
		virtual void onEnd(bool isWinner) { }
		virtual void onFrame() {
			_data.act();
		}
		virtual void onSendText(std::string text) { }
		virtual void onReceiveText(BWAPI::Player* player, std::string text) { }
		virtual void onPlayerLeft(BWAPI::Player* player) { }
		virtual void onNukeDetect(BWAPI::Position target) { }
		virtual void onUnitDiscover(BWAPI::Unit* unit) { }
		virtual void onUnitEvade(BWAPI::Unit* unit) { }
		virtual void onUnitShow(BWAPI::Unit* unit) { }
		virtual void onUnitHide(BWAPI::Unit* unit) { }
		virtual void onUnitCreate(BWAPI::Unit* unit) { }
		virtual void onUnitDestroy(BWAPI::Unit* unit) {
			if (unit->getPlayer() == _data.player) {
				if (unit->getType().isBuilding()) {
					if (unit->getType().isResourceDepot())
						_data.removeHatchery(unit);
				} else
					_data.allUnits.remove(unit);
			}
		}
		virtual void onUnitMorph(BWAPI::Unit* unit) { }
		virtual void onUnitRenegade(BWAPI::Unit* unit) { }
		virtual void onSaveGame(std::string gameName) { }
		virtual void onUnitComplete(BWAPI::Unit *unit) {
			if (unit->getPlayer() == _data.player) {
				if (unit->getType().isBuilding()) {
					if (unit->getType().isResourceDepot())
						_data.newHatchery(unit);
				} else
					_data.allUnits.add(unit);
			}
		}
		// Everything below this line is safe to modify.

	};
}