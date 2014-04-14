#pragma once
#include "Util.h"
#include "Production.h"

namespace Cerebrate {
	namespace Industry {
		struct Production;
	};
	namespace Economy {
		struct Budget {
			static unsigned start;
			
			unsigned id;
			int minerals;
			int gas;
		};
		
		struct Economist {
			std::vector<Budget> budgets;
			
			void add(Industry::Production type);
			
			int minerals() const;
			int gas() const;
		};
	};
};