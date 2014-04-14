#pragma once

#include <string>
#include <cstdlib>
#ifdef SAVE_CSV
#include <fstream>
#endif

#include "Production.h"
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
		double strategies[5][3];

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

		Cerebrate(bool debug = true) : player(0),debug(debug),makeDrones(true),stage(Action),passivity(0.7) { }
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
			
			double airdistance = 0, grounddistance = 0, naturaldist = 0;

			if (publicWorks.bases) {				
				if (publicWorks.bases->enemyKnown()) {
					unsigned index = publicWorks.bases->enemyIndex - (publicWorks.bases->selfIndex > publicWorks.bases->enemyIndex ? 0 : 1);

					airdistance = publicWorks.bases->self().air[index];
					grounddistance = publicWorks.bases->self().ground[index];
					
					naturaldist = publicWorks.bases->self().natural.info->base->getGroundDistance(publicWorks.bases->enemy().natural.info->base);
				} else {
					for (i = 0; i < publicWorks.bases->self().ground.size(); i++){
						airdistance += publicWorks.bases->self().air[i];
						grounddistance += publicWorks.bases->self().ground[i];
					}
					airdistance /= i;
					grounddistance /= i;

					for(i = 0; i < publicWorks.bases->startLocations.size(); i++)
						if (i != publicWorks.bases->selfIndex)
							naturaldist += publicWorks.bases->self().natural.info->base->getGroundDistance(publicWorks.bases->startLocations[i].natural.info->base);
					naturaldist /= i-1;
				}
			}

			airdistance		/= 1000;
			grounddistance	/= 1000;
			naturaldist		/= 1000;

			//Distance values

			strategies[0][0] = 1 - (tanh(2*(grounddistance-5)) + 1)/2;
			strategies[0][0] *= strategies[0][0];

			strategies[1][0] = grounddistance - 4.5;
			strategies[1][0] *= strategies[1][0];
			strategies[1][0] = exp(-strategies[1][0]/0.32);

			strategies[2][0] = grounddistance - 5;
			strategies[2][0] *= strategies[2][0];
			strategies[2][0] = exp(-strategies[2][0]/0.8);

			strategies[3][0] = grounddistance - 5.5;
			strategies[3][0] *= strategies[3][0];
			strategies[3][0] = exp(-strategies[3][0]/0.64);

			strategies[4][0] = (tanh(grounddistance-4.5) + 1)/2;

			//Passiveness values
			
			strategies[0][1] = 1-(tanh(5*passivity-2.5)+1)/2;
			strategies[0][1] *= strategies[0][1];
			
			strategies[1][1] = passivity-0.2;
			strategies[1][1] *= strategies[1][1];
			strategies[1][1] = exp(-strategies[1][1]/0.08);
			
			strategies[2][1] = passivity-0.3;
			strategies[2][1] *= strategies[2][1];
			strategies[2][1] = exp(-strategies[2][1]/0.12);
			
			strategies[3][1] = passivity-0.5;
			strategies[3][1] *= strategies[3][1];
			strategies[3][1] = exp(-strategies[3][1]/0.06);
			
			strategies[4][1] = (tanh(2*(passivity-0.1)) + 1)/2;
			

			for (i = 0; i < 5; i++)
				strategies[i][2] = strategies[i][0]+strategies[i][1]-strategies[i][0]*strategies[i][1];

			int maior = 0;
			int segundomaior = 0;
			for (i = 1; i < 5; i++)
				if(strategies[i][2] > strategies[maior][2])
					maior = i;
			if (maior == 0)
				segundomaior = 1;
			for (i = 1; i < 5; i++)
				if (i != maior && strategies[i][2] > strategies[segundomaior][2])
					segundomaior = i;

			double total = strategies[maior][2] + strategies[segundomaior][2];
			double chances[5];
			for(i = 0; i < 5; i++)
				if (i == maior)
					chances[i] = strategies[maior][0]*100/total;
				else if (i == segundomaior)
					chances[i] = strategies[segundomaior][0]*100/total;
				else
					chances[i] = 0;
			
			srand((unsigned)this);
			double random = rand() % 100;
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
				//5 pool
				case 0:
					industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Pool));
					industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Drone));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Ling));
					break;
				//9 speed
				case 1:
					for (i = 0; i < 5; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Pool));
					industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Overlord));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Ling));
					break;
				//9 pool
				case 2:
					for (i = 0; i < 5; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Pool));
					industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Overlord));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Ling));
					break;
				//12 pool
				case 3:
					for (i = 0; i < 5; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Overlord));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Pool));
					industry.add(Industry::Production(Hatch));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Ling));
					break;
				//12 hatch
				case 4:
					for (i = 0; i < 5; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Overlord));
					for (i = 0; i < 3; i++)
						industry.add(Industry::Production(Drone));
					industry.add(Industry::Production(Hatch));
					industry.add(Industry::Production(Pool));
					break;
			}
		}

		void update() {
			#ifdef SAVE_CSV
			file << player->gatheredMinerals() - 50 << "," << BWAPI::Broodwar->getAPM() << "\n";
			#endif
			publicWorks.update(industry);
			mines.update();			
			
			if (publicWorks.bases)
				publicWorks.bases->update();
			
			industry.update(0.3,publicWorks,mines,finances);
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

			publicWorks.act();
			mines.act();
			
			for (Unitset::iterator u = allUnits.units.begin(); u != allUnits.units.end(); u++) {
				if (isntValid(*u))
					continue;

				if ((*u)->isIdle()) {
					if ((*u)->getType().isWorker()) {
						if (!publicWorks.builders.in(*u))
							mines.idleWorker(*u);
					} else {
					}
				}
			}
		}
		void draw() {
			unsigned i = 0;

			BWAPI::Broodwar->setTextSize(0);
			
			publicWorks.draw();
			mines.draw();
			

			#pragma region Text
			BWAPI::Broodwar->setTextSize(2);
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 10, "HATCHERIES: %d, %d", publicWorks.hatcheries.size(), mines.minerals.size());

			BWAPI::Broodwar->drawTextScreen(550, 50, "\x07 choke: %2.0f", Infrastructure::BaseInfo::wchoke);
			BWAPI::Broodwar->drawTextScreen(550, 60, "\x07 poly:  %2.0f", Infrastructure::BaseInfo::wpoly);
			BWAPI::Broodwar->drawTextScreen(550, 70, "\x07 patch: %2.0f", Infrastructure::BaseInfo::wpatch);
			
			BWAPI::Broodwar->drawTextScreen(400, 50, "\x07 5pool:   %.3f %.3f = %.3f", strategies[0][0],strategies[0][1],strategies[0][2]);
			BWAPI::Broodwar->drawTextScreen(400, 60, "\x07 9speed:  %.3f %.3f = %.3f", strategies[1][0],strategies[1][1],strategies[1][2]);
			BWAPI::Broodwar->drawTextScreen(400, 70, "\x07 9pool:   %.3f %.3f = %.3f", strategies[2][0],strategies[2][1],strategies[2][2]);
			BWAPI::Broodwar->drawTextScreen(400, 80, "\x07 12pool:  %.3f %.3f = %.3f", strategies[3][0],strategies[3][1],strategies[3][2]);
			BWAPI::Broodwar->drawTextScreen(400, 90, "\x07 12hatch: %.3f %.3f = %.3f", strategies[4][0],strategies[4][1],strategies[4][2]);

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
				for (i = 0; i < industry.queue.size(); i++)
					BWAPI::Broodwar->drawTextScreen(5,65+10*i,"\x04%s:%.3f",industry.queue[i].name().c_str(),industry.queue[i].priority);
			}
			#pragma endregion
		}


		void newHatchery(Unit hatch) {
			publicWorks.addHatch(hatch);
			if (publicWorks.bases)
				publicWorks.bases->expanded(hatch->getPosition());
			mines.add(hatch);
		}
		void removeHatchery(Unit hatch) {
			unsigned index = 0;
			for(std::vector<Infrastructure::Hatchery>::iterator i = publicWorks.hatcheries.begin(); i != publicWorks.hatcheries.end(); i++, index++)
				if (i->hatch == hatch) {
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
				if (unit->getType().isBuilding())
					allUnits.remove(unit);
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
				target->publicWorks.bases = graph;

				for (unsigned i = 0; i < target->publicWorks.hatcheries.size(); i++)
					target->publicWorks.bases->expanded(target->publicWorks.hatcheries[i].hatch->getPosition());

				target->publicWorks.update(target->industry);
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
			if (!text.compare(0,6,"patch "))
				Infrastructure::BaseInfo::wpatch = atof(&text.c_str()[6]);
			else if (!text.compare(0,6,"choke "))
				Infrastructure::BaseInfo::wchoke = atof(&text.c_str()[6]);
			else if (!text.compare(0,5,"poly "))
				Infrastructure::BaseInfo::wpoly = atof(&text.c_str()[5]);
		}
		virtual void onReceiveText(BWAPI::Player* player, std::string text) { }
		virtual void onPlayerLeft(BWAPI::Player* player) { }
		virtual void onNukeDetect(BWAPI::Position target) { }
		virtual void onUnitDiscover(BWAPI::Unit* unit) {
			if (unit->getPlayer() == BWAPI::Broodwar->enemy()) {
				if (unit->getType().isResourceDepot())
					_data.publicWorks.bases->enemySighted(unit->getPosition());
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