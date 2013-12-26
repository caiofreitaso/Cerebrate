#pragma once
#include <BWAPI.h>
#include <BWAPI/Client.h>
#include <BWTA.h>
#include <vector>

namespace Cerebrate {
	template<typename T>
	bool has(std::vector<T>& vector, T value) {
		for (unsigned i = 0; i < vector.size(); i++)
			if (vector[i] == value)
				return true;
		return false;
	}

	typedef BWAPI::Unit* Unit;
	typedef BWAPI::Player* Player;

	typedef std::vector<BWAPI::Unit*> Unitset;

	typedef BWTA::BaseLocation* Base;
	typedef std::vector<Base> Baseset;

	typedef BWTA::Chokepoint* Choke;
	typedef std::vector<Choke> Chokeset;
	typedef std::set<Choke>::const_iterator Choke_it;

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
}