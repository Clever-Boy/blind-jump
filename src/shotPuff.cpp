//
//  shotPoof.cpp
//  Blind Jump
//
//  Created by Evan Bowman on 10/14/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#include "shotPuff.hpp"

shotPuff::shotPuff(sf::Sprite sprIn[4], float x, float y, char dir, int p) {
	xPos = x;
	yPos = y;
	//Store all of the sprites for later
	for (int i = 0; i < 4; i++) {
		sprPuff[i] = sprIn[i];
	}
	frameIndex = 0;
	frameTimer = 0;
	killFlag = false;
	xInit = x;
	yInit = y;
}

//Updates the position of the effect
void shotPuff::update(float xOffset, float yOffset, sf::Time & elapsedTime) {
	xPos = xInit + xOffset;
	yPos = yInit + yOffset;
	frameTimer += elapsedTime.asMilliseconds();
	if (frameTimer > 50) {
		frameTimer -= 50;
		frameIndex += 1;
		if (frameIndex == 4) {
			frameIndex = 3;
			killFlag = true;
		}
	}
}

sf::Sprite shotPuff::getSprite() {
	sprPuff[frameIndex].setPosition(round(xPos), round(yPos));
	return sprPuff[frameIndex];
}

bool shotPuff::getKillFlag() {
	return killFlag;
}

float shotPuff::getYpos() {
	return yPos;
}