#pragma once

#include <BWAPI.h>
#include <BWAPI/Client.h>
#include <BWTA.h>
#include <cmath>
#include <string>
#include <sstream>
#ifdef SAVE_CSV
#include <fstream>
#endif
#include <vector>
#include <algorithm>

namespace Cerebrate {
	
	typedef BWAPI::Unit* Unit;
	typedef BWAPI::Player* Player;
	typedef std::vector<BWAPI::Unit*> Unitset;
	typedef BWTA::BaseLocation* Base;
	typedef std::vector<Base> Baseset;
	typedef BWTA::Chokepoint* Choke;
	typedef std::vector<Base> Chokeset;


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

				Baseset basesTaken;

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

				Unit getNearestHatch(BWAPI::Position where) {
					Unitset::iterator hatch = hatcheries.begin();
					
					double distance = where.getDistance((*hatch)->getPosition());
					Unit ret = *hatch;
					
					hatch++;
					

					for (; hatch != hatcheries.end(); hatch++)
						if (where.getDistance((*hatch)->getPosition()) < distance) {
							distance = where.getDistance((*hatch)->getPosition());
							ret = *hatch;
						}

					return ret;
				}
				unsigned getNearestHatchIndex(BWAPI::Position where) {
					double distance = where.getDistance(hatcheries[0]->getPosition());
					unsigned ret = 0, i = 0;
					
					i++;

					for (; i < hatcheries.size(); i++)
						if (where.getDistance(hatcheries[i]->getPosition()) < distance) {
							distance = where.getDistance(hatcheries[i]->getPosition());
							ret = i;
						}

					return ret;
				}
			};
		};
		//Need for resources
		namespace Resources {
			enum MinerStates {
				Idle,
				Waiting,
				Mining,
				Returning
			};
			
			struct MinerDrone {
				Unit drone;
				MinerStates state;
				MinerDrone(Unit u, MinerStates s):drone(u),state(s) { }
			};

			typedef std::vector<MinerDrone> Minerset;

			struct Mineralset {
				std::vector<Unit> patches;
				std::vector<bool> mining;
				std::vector<Minerset> miners;
				BWAPI::Position hatch;

				Mineralset() { }
				Mineralset(Unitset const& minerals, BWAPI::Position hatchery) {
					for (unsigned i = 0; i < minerals.size(); i++) {
						patches.push_back(minerals[i]);
						mining.push_back(false);
						miners.push_back(Minerset());
					}
					hatch = hatchery;
				}

				void update() {
					std::vector<unsigned> empties;
					for (unsigned i = 0; i < patches.size(); i++)
						if (patches[i]->getResources() == 0)
							empties.push_back(i);
					for (unsigned i = 0; i < empties.size(); i++) {
						patches.erase(patches.begin()+empties[i]-i);
						mining.erase(mining.begin()+empties[i]-i);
						miners.erase(miners.begin()+empties[i]-i);
					}

					for (unsigned i = 0; i < miners.size(); i++)
						for (unsigned j = 0; j < miners[i].size();)
							if (miners[i][j].drone->exists())
								j++;
							else
								miners[i].erase(miners[i].begin()+j);
				}

				Unitset getMiners() const {
					Unitset ret;

					for(unsigned i = 0; i < miners.size(); i++)
						for (unsigned j = 0; j < miners[i].size(); j++)
							ret.push_back(miners[i][j].drone);

					return ret;
				}
				unsigned size() const { return patches.size(); }
				unsigned indexOf(Unit mineral) const {
					unsigned i = 0;
					for (; i < patches.size(); i++)
						if (patches[i] == mineral)
							break;
					return i;
				}

				void addMiner(unsigned index, MinerDrone drone) { miners[index].push_back(drone); }
				void addMiner(Unit mineral, MinerDrone drone) { miners[indexOf(mineral)].push_back(drone); }
				void act() {
					for (unsigned i = 0; i < miners.size(); i++)
						for (unsigned j = 0; j < miners[i].size(); j++)
							switch(miners[i][j].state) {
								case Idle:
									if (miners[i][j].drone->getOrder() != BWAPI::Orders::ReturnMinerals)
										miners[i][j].state = Waiting;
									break;
								case Waiting:
									if (!mining[i]) {
										miners[i][j].drone->gather(patches[i]);
										mining[i] = true;
										miners[i][j].state = Mining;
									} else
										miners[i][j].drone->follow(patches[i]);
									break;
								case Mining:
									if (miners[i][j].drone->isCarryingMinerals()) {
										miners[i][j].state = Returning;
										mining[i] = false;
									}
									break;
								case Returning:
									if (miners[i][j].drone->isCarryingMinerals() && miners[i][j].drone->getOrder() != BWAPI::Orders::ReturnMinerals)
										miners[i][j].drone->returnCargo();
									else
										miners[i][j].state = Idle;
									break;
							}
				}

				Unit getBestMineral() {
					Unitset candidates;
					unsigned minSize = 4;
					for (unsigned i = 0; i < miners.size(); i++)
						if (miners[i].size() < minSize)
							minSize = miners[i].size();

					for (unsigned i = 0; i < miners.size(); i++)
						if (miners[i].size() == minSize)
							candidates.push_back(patches[i]);

					if (candidates.size()) {
						double distance = hatch.getDistance(candidates[0]->getPosition());
						Unit ret = candidates[0];

						for (unsigned i = 1; i < candidates.size(); i++)
							if (hatch.getDistance(candidates[i]->getPosition()) < distance) {
								distance = hatch.getDistance(candidates[i]->getPosition());
								ret = candidates[i];
							}

						return ret;
					} else
						return 0;
				}
				Unit getDrone() {
					std::vector<unsigned> candidates;
					for (unsigned i = 0; i < miners.size(); i++)
						if (miners[i].size() > 0)
							for (unsigned j = 0; j < candidates.size(); j++)
								if (hatch.getDistance(patches[i]->getPosition()) < hatch.getDistance(patches[j]->getPosition())) {
									candidates.insert(candidates.begin()+j,i);
									break;
								}

					if (candidates.size())
						for (unsigned i = 0; i < candidates.size(); i++)
							for(unsigned j = 0; j < miners[candidates[i]].size(); j++)
								if (miners[candidates[i]][j].state != Mining) {
									Unit ret = miners[candidates[i]][j].drone;
									miners[candidates[i]].erase(miners[candidates[i]].begin()+j);
									return ret;
								}
					else
						return 0;
				}
			};

			//typedef std::vector<Mineral> Mineralset;

			#pragma region Functions
			Unitset nearbyMinerals(Unit hatch) {
				Unitset ret;
				std::set<Unit> units = hatch->getUnitsInRadius(300);
				for (std::set<Unit>::iterator i = units.begin(); i != units.end(); i++)
					if ((*i)->getType().isMineralField())
						ret.push_back(*i);
				return ret;
			}

			#pragma endregion


			struct Miner {
				std::vector<Unitset> dronesPerExtrator;
				std::vector<Mineralset> minerals;


				void add(Unit hatch) {
					minerals.insert(minerals.begin(),Mineralset(nearbyMinerals(hatch),hatch->getPosition()));
				}

				void remove(unsigned index) {
					minerals.erase(minerals.begin()+index);
				}

				void update() {
					for (unsigned i = 0; i < minerals.size(); i++)
						minerals[i].update();

					std::vector<unsigned> empties;
					for (unsigned i = 0; i < minerals.size(); i++)
						if (minerals[i].size() == 0)
							empties.push_back(i);
					for (unsigned i = 0; i < empties.size(); i++)
						minerals.erase(minerals.begin()+empties[i]-i);
				}

				Unitset getAllMiners() const {
					Unitset ret;

					for(unsigned i = 0; i < minerals.size(); i++) {
						Unitset tmp = minerals[i].getMiners();
						ret.insert(ret.begin(), tmp.begin(), tmp.end());
					}

					return ret;
				}


				void idleWorker(Unit unit) {
					#ifndef NORMAL_MINING
					if (!has(getAllMiners(),unit))
					#endif
					{
						Unit mineral = 0;
						unsigned i = 0;
						for (; i < minerals.size(); i++) {
							mineral = minerals[i].getBestMineral();
							if (mineral)
								break;
						}
						#ifdef NORMAL_MINING
						unit->gather(mineral);
						#else
						minerals[i].addMiner(mineral,MinerDrone(unit,Idle));
						#endif
					}
				}

				void act() {
					for (unsigned i = 0; i < minerals.size(); i++)
						minerals[i].act();
				}
				Unit getDrone(BWAPI::Position where) {
					double distance = where.getDistance(minerals[0].hatch);
					unsigned index = 0, i = 0;
					
					i++;

					for (; i < minerals.size(); i++)
						if (where.getDistance(minerals[i].hatch) < distance) {
							distance = where.getDistance(minerals[i].hatch);
							index = i;
						}
					return minerals[index].getDrone();
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
					double	nope = 9*incomePerHatch*incomePerHatch;
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
			struct Agent {
				Baseset enemyBases;

				void sendScout() {

				}
			};
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
		#ifdef SAVE_CSV
		std::ofstream file;
		#endif

		struct Thresholds {
			double drones;
			double overlords;
			
			double priority;

			Thresholds() : drones(.25),overlords(.75),priority(.5) { }
		} thresholds;

		Cerebrate(bool debug = true) : player(0),debug(debug) { }
		~Cerebrate() {
			#ifdef SAVE_CSV
			file.close();
			#endif
		}
		void start() {
			#pragma region BWTA
			BWTA::readMap();
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)analyzeThread, this, 0, NULL);
			#pragma endregion
			
			#pragma region Set Broodwar commands
			BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
			BWAPI::Broodwar->setCommandOptimizationLevel(2);
			//BWAPI::Broodwar->setLocalSpeed(0);
			if (!debug)
				BWAPI::Broodwar->setGUI(false);
			player = BWAPI::Broodwar->self();
			#pragma endregion
			
			#ifdef SAVE_CSV
			#ifdef NORMAL_MINING
			file.open("normal.csv");
			#else
			file.open("cerebrate.csv");
			#endif
			#endif
		}

		void update() {
			finances.update(player, BWAPI::Broodwar->getFrameCount());
			#ifdef SAVE_CSV
			file << player->gatheredMinerals() - 50 << "\n";
			#endif
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

			mines.act();

			for (Unitset::iterator u = allUnits.units.begin(); u != allUnits.units.end(); u++) {
				if (Micro::isntValid(*u))
					continue;

				if ((*u)->isIdle()) {
					if ((*u)->getType().isWorker())
						mines.idleWorker(*u);
					else {
					}
				}
			}
		}
		void draw() {
			unsigned i = 0;

			#pragma region Resources
			for (; i < publicWorks.hatcheries.size(); i++) {
				BWAPI::Position hatch = publicWorks.hatcheries[i]->getPosition();

				for (unsigned j = 0; j < mines.minerals[i].size(); j++) {
					BWAPI::Position mineral = mines.minerals[i].patches[j]->getPosition();
					int resources = mines.minerals[i].patches[j]->getResources();
					
					BWAPI::Broodwar->setTextSize(0);
					for (unsigned k = 0; k < mines.minerals[i].miners[j].size(); k++) {
						BWAPI::Position drone = mines.minerals[i].miners[j][k].drone->getPosition();
						std::string state;
						BWAPI::Color color;
						switch (mines.minerals[i].miners[j][k].state) {
							case Macro::Resources::Idle:
								state = "\x05Idle";
								color = BWAPI::Colors::Grey;
								break;
							case Macro::Resources::Waiting:
								state = "\x16Wait";
								color = BWAPI::Colors::White;
								break;
							case Macro::Resources::Mining:
								state = "\x1FMine";
								color = BWAPI::Colors::Cyan;
								break;
							case Macro::Resources::Returning:
								state = "\x0FReturn";
								color = BWAPI::Colors::Teal;
								break;
						}
						BWAPI::Broodwar->drawLineMap(drone.x(), drone.y(), mineral.x(), mineral.y(), color);
						BWAPI::Broodwar->drawTextMap(drone.x()-8, drone.y()-4, "%s", state.c_str());
					}
					BWAPI::Broodwar->setTextSize(2);
					BWAPI::Broodwar->drawTextMap(mineral.x()-17, mineral.y()-8, "\x1F%d", resources);
					BWAPI::Broodwar->drawTextMap(mineral.x()-3, mineral.y()-4, "%d", mines.minerals[i].miners[j].size());
				}
				BWAPI::Broodwar->setTextSize(2);
				BWAPI::Broodwar->drawTextMap(hatch.x()-28, hatch.y()-16, "\x11Miners: %d", mines.minerals[i].getMiners().size());
			}
			#pragma endregion

			for (i = 0; i < publicWorks.basesTaken.size(); i++) {
				BWTA::Region* base = publicWorks.basesTaken[i]->getRegion();
				BWTA::Polygon drawing = base->getPolygon();

				if (drawing.size()) {
					BWAPI::Broodwar->drawLineMap(drawing[0].x(), drawing[0].y(), drawing[drawing.size()-1].x(), drawing[drawing.size()-1].y(), BWAPI::Colors::Purple);
					for (unsigned j = 0; j < drawing.size()-1; j++)
						BWAPI::Broodwar->drawLineMap(drawing[j].x(), drawing[j].y(), drawing[j+1].x(), drawing[j+1].y(), BWAPI::Colors::Purple);
				}

				std::set<Choke> chokes = base->getChokepoints();
				for (std::set<Choke>::iterator c = chokes.begin(); c != chokes.end(); c++)
					BWAPI::Broodwar->drawEllipseMap((*c)->getCenter().x(), (*c)->getCenter().y(), (*c)->getWidth(), (*c)->getWidth(), BWAPI::Colors::Red);
			}

			#pragma region Text
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 0, "Income: %f/%f %d", finances.income[0].mineral, finances.income[0].gas, finances.income[0].frame);
			BWAPI::Broodwar->drawTextScreen(5, 10, "HATCHERIES: %d, %d", publicWorks.hatcheries.size(), mines.minerals.size());
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


		void deleteUnit(Unit unit) {
			if (unit->getPlayer() == player) {
				if (unit->getType().isBuilding()) {
					if (unit->getType().isResourceDepot())
						removeHatchery(unit);
				} else
					allUnits.remove(unit);
			}
		}
		void newUnit(Unit unit) {
			if (unit->getPlayer() == player) {
				if (unit->getType().isBuilding()) {
					if (unit->getType().isResourceDepot())
						newHatchery(unit);
				} else
					allUnits.add(unit);
			}
		}
	
		static DWORD WINAPI analyzeThread(LPVOID pointer) {
			BWTA::analyze();
			Cerebrate* target = (Cerebrate*) pointer;

			if (BWTA::getStartLocation(BWAPI::Broodwar->self()) != 0)
				target->publicWorks.basesTaken.push_back(BWTA::getStartLocation(BWAPI::Broodwar->self()));
			return 0;
		}
	};


	class AIModule : public BWAPI::AIModule
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
			_data.deleteUnit(unit);
		}
		virtual void onUnitMorph(BWAPI::Unit* unit) { }
		virtual void onUnitRenegade(BWAPI::Unit* unit) { }
		virtual void onSaveGame(std::string gameName) { }
		virtual void onUnitComplete(BWAPI::Unit *unit) {
			_data.newUnit(unit);
		}
		// Everything below this line is safe to modify.

	};
}