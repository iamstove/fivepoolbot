#include "fivepool.h"
#include <iostream>
#include "SimpleBuild.cpp"

using namespace BWAPI;
using namespace Filter;

void fivepool::onStart()
{
  // Hello World!
  Broodwar->sendText("Hello world!");

  // Print the map name.
  // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
  Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

  // Enable the UserInput flag, which allows us to control the bot and type messages.
  Broodwar->enableFlag(Flag::UserInput);

  // Uncomment the following line and the bot will know about everything through the fog of war (cheat).
  //Broodwar->enableFlag(Flag::CompleteMapInformation);

  // Set the command optimization level so that common commands can be grouped
  // and reduce the bot's APM (Actions Per Minute).
  Broodwar->setCommandOptimizationLevel(2);

  // Check if this is a replay
  if ( Broodwar->isReplay() )
  {

    // Announce the players in the replay
    Broodwar << "The following players are in this replay:" << std::endl;
    
    // Iterate all the players in the game using a std:: iterator
    Playerset players = Broodwar->getPlayers();
    for(auto p : players)
    {
      // Only print the player if they are not an observer
      if ( !p->isObserver() )
        Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
    }

  }
  else // if this is not a replay
  {
    // Retrieve you and your enemy's races. enemy() will just return the first enemy.
    // If you wish to deal with multiple enemies then you must use enemies().
    if ( Broodwar->enemy() ) // First make sure there is an enemy
      Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
  }

}

void fivepool::onEnd(bool isWinner)
{
  // Called when the game ends
  if ( isWinner )
  {
    // Log your win here!
  }
}

void fivepool::onFrame()
{
  // Called once every game frame

  // Display the game frame rate as text in the upper left area of the screen
  Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS() );
  Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS() );

  // Return if the game is a replay or is paused
  if ( Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self() )
    return;

  // Prevent spamming by only running our onFrame once every number of latency frames.
  // Latency frames are the number of frames before commands are processed.
  if ( Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0 )
    return;

  // Iterate through all the units that we own
  for (auto &u : Broodwar->self()->getUnits())
  {
    // Ignore the unit if it no longer exists
    // Make sure to include this block when handling any Unit pointer!
    if ( !u->exists() )
      continue;

    // Ignore the unit if it has one of the following status ailments
    if ( u->isLockedDown() || u->isMaelstrommed() || u->isStasised() )
      continue;

    // Ignore the unit if it is in one of the following states
    if ( u->isLoaded() || !u->isPowered() || u->isStuck() )
      continue;

    // Ignore the unit if it is incomplete or busy constructing
    if ( !u->isCompleted() || u->isConstructing() )
      continue;


    // Finally make the unit do some stuff!


    // If the unit is a worker unit
    if ( u->getType().isWorker() )
    {
		if (buildPool()) { //for building a spawning pool
			Broodwar->sendText("trying to make a pool");
			TilePosition targetBuildLocation = Broodwar->getBuildLocation(BWAPI::UnitTypes::Zerg_Spawning_Pool, u->getTilePosition());
			if (targetBuildLocation)
			{
				// Register an event that draws the target build location
				Broodwar->registerEvent([targetBuildLocation, u](Game*)
				{
					Broodwar->drawBoxMap(Position(targetBuildLocation),
						Position(targetBuildLocation + BWAPI::UnitTypes::Zerg_Spawning_Pool.tileSize()),
						Colors::Blue);
				},
					nullptr,  // condition
					BWAPI::UnitTypes::Zerg_Spawning_Pool.buildTime() + 100);  // frames to run

				// Order the builder to construct the spawning pool
				u->build(BWAPI::UnitTypes::Zerg_Spawning_Pool, targetBuildLocation);
			}
		}
      // if our worker is idle
      if ( u->isIdle() )
      {
        // Order workers carrying a resource to return them to the center,
        // otherwise find a mineral patch to harvest.
        if ( u->isCarryingGas() || u->isCarryingMinerals() )
        {
          u->returnCargo();
        }
        else if ( !u->getPowerUp() )  // The worker cannot harvest anything if it
        {                             // is carrying a powerup such as a flag
          // Harvest from the nearest mineral patch or gas refinery
          if ( !u->gather( u->getClosestUnit( IsMineralField || IsRefinery )) )
          {
            // If the call fails, then print the last error message
            Broodwar << Broodwar->getLastError() << std::endl;
          }

        } // closure: has no powerup
      } // closure: if idle

    }
    else if ( u->getType().isResourceDepot() ) // A resource depot is a Command Center, Nexus, or Hatchery
    {
	int unit = unitBuild();
      // Order the depot to construct more workers! But only when it is idle.
	if (unit == 0) {
		if (u->isIdle() && !u->train(u->getType().getRace().getWorker()))
		{
			// If that fails, draw the error at the location so that you can visibly see what went wrong!
			// However, drawing the error once will only appear for a single frame
			// so create an event that keeps it on the screen for some frames
			Position pos = u->getPosition();
			Error lastErr = Broodwar->getLastError();
			Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
				nullptr,    // condition
				Broodwar->getLatencyFrames());  // frames to run

			// Retrieve the supply provider type in the case that we have run out of supplies
			UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
			static int lastChecked = 0;

			// If we are supply blocked and haven't tried constructing more recently
			if (lastErr == Errors::Insufficient_Supply &&
				lastChecked + 400 < Broodwar->getFrameCount() &&
				Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
			{
				lastChecked = Broodwar->getFrameCount();

				// Retrieve a unit that is capable of constructing the supply needed
				Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
					(IsIdle || IsGatheringMinerals) &&
					IsOwned);
				// If a unit was found
				if (supplyBuilder)
				{
					if (supplyProviderType.isBuilding())
					{
						TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
						if (targetBuildLocation)
						{
							// Register an event that draws the target build location
							Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
							{
								Broodwar->drawBoxMap(Position(targetBuildLocation),
									Position(targetBuildLocation + supplyProviderType.tileSize()),
									Colors::Blue);
							},
								nullptr,  // condition
								supplyProviderType.buildTime() + 100);  // frames to run

							// Order the builder to construct the supply structure
							supplyBuilder->build(supplyProviderType, targetBuildLocation);
						}
					}
					else
					{
						// Train the supply provider (Overlord) if the provider is not a structure
						supplyBuilder->train(supplyProviderType);
					}
				} // closure: supplyBuilder is valid
			} // closure: insufficient supply
		} // closure: failed to train idle unit
	}
	else if (unit == 1) {
		if (!u->train(BWAPI::UnitTypes::Zerg_Zergling)) {
			//Broodwar->sendText("Damn I can't make zerglings");
		}
	}

    }

  } // closure: unit iterator
}

void fivepool::onSendText(std::string text)
{

  // Send the text to the game if it is not being processed.
  Broodwar->sendText("%s", text.c_str());


  // Make sure to use %s and pass the text as a parameter,
  // otherwise you may run into problems when you use the %(percent) character!

}

void fivepool::onReceiveText(BWAPI::Player player, std::string text)
{
  // Parse the received text
  Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void fivepool::onPlayerLeft(BWAPI::Player player)
{
  // Interact verbally with the other players in the game by
  // announcing that the other player has left.
  Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void fivepool::onNukeDetect(BWAPI::Position target)
{

  // Check if the target is a valid position
  if ( target )
  {
    // if so, print the location of the nuclear strike target
    Broodwar << "Nuclear Launch Detected at " << target << std::endl;
  }
  else 
  {
    // Otherwise, ask other players where the nuke is!
    Broodwar->sendText("Where's the nuke?");
  }

  // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void fivepool::onUnitDiscover(BWAPI::Unit unit)
{
}

void fivepool::onUnitEvade(BWAPI::Unit unit)
{
}

void fivepool::onUnitShow(BWAPI::Unit unit)
{
}

void fivepool::onUnitHide(BWAPI::Unit unit)
{
}

void fivepool::onUnitCreate(BWAPI::Unit unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void fivepool::onUnitDestroy(BWAPI::Unit unit)
{
}

void fivepool::onUnitMorph(BWAPI::Unit unit)
{
  if ( Broodwar->isReplay() )
  {
    // if we are in a replay, then we will print out the build order of the structures
    if ( unit->getType().isBuilding() && !unit->getPlayer()->isNeutral() )
    {
      int seconds = Broodwar->getFrameCount()/24;
      int minutes = seconds/60;
      seconds %= 60;
      Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
    }
  }
}

void fivepool::onUnitRenegade(BWAPI::Unit unit)
{
}

void fivepool::onSaveGame(std::string gameName)
{
  Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void fivepool::onUnitComplete(BWAPI::Unit unit)
{
}
