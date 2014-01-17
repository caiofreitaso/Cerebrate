#pragma once
#include "Infrastructure.h"


namespace Cerebrate {
	namespace Intelligence {
		struct Agent {
			double defensability(Infrastructure::Hatchery& hatch) const;
		};
	};
};