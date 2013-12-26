#pragma once
#include "Util.h"
#include <cmath>

namespace Cerebrate {
	namespace Economy {
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

			void update(Player player, int frame, unsigned droneCount);
			bool support(BWAPI::UnitType unit, int quantity = 1);
			Resource projectResources(int frames);

			void draw();
		};
	};
};