#ifndef AI_H
#define AI_H

#include "BaseAI.h"
#include <deque>
#include <map>

struct Point
{
  int x;
  int y;
  Point(){}
  Point(int x, int y):x(x),y(y){}
};

///The class implementing gameplay logic.
class AI: public BaseAI
{
  //returns true if the position is on your side of the field
  bool onMySide(int x)
  {
    if(playerID() == 0)
    {
      return (x < mapWidth()/2);
    }
    else
    {
      return (x >= mapWidth()/2);
    }
  }

  //returns the first thief encountered on x, y or NULL if no thief
  Thief* getThief(int x, int y)
  {
    if(x < 0 || x >= mapWidth() || y < 0 || y >= mapHeight())
    {
      return NULL;
    }
    for(int i = 0; i < thiefs.size(); ++i)
    {
      if(thiefs[i].x() == x && thiefs[i].y() == y)
      {
        return &thiefs[i];
      }
    }
    return NULL;
  }

  //returns the tile at the given x,y position or NULL otherwise
  Tile* getTile(int x, int y)
  {
    if(x < 0 || x >= mapWidth() || y < 0 || y >= mapHeight())
    {
      return NULL;
    }
    return &tiles[y + x * mapHeight()];
  }

  //returns the trap at the given x,y position or NULL otherwise
  Trap* getTrap(int x, int y)
  {
    if(x < 0 || x>= mapWidth() || y < 0 || y >= mapHeight())
    {
      return NULL;
    }
    for(int i = 0; i < traps.size(); ++i)
    {
      if(traps[i].x() == x && traps[i].y() == y)
      {
        return &traps[i];
      }
    }
    return NULL;
  }

  //returns a vector of all of your traps
  std::vector<Trap*> getMyTraps()
  {
    std::vector<Trap*> toReturn;
    for(unsigned i = 0; i < traps.size(); ++i)
    {
      if(traps[i].owner() == playerID())
      {
        toReturn.push_back(&traps[i]);
      }
    }
    return toReturn;
  }

  //returns a vector of all of your enemy's traps
  std::vector<Trap*> getEnemyTraps()
  {
    std::vector<Trap*> toReturn;
    for(unsigned i = 0; i < traps.size(); ++i)
    {
      if(traps[i].owner() != playerID())
      {
        toReturn.push_back(&traps[i]);
      }
    }
  }

  //returns a vector of all of your spawn tiles
  std::vector<Tile*> getMySpawns()
  {
    std::vector<Tile*> toReturn;
    for(unsigned i = 0; i < tiles.size(); ++i)
    {
      if(!onMySide(tiles[i].x()) && tiles[i].type() == Tile::SPAWN)
      {
        toReturn.push_back(&tiles[i]);
      }
    }
    return toReturn;
  }

  //return a vector of all of your theives
  std::vector<Thief*> getMyThieves()
  {
    std::vector<Thief*> toReturn;
    for(unsigned i = 0; i < thiefs.size(); ++i)
    {
      if(thiefs[i].owner() == playerID())
      {
        toReturn.push_back(&thiefs[i]);
      }
    }
    return toReturn;
  }

  //return a vector of all of the enemy thieves
  std::vector<Thief*> getEnemyThieves()
  {
    std::vector<Thief*> toReturn;
    for(unsigned i = 0; i < thiefs.size(); ++i)
    {
      if(thiefs[i].owner() != playerID())
      {
        toReturn.push_back(&thiefs[i]);
      }
    }
    return toReturn;
  }

  //returns a path from start to end, or nothing if no path is found.
  std::deque<Point> findPath(Point start, Point end)
  {
    std::deque<Point> toReturn;
    //the set of open tiles to look at
    std::deque<Tile*> openSet;
    //points back to parent tile
    std::map<Tile*, Tile*> parent;
    //push back the starting tile
    openSet.push_back(getTile(start.x, start.y));
    //the start tile has no parent
    parent[getTile(start.x, start.y)] = NULL;
    //the end tile
    Tile* endTile = getTile(end.x, end.y);
    //as long as the end tile has no parent
    while(parent.count(endTile) == 0)
    {
    	//if there are no tiles in the openSet the there is no path
      if(openSet.empty())
      {
        return toReturn;
      }
      //check tiles from the front
      Tile* curTile = openSet.front();
      //and remove it
      openSet.pop_front();
      const int xChange[] = { 0, 0, -1, 1};
      const int yChange[] = {-1, 1,  0, 0};
      //look in all directions
      for(unsigned i = 0; i < 4; ++i)
      {
        Point loc(curTile->x() + xChange[i], curTile->y() + yChange[i]);
        Tile* toAdd = getTile(loc.x, loc.y);
        //if a tile exists there
        if(toAdd != NULL)
        {
        	//if it's an open file and it doesn't have a parent
          if(toAdd->type() == Tile::EMPTY && parent.count(toAdd) == 0)
          {
          	//add the tile to the open set; and mark its parent as the current tile
            openSet.push_back(toAdd);
            parent[toAdd] = curTile;
          }
        }
      }
    }
    //find the path back
    for(Tile* tile = getTile(end.x, end.y); parent[tile] != NULL; tile = parent[tile])
    {
      toReturn.push_front(Point(tile->x(), tile->y()));
    }
    return toReturn;
  }

  Player* me;
  Trap* mySarcophagus;
  Trap* enemySarcophagus;

public:
  AI(Connection* c);
  virtual const char* username();
  virtual const char* password();
  virtual void init();
  virtual bool run();
  virtual void end();
};

#endif
