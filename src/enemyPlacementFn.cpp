//
//  enemyPlacementFn.cpp
//  Blind Jump
//
//  Created by Evan Bowman on 2/13/16.
//  Copyright © 2016 Evan Bowman. All rights reserved.
//

#include <stdio.h>
#include "enemyPlacementFn.hpp"
#include "gameMap.hpp"
#include "enemyCreationFunctions.hpp"
#include <cmath>
#include <array>

int initEnemies(GameMap * gm) {
	constexpr static std::array<int, 4> targetLevel = {{
	    4   /*Scoot*/,
		5   /*Critter*/,
		20  /*Dasher*/,
		28  /*Turrets*/
	}};
	
	int currentLevel = gm->getLevel();
	enemyController & enemies = gm->getEnemyController();
	tileController & tiles = gm->getTileController();

	// Count sum of energy values for all enemies, used in health station cost calculations
	int count = 0;
	std::vector<std::pair<int, int>>* pVec = gm->getEnemySelectVec();
	// Not all enemies are available for placement initially (don't want to place a high level enemy on the first stage!)
	std::pair<int, int> scoot, dasher, turret, critter;
	switch (currentLevel) {
		case 1:
			// push new enemies to the enemy creation vector
			scoot.first = 1;
			scoot.second = abs(currentLevel - targetLevel[0]);
			pVec->push_back(scoot);
			critter.first = 1;
			critter.second = abs(currentLevel - targetLevel[1]);
			pVec->push_back(critter);
			break;
			
			
		case 2:
			dasher.first = 2;
			dasher.second = abs(currentLevel - targetLevel[2]);
			pVec->push_back(dasher);
			break;
			
		case 4:
			turret.first = 3;
			turret.second = abs(currentLevel - targetLevel[3]);
			pVec->push_back(turret);
			break;
			
		default:
			// Do nothing
			break;
	}
	size_t enemyVecLen = gm->getEnemySelectVec()->size();
	
	// Collect enemy weight terms under a variable and init to 0
	int collector = 0;
	int diff;
	std::vector<int> intervals(enemyVecLen);
	// First loop through all enemies and update their probability values based on current level
	for (size_t i = 0; i < enemyVecLen; i++) {
		// Set the weight to 100 divided by the difference between the current level and the ideal level
		// Max function to prevent divide by 0
		diff = (100 + currentLevel) / std::max(abs(currentLevel - targetLevel[i]), 1);
		(*pVec)[i].second = diff;
		collector += diff;
		intervals[i] = collector;
	}

	// int divisibility = static_cast<int>(floor(static_cast<float>(currentLevel) / 7.f));
	// int normalizedLevel = (currentLevel < 8) ? currentLevel : currentLevel - 7 * divisibility;
	// int iters = divisibility * 2 + pow(normalizedLevel, 1.2);
	int iters = 2 + pow(currentLevel, 1.1f);
	if (iters > 15) {
		iters = 15;
	}
	for (int i = 0; i < iters; i++) {
		// Generate a random number on the range of 0 to the sum of all enemy weights
		int select = rand() % std::max(collector, 1);
		// Find the interval that the selected value falls into in intervals[]
		int selectedIndex = 0;
		for (size_t i = 0; i < enemyVecLen; i++) {
			if (i == 0) {
				if (select < intervals[0]) {
					selectedIndex = 0;
					break;
				}
			}
			
			else {
				if (select < intervals[i]) {
					selectedIndex = static_cast<int>(i);
					break;
				}
			}
		}
		// Now place enemies based on the selected index
		switch (selectedIndex) {
			case 0:
				enemies.addScoot(&tiles);
				count += 1;
				break;
				
			case 1:
				addCritter(tiles.mapArray, tiles.descriptionArray, enemies, tiles.posX, tiles.posY, gm->windowW, gm->windowH, tiles.emptyMapLocations, 1);
				count += 2;
				break;
				
				
			case 2:
				enemies.addDasher(&tiles);
				count += 3;
				break;
				
			case 3:
				addTurret(tiles.mapArray, tiles.descriptionArray, enemies, tiles.posX, tiles.posY, gm->windowW, gm->windowH, tiles.emptyMapLocations);
				count += 8;
				break;
				
			default:
				break;
		}
		
	}

	// Return the count
	return count;
}
