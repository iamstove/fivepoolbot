#include "fivepool.h"

using namespace BWAPI;


bool buildPool() {
	if (Broodwar->self()->hasUnitTypeRequirement(BWAPI::UnitTypes::Zerg_Spawning_Pool)) {
		return false;
	}
	if (BWAPI::Broodwar->self()->supplyUsed() == 10 && Broodwar->self()->minerals() >= 200) {// the supply count is actually double because zerglings are .5 supply
		return true;
	}
	return false;
}
int unitBuild() {  // 0 is worker or overlord,  1 is zergling
	if (BWAPI::Broodwar->self()->supplyUsed() >= 10 && BWAPI::Broodwar->self()->supplyUsed() >= BWAPI::Broodwar->self()->supplyTotal()) {
		return 1;
	}
	else if (!Broodwar->self()->hasUnitTypeRequirement(BWAPI::UnitTypes::Zerg_Zergling) && Broodwar->self()->minerals() <= 200) {
		return 1;
	}
	else {
		return 0;
	}
}
