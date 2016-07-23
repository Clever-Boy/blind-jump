//========================================================================//
// Copyright (C) 2016 Evan Bowman                                         //
//                                                                        //
// This program is free software: you can redistribute it and/or modify   //
// it under the terms of the GNU General Public License as published by   //
// the Free Software Foundation, either version 3 of the License, or      //
// (at your option) any later version.                                    //
//                                                                        //
// This program is distributed in the hope that it will be useful,        //
// but WITHOUT ANY WARRANTY; without even the implied warranty of         //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
// GNU General Public License for more details.                           //
//                                                                        //
// You should have received a copy of the GNU General Public License      //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.  //
//========================================================================//

#include "mappingFunctions.hpp"
#include "initMapVectors.hpp"
#include <cmath>
#include "ResourcePath.hpp"
#include "enemyPlacementFn.hpp"
#include "lightingMap.h"
#include "pillarPlacement.h"
#include "math.h"
#include "enemyCreationFunctions.hpp"
#include "scene.hpp"
#include "easingTemplates.hpp"

Coordinate pickLocation(std::vector<Coordinate>& emptyLocations) {
	int locationSelect = std::abs(static_cast<int>(globalRNG())) % emptyLocations.size();
	Coordinate c = emptyLocations[locationSelect];
	emptyLocations[locationSelect] = emptyLocations.back();
	emptyLocations.pop_back();
	return c;
}

Scene::Scene(float _windowW, float _windowH, InputController * _pInput, FontController * _pFonts)
	: windowW{_windowW},
	  windowH{_windowH},
	  transitionState{TransitionState::None},
	  pInput{_pInput},
	  player{_windowW / 2, _windowH / 2},
	  UI{_windowW / 2, _windowH / 2},
	  tiles{}, // TODO: remove default constructible members
	  en{},
	  pFonts{_pFonts},
	  level{0},
	  stashed{false},
	  preload{false},
	  teleporterCond{false},
	  timer{0}
{
	//===========================================================//
	// Set up post processing effect textures and shapes         //
	//===========================================================//
	target.create(windowW, windowH);
	secondPass.create(windowW, windowH);
	secondPass.setSmooth(true);
	thirdPass.create(windowW, windowH);
	thirdPass.setSmooth(true);
	stash.create(windowW, windowH);
	stash.setSmooth(true);
	lightingMap.create(windowW, windowH);
	shadowShape.setFillColor(sf::Color(190, 190, 210, 255));
	shadowShape.setSize(sf::Vector2f(windowW, windowH));
	vignetteSprite.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::vignette));
	vignetteSprite.setScale(windowW / 450, windowH / 450);
	vignetteShadowSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::vignetteShadow));
	vignetteShadowSpr.setScale(windowW / 450, windowH / 450);
	vignetteShadowSpr.setColor(sf::Color(255,255,255,100));
	
	//===========================================================//
	// Set the views                                             //
	//===========================================================//
	worldView.setSize(windowW, windowH);
	worldView.setCenter(windowW / 2, windowH / 2);
	hudView.setSize(windowW, windowH);
	hudView.setCenter(windowW / 2, windowH / 2);
	
	bkg.giveWindowSize(windowW, windowH);
	
	UI.setView(&worldView);
	
	//Let the tile controller know where player is
	tiles.setPosition((windowW / 2) - 16, (windowH / 2));
	tiles.setWindowSize(windowW, windowH);
	
	// Completely non-general code only used by the intro level
	details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (6 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (0 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (11 * 32), tiles.posY + 200 - 3 + (11 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<2>(tiles.posX - 180 + 16 + (10 * 32), tiles.posY + 200 + 8 + (-9 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
	details.add<4>(tiles.posX - 192 + 6 * 32, tiles.posY + 301,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::introWall));
	sf::Sprite podSpr;
	podSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
	podSpr.setTextureRect(sf::IntRect(164, 145, 44, 50));
	details.add<6>(tiles.posX + 3 * 32, tiles.posY + 4 + 17 * 26, podSpr);;
	tiles.teleporterLocation.x = 8;
	tiles.teleporterLocation.y = -7;
	for (auto it = global_levelZeroWalls.begin(); it != global_levelZeroWalls.end(); ++it) {
		wall w;
		w.setXinit(it->first);
		w.setYinit(it->second);
		tiles.walls.push_back(w);
	}
	
	en.setWindowSize(windowW, windowH);
	
	pFonts->setWaypointText(level);
	
	details.add<0>(tiles.posX - 178 + 8 * 32, tiles.posY + 284 + -7 * 26,
				   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
				   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
	
	bkg.setBkg(0);
	
	sndCtrl.playMusic(0);
	beamGlowTxr.loadFromFile(resourcePath() + "teleporterBeamGlow.png");
	beamGlowSpr.setTexture(beamGlowTxr);
	beamGlowSpr.setPosition(windowW / 2 - 200, windowH / 2 - 200 + 30);
	beamGlowSpr.setColor(sf::Color(0, 0, 0, 255));
	
	transitionShape.setSize(sf::Vector2f(windowW, windowH));
	transitionShape.setFillColor(sf::Color(0, 0, 0, 0));
	
	vignetteSprite.setColor(sf::Color(255, 255, 255));
}

#ifdef DEBUG
template<typename T>
void drawHBox(std::vector<T> & vec, sf::RenderWindow & target) {
	for (auto & element : vec) {
		target.draw(element.getHitBox().getDrawableRect());
	}
}
#endif

void Scene::update(sf::RenderWindow & window, sf::Time & elapsedTime) {
	target.clear(sf::Color::Transparent);

	float xOffset{player.getWorldOffsetX()};
	float yOffset{player.getWorldOffsetY()};
	
	// Blurring is graphics intensive, the game caches frames in a RenderTexture when possible
	if (stashed && UI.getState() != UserInterface::State::statsScreen && UI.getState() != UserInterface::State::menuScreen) {
		stashed = false;
	}
	if (!stashed || preload) {
		bkg.setOffset(xOffset, yOffset);
		bkg.drawBackground(target);
		tiles.setOffset(xOffset, yOffset);
		tiles.drawTiles(target, &glowSprs1, &glowSprs2, level);
		glowSprs2.clear();
	
		details.update(xOffset, yOffset, elapsedTime);
		
		drawGroup(details, gameObjects, gameShadows, glowSprs1, glowSprs2, target);

		// TODO: clean up enemyController::update()...
		en.update(gameObjects, gameShadows, player.getWorldOffsetX(), player.getWorldOffsetY(), effectGroup, tiles.walls, !UI.isOpen(), &tiles, &ssc, *pFonts, elapsedTime);
		if (player.visible) {
			// Draw the player to the window, as long as the object is visible
			player.update(this, elapsedTime);
			player.draw(gameObjects, gameShadows, elapsedTime);
		}
	
		// If player was hit rumble the screen.
		if (player.scrShakeState) {
			ssc.rumble();
		}
	
		glowSprs1.clear();
		if (!UI.isOpen() || (UI.isOpen() && player.getState() == Player::State::dead)) {
			effectGroup.update(xOffset, yOffset, elapsedTime);
		}
		drawGroup(effectGroup, gameObjects, glowSprs1);
	
		// Draw shadows to the target
		if (!gameShadows.empty()) {
			for (auto & element : gameShadows) {
				target.draw(std::get<0>(element));
			}
		}
		gameShadows.clear();
	
		// Sort the game object based on y-position (performance for this is fine, only sorts objects inside the window, on the ordr of 10 in most cases)
		std::sort(gameObjects.begin(), gameObjects.end(), [](const std::tuple<sf::Sprite, float, Rendertype, float> & arg1, const std::tuple<sf::Sprite, float, Rendertype, float> & arg2) {
				return (std::get<1>(arg1) < std::get<1>(arg2));
			});
	
		lightingMap.clear(sf::Color::Transparent);
	
		window.setView(worldView);

		//===========================================================//
		// Object shading                                            //
		//===========================================================//
		if (!gameObjects.empty()) {
			sf::Shader & colorShader = globalResourceHandler.getShader(ResourceHandler::Shader::color);
			for (auto & element : gameObjects) {
				switch (std::get<2>(element)) {
				case Rendertype::shadeDefault:
					std::get<0>(element).setColor(sf::Color(190, 190, 210, std::get<0>(element).getColor().a));
					lightingMap.draw(std::get<0>(element));
					break;
					
				case Rendertype::shadeNone:
					lightingMap.draw(std::get<0>(element));
					break;
					
				case Rendertype::shadeWhite:
					colorShader.setParameter("amount", std::get<3>(element));
					colorShader.setParameter("targetColor", sf::Vector3f(1.00, 1.00, 1.00));
					lightingMap.draw(std::get<0>(element), &colorShader);
					break;
					
				case Rendertype::shadeRed:
					colorShader.setParameter("amount", std::get<3>(element));
					colorShader.setParameter("targetColor", sf::Vector3f(0.98, 0.22, 0.03));
					lightingMap.draw(std::get<0>(element), &colorShader);
					break;
					
				case Rendertype::shadeCrimson:
					colorShader.setParameter("amount", std::get<3>(element));
					colorShader.setParameter("targetColor", sf::Vector3f(0.94, 0.09, 0.34));
					lightingMap.draw(std::get<0>(element), &colorShader);
					break;
					
				case Rendertype::shadeNeon:
					colorShader.setParameter("amount", std::get<3>(element));
					colorShader.setParameter("targetColor", sf::Vector3f(0.29, 0.99, 0.99));
					lightingMap.draw(std::get<0>(element), &colorShader);
					break;
				}
			}
		}
	
		sf::Color blendAmount(185, 185, 185, 255);
		sf::Sprite tempSprite;
		for (auto element : glowSprs2) {
			tempSprite = *element;
			tempSprite.setColor(blendAmount);
			tempSprite.setPosition(tempSprite.getPosition().x, tempSprite.getPosition().y - 12);
			lightingMap.draw(tempSprite, sf::BlendMode(sf::BlendMode(sf::BlendMode::SrcAlpha, sf::BlendMode::One, sf::BlendMode::Add, sf::BlendMode::DstAlpha, sf::BlendMode::Zero, sf::BlendMode::Add)));
		}
	
		lightingMap.display();
		target.draw(sf::Sprite(lightingMap.getTexture()));
	
		// Clear out the vectors for the next round of drawing
		gameObjects.clear();
		bkg.drawForeground(target);
		target.draw(vignetteSprite, sf::BlendMultiply);
		target.draw(vignetteShadowSpr);
		target.display();
	}
	
	//===========================================================//
	// Post Processing Effects                                   //
	//===========================================================//
	if (UI.blurEnabled() && UI.desaturateEnabled()) {
		if (stashed) {
			window.draw(sf::Sprite(stash.getTexture()));
		} else {
			sf::Shader & blurShader = globalResourceHandler.getShader(ResourceHandler::Shader::blur);
			sf::Shader & desaturateShader = globalResourceHandler.getShader(ResourceHandler::Shader::desaturate);
			secondPass.clear(sf::Color::Transparent);
			thirdPass.clear(sf::Color::Transparent);
			sf::Vector2u textureSize = target.getSize();
			// Get the blur amount from the UI controller
			float blurAmount = UI.getBlurAmount();
			blurShader.setParameter("blur_radius", sf::Vector2f(0.f, blurAmount / textureSize.y));
			secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
			secondPass.display();
			blurShader.setParameter("blur_radius", sf::Vector2f(blurAmount / textureSize.x, 0.f));
			thirdPass.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
			thirdPass.display();
			desaturateShader.setParameter("amount", UI.getDesaturateAmount());
			window.draw(sf::Sprite(thirdPass.getTexture()), &desaturateShader);
			if (!stashed && (UI.getState() == UserInterface::State::statsScreen
							 || UI.getState() == UserInterface::State::menuScreen)) {
				stash.clear(sf::Color::Black);
				stash.draw(sf::Sprite(thirdPass.getTexture()), &desaturateShader);
				stash.display();
				stashed = true;
			}
		}
	} else if (UI.blurEnabled() && !UI.desaturateEnabled()) {
		if (stashed) {
			if (pInput->escapePressed()) {
				preload = true;
			}
			window.draw(sf::Sprite(stash.getTexture()));
		} else {
			sf::Shader & blurShader = globalResourceHandler.getShader(ResourceHandler::Shader::blur);
			secondPass.clear(sf::Color::Transparent);
			sf::Vector2u textureSize = target.getSize();
			// Get the blur amount from the UI controller
			float blurAmount = UI.getBlurAmount();
			blurShader.setParameter("blur_radius", sf::Vector2f(0.f, blurAmount / textureSize.y));
			secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
			secondPass.display();
			blurShader.setParameter("blur_radius", sf::Vector2f(blurAmount / textureSize.x, 0.f));
			window.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
			if (!stashed && (UI.getState() == UserInterface::State::statsScreen
							 || UI.getState() == UserInterface::State::menuScreen)) {
				stash.clear(sf::Color::Black);
				stash.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
				stash.display();
				stashed = true;
				preload = false;
			}
		}
	} else if (!UI.blurEnabled() && UI.getDesaturateAmount()) {
		sf::Shader & desaturateShader = globalResourceHandler.getShader(ResourceHandler::Shader::desaturate);
		desaturateShader.setParameter("amount", UI.getDesaturateAmount());
		window.draw(sf::Sprite(target.getTexture()), &desaturateShader);
	} else {
		window.draw(sf::Sprite(target.getTexture()));
	}

	// In debug mode draw visual hitboxes
	#ifdef DEBUG
	drawHBox(effectGroup.get<4>(), window);
	drawHBox(effectGroup.get<5>(), window);
	drawHBox(effectGroup.get<6>(), window);
	drawHBox(effectGroup.get<7>(), window);
	drawHBox(effectGroup.get<8>(), window);
	drawHBox(effectGroup.get<9>(), window);
	window.draw(player.getHitBox().getDrawableRect());
	drawHBox(en.getCritters(), window);
	drawHBox(en.getScoots(), window);
	drawHBox(en.getDashers(), window);
	drawHBox(en.getTurrets(), window);
	#endif

	//===========================================================//
	// UI effects & fonts                                        //
	//===========================================================//
	if (player.getState() == Player::State::dead) {
		UI.dispDeathSeq();
		// If the death sequence is complete and the UI controller is finished playing its animation
		if (UI.isComplete()) {
			// Reset the UI controller
			UI.reset();
			// Reset the player
			player.reset();
			// Reset the map. Reset() increments level, set to -1 so that it will be zero
			level = -1;
			enemySelectVec.clear();
			Reset();
			pFonts->reset();
			pFonts->updateMaxHealth(4, 4);
			pFonts->setWaypointText(level);
		}
		UI.update(window, player, *pFonts, pInput, elapsedTime);
	} else {
		if (transitionState == TransitionState::None) {
			UI.update(window, player, *pFonts, pInput, elapsedTime);
		}
		pFonts->updateHealth(player.getHealth());
		pFonts->print(window);
	}
	// Update the screen shake controller
	ssc.update(player);
	
	window.setView(worldView);

    updateTransitions(xOffset, yOffset, elapsedTime, window);
}

void Scene::updateTransitions(float xOffset, float yOffset, const sf::Time & elapsedTime, sf::RenderWindow & window) {
	switch (transitionState) {
	case TransitionState::None:
		{
			// If the player is near the teleporter, snap to its center and deactivate the player
			float teleporterX = details.get<0>().back().getPosition().x;
			float teleporterY = details.get<0>().back().getPosition().y;
			if ((std::abs(player.getXpos() - teleporterX) < 10 && std::abs(player.getYpos() - teleporterY + 12) < 8)) {
				player.setWorldOffsetX(xOffset + (player.getXpos() - teleporterX) + 2);
				player.setWorldOffsetY(yOffset + (player.getYpos() - teleporterY) + 16);
				player.setState(Player::State::deactivated);
				transitionState = TransitionState::ExitBeamEnter;
			}
			beamShape.setFillColor(sf::Color(114, 255, 229, 6));
			beamShape.setPosition(windowW / 2 - 1.5, windowH / 2 + 36);
			beamShape.setSize(sf::Vector2f(2, 0));
		}
		break;

	case TransitionState::ExitBeamEnter:
		timer += elapsedTime.asMilliseconds();
		{
			float beamHeight = Easing::easeIn<1>(timer, 500) * -(windowH / 2 + 36 /* Magic number, why does it work...? */);
			beamShape.setSize(sf::Vector2f(2, beamHeight));
			uint_fast8_t alpha = Easing::easeIn<1>(timer, 450) * 255;
			beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
		}
		if (timer > 500) {
			timer = 0;
			beamShape.setSize(sf::Vector2f(2, -(windowH / 2 + 36)));
			beamShape.setFillColor(sf::Color(114, 255, 229, 255));
			transitionState = TransitionState::ExitBeamInflate;
		}
		window.draw(beamShape);
		break;

	case TransitionState::ExitBeamInflate:
		timer += elapsedTime.asMilliseconds();
		{
			float beamWidth = std::max(2.f, Easing::easeIn<2>(timer, 250) * 20.f);
			float beamHeight = beamShape.getSize().y;
			beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
			beamShape.setPosition(windowW / 2 - 0.5f - beamWidth / 2.f, windowH / 2 + 36);
		}
		if (timer > 250) {
			timer = 0;
			transitionState = TransitionState::ExitBeamDeflate;
			player.visible = false;
		}
		window.draw(beamShape);
		break;

	case TransitionState::ExitBeamDeflate:
		timer += elapsedTime.asMilliseconds();
		{
			// beamWidth is carefully calibrated, be sure to recalculate the regression based on the timer if you change it...
		    float beamWidth = 0.9999995 * std::exp(-0.0050125355 * static_cast<double>(timer)) * 20.f;
			float beamHeight = beamShape.getSize().y;
			beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
			beamShape.setPosition(windowW / 2 - 0.5f - beamWidth / 2.f, windowH / 2 + 36);
			if (timer >= 640) {
				timer = 0;
				transitionState = TransitionState::TransitionOut;
			}
		}
		window.draw(beamShape);
		break;

	case TransitionState::TransitionOut:
		timer += elapsedTime.asMilliseconds();
		// TODO: On intro level display title text and go to alternate fade out state?
		if (timer > 100) {
			uint_fast8_t alpha = Easing::easeIn<1>(timer - 100, 900) * 255;
			transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
		}
		if (timer >= 1000) {
			transitionState = TransitionState::TransitionIn;
			timer = 0;
			transitionShape.setFillColor(sf::Color(0, 0, 0, 255));
			teleporterCond = true;
		}
		window.draw(transitionShape);
		break;

	case TransitionState::TransitionIn:
	    timer += elapsedTime.asMilliseconds();
		{
			uint_fast8_t alpha = Easing::easeOut<1>(timer, 800) * 255;
			transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
		}
		if (timer >= 800) {
			transitionState = TransitionState::EntryBeamDrop;
			timer = 0;
			beamShape.setSize(sf::Vector2f(4, 0));
			beamShape.setPosition(windowW / 2 - 2, 0);
			beamShape.setFillColor(sf::Color(114, 255, 229, 240));
		}
		window.draw(transitionShape);
		break;

	case TransitionState::EntryBeamDrop:
		timer += elapsedTime.asMilliseconds();
		{
			float beamHeight = Easing::easeIn<2>(timer, 300) * (windowH / 2 + 26);
			beamShape.setSize(sf::Vector2f(4, beamHeight));
		}
		if (timer > 300) {
			transitionState = TransitionState::EntryBeamFade;
			timer = 0;
			player.visible = true;
			ssc.shake();
		}
		window.draw(beamShape);
		break;

	case TransitionState::EntryBeamFade:
		timer += elapsedTime.asMilliseconds();
		{
			uint_fast8_t alpha = Easing::easeOut<1>(timer, 250) * 240;
			beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
		}
		if (timer > 250) {
			transitionState = TransitionState::None;
			player.setState(Player::State::nominal);
			timer = 0;
		}
		window.draw(beamShape);
		break;
	}
}

void Scene::Reset() {
	level += 1;
	pFonts->setWaypointText(level);
	tiles.clear();
	effectGroup.clear();
	details.clear();
	player.setWorldOffsetX(0);
	player.setWorldOffsetY(0);
	en.clear();
	teleporterCond = 0;
	
	set = tileController::Tileset::intro;
	
	if (level == 0) {
		set = tileController::Tileset::intro;
	} else {
	    // /if (std::abs(static_cast<int>(globalRNG())) % 2) {
		// 	set = tileController::Tileset::nova;
		// 	objectShadeColor = sf::Color(210, 195, 195, 255);
		// } else {
			set = tileController::Tileset::regular;
		// }
	}
	
	vignetteSprite.setColor(sf::Color(255, 255, 255, 255));
	
	if (set != tileController::Tileset::intro) {
		//Now call the mapping function again to generate a new map, and make sure it's large enough
		int count = mappingFunction(tiles.mapArray, level, set != tileController::Tileset::nova);
			while (count < 150) {
			count = mappingFunction(tiles.mapArray, level, set != tileController::Tileset::nova);
		}
	}
	
	tiles.rebuild(set);
	bkg.setBkg(tiles.getWorkingSet());
	tiles.setPosition((windowW / 2) - 16, (windowH / 2));
	bkg.setPosition((tiles.posX / 2) + 206, tiles.posY / 2);

	if (level != 0) {
		Coordinate c = tiles.getTeleporterLoc();
		details.add<0>(tiles.posX + c.x * 32 + 2, tiles.posY + c.y * 26 - 4,
				   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
				   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
		
		initEnemies(this);
	
		// 50 / 50 : place an item chest
		if (std::abs(static_cast<int>(globalRNG())) % 2) {
			Coordinate c = pickLocation(tiles.emptyMapLocations);
			details.add<1>(c.x * 32 + tiles.posX, c.y * 26 + tiles.posY,
						   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects), 0);
		}
		
		glowSprs1.clear();
		glowSprs2.clear();
		
		if (set == tileController::Tileset::regular) {
			// Put rock/pillar detail things on map
			getRockPositions(tiles.mapArray, rockPositions);
			for (auto element : rockPositions) {
				details.add<3>(tiles.posX + 32 * element.x, tiles.posY + 26 * element.y - 35,
							   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
			}
			rockPositions.clear();
			// // TODO: Delete rocks close to the teleporter
			// std::vector<Rock> & rocks = details.get<3>();
			// for (auto it = rocks.begin(); it != rocks.end();) {
			// 	if (fabsf(it->getPosition().x - teleporter.getPosition().x) < 80 && fabsf(it->getPosition().y - teleporter.getPosition().y) < 80) {
			// 		it = rocks.erase(it);
			// 	} else {
			// 		++it;
			// 	}
			// }
		}
		
		// Put light sources on the map
		getLightingPositions(tiles.mapArray, lightPositions);
		size_t len = lightPositions.size();
		for (size_t i = 0; i < len; i++) {
			details.add<2>(tiles.posX + 16 + (lightPositions[i].x * 32), tiles.posY - 3 + (lightPositions[i].y * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		}
		lightPositions.clear();
	} else if (set == tileController::Tileset::intro) {
		details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (6 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (5 * 32), tiles.posY + 200 - 3 + (0 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (11 * 32), tiles.posY + 200 - 3 + (11 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<2>(tiles.posX - 180 + 16 + (10 * 32), tiles.posY + 200 + 8 + (-9 * 26),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::lamplight));
		details.add<4>(tiles.posX - 192 + 6 * 32, tiles.posY + 301,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::introWall));
		sf::Sprite podSpr;
		podSpr.setTexture(globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects));
		podSpr.setTextureRect(sf::IntRect(164, 145, 44, 50));
		details.add<6>(tiles.posX + 3 * 32, tiles.posY + 4 + 17 * 26, podSpr);;
		tiles.teleporterLocation.x = 8;
		tiles.teleporterLocation.y = -7;
		details.add<0>(tiles.posX - 178 + 8 * 32, tiles.posY + 284 + -7 * 26,
					   globalResourceHandler.getTexture(ResourceHandler::Texture::gameObjects),
					   globalResourceHandler.getTexture(ResourceHandler::Texture::teleporterGlow));
		for (auto it = global_levelZeroWalls.begin(); it != global_levelZeroWalls.end(); ++it) {
			wall w;
			w.setXinit(it->first);
			w.setYinit(it->second);
			tiles.walls.push_back(w);
		}
	}
}

DetailGroup & Scene::getDetails() {
	return details;
}

enemyController & Scene::getEnemyController() {
	return en;
}

tileController & Scene::getTileController() {
	return tiles;
}

Player & Scene::getPlayer() {
	return player;
}

EffectGroup & Scene::getEffects() {
	return effectGroup;
}

InputController * Scene::getPInput() {
	return pInput;
}

ScreenShakeController * Scene::getPSSC() {
	return &ssc;
}

UserInterface & Scene::getUI() {
	return UI;
}

FontController * Scene::getPFonts() {
	return pFonts;
}

bool Scene::getTeleporterCond() {
	return teleporterCond;
}

std::vector<std::pair<int, int>>* Scene::getEnemySelectVec() {
	return &enemySelectVec;
}

int Scene::getLevel() {
	return level;
}

