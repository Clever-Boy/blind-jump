//
//  userInterface.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 10/22/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#pragma once
#ifndef userInterface_hpp
#define userInterface_hpp

#include "SFML/Graphics.hpp"
#include "fontController.hpp"
#include "effectsController.hpp"
#include "inputController.hpp"
#include <array>

// This part of the code is so tied to the aesthetic appearance of the game,
// making it difficult to describe. If there is a constant and you can't tell
// where it came from, it't purpose is that it makes an effect move across
// the screen or fade in at a rate that I think looks good
class Player;

class userInterface {
	
	enum class State {
		opening,
		open,
		closing,
		closed,
		deathScreenEntry,
		deathScreen,
		deathScreenExit,
		statsScreen,
		complete
	};
	
public:
	userInterface();
	void drawMenu(sf::RenderWindow&, Player*, FontController&, effectsController&, float, float, InputController*, sf::Time&);
	void setup(float, float, sf::View *);
	sf::Texture itemTextures[3][3];
	void addItem(char, effectsController&, float, float, FontController&, Player&);
	
	uint8_t selectedColumn;
	std::array<unsigned char, 3> rowIndices;
	
	// Function to display the death sequence
	void dispDeathSeq();
	bool isComplete();
	bool canHeal;
	
	float getDesaturateAmount();
	
	bool isVisible();
	
	// Function to reset the ui controller
	void reset();
	void setEnemyValueCount(int);
	
	bool isOpen();
	
	bool blurEnabled();
	bool desaturateEnabled();
	
	// Accessor for blur amount
	float getBlurAmount();
	
private:
	State state;
	float xPos, yPos;
	bool visible;
	std::array<sf::Sprite, 3> gunSprites;
	float weaponDispOffset;
        
	int enemyValueCount;
	
    int32_t timer, timerAlt;
	
	float zoomDegree;
	
	// The amount of blur to use when opening the items menu
	float blurAmount;
	
	sf::View * pWorldView;
	sf::View cachedView;
	
	float desaturateAmount;
	float textAlpha;
	bool keyPressed;
	
	bool deathSeq, deathSeqComplete;
		
	sf::Texture selectorShadowTexture;
	sf::Sprite selectorShadowSprite;
	
	std::vector<sf::Sprite> textToDisplay;
	// Store the amount the text moves during the entry animation to reset it to the original value
	float textDisplacement;
};
#endif /* userInterface_hpp */
