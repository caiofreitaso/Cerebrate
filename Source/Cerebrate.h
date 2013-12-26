#pragma once

#include <BWAPI/Client.h>
#include <string>
#include <cstdlib>
#ifdef SAVE_CSV
#include <fstream>
#endif

#include "Economy.h"
#include "Resources.h"
#include "Intelligence.h"

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
	//Need for unit
	namespace Industry {
		#pragma region Function
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
				/*if (unitQueue.size())
					for (ProductionQueue::iterator i = unitQueue.begin(); i != unitQueue.end();) {
						if (i->priority < priorityThreshold || i->type == Drone || i->type == Overlord) {
							unitQueue.erase(i);
							i = unitQueue.begin();
						} else
							i++;
					}
				*/
			}
			void queueCivil(double droneThreshold, double overlordThreshold) {
				//if (drones >= droneThreshold)
				//	unitQueue.push_back(Production(Drone,drones));
				//if (overlords >= overlordThreshold)
				//	unitQueue.push_back(Production(Overlord,overlords));
				//std::sort(unitQueue.begin(), unitQueue.end(), compare);
			}

			void morph(Infrastructure::Builder& builder) {
				Unitset larvae = builder.getLarva();

				unsigned size = larvae.size();
				size = size < unitQueue.size() ? size : unitQueue.size();

				for (Unitset::iterator larva = larvae.begin(); larva != larvae.end() && unitQueue.size(); larva++)
					if ((*larva)->morph(unitQueue[0].type)) {
						unitQueue.erase(unitQueue.begin());
						break;
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

	struct Cerebrate {
		Player player;

		UnitManager allUnits;

		Resources::Miner			mines;
		Economy::Economist			finances;
		Industry::Manager			industry;
		Infrastructure::Builder		publicWorks;
		Intelligence::Agent			intel;
		//Technology::Scientist		science;
		//Defense::General			defense;

		enum Stage {
			Action,
			Reaction
		};

		Stage stage;
		double passivity;

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

		Cerebrate(bool debug = true) : player(0),debug(debug),makeDrones(true),stage(Action),passivity(rand()/RAND_MAX) { }
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

			#pragma region Set Broodwar command
			BWAPI::Broodwar->enableFlag(BWAPI::Flag::UserInput);
			BWAPI::Broodwar->setCommandOptimizationLevel(2);
			BWAPI::Broodwar->setLocalSpeed(10);
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

		void setOpening() {
			unsigned i = 0;
			
			double strategies[5][2];
			double airdistance = 0, grounddistance = 0, naturaldist = 0;

			if (intel.bases) {				
				if (intel.bases->enemyKnown()) {
					unsigned index = intel.bases->enemyIndex - (intel.bases->selfIndex > intel.bases->enemyIndex ? 0 : 1);

					airdistance = intel.bases->self().air[index];
					grounddistance = intel.bases->self().ground[index];
					
					naturaldist = intel.bases->self().natural.info->base->getGroundDistance(intel.bases->enemy().natural.info->base);
				} else {
					for (i = 0; i < intel.bases->self().ground.size(); i++){
						airdistance += intel.bases->self().air[i];
						grounddistance += intel.bases->self().ground[i];
					}
					airdistance /= i;
					grounddistance /= i;

					for(i = 0; i < intel.bases->startLocations.size(); i++)
						if (i != intel.bases->selfIndex)
							naturaldist += intel.bases->self().natural.info->base->getGroundDistance(intel.bases->startLocations[i].natural.info->base);
					naturaldist /= i-1;
				}
			}

			airdistance		/= 1000;
			grounddistance	/= 1000;
			naturaldist		/= 1000;

			//Distance values

			strategies[0][0] = 1 - (tanh(2*(grounddistance-4.8)) + 1)/2;
			strategies[0][0] *= strategies[0][0];

			strategies[1][0] = 1 - (tanh(0.75*(grounddistance-5)) + 1)/2;

			strategies[2][0] = naturaldist - 3.25;
			strategies[2][0] *= strategies[2][0];
			strategies[2][0] = exp(-strategies[2][0]/0.16);

			strategies[3][0] = (tanh(1.5*(naturaldist-3.3)) + 1)/2;

			strategies[4][0] = 1 - (tanh(2.25*(airdistance-2.3)) + 1)/2;

			//Passiveness valuess
			
			strategies[0][1] = exp(-(passivity-0.2)*(passivity-0.2)/0.08);
			strategies[1][1] = exp(-(passivity-0.3)*(passivity-0.3)/0.12);
			strategies[2][1] = exp(-(passivity-0.43)*(passivity-0.43)/0.06);
			strategies[3][1] = (tanh(3*passivity-0.6)+1)/2;
			strategies[4][1] = 1-(tanh(5*passivity-2.5)+1)/2;
			strategies[4][1] *= strategies[4][1];

			for (i = 0; i < 5; i++)
				strategies[i][0] = strategies[i][0]+strategies[i][1]-strategies[i][0]*strategies[i][1];

			int maior = 0;
			int segundomaior = 0;
			for (i = 1; i < 5; i++)
				if(strategies[i][0] > strategies[maior][0])
					maior = i;
			if (maior == 0)
				segundomaior = 1;
			for (i = 1; i < 5; i++)
				if (i != maior && strategies[i][0] > strategies[segundomaior][0])
					segundomaior = i;

			double total = strategies[maior][0] + strategies[segundomaior][0];
			double chances[5];
			for(i = 0; i < 5; i++)
				if (i == maior)
					chances[i] = strategies[maior][0]*100/total;
				else if (i == segundomaior)
					chances[i] = strategies[segundomaior][0]*100/total;
				else
					chances[i] = 0;
			
			double random = rand()/RAND_MAX;
			int found = -1;
			for(i = 0; i < 5; i++) {
				if (!chances[i])
					continue;
				if (found == -1) {
					found = i;
					if (random <= chances[i])
						break;
				} else {
					found = i;
					break;
				}
			}
			switch(found) {
				case 0:
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Pool,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Drone,1));
					for (i = 0; i < 3; i++)
						industry.unitQueue.push_back(Production(Ling,1));
					break;
				case 1:
					for (i = 0; i < 5; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Pool,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					break;
				case 2:
					for (i = 0; i < 5; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					for (i = 0; i < 4; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Pool,1));
					break;
				case 3:
					for (i = 0; i < 5; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					for (i = 0; i < 4; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Hatch,1));
					break;
				case 4:
					for (i = 0; i < 5; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					for (i = 0; i < 5; i++)
						industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Pool,1));
					industry.unitQueue.push_back(Production(Extractor,1));
					industry.unitQueue.push_back(Production(Lair,1));
					industry.unitQueue.push_back(Production(Ling,1));
					industry.unitQueue.push_back(Production(Ling,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Drone,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					industry.unitQueue.push_back(Production(Overlord,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					industry.unitQueue.push_back(Production(Muta,1));
					break;
			}
		}

		void update() {
			finances.update(player, BWAPI::Broodwar->getFrameCount(), mines.getAllMiners().size());
			#ifdef SAVE_CSV
			file << player->gatheredMinerals() - 50 << "," << BWAPI::Broodwar->getAPM() << "\n";
			#endif
			mines.update();
			industry.needForDrones(publicWorks, finances);
			industry.needForOverlords(publicWorks, allUnits, player);
			if (intel.bases)
				intel.bases->update();
		}
		void act() {
			if (BWAPI::Broodwar->isReplay() || !BWAPI::Broodwar->getFrameCount())
				return;

			if (BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self())
				return;

			if (debug)
				draw();

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

			#pragma region Base
			BWAPI::Broodwar->setTextSize(0);
			intel.draw();
			#pragma endregion

			#pragma region Resource
			mines.draw();
			#pragma endregion

			#pragma region Text
			BWAPI::Broodwar->setTextSize(2);
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 0, "Income: %.2f min (%.3f per drone)\t%.2f gas\t[%d]", finances.income[0].mineral, finances.income[0].mineral/allUnits.getType(Drone).size(), finances.income[0].gas, BWAPI::Broodwar->getFrameCount());
			BWAPI::Broodwar->drawTextScreen(5, 10, "HATCHERIES: %d, %d", publicWorks.hatcheries.size(), mines.minerals.size());
			BWAPI::Broodwar->drawTextScreen(5, 20, "Need for a) drones: %.2f (%+.0f)\tb) overlords: %.2f", industry.drones, (0.25/(finances.income[0].mineral/publicWorks.hatcheries.size()) - 1)*allUnits.getType(Drone).size(), industry.overlords);
			BWAPI::Broodwar->drawTextScreen(5, 30, "Building a) drones: %d [%d]\tb) overlords: %d [%d]",
				allUnits.eggsMorphing(Drone),allUnits.notCompleted(Drone),
				allUnits.eggsMorphing(Overlord),allUnits.notCompleted(Overlord));

			BWAPI::Broodwar->drawTextScreen(550, 50, "\x07 choke: %2.0f", Intelligence::BaseInfo::wchoke);
			BWAPI::Broodwar->drawTextScreen(550, 66, "\x07 poly:  %2.0f", Intelligence::BaseInfo::wpoly);
			BWAPI::Broodwar->drawTextScreen(550, 82, "\x07 patch: %2.0f", Intelligence::BaseInfo::wpatch);

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
			finances.draw();
			#pragma endregion

			
		}


		void newHatchery(Unit hatch) {
			publicWorks.addHatch(hatch);
			if (intel.bases)
				intel.bases->expanded(hatch->getPosition());
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
				Intelligence::BaseGraph* graph = new Intelligence::BaseGraph();
				graph->populate();
				for (unsigned i = 0; i < graph->startLocations.size(); i++)
					if (graph->startLocations[i].info->base == start) {
						graph->selfIndex = i;
						break;
					}
				if (graph->startLocations.size() == 2) {
					graph->enemyIndex = graph->selfIndex ? 0 : 1;
					graph->enemy().info->owner = Intelligence::His;
				}
				target->intel.bases = graph;

				for (unsigned i = 0; i < target->publicWorks.hatcheries.size(); i++)
					target->intel.bases->expanded(target->publicWorks.hatcheries[i]->getPosition());

				target->setOpening();
				/*Intelligence::BaseInfo::map = new Intelligence::Map(BWAPI::Broodwar->mapWidth(), BWAPI::Broodwar->mapHeight());
				for (unsigned i = 0; i < Intelligence::BaseInfo::map->x; i++)
					for (unsigned j = 0; j < Intelligence::BaseInfo::map->y; j++)
						Intelligence::BaseInfo::map->data[i][j] = */
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
			if (!text.compare("expand")) {
				BWAPI::TilePosition position = _data.intel.nextBase();
				Unit myDrone = _data.mines.getDrone(BWAPI::Position(position));
				myDrone->build(position, Hatch);
			} else if (!text.compare("drone")) {
				_data.makeDrones = !_data.makeDrones;
			} else if (!text.compare(0,6,"patch "))
				Intelligence::BaseInfo::wpatch = atof(&text.c_str()[6]);
			else if (!text.compare(0,6,"choke "))
				Intelligence::BaseInfo::wchoke = atof(&text.c_str()[6]);
			else if (!text.compare(0,5,"poly "))
				Intelligence::BaseInfo::wpoly = atof(&text.c_str()[5]);
		}
		virtual void onReceiveText(BWAPI::Player* player, std::string text) { }
		virtual void onPlayerLeft(BWAPI::Player* player) { }
		virtual void onNukeDetect(BWAPI::Position target) { }
		virtual void onUnitDiscover(BWAPI::Unit* unit) {
			if (unit->getPlayer() == BWAPI::Broodwar->enemy()) {
				if (unit->getType().isResourceDepot())
					_data.intel.bases->enemySighted(unit->getPosition());
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