#include "AI.h"
#include "util.h"

#include <cstdlib>
#include <ctime>

AI::AI(Connection* conn) : BaseAI(conn) {}

/// Use this to give your AI a name
const char* AI::username()
{
  return "Shell AI";
}

const char* AI::password()
{
  return "password";
}

///This function is run once, before your first turn.
void AI::init()
{
  //set up the random number generator
  srand(time(NULL));
  //Find which player I am
  me = &players[playerID()];
}

///This function is called each time it is your turn.
///Return true to end your turn, return false to ask the server for updated information.
bool AI::run()
{
  //vectors for sarcophagi
  std::vector<Trap*> mySarcophagi;
  std::vector<Trap*> enemySarcophagi;
  std::vector<Point> newLocations;

  //if it's time to place traps...
  if (roundTurnNumber() == 0 || roundTurnNumber() == 1)
  {
    int trapScarabs = me->scarabs();

    //find my sarcophagi
    for (unsigned i = 0; i < traps.size(); ++i)
    {
      Trap& trap = traps[i];
      if (trap.owner() == playerID() && trap.trapType() == TrapType::SARCOPHAGUS)
	  {
        mySarcophagi.push_back(&trap);
      }
	}
    //find the first open tiles and place the sarcophagi there
    unsigned sarcophagusCount = mySarcophagi.size();
    for (unsigned i = 0; i < tiles.size(); ++i)
    {
      //if the tile is on my side and is empty
      Tile& tile = tiles[i];
      if (onMySide(tile.x()) && tile.type() == Tile::EMPTY)
      {
        //move my sarcophagus to that location
        me->placeTrap(tile.x(), tile.y(), TrapType::SARCOPHAGUS);
        //store the new locations of my sarcophagi
        newLocations.push_back(Point(tile.x(), tile.y()));
        --sarcophagusCount;
        if (sarcophagusCount == 0)
        {
          break;
        }
      }
    }

    //make sure there aren't too many traps spawned
    std::vector<unsigned> trapCount(trapTypes.size());
    //continue spawning traps until there isn't enough money to spend
    for (unsigned i = 0; i < tiles.size(); ++i)
    {
      //if the tile is on my side
      Tile& tile = tiles[i];
      if (onMySide(tile.x()))
      {
        //make sure there isn't a trap on that tile
        bool occupied = false;
        for (int i = 0; i < newLocations.size(); i++)
        {
	  if (newLocations[i].x == tile.x() && newLocations[i].y == tile.y())
	    occupied = true;
        }
        if (!occupied)
        {
          //select a random trap type (make sure it isn't a sarcophagus)
          int trapType = (rand() % (trapTypes.size() - 1)) + 1;
          //make sure another can be spawned
          if (trapCount[trapType] >= trapTypes[trapType].maxInstances())
          {
            continue;
          }
	  //if there are enough scarabs
          if (trapScarabs >= trapTypes[trapType].cost())
	  {
	    //check if the tile is the right type (wall or empty)
	    if (trapTypes[trapType].canPlaceOnWalls() && tile.type() == Tile::WALL)
	    {
	      me->placeTrap(tile.x(), tile.y(), trapType);
	      ++trapCount[trapType];
	      trapScarabs -= trapTypes[trapType].cost();
	    }
	    else if (!trapTypes[trapType].canPlaceOnWalls() && tile.type() == Tile::EMPTY)
	    {
	      me->placeTrap(tile.x(), tile.y(), trapType);
	      ++trapCount[trapType];
	      trapScarabs -= trapTypes[trapType].cost();
	    }
	  }
          else
          {
	    break;
	  }
        }
      }
    }
  }
  //otherwise it's time to move and purchase thieves and activate traps
  else
  {
    //find my sarcophagi and the enemy sarcophagi
    for(unsigned i = 0; i < traps.size(); ++i)
    {
      Trap& trap = traps[i];
      if(trap.trapType() == TrapType::SARCOPHAGUS)
      {
        if(trap.owner() != playerID())
        {
          enemySarcophagi.push_back(&trap);
        }
        else
        {
          mySarcophagi.push_back(&trap);
        }
      }
    }
    //find my spawn tiles
    std::vector<Tile*> spawnTiles = getMySpawns();
    //select a random thief type
    int thiefNo = rand() % thiefTypes.size();
    //if you can afford the thief
    if(me->scarabs() >= thiefTypes[thiefNo].cost())
    {
      //make sure another can be spawned
      int max = thiefTypes[thiefNo].maxInstances();
      int count = 0;
      std::vector<Thief*> myThieves = getMyThieves();
      for(unsigned i = 0; i < myThieves.size(); ++i)
      {
        if(myThieves[i]->thiefType() == thiefNo)
        {
          ++count;
        }
      }
      //only spawn if there aren't too many
      if(count < max)
      {
        //select a random spawn location
        int spawnLoc = rand() % spawnTiles.size();
        //spawn a thief there
        Tile* spawnTile = spawnTiles[spawnLoc];
        me->purchaseThief(spawnTile->x(), spawnTile->y(), thiefNo);
      }
    }
    //move my thieves
    std::vector<Thief*> myThieves = getMyThieves();
    for(unsigned i = 0; i < myThieves.size(); ++i)
    {
      Thief* thief = myThieves[i];
      //if the thief is alive and not frozen
      if(thief->alive() && thief->frozenTurnsLeft() == 0)
      {
        const int xChange[] = {-1, 1,  0, 0};
        const int yChange[] = { 0, 0, -1, 1};
        //try to dig or use a bomb before moving
        if(thief->thiefType() == ThiefType::DIGGER && thief->specialsLeft() > 0)
        {
          for(unsigned i = 0; i < 4; ++i)
          {
            //if there is a wall adjacent and an empty space on the other side
            int checkX = thief->x() + xChange[i];
            int checkY = thief->y() + yChange[i];
            Tile* wallTile = getTile(checkX, checkY);
            Tile* emptyTile = getTile(checkX + xChange[i], checkY + yChange[i]);
            //must be on the map, and not trying to dig to the other side
            if(wallTile != NULL && emptyTile != NULL && !onMySide(checkX + xChange[i]))
            {
              //if there is a wall with an empty tile on the other side
              if(wallTile->type() == Tile::WALL && emptyTile->type() == Tile::EMPTY)
              {
                //dig through the wall
                thief->useSpecial(checkX, checkY);
                //break out of the loop
                break;
              }
            }
          }
        }
        else if(thief->thiefType() == ThiefType::BOMBER && thief->specialsLeft() > 0)
        {
          for(unsigned i = 0; i < 4; ++i)
          {
            //the place to check for things to blow up
            int checkX = thief->x() + xChange[i];
            int checkY = thief->y() + yChange[i];
            //make sure that the spot isn't on the other side
            if(!onMySide(checkX))
            {
              //if there is a wall tile there, blow it up
              Tile* checkTile = getTile(checkX, checkY);
              if(checkTile != NULL && checkTile->type() == Tile::WALL)
              {
                //blow up the wall
                thief->useSpecial(checkX, checkY);
                //break out of the loop
                break;
              }
              //otherwise check if there is a trap there
              Trap* checkTrap = getTrap(checkX, checkY);
              //don't want to blow up the sarcophagus!
              if(checkTrap != NULL && checkTrap->trapType() != TrapType::SARCOPHAGUS)
              {
                //blow up the trap
                thief->useSpecial(checkX, checkY);
                //break out of the loop
                break;
              }
            }
          }
        }
        //if the thief has any movement left
        if(thief->movementLeft() > 0)
        {
          //find a path from the thief's location to the enemy sarcophagus
          std::deque<Point> path;
          int endX = enemySarcophagi[0]->x();
          int endY = enemySarcophagi[0]->y();
          path = findPath(Point(thief->x(), thief->y()), Point(endX, endY));
          //if a path exists then move forward on the path
          if(path.size() > 0)
          {
            thief->move(path[0].x, path[0].y);
          }
        }
      }
    }
    //do things with traps now
    std::vector<Trap*> myTraps = getMyTraps();
    for(unsigned i = 0; i < myTraps.size(); ++i)
    {
      const int xChange[] = {-1, 1,  0, 0};
      const int yChange[] = { 0, 0, -1, 1};
      Trap* trap = myTraps[i];
      //make sure trap can be used
      if(trap->active())
      {
        //if trap is a boulder
        if(trap->trapType() == TrapType::BOULDER)
        {
          //if there is an enemy thief adjancent
          for(unsigned i = 0; i < 4; ++i)
          {
            Thief* enemyThief = getThief(trap->x() + xChange[i], trap->y() + yChange[i]);
            //roll over the thief
            if(enemyThief != NULL)
            {
              trap->act(trap->x() + xChange[i], trap->y() + yChange[i]);
              break;
            }
          }
        }
        else if(trap->trapType() == TrapType::MUMMY)
        {
          //move around randomly if a mummy
          int dir = rand() % 4;
          int checkX = trap->x() + xChange[dir];
          int checkY = trap->y() + yChange[dir];
          Tile* checkTile = getTile(checkX, checkY);
          Trap* checkTrap = getTrap(checkX, checkY);
          //if the tile is empty, and there isn't a sarcophagus there
          if(checkTrap == NULL || checkTrap->trapType() != TrapType::SARCOPHAGUS)
          {
            if(checkTile != NULL && checkTile->type() == Tile::EMPTY)
            {
              //move on that tile
              trap->act(checkX, checkY);
            }
          }
        }
      }
    }
  }
  return true;
}

///This function is run once, after your last turn.
void AI::end(){}
