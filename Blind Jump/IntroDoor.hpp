//
//  Pillar.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 3/15/16.
//  Copyright © 2016 Evan Bowman. All rights reserved.
//

#ifndef IntroDoor_hpp
#define IntroDoor_hpp

#include "detailParent.hpp"
#include "inputController.hpp"

#define DORMANT 'd'
#define OPENING 'o'
#define OPENED 'O'

class ScreenShakeController;

class IntroDoor : public detailParent {
private:
    sf::Sprite sprite[4];
    char frameIndex, frameRate, state;
    
public:
    void update(float, float, InputController * pInput, ScreenShakeController * pscr);
    IntroDoor(float, float, sf::Sprite*, int, float, float);
    sf::Sprite* getSprite();
};

#endif /* Pillar_hpp */
