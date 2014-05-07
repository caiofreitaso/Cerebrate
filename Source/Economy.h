#pragma once
#include "Util.h"
#include "Production.h"

namespace Cerebrate {
	namespace Industry {
		struct Production;
	};
	namespace Economy {
		struct Budget {
			int minerals;
			int gas;
		};
		
		struct Economist {
			static unsigned start;
			
			std::map<unsigned, Budget> budgets;
			typedef std::map<unsigned, Budget>::const_iterator budget_const;
			typedef std::map<unsigned, Budget>::iterator budget_it;

			
			int add(Industry::Production);
			int add(Budget);
			void remove(unsigned);
			
			int minerals() const;
			int gas() const;
		};
	};
};