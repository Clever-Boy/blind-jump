//
//  playerCollisionFunctions.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 11/14/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#pragma once
#ifndef playerCollisionFunctions_hpp
#define playerCollisionFunctions_hpp

#include "wall.hpp"
#include "treasureChest.hpp"
#include <cmath>

inline void checkCollisionWall(std::vector<wall> walls, bool& CollisionDown, bool& CollisionUp, bool& CollisionRight, bool& CollisionLeft, float posY, float posX) {
	bool foundCollision[4] = {0, 0, 0, 0};
	for (size_t i = 0; i < walls.size(); i++) {
		//if (walls[i].isInsideWindow()) {
		if ((posX + 6 < (walls[i].getPosX() + walls[i].getWidth()) && (posX + 6 > (walls[i].getPosX()))) && (fabs((posY + 16) - walls[i].getPosY()) <= 13) && foundCollision[0] == 0)  {
			CollisionLeft =  1;
			foundCollision[0] = 1;
		}
		
		if ((posX + 24 > (walls[i].getPosX()) && (posX + 24 < (walls[i].getPosX() + walls[i].getWidth()))) && (fabs((posY + 16) - walls[i].getPosY()) <= 13) && foundCollision[1] == 0)  {
			CollisionRight =  1;
			foundCollision[1] = 1;
		}
		
		if (((posY + 22 < (walls[i].getPosY() + walls[i].getHeight())) && (posY + 22 > (walls[i].getPosY()))) && (fabs((posX) - walls[i].getPosX()) <= 16) && foundCollision[2] == 0)  {
			CollisionUp =  1;
			foundCollision[2] = 1;
		}
		
		if (((posY + 36 > walls[i].getPosY()) && (posY + 36 < walls[i].getPosY() + walls[i].getHeight())) && (fabs((posX) - walls[i].getPosX()) <= 16) && foundCollision[3] == 0)  {
			CollisionDown =  1;
			foundCollision[3] = 1;
		}
	//}
	}
}

inline void checkCollisionChest (std::vector<TreasureChest> chests, bool & CollisionDown, bool & CollisionUp, bool & CollisionRight, bool & CollisionLeft, float posY, float posX) {
	bool foundCollision[4] = {0, 0, 0, 0};
	for (size_t i = 0; i < chests.size(); i++) {
		if ((posX + 6 < (chests[i].getPosition().x + 16) && (posX + 6 > (chests[i].getPosition().x))) && (fabs((posY + 16) - chests[i].getPosition().y) <= 8) && foundCollision[0] == 0)  {
			CollisionLeft =  1;
			foundCollision[0] = 1;
		}
		
		if ((posX + 24 > (chests[i].getPosition().x) && (posX + 24 < (chests[i].getPosition().x + 16))) && (fabs((posY + 16) - chests[i].getPosition().y) <= 8) && foundCollision[1] == 0)  {
			CollisionRight =  1;
			foundCollision[1] = 1;
		}
		
		if (((posY + 22 < (chests[i].getPosition().y + 16)) && (posY + 22 > (chests[i].getPosition().y))) && (fabs((posX) - chests[i].getPosition().x + 6) <= 12) && foundCollision[2] == 0)  {
			CollisionUp =  1;
			foundCollision[2] = 1;
		}
		
		if (((posY + 36 > chests[i].getPosition().y) && (posY + 36 < chests[i].getPosition().y + 16)) && (fabs((posX) - chests[i].getPosition().x + 6) <= 12) && foundCollision[3] == 0)  {
			CollisionDown =  1;
			foundCollision[3] = 1;
		}
	}
}

#endif /* playerCollisionFunctions_hpp */
