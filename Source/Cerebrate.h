#pragma once

#include <string>
#include <cstdlib>
#ifdef SAVE_CSV
#include <fstream>
#endif

#include "Production.h"
#include "Intelligence.h"

namespace Cerebrate {
	bool isntValid(Unit u) {
		return !u->exists() || u->isLockedDown() || u->isMaelstrommed() || u->isStasised() ||
				u->isLoaded() || u->isUnpowered() || u->isStuck() || !u->isCompleted() ||
				u->isConstructing() || u->getType().isBuilding();
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

		bool debug;
		#ifdef SAVE_CSV
		std::ofstream file;
		#endif

		Cerebrate(bool debug = true) : debug(debug),stage(Action),passivity(0.7) { }
		~Cerebrate() {
			#ifdef SAVE_CSV
			file.close();
			#endif
		}
		
		std::set<Unit> allUnits() const {
			return BWAPI::Broodwar->self()->getUnits();
		}
		typedef std::set<Unit>::iterator unit_it;
		
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
			#pragma endregion

			#pragma region Set main hatch
			Unit hatch = 0;
			std::set<Unit> units = allUnits();
			for (unit_it it = units.begin(); it != units.end(); it++)
				if ((*it)->getType() == Hatch) {
					hatch = *it;
					break;
				}
			
			industry.hatcheries.push_back(hatch);
			publicWorks.addHatch(hatch,intel);
			if (intel.graph)
				intel.graph->expanded(hatch->getPosition());
			mines.add(hatch);
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

			if (intel.graph) {				
				if (intel.graph->enemyKnown()) {
					unsigned index = intel.graph->enemyIndex - (intel.graph->selfIndex > intel.graph->enemyIndex ? 0 : 1);

					airdistance = intel.graph->self().air[index];
					grounddistance = intel.graph->self().ground[index];
					
					naturaldist = intel.graph->self().natural.info->base->getGroundDistance(intel.graph->enemy().natural.info->base);
				} else {
					for (i = 0; i < intel.graph->self().ground.size(); i++){
						airdistance += intel.graph->self().air[i];
						grounddistance += intel.graph->self().ground[i];
					}
					airdistance /= i;
					grounddistance /= i;

					for(i = 0; i < intel.graph->startLocations.size(); i++)
						if (i != intel.graph->selfIndex)
							naturaldist += intel.graph->self().natural.info->base->getGroundDistance(intel.graph->startLocations[i].natural.info->base);
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
			intel.update();
			publicWorks.update(industry,intel);
			mines.update();
			
			industry.update(0.3,publicWorks,mines,finances,intel);
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

			publicWorks.act(finances,industry,mines,intel);
			mines.act();
			
			std::set<Unit> units = allUnits();
			for (unit_it u = units.begin(); u != units.end(); u++) {
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
			
			intel.draw();
			publicWorks.draw(intel);
			mines.draw();
			

			#pragma region Text
			BWAPI::Broodwar->setTextSize(2);
			BWAPI::Broodwar->drawTextScreen(300, 0, "FPS: %0d", BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->setTextSize(0);
			BWAPI::Broodwar->drawTextScreen(305, 16, "APM: %d", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(5, 10, "HATCHERIES: %d, %d", publicWorks.hatcheries.size(), mines.minerals.size());

			BWAPI::Broodwar->drawTextScreen(550, 50, "\x07 choke: %2.0f", Infrastructure::PotentialField::wchoke);
			BWAPI::Broodwar->drawTextScreen(550, 60, "\x07 poly:  %2.0f", Infrastructure::PotentialField::wpoly);
			BWAPI::Broodwar->drawTextScreen(550, 70, "\x07 patch: %2.0f", Infrastructure::PotentialField::wpatch);
			
			BWAPI::Broodwar->drawTextScreen(400, 50, "\x07 5pool:   %.3f %.3f = %.3f", strategies[0][0],strategies[0][1],strategies[0][2]);
			BWAPI::Broodwar->drawTextScreen(400, 60, "\x07 9speed:  %.3f %.3f = %.3f", strategies[1][0],strategies[1][1],strategies[1][2]);
			BWAPI::Broodwar->drawTextScreen(400, 70, "\x07 9pool:   %.3f %.3f = %.3f", strategies[2][0],strategies[2][1],strategies[2][2]);
			BWAPI::Broodwar->drawTextScreen(400, 80, "\x07 12pool:  %.3f %.3f = %.3f", strategies[3][0],strategies[3][1],strategies[3][2]);
			BWAPI::Broodwar->drawTextScreen(400, 90, "\x07 12hatch: %.3f %.3f = %.3f", strategies[4][0],strategies[4][1],strategies[4][2]);

			BWAPI::Broodwar->drawTextScreen(5, 40, "Budgets: %d", finances.budgets.size());

			if (publicWorks.hatcheries.size()) {
				BWAPI::Broodwar->setTextSize(1);
				BWAPI::Broodwar->drawTextScreen(5, 60, "Queue:");
				BWAPI::Broodwar->setTextSize(0);
				for (i = 0; i < industry.queue.size(); i++)
					BWAPI::Broodwar->drawTextScreen(5,70+10*i,"\x04%s:%.3f",industry.queue[i].name().c_str(),industry.queue[i].priority);
			}
			#pragma endregion
			
			#pragma region Heat
			/*if (heat) {
				BWAPI::TilePosition tile(base->getTilePosition());

				#ifdef SAVE_IMG
				std::ofstream image;
				image.open("heat.ppm");
				image << "P3\n21 21\n255\n";
				#endif

				for (int j = -10; j < 11; j++)
					 for (int i = -10; i < 11; i++) {
						BWAPI::TilePosition k(tile.x()+i,tile.y()+j);

						unsigned walk = walkTiles(k);

						int strength = 255*(tileValue(k)+40)/40;
						strength = strength > 255 ? 255 : strength;
						strength = strength < 0 ? 0 : strength;

						int wstr = walk ? 64 + 8*walk : 0;
						if (walk == 16) wstr = 255;

						#ifdef SAVE_IMG
						if (k == tile)
							image << strength << " 255 " << wstr;
						else
							image << strength << " 0 " << wstr;

						if (j == 10)
							image << "\r\n";
						else
							image << "\t";
						#endif


						BWAPI::Broodwar->drawCircleMap(BWAPI::Position(k).x()+16,BWAPI::Position(k).y()+16, 8, BWAPI::Color(strength,0,wstr), true);
						BWAPI::Broodwar->drawTextMap(BWAPI::Position(k).x()+16,BWAPI::Position(k).y()+16, "%.2f", tileValue(k));
					}

				#ifdef SAVE_IMG
				image.close();
				#endif

				std::vector<BWAPI::TilePosition> path;
				bool validAdjacents = true;
				while (validAdjacents) {
					std::vector<BWAPI::TilePosition> adjacents;
					adjacents.push_back(BWAPI::TilePosition(tile.x(), tile.y()+1));
					adjacents.push_back(BWAPI::TilePosition(tile.x(), tile.y()-1));
					adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()));
					adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()));

					adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()+1));
					adjacents.push_back(BWAPI::TilePosition(tile.x()+1, tile.y()-1));
					adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()+1));
					adjacents.push_back(BWAPI::TilePosition(tile.x()-1, tile.y()-1));

					for (unsigned i = 0; i < 8; i++)
						if (!BWAPI::Broodwar->isBuildable(adjacents[i]) && walkTiles(adjacents[i]) < 16) {
							validAdjacents = false;
							break;
						}
					if (!validAdjacents)
						break;

					double value = -100;
					unsigned index = -1;
					for (unsigned i = 0; i < 8; i++)
						if (!has(path,adjacents[i]))
							if (tileValue(adjacents[i]) > value) {
								value = tileValue(adjacents[i]);
								index = i;
							}

					if (index == -1)
						break;

					tile.y() = adjacents[index].y();
					tile.x() = adjacents[index].x();
					path.push_back(tile);
				}

				for(unsigned i = 0; i < path.size(); i++) {
					//int strength = 255*(tileValue(path[i])+15)/20;
					//strength = strength > 255 ? 255 : strength;
					//strength = strength < 0 ? 0 : strength;
					//BWAPI::Broodwar->drawCircleMap(BWAPI::Position(path[i]).x()+16,BWAPI::Position(path[i]).y()+16, 16, BWAPI::Color(0,0,strength), true);
					BWAPI::Broodwar->drawTextMap(BWAPI::Position(path[i]).x()+16,BWAPI::Position(path[i]).y()+16, "%.2f", tileValue(path[i]));
					BWAPI::Broodwar->drawTextMap(BWAPI::Position(path[i]).x(),BWAPI::Position(path[i]).y(), "\x1F%d", i);
				}

				BWAPI::Broodwar->drawLineMap(BWAPI::Position(base->getTilePosition()).x(),BWAPI::Position(base->getTilePosition()).y(),BWAPI::Position(path[path.size()-1]).x(),BWAPI::Position(path[path.size()-1]).y(),BWAPI::Colors::Green);

			}*/
			#pragma endregion
		}

		static DWORD WINAPI analyzeThread(LPVOID pointer) {
			BWTA::analyze();
			Cerebrate* target = (Cerebrate*) pointer;

			Base start = BWTA::getStartLocation(BWAPI::Broodwar->self());
			if (start != 0) {
				Intelligence::BaseGraph* graph = new Intelligence::BaseGraph();
				graph->populate();
				for (unsigned i = 0; i < graph->startLocations.size(); i++) {
					if (graph->startLocations[i].info->base == start) {
						graph->selfIndex = i;
						break;
					}
					graph->startLocations[i].info->owner = Intelligence::His;
				}
				target->intel.graph = graph;

				for (unsigned i = 0; i < target->publicWorks.hatcheries.size(); i++)
					target->intel.graph->expanded(graph->main()->getPosition());

				target->publicWorks.update(target->industry,target->intel);
				target->setOpening();
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
				Infrastructure::PotentialField::wpatch = atof(&text.c_str()[6]);
			else if (!text.compare(0,6,"choke "))
				Infrastructure::PotentialField::wchoke = atof(&text.c_str()[6]);
			else if (!text.compare(0,5,"poly "))
				Infrastructure::PotentialField::wpoly = atof(&text.c_str()[5]);
		}
		virtual void onReceiveText(BWAPI::Player* player, std::string text) { }
		virtual void onPlayerLeft(BWAPI::Player* player) { }
		virtual void onNukeDetect(BWAPI::Position target) { }
		virtual void onUnitDiscover(BWAPI::Unit* unit) {
			if (unit->getPlayer() == BWAPI::Broodwar->enemy()) {
				if (unit->getType().isResourceDepot())
					_data.intel.graph->enemySighted(unit->getPosition());
			}
		}
		virtual void onUnitEvade(BWAPI::Unit* unit) { }
		virtual void onUnitShow(BWAPI::Unit* unit) { }
		virtual void onUnitHide(BWAPI::Unit* unit) { }
		virtual void onUnitCreate(BWAPI::Unit* unit) { }
		virtual void onUnitDestroy(BWAPI::Unit* unit) { }
		virtual void onUnitMorph(BWAPI::Unit* unit) { }
		virtual void onUnitRenegade(BWAPI::Unit* unit) { }
		virtual void onSaveGame(std::string gameName) { }
		virtual void onUnitComplete(BWAPI::Unit *unit) { }
		// Everything below this line is safe to modify.

	};
}