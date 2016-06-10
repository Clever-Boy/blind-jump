//
//  effectsController.hpp
//  Blind Jump
//
//  Created by Evan Bowman on 10/20/15.
//  Copyright © 2015 Evan Bowman. All rights reserved.
//

#pragma once
#ifndef effectsController_hpp
#define effectsController_hpp

#include <stdio.h>
#include "SFML/Graphics.hpp"
#include "turretFlashEffect.hpp"
#include "bulletType1.hpp"
#include "shotPuff.hpp"
#include "turretShot.hpp"
#include "explosion32effect.hpp"
#include "teleporterSmoke.hpp"
#include "enemyShot.hpp"
#include "dasherShot.hpp"
#include "healthEffect.hpp"
#include "fontController.hpp"
#include "energyBeam.hpp"
#include "dashSmoke.hpp"
#include "FireExplosion.hpp"
#include "smallExplosion.hpp"
#include "Powerup.hpp"
#include "ResourcePath.hpp"
#include "RenderType.hpp"
#include "textureManager.hpp"

class ScreenShakeController;

class effectsController {
private:
	sf::Texture fireExplosionGlowTxtr;
	sf::Sprite blueFireGlowSpr;
	sf::Sprite energyBeamSprites[6];
	sf::Texture energyBeamTextures[6];
	sf::Sprite bulletSprites[3];
	sf::Texture bulletTexture[2];
	sf::Texture exp32Texture[6];
	sf::Sprite exp32Sprites[6];
	sf::Sprite warpEffectSprites[6];
	sf::Texture warpEffectTextures[6];
	sf::Sprite smokeSprites[6];
	sf::Texture smokeTextures[6];
	TextureManager * pTM;
	std::vector<SmallExplosion> smallExplosions;
	std::vector<EnergyBeam> energyBeams;
	std::vector<FireExplosion> fireExplosions;
	std::vector<Enemyshot> enemyShots;
	std::vector<turretFlashEffect> turretFlashes;
	std::vector<bulletType1> bullets;
	std::vector<shotPuff> puffs;
	std::vector<bulletType1> bulletLowerLayer;
	std::vector<turretShot> turretShots;
	std::vector<TeleporterSmoke> warpEffects;
	std::vector<DasherShot> dasherShots;
	std::vector<sf::Sprite*> glowSprs;
	std::vector<sf::Sprite*> glowSprs2;
	std::vector<Powerup> hearts;
	std::vector<Powerup> coins;
	
public:
	effectsController();
	void draw(sf::RenderTexture&, std::vector<std::tuple<sf::Sprite, float, Rendertype, float>>&);
	void update(float, float, ScreenShakeController*, sf::Time &);
	void addTurretFlash(float, float);
	void addBullet(bool, char, float, float);
	void drawLower(sf::RenderTexture&);
	void clear();
	void addTurretShot(float, float, short);
	void addDasherShot(float, float, short);
	void addScootShot(float, float, short, float, float);
	void addWarpEffect(float, float);
	void addFireExplosion(float, float);
	void addExplosion(float, float);
	void addSmallExplosion(float, float);
	void addHearts(float, float);
	void addCoins(float, float);
	void addPlayerHealthEffect(float, float, float, float);
	void addHpRestored(float, float);
	void addNewItem(float, float);
	void addPuff(float, float);
	void addEnergyBeam(float, float, float, float);
	void addMissile(float, float);
	void addOrbshotTrail(float, float);
	void addEnemyShot(float, float, short);
	void addOrbShot(unsigned char, float, float, float, float);
	std::vector<bulletType1>& getBulletLayer1();
	std::vector<bulletType1>& getBulletLayer2();
	void addSmokeEffect(float, float);
	// Now accessor functions for the class' private member datafields
	std::vector<Enemyshot>* getEnemyShots();
	std::vector<Powerup>* getHearts();
	std::vector<Powerup>* getCoins();
	std::vector<turretShot>* getTurretShots();
	std::vector<DasherShot>* getDasherShots();
	std::vector<sf::Sprite*>* getGlowSprs();
	std::vector<sf::Sprite*>* getGlowSprs2();
	std::vector<FireExplosion>* getExplosions();
	void condClearGlowSpr(sf::Sprite*);
	void setTextureManager(TextureManager * pTM);
};

#endif /* effectsController_hpp */
