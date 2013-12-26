#pragma once
#include "Util.h"

namespace Cerebrate {
	namespace Infrastructure {
		struct Builder {
			Unitset hatcheries;
			Unitset techBuildings;

			Unitset getLarva() const;
			Unit getNearestHatch(BWAPI::Position where) const;
			unsigned getNearestHatchIndex(BWAPI::Position where) const;
			void addHatch(Unit hatch);

			void draw() const;
		};
	};
};