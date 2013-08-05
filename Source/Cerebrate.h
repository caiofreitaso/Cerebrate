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
	typedef std::vector<Choke> Chokeset;


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
	const BWAPI::UnitType Guardian = BWAPI::UnitType(44);

	const BWAPI::UnitType Hatch = BWAPI::UnitType(131);
	const BWAPI::UnitType Lair = BWAPI::UnitType(132);
	const BWAPI::UnitType Hive = BWAPI::UnitType(133);

	const BWAPI::UnitType Extractor = BWAPI::UnitType(149);
	
	const BWAPI::UnitType Pool = BWAPI::UnitType(142);
	const BWAPI::UnitType HydraDen = BWAPI::UnitType(135);
	const BWAPI::UnitType Spire = BWAPI::UnitType(141);
	const BWAPI::UnitType QueensNest = BWAPI::UnitType(138);
	const BWAPI::UnitType DefilerMound = BWAPI::UnitType(136);
	const BWAPI::UnitType GSpire = BWAPI::UnitType(137);

	const BWAPI::UnitType EvoChamber = BWAPI::UnitType(139);

	const BWAPI::UnitType CreepC = BWAPI::UnitType(143);
	const BWAPI::UnitType Spore  = BWAPI::UnitType(144);
	const BWAPI::UnitType Sunken = BWAPI::UnitType(146);
	#pragma endregion

	namespace Types {
		enum Role {
			Civil,
			Massable,
			Harass,
			AntiAir,
			Siege,
			AOE,
			Air,
			Caster,
			Tank,

			Unknown
		};
		const Role Ling_Roles[2] = { Massable, Harass };
		const Role Hydra_Roles[2] = { Massable, AntiAir };
		const Role Lurker_Roles[2] = { Siege, AOE };
		const Role Muta_Roles[2] = { Air, Harass };
		const Role Scourge_Roles[2] = { Air, AntiAir };
		const Role Queen_Roles[2] = { Air, Caster };
		const Role Ultra_Roles[2] = { Tank, Unknown };
		const Role Defiler_Roles[2] = { AOE, Caster };
		const Role Guardian_Roles[2] = { Air, Siege };

		struct UnitType {
			BWAPI::UnitType type;
			Role roles[2];
			unsigned tier;

			UnitType(BWAPI::UnitType t, const Role r[], unsigned tr)
			: type(t),tier(tr) {
				roles[0] = r[0];
				roles[1] = r[1];
			}
		};
		
		const UnitType units[] = {
			UnitType(Ling,		Ling_Roles,		1),
			UnitType(Hydra,		Hydra_Roles,	1),
			UnitType(Lurker,	Lurker_Roles,	2),
			UnitType(Muta,		Muta_Roles,		2),
			UnitType(Scourge,	Scourge_Roles,	2),
			UnitType(Queen,		Queen_Roles,	2),
			UnitType(Ultra,		Ultra_Roles,	3),
			UnitType(Guardian,	Guardian_Roles,	3),
			UnitType(Defiler,	Defiler_Roles,	3)
		};
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

	
	namespace Micro {
		bool isntValid(Unit u) {
			return !u->exists() || u->isLockedDown() || u->isMaelstrommed() || u->isStasised() ||
					u->isLoaded() || u->isUnpowered() || u->isStuck() || !u->isCompleted() || u->isConstructing();
		}
	};

	namespace Macro {
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
		
		//Need for income
		namespace Economy
		{
			struct Resource {
				double mineral;
				double gas;
				
				int frame;
				unsigned droneCount;

				Resource() : mineral(0), gas(0), frame(0), droneCount(0) { }
			};
			struct Economist {
				static const int SIZE = 200;
				Resource states[SIZE];
				Resource income[SIZE];
				Resource incomeGrowth;

				void update(Player player, int frame, unsigned droneCount) {
					int i;
					if (frame - states[0].frame >= 20) {

						i = SIZE-1;
						for (; i > 0; i--) {
							states[i].mineral = states[i-1].mineral;
							states[i].gas = states[i-1].gas;
							states[i].frame = states[i-1].frame;
							states[i].droneCount = states[i-1].droneCount;
						}

						states[0].mineral = player->gatheredMinerals() - 50;
						states[0].gas = player->gatheredGas();
						states[0].frame = frame;
						states[0].droneCount = droneCount;

						for (i = SIZE-1; i > 0; i--) {
							income[i].mineral = income[i-1].mineral;
							income[i].gas = income[i-1].gas;
							income[i].frame = income[i-1].frame;
							income[i].droneCount = income[i-1].droneCount;
						}

						income[0] = Resource();
						for (i = 0; i < SIZE-1; i++)
							if (frame - states[i].frame > 500)
								break;
			
						income[0].mineral = (states[0].mineral - states[i].mineral) / (frame - states[i].frame);
						income[0].gas = (states[0].gas - states[i].gas) / (frame - states[i].frame);
						income[0].frame = frame;
						income[0].droneCount = droneCount;
			
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
			enum Ownership {
				None,
				Mine,
				His
			};
			
			struct BaseInfo {
				Ownership owner;
				std::vector< std::pair<Unit,int> > patches;
				int gas;
				Base base;
				int minerals() const {
					int r = 0;
					for (unsigned i = 0; i < patches.size(); i++)
						r += patches[i].second;
					return r;
				}
			};

			struct Location {
				BaseInfo* info;
				std::vector<BaseInfo*> bases;
				std::vector<double> ground;
				std::vector<double> potential;
				
				virtual void addBase(BaseInfo* b) {
					double distance = b->base->getGroundDistance(info->base);
					unsigned i = 0;
					for (; i < ground.size(); i++)
						if (ground[i] > distance)
							break;
					
					bases.insert(bases.begin()+i, b);
					ground.insert(ground.begin()+i, distance);
					potential.push_back(1);
				}
				
				bool compare(unsigned i, unsigned j) {
					if (bases[i]->owner != bases[j]->owner)
						if (bases[i]->owner == None)
							return true;
						else if (bases[j]->owner == None)
							return false;
						else if (bases[j]->owner == Mine)
							return true;
						else
							return false;
					else
						return potential[i] > potential[j];
				}
				
				virtual void sort() {
					for (unsigned i = 1; i < bases.size(); i++)
						for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
							std::swap(bases[j], bases[j-1]);
							std::swap(ground[j], ground[j-1]);
							std::swap(potential[j], potential[j-1]);
						}
				}
			};

			struct StartLocation : Location {
				std::vector<double> air;
				Location natural;
				
				virtual void addBase(BaseInfo* b) {
					double distance = b->base->getGroundDistance(info->base);
					unsigned i = 0;
					for (; i < ground.size(); i++)
						if (ground[i] > distance)
							break;
					
					bases.insert(bases.begin()+i, b);
					ground.insert(ground.begin()+i, distance);
					air.insert(air.begin()+i, b->base->getAirDistance(info->base));
					potential.push_back(1);					
				}
				
				virtual void sort() {
					for (unsigned i = 1; i < bases.size(); i++)
						for (unsigned j = i; j > 0 && compare(j,j-1); j--) {
							std::swap(bases[j], bases[j-1]);
							std::swap(ground[j], ground[j-1]);
							std::swap(potential[j], potential[j-1]);
							std::swap(air[j], air[j-1]);
						}
				}
			};

			struct BaseGraph {
				std::vector<BaseInfo> bases;
				std::vector<StartLocation> startLocations;
				unsigned selfIndex;
				unsigned enemyIndex;

				Base main() const { return startLocations[selfIndex].info->base; }
				StartLocation const& self() const { return startLocations[selfIndex]; }
				Base enemyMain() const { return startLocations[enemyIndex].info->base; }
				StartLocation const& enemy() const { return startLocations[enemyIndex]; }
				
				bool enemyKnown() const { return enemyIndex < startLocations.size(); }
				
				void expanded(BWAPI::Position where) {
					for (unsigned i = 0; i < bases.size(); i++)
						if (where == bases[i].base->getPosition()) {
							bases[i].owner = Mine;
							return;
						}
				}
				void enemySighted(BWAPI::Position where) {
					for (unsigned i = 0; i < bases.size(); i++)
						if (where == bases[i].base->getPosition()) {
							bases[i].owner = His;
							break;
						}
					if (!enemyKnown())
						for (unsigned i = 0; i < startLocations.size(); i++)
							if (startLocations[i].info->owner == His) {
								enemyIndex = i;
								return;
							}
				}

				void populate() {
					bases.reserve(BWTA::getBaseLocations().size());
					for (std::set<Base>::const_iterator base = BWTA::getBaseLocations().begin(); base != BWTA::getBaseLocations().end(); base++) {
						bases.push_back(BaseInfo());
						bases[bases.size()-1].base = *base;
						for (std::set<Unit>::const_iterator i = (*base)->getStaticMinerals().begin(); i != (*base)->getStaticMinerals().end(); i++)
							bases[bases.size()-1].patches.push_back(std::pair<Unit,int>(*i,1500));
						bases[bases.size()-1].owner = None;
						bases[bases.size()-1].gas = (*base)->isMineralOnly() ? 0 : 5000;
					}
					for (unsigned i = 0; i < bases.size(); i++)
						if (bases[i].base->isStartLocation()) {
							startLocations.push_back(StartLocation());
							startLocations[startLocations.size()-1].info = &bases[i];
							
							for (unsigned j = 0; j < bases.size(); j++)
								if (bases[j].base->isStartLocation() && i != j)
									startLocations[startLocations.size()-1].addBase(&bases[j]);
							
							double distance = 1e37;
							for (unsigned j = 0; j < bases.size(); j++)
								if (!bases[j].base->isStartLocation() && !bases[j].base->isIsland()) {
									double thisDistance = bases[j].base->getGroundDistance(bases[i].base);
									if (thisDistance < distance) {
										distance = thisDistance;
										startLocations[startLocations.size()-1].natural.info = &bases[j];
									}
								}
							
							for (unsigned j = 0; j < bases.size(); j++)
								if (j != i && &bases[j] != startLocations[startLocations.size()-1].natural.info)
									startLocations[startLocations.size()-1].natural.addBase(&bases[j]);
						}
					selfIndex = enemyIndex = startLocations.size();
				}
				
				void update() {
					std::stringstream tmp;
					tmp.setf(std::ios::fixed);
					tmp.precision(2);
					for (unsigned i = 0; i < self().natural.bases.size(); i++) {						
						double nearMe, farFromHim;
						double minerals, patches;
						double gas;
						
						for (unsigned j = 0; j < self().natural.bases[i]->patches.size(); j++)
							if (self().natural.bases[i]->patches[j].first->isVisible())
								self().natural.bases[i]->patches[j].second = self().natural.bases[i]->patches[j].first->getResources();
							
						for (std::set<Unit>::const_iterator j = self().natural.bases[i]->base->getGeysers().begin(); j != self().natural.bases[i]->base->getGeysers().end(); j++)
							if ((*j)->isVisible())
								self().natural.bases[i]->gas = (*j)->getResources();
						
						if (startLocations[selfIndex].natural.bases[i]->base->isIsland()) {
							nearMe = 0;
							farFromHim = 1;
						} else {
							nearMe = 1-((1-exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2)))/(1+exp(-3*(startLocations[selfIndex].natural.ground[i]/1000-2))) + 1)/2; //min(1,max(0,-(startLocations[selfIndex].natural.ground[i]/1000)+3));
							farFromHim = 1;
							if (enemyKnown()) {
								unsigned j = 0;
								for (; j < startLocations[enemyIndex].natural.bases.size(); j++)
									if (startLocations[enemyIndex].natural.bases[j] == startLocations[selfIndex].natural.bases[i])
										break;
								
								if (j == startLocations[enemyIndex].natural.bases.size())
									farFromHim = 0;
								else
									farFromHim = ((1-exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2)))/(1+exp(-3*(startLocations[enemyIndex].natural.ground[j]/1000-2))) + 1)/2;//1-min(1,max(0,-(startLocations[enemyIndex].natural.ground[j]/1000)+3));
							}
						}
							
						
						double aux = 0;
						
						aux = startLocations[selfIndex].natural.bases[i]->base->getStaticMinerals().size();
						patches = ((1-exp(-aux+5))/(1+exp(-aux+5)) + 1)/2;
							
						aux = startLocations[selfIndex].natural.bases[i]->minerals();
						aux /= 1000;
						minerals = ((1-exp(.6*(-aux+6)))/(1+exp(.6*(-aux+6))) + 1)/2;
						
						aux = startLocations[selfIndex].natural.bases[i]->gas;
						aux /= 10000;
						gas = aux + 0.5;
						
						tmp.str("");
						tmp << (startLocations[selfIndex].natural.bases[i]->owner == His ? "\x08" : "") <<  "[" << i << "]\tnearMe:" << nearMe << "\tfarFromHim:" << farFromHim << "\tminerals:" << minerals << "\tpatches:" << patches;
						BWAPI::Broodwar->drawTextScreen(340,25+i*9,tmp.str().c_str());
						tmp.flush();
						
						startLocations[selfIndex].natural.potential[i] = (nearMe*nearMe * farFromHim) * (minerals * patches*patches*patches * gas);
					}
					
					startLocations[selfIndex].natural.sort();					
				}
			};


			struct Builder {
				Unitset hatcheries;
				Unitset techBuildings;

				BaseGraph* allBases;

				Builder():allBases(0) { }
				
				Unitset getLarva() const {
					Unitset ret;
					for (Unitset::const_iterator hatch = hatcheries.begin(); hatch != hatcheries.end(); hatch++) {
						std::set<Unit> larvae = (*hatch)->getLarva();
						for (std::set<Unit>::iterator larva = larvae.begin(); larva != larvae.end(); larva++)
							ret.push_back(*larva);
					}
					return ret;
				}

				Unit getNearestHatch(BWAPI::Position where) const {
					Unitset::const_iterator hatch = hatcheries.begin();
					
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
				unsigned getNearestHatchIndex(BWAPI::Position where) const {
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

				void addHatch(Unit hatch) {
					hatcheries.insert(hatcheries.begin(),hatch);
					if (allBases)
						allBases->expanded(hatch->getPosition());
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
									} else if (miners[i][j].drone->getOrder() != BWAPI::Orders::HoldPosition) {
										miners[i][j].drone->gather(patches[i]);
										miners[i][j].drone->holdPosition(true);
									}
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
					unsigned minSize = 3;
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
					Minerset candidates;
					unsigned x,y;

					for (unsigned i = 0; i < miners.size(); i++)
						for (unsigned j = 0; j < miners[i].size(); j++)
							if (!miners[i][j].drone->isCarryingMinerals()) {
								unsigned k = 0;
								for (; k < candidates.size(); k++)
									if ((miners[i][j].state == candidates[k].state && miners[i][j].drone->getPosition().getDistance(hatch) < candidates[k].drone->getPosition().getDistance(hatch)) ||
										miners[i][j].state == Idle ||
										(miners[i][j].state == Waiting && candidates[k].state == Mining))
										break;
								candidates.insert(candidates.begin()+k, miners[i][j]);
								if (!k) {
									x = i;
									y = j;
								}
							}
					if (candidates.size()) {
						miners[x].erase(miners[x].begin()+y);
						if (candidates[0].state == Mining)
							mining[x] = false;
						return candidates[0].drone;
					} else
						return 0;
				}
			};

			Unitset nearbyMinerals(Unit hatch) {
				Unitset ret;
				std::set<Unit> units = hatch->getUnitsInRadius(300);
				for (std::set<Unit>::iterator i = units.begin(); i != units.end(); i++)
					if ((*i)->getType().isMineralField())
						ret.push_back(*i);
				return ret;
			}

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


				void idleWorker(Unit unit, Infrastructure::Builder& builder) {
					#ifndef NORMAL_MINING
					if (!has(getAllMiners(),unit))
					#endif
					{
						Unit mineral = 0;
						unsigned i = 0;
						for (; i < minerals.size(); i++)
							if (builder.hatcheries[i]->isCompleted()) {
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
				if (Micro::isntValid(*u))
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
					case Macro::Infrastructure::None:
						color = BWAPI::Colors::Brown;
						break;
					case Macro::Infrastructure::Mine:
						color = BWAPI::Colors::Blue;
						break;
					case Macro::Infrastructure::His:
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
						case Macro::Infrastructure::None:
							color = BWAPI::Colors::Brown;
							break;
						case Macro::Infrastructure::Mine:
							color = BWAPI::Colors::Blue;
							break;
						case Macro::Infrastructure::His:
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
				Macro::Infrastructure::BaseGraph* graph = new Macro::Infrastructure::BaseGraph();
				graph->populate();
				for (unsigned i = 0; i < graph->startLocations.size(); i++)
					if (graph->startLocations[i].info->base == start) {
						graph->selfIndex = i;
						break;
					}
				if (graph->startLocations.size() == 2) {
					graph->enemyIndex = graph->selfIndex ? 0 : 1;
					graph->enemy().info->owner = Macro::Infrastructure::His;
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