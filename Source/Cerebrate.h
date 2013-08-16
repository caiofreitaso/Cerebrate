#pragma once

#include <BWAPI/Client.h>
#include <string>
#ifdef SAVE_CSV
#include <fstream>
#endif

#include "Economy.h"
#include "Resources.h"

namespace Cerebrate {
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

		unsigned size() const { return units.size(); }
		Unit& operator[](unsigned index) { return units[index]; }
		Unit const& operator[](unsigned index) const { return units[index]; }

		Unitset getType(BWAPI::UnitType type) const  {
			Unitset ret;
			for (unsigned i = 0; i < units.size(); i++)
				if (units[i]->getType() == type)
					ret.push_back(units[i]);
			return ret;
		}

		unsigned eggsMorphing(BWAPI::UnitType type) const {
			Unitset eggs = getType(type);
			unsigned ret = 0;
			for (unsigned i = 0; i < eggs.size(); i++)
				if (eggs[i]->getBuildType() == type)
					ret++;
			return ret;
		}
		unsigned notCompleted(BWAPI::UnitType type) const {
			unsigned ret = 0;
			Unitset all = getType(type);
			for (Unitset::iterator unit = all.begin(); unit != all.end(); unit++)
				if (!(*unit)->isCompleted())
					ret++;
			return ret;
		}
	};

	
	
	bool isntValid(Unit u) {
		return !u->exists() || u->isLockedDown() || u->isMaelstrommed() || u->isStasised() ||
				u->isLoaded() || u->isUnpowered() || u->isStuck() || !u->isCompleted() || u->isConstructing();
	}
	struct Production_Test {
		BWAPI::UnitType type;
		BWAPI::TechType tech;
		bool unit;
		double priority;

		Production_Test(BWAPI::UnitType t, double p) : type(t),unit(true),priority(p) { }
		Production_Test(BWAPI::TechType t, double p) : tech(t),unit(false),priority(p) { }
	};
	bool compare_Test(Production_Test a, Production_Test b) { return a.priority > b.priority; }
	
	class ProductionQueue_Test {
		std::vector<Production_Test> _data;
		public:
			void add(Production_Test p) { _data.push_back(p); std::sort(_data.begin(),_data.end(),compare_Test); }
			
			Production_Test& top() { return _data[0]; }
			Production_Test const& top() const { return _data[0]; }

			Production_Test& operator[](unsigned i) { return _data[i]; }
			Production_Test const& operator[](unsigned i) const { return _data[i]; }

			void pop() { _data.erase(_data.begin()); }

			unsigned size() const { return _data.size(); }

			void update(double priorityThreshold) {
				if (_data.size())	
					for (std::vector<Production_Test>::iterator i = _data.begin(); i != _data.end();) {
						if (i->priority < priorityThreshold || i->type == Drone || i->type == Overlord) {
							_data.erase(i);
							i = _data.begin();
						} else
							i++;
					}
			}

			void morph(Unitset larvae) {
				unsigned size = larvae.size();
				size = size < _data.size() ? size : _data.size();
	
				unsigned i = 0;
				for (Unitset::iterator larva = larvae.begin(); larva != larvae.end() && i < size; larva++, i++) {
					if ((*larva)->morph(top().type))
						_data.erase(_data.begin());
				}
			}

			void build(Unit drone, BWAPI::TilePosition position) { drone->build(position,top().unit); }

			void research(UnitManager const& units) {
				(*units.getType(top().tech.whatResearches()).begin())->research(top().tech);
			}

			Production_Test& overlord() {
				unsigned i = 0;
				for (; i < _data.size(); i++)
					if (_data[i].unit)
						if (_data[i].type == Overlord)
							break;
				return _data[i];
			}
			
			Production_Test& hatchery() {
				unsigned i = 0;
				for (; i < _data.size(); i++)
					if (_data[i].unit)
						if (_data[i].type == Hatch)
							break;
				return _data[i];
			}
	};

	
	struct Production {
		BWAPI::UnitType type;
		double priority;

		Production(BWAPI::UnitType t, double p) : type(t),priority(p) { }
	};
	bool compare(Production a, Production b) { return a.priority > b.priority; }
	typedef std::vector<Production> ProductionQueue;
	
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

			void needForOverlords(Infrastructure::Builder const& builder, UnitManager const& units, Player p) {
				if (units.eggsMorphing(Overlord) != 0 || units.notCompleted(Overlord) != 0) {
					overlords = 0;
				} else {
					int extraSupply = 0;
					unsigned larvaCount = builder.getLarva().size();

					unsigned maxIndex = larvaCount < unitQueue.size() ? larvaCount : unitQueue.size();

					for (unsigned i = 0; i < maxIndex; i++)
						if (!unitQueue[i].type.isWorker())
							extraSupply += unitQueue[i].type.supplyRequired();

					if (extraSupply)
						;
					else
						overlords = supply_diff(p, p->supplyUsed());
				}
			}
			void needForDrones(Infrastructure::Builder const& builder, Economy::Economist const& economist) {
				double	incomePerHatch = economist.income[0].mineral/(builder.hatcheries.size()+1);
				double	nope = 7*incomePerHatch*incomePerHatch;
				if (nope > 1)
					nope = 1;
				drones = 1 - nope;
				drones *= drones;
				drones *= drones;
			}
			
			void clean(double priorityThreshold, UnitManager units) {
				if (unitQueue.size())
					for (ProductionQueue::iterator i = unitQueue.begin(); i != unitQueue.end();) {
						if (i->priority < priorityThreshold || i->type == Drone || i->type == Overlord) {
							unitQueue.erase(i);
							i = unitQueue.begin();
						} else
							i++;
					}
			}
			void queueCivil(double droneThreshold, double overlordThreshold) {
				//if (drones >= droneThreshold)
					unitQueue.push_back(Production(Drone,drones));
				//if (overlords >= overlordThreshold)
				//	unitQueue.push_back(Production(Overlord,overlords));
				std::sort(unitQueue.begin(), unitQueue.end(), compare);
			}

			void morph(Infrastructure::Builder& builder) {
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
	
	struct Cerebrate {
		Player player;
		
		UnitManager allUnits;
		
		Resources::Miner			mines;
		Economy::Economist			finances;
		Industry::Manager			industry;
		Infrastructure::Builder		publicWorks;
		Technology::Scientist		science;
		Defense::General			defense;

		bool debug, makeDrones;
		#ifdef SAVE_CSV
		std::ofstream file;
		#endif

		struct Thresholds {
			double drones;
			double overlords;
			
			double priority;

			Thresholds() : drones(.25),overlords(.75),priority(.5) { }
		} thresholds;

		Cerebrate(bool debug = true) : player(0),debug(debug),makeDrones(true) { }
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
			BWAPI::Broodwar->setLocalSpeed(20);
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
			finances.update(player, BWAPI::Broodwar->getFrameCount(), allUnits.getType(Drone).size());
			#ifdef SAVE_CSV
			file << player->gatheredMinerals() - 50 << "," << BWAPI::Broodwar->getAPM() << "\n";
			#endif
			mines.update();
			industry.needForDrones(publicWorks, finances);
			industry.needForOverlords(publicWorks, allUnits, player);
			if (publicWorks.allBases)
				publicWorks.allBases->update();
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
			if (makeDrones)
				industry.queueCivil(thresholds.drones, thresholds.overlords);

			industry.morph(publicWorks);

			mines.act();

			for (Unitset::iterator u = allUnits.units.begin(); u != allUnits.units.end(); u++) {
				if (isntValid(*u))
					continue;

				if ((*u)->isIdle()) {
					if ((*u)->getType().isWorker())
						mines.idleWorker(*u,publicWorks);
					else {
					}
				}
			}
		}
		void draw() {
			unsigned i = 0;

			#pragma region Bases
			BWAPI::Broodwar->setTextSize(0);
			if (publicWorks.allBases) {
				BWAPI::Position position = publicWorks.allBases->self().info->base->getPosition();
				BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Blue);
				BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,BWAPI::Colors::Green);
				BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),72,BWAPI::Colors::Blue);
				
				for (i = 0; i < publicWorks.allBases->self().bases.size(); i++) {
					position = publicWorks.allBases->self().bases[i]->base->getPosition();
					BWAPI::Broodwar->drawTextMap(position.x()-45, position.y()+27, "M.G:%.0f\nM.A:%.0f",publicWorks.allBases->self().ground[i],publicWorks.allBases->self().air[i]);
				}
				position = publicWorks.allBases->self().natural.info->base->getPosition();
				BWAPI::Color color;
				switch(publicWorks.allBases->self().natural.info->owner) {
					case Infrastructure::None:
						color = BWAPI::Colors::Brown;
						break;
					case Infrastructure::Mine:
						color = BWAPI::Colors::Blue;
						break;
					case Infrastructure::His:
						color = BWAPI::Colors::Red;
						break;
				}
				BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,color);
				BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Yellow);
				BWAPI::Broodwar->setTextSize(1);
				std::set<Choke> chokes = publicWorks.allBases->self().natural.info->base->getRegion()->getChokepoints();
				for (std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++) {
					BWAPI::Broodwar->drawLineMap((*choke)->getSides().first.x(), (*choke)->getSides().first.y(), (*choke)->getSides().second.x(),(*choke)->getSides().second.y(), BWAPI::Colors::Grey);
					BWAPI::Broodwar->drawTextMap((*choke)->getCenter().x(), (*choke)->getCenter().y(), "\x1D%.0f",(*choke)->getWidth());
					//BWAPI::Broodwar->drawCircleMap((*choke)->getCenter().x(), (*choke)->getCenter().y(), (*choke)->getWidth()/2, BWAPI::Colors::Grey);
				}
				
				for (i = 0; i < publicWorks.allBases->self().natural.bases.size(); i++) {
					BWAPI::Broodwar->setTextSize(0);
					position = publicWorks.allBases->self().natural.bases[i]->base->getPosition();
					BWAPI::Broodwar->drawTextMap(position.x(), position.y()+16, "%d (%.3f)\n\x0F%d\x02\nN.G:%.0f",i,publicWorks.allBases->self().natural.potential[i],publicWorks.allBases->self().natural.bases[i]->patches.size(),publicWorks.allBases->self().natural.ground[i]);
					switch(publicWorks.allBases->self().natural.bases[i]->owner) {
						case Infrastructure::None:
							color = BWAPI::Colors::Brown;
							break;
						case Infrastructure::Mine:
							color = BWAPI::Colors::Blue;
							break;
						case Infrastructure::His:
							color = BWAPI::Colors::Red;
							break;
					}
					BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),70,color);
					if (publicWorks.allBases->self().natural.bases[i]->base->isStartLocation()) {
						BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Green);
						BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),72,BWAPI::Colors::Green);
					} else if (publicWorks.allBases->enemyIndex < publicWorks.allBases->startLocations.size())
						if (publicWorks.allBases->self().natural.bases[i] == publicWorks.allBases->enemy().natural.info) {
							BWAPI::Broodwar->drawCircleMap(position.x(),position.y(),68,BWAPI::Colors::Yellow);
						}
					
					BWAPI::Broodwar->setTextSize(1);
					std::set<Choke> chokes = publicWorks.allBases->self().natural.bases[i]->base->getRegion()->getChokepoints();
					for (std::set<Choke>::const_iterator choke = chokes.begin(); choke != chokes.end(); choke++) {
						BWAPI::Broodwar->drawLineMap((*choke)->getSides().first.x(), (*choke)->getSides().first.y(), (*choke)->getSides().second.x(),(*choke)->getSides().second.y(), BWAPI::Colors::Grey);
						BWAPI::Broodwar->drawTextMap((*choke)->getCenter().x(), (*choke)->getCenter().y(), "\x1D%.0f",(*choke)->getWidth());
						//BWAPI::Broodwar->drawCircleMap((*choke)->getCenter().x(), (*choke)->getCenter().y(), (*choke)->getWidth()/2, BWAPI::Colors::Grey);
					}
				}
				BWAPI::Broodwar->setTextSize(0);
				if (publicWorks.allBases->enemyKnown())
					for (i = 0; i < publicWorks.allBases->enemy().natural.bases.size(); i++) {
						position = publicWorks.allBases->enemy().natural.bases[i]->base->getPosition();
						BWAPI::Broodwar->drawTextMap(position.x(), position.y()+49, "E.G:%.0f",publicWorks.allBases->enemy().natural.ground[i]);
					}
			}
			#pragma endregion
			
			#pragma region Resources
			for (i = 0; i < publicWorks.hatcheries.size(); i++) {
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
							case Resources::Idle:
								state = "\x05Idle";
								color = BWAPI::Colors::Grey;
								break;
							case Resources::Waiting:
								state = "\x16Wait";
								color = BWAPI::Colors::White;
								break;
							case Resources::Mining:
								state = "\x1FMine";
								color = BWAPI::Colors::Cyan;
								break;
							case Resources::Returning:
								state = "\x0FReturn";
								color = BWAPI::Colors::Teal;
								break;
						}
						BWAPI::Broodwar->drawLineMap(drone.x(), drone.y(), mineral.x(), mineral.y(), color);
						BWAPI::Broodwar->drawTextMap(drone.x()-8, drone.y()-4, "%s", state.c_str());
					}
					BWAPI::Broodwar->setTextSize(2);
					BWAPI::Broodwar->drawTextMap(mineral.x()-17, mineral.y()-16, "\x1F%d", resources);
					BWAPI::Broodwar->setTextSize(1);
					BWAPI::Broodwar->drawTextMap(mineral.x()-3, mineral.y()-4, "\x1B%d", mines.minerals[i].miners[j].size());
				}
				BWAPI::Broodwar->setTextSize(2);
				BWAPI::Broodwar->drawTextMap(hatch.x()-28, hatch.y()-30, "\x11Miners: %d", mines.minerals[i].getMiners().size());
			}
			#pragma endregion
			
			#pragma region Text
			BWAPI::Broodwar->setTextSize(2);
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 0, "Income: %.2f min (%.3f per drone)\t%.2f gas\t[%d]", finances.income[0].mineral, finances.income[0].mineral/allUnits.getType(Drone).size(), finances.income[0].gas, finances.income[0].frame);
			BWAPI::Broodwar->drawTextScreen(5, 10, "HATCHERIES: %d, %d", publicWorks.hatcheries.size(), mines.minerals.size());
			BWAPI::Broodwar->drawTextScreen(5, 20, "Need for a) drones: %.2f (%+.0f)\tb) overlords: %.2f", industry.drones, (0.25/(finances.income[0].mineral/publicWorks.hatcheries.size()) - 1)*allUnits.getType(Drone).size(), industry.overlords);
			BWAPI::Broodwar->drawTextScreen(5, 30, "Building a) drones: %d [%d]\tb) overlords: %d [%d]",
				allUnits.eggsMorphing(Drone),allUnits.notCompleted(Drone),
				allUnits.eggsMorphing(Overlord),allUnits.notCompleted(Overlord));
			
			std::ostringstream units_decl;
			units_decl << "UNITS = ";
			std::vector<BWAPI::UnitType> alreadyDone;
			BWAPI::UnitType type;
			type = allUnits[0]->getType();
			units_decl << "\x04";
			units_decl << &type.c_str()[5];
			units_decl << ":\x02";
			units_decl << allUnits.getType(type).size();
			alreadyDone.push_back(type);
			
			for (i = 1; i < allUnits.size(); i++) {
				type = allUnits[i]->getType();
				if (!has(alreadyDone, type)) {
					units_decl << ", \x04";
					units_decl << &type.c_str()[5];
					units_decl << ":\x02";
					units_decl << allUnits.getType(type).size();
					alreadyDone.push_back(type);
				}
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
				BWAPI::Broodwar->drawLineScreen(200-i,300-(int)(finances.income[i].droneCount > 0 ? ((10000/42)*finances.income[i].mineral/finances.income[i].droneCount) : 0),
												200-i-1,300-(int)(finances.income[i+1].droneCount > 0 ? ((10000/42)*finances.income[i+1].mineral/finances.income[i+1].droneCount) : 0), BWAPI::Colors::Teal);
				BWAPI::Broodwar->drawLineScreen(200-i,300-(int)((10000/42)*finances.income[i].gas),
												200-i-1,300-(int)((10000/42)*finances.income[i+1].gas),BWAPI::Colors::Green);
			}
			#pragma endregion
			
		}

		
		void newHatchery(Unit hatch) {
			publicWorks.addHatch(hatch);
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
		void morphUnit(Unit unit) {
			if (unit->getPlayer() == player)
				if (unit->getType().isBuilding()) {
					allUnits.remove(unit);
					if (unit->getType().isResourceDepot())
						newHatchery(unit);
				}
		}
	
		static DWORD WINAPI analyzeThread(LPVOID pointer) {
			BWTA::analyze();
			Cerebrate* target = (Cerebrate*) pointer;

			Base start = BWTA::getStartLocation(BWAPI::Broodwar->self());
			if (start != 0) {
				Infrastructure::BaseGraph* graph = new Infrastructure::BaseGraph();
				graph->populate();
				for (unsigned i = 0; i < graph->startLocations.size(); i++)
					if (graph->startLocations[i].info->base == start) {
						graph->selfIndex = i;
						break;
					}
				if (graph->startLocations.size() == 2) {
					graph->enemyIndex = graph->selfIndex ? 0 : 1;
					graph->enemy().info->owner = Infrastructure::His;
				}
				target->publicWorks.allBases = graph;
				
				for (unsigned i = 0; i < target->publicWorks.hatcheries.size(); i++)
					target->publicWorks.allBases->expanded(target->publicWorks.hatcheries[i]->getPosition());
			}
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
		virtual void onSendText(std::string text) {
			/*if (!text.compare("expand")) {
				Base next = _data.publicWorks.bases[0].base;
				Unit myDrone = _data.mines.getDrone(next->getPosition());
				myDrone->build(next->getTilePosition(), Hatch);
			} else*/ if (!text.compare("drone")) {
				_data.makeDrones = !_data.makeDrones;
			}
		}
		virtual void onReceiveText(BWAPI::Player* player, std::string text) { }
		virtual void onPlayerLeft(BWAPI::Player* player) { }
		virtual void onNukeDetect(BWAPI::Position target) { }
		virtual void onUnitDiscover(BWAPI::Unit* unit) {
			if (unit->getPlayer() == BWAPI::Broodwar->enemy()) {
				if (unit->getType().isResourceDepot())
					_data.publicWorks.allBases->enemySighted(unit->getPosition());
			}
		}
		virtual void onUnitEvade(BWAPI::Unit* unit) { }
		virtual void onUnitShow(BWAPI::Unit* unit) { }
		virtual void onUnitHide(BWAPI::Unit* unit) { }
		virtual void onUnitCreate(BWAPI::Unit* unit) { }
		virtual void onUnitDestroy(BWAPI::Unit* unit) {
			_data.deleteUnit(unit);
		}
		virtual void onUnitMorph(BWAPI::Unit* unit) {
			_data.morphUnit(unit);
		}
		virtual void onUnitRenegade(BWAPI::Unit* unit) { }
		virtual void onSaveGame(std::string gameName) { }
		virtual void onUnitComplete(BWAPI::Unit *unit) {
			_data.newUnit(unit);
		}
		// Everything below this line is safe to modify.

	};
}