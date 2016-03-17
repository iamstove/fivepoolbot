#include "fivepool.h"

using namespace BWAPI;
bool poolBuilt;

bool buildPool() {
	if (poolBuilt) {
		return false;
	}
	if (BWAPI::Broodwar->self()->supplyUsed() == 5) {
		poolBuilt = true;
		return true;
	}
	return false;
}
int unitBuild() {
	if (BWAPI::Broodwar->self()->supplyUsed() >= 6) {

	}
}
