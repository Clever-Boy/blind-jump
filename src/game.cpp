//========================================================================//
// Copyright (C) 2016 Evan Bowman                                         //
// Liscensed under GPL 3, see: <http://www.gnu.org/licenses/>.            //
//========================================================================//

#include "game.hpp"
#include "ResourcePath.hpp"
#include "easingTemplates.hpp"
#include "initMapVectors.hpp"
#include "lightingMap.hpp"
#include "mappingFunctions.hpp"
#include "math.h"
#include "pillarPlacement.h"

Game::Game(const ConfigData & conf)
    : viewPort(conf.drawableArea),
      transitionState(TransitionState::TransitionIn), slept(false),
      window(sf::VideoMode::getDesktopMode(), EXECUTABLE_NAME,
             sf::Style::Fullscreen, sf::ContextSettings(0, 0, 6)),
      camera(viewPort, window.getSize()),
      uiFrontend(
          sf::View(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y)),
          viewPort.x / 2, viewPort.y / 2),
      hasFocus(false), level(0), stashed(false), preload(false),
      worldView(sf::Vector2f(viewPort.x / 2, viewPort.y / 2), viewPort),
      timer(0) {
    sf::View windowView;
    static const float visibleArea = 0.75f;
    const sf::Vector2f vignetteMaskScale(
        (viewPort.x * (visibleArea + 0.02)) / 450,
        (viewPort.y * (visibleArea + 0.02)) / 450);
    vignetteSprite.setScale(vignetteMaskScale);
    vignetteShadowSpr.setScale(vignetteMaskScale);
    windowView.setSize(window.getSize().x, window.getSize().y);
    windowView.zoom(visibleArea);
    camera.setWindowView(windowView);
    gfxContext.targetRef = &target;
    window.setMouseCursorVisible(conf.showMouse);
    window.setFramerateLimit(conf.framerateLimit);
    window.setVerticalSyncEnabled(conf.enableVsync);
    // TODO: refactor out global pointer! (only needed for old C++ logic)
    setgResHandlerPtr(&resHandler);
}

void Game::init() {
    tiles.init();
    uiFrontend.init();
    bkg.init();
    target.create(viewPort.x, viewPort.y);
    secondPass.create(viewPort.x, viewPort.y);
    secondPass.setSmooth(true);
    thirdPass.create(viewPort.x, viewPort.y);
    thirdPass.setSmooth(true);
    stash.create(viewPort.x, viewPort.y);
    stash.setSmooth(true);
    lightingMap.create(viewPort.x, viewPort.y);
    vignetteSprite.setTexture(
        getgResHandlerPtr()->getTexture("textures/vignetteMask.png"));
    vignetteShadowSpr.setTexture(
        getgResHandlerPtr()->getTexture("textures/vignetteShadow.png"));
    beamGlowSpr.setTexture(
        getgResHandlerPtr()->getTexture("textures/teleporterBeamGlow.png"));
    vignetteShadowSpr.setColor(sf::Color(255, 255, 255, 100));
    hudView.setSize(viewPort.x, viewPort.y);
    hudView.setCenter(viewPort.x / 2, viewPort.y / 2);
    bkg.giveWindowSize(viewPort.x, viewPort.y);
    tiles.setPosition((viewPort.x / 2) - 16, (viewPort.y / 2));
    tiles.setWindowSize(viewPort.x, viewPort.y);
    beamGlowSpr.setColor(sf::Color(0, 0, 0, 255));
    transitionShape.setSize(sf::Vector2f(viewPort.x, viewPort.y));
    transitionShape.setFillColor(sf::Color(0, 0, 0, 0));
    vignetteSprite.setColor(sf::Color::White);
    level = -1;
    this->nextLevel();
}

void Game::eventLoop() {
    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Closed:
            window.close();
            throw ShutdownSignal();
            break;

        case sf::Event::GainedFocus:
            sounds.unpause(SoundController::Sound | SoundController::Music);
            this->hasFocus = true;
            break;

        case sf::Event::LostFocus:
            sounds.pause(SoundController::Sound | SoundController::Music);
            this->hasFocus = false;
            break;

        default:
            input.recordEvent(event);
            break;
        }
    }
}

void Game::updateGraphics() {
    window.clear();
    if (!hasFocus) {
        this->setSleep(std::chrono::microseconds(200000));
        return;
    }
    target.clear(sf::Color::Transparent);
    if (!stashed || preload) {
        {
            std::lock_guard<std::mutex> overworldLock(overworldMutex);
            lightingMap.setView(camera.getOverworldView());
            bkg.drawBackground(target, worldView, camera);
            tiles.draw(target, &gfxContext.glowSprs1, level, worldView,
                       camera.getOverworldView());
            gfxContext.glowSprs2.clear();
            gfxContext.glowSprs1.clear();
            gfxContext.shadows.clear();
            gfxContext.faces.clear();
            target.setView(camera.getOverworldView());
            for (auto & kvp : entityTable) {
                for (auto & entity : kvp.second) {
                    auto fg = entity->getSheet();
                    if (fg) {
                        fg->setFrame(entity->getKeyframe());
                        fg->getSprite().setPosition(entity->getPosition());
                        gfxContext.faces.emplace_back(
                            fg->getSprite(), entity->getZOrder(),
                            // TODO: add shading options to
                            //       lua API
                            Rendertype::shadeDefault, 0.f);
                    }
                }
            }
	    for (auto & light : lights) {
	        auto sheet = light.getSheet();
		sheet->getSprite().setPosition(light.getPosition());
		gfxContext.glowSprs1.push_back(sheet->getSprite());
		gfxContext.glowSprs2.push_back(sheet->getSprite());
	    }
            sounds.update();
        }
        if (!gfxContext.shadows.empty()) {
            for (const auto & element : gfxContext.shadows) {
                target.draw(std::get<0>(element));
            }
        }
        target.setView(worldView);
        lightingMap.clear(sf::Color::Transparent);
        static const size_t zOrderIdx = 1;
        std::sort(
            gfxContext.faces.begin(), gfxContext.faces.end(),
            [](const drawableMetadata & arg1, const drawableMetadata & arg2) {
                return (std::get<zOrderIdx>(arg1) < std::get<zOrderIdx>(arg2));
            });
        static const size_t sprIdx = 0;
        static const size_t shaderIdx = 3;
        sf::Shader & colorShader =
            getgResHandlerPtr()->getShader("shaders/color.frag");
        for (auto & element : gfxContext.faces) {
            switch (std::get<2>(element)) {
            case Rendertype::shadeDefault:
                std::get<0>(element).setColor(sf::Color(
                    190, 190, 210, std::get<sprIdx>(element).getColor().a));
                lightingMap.draw(std::get<sprIdx>(element));
                break;

            case Rendertype::shadeNone:
                lightingMap.draw(std::get<sprIdx>(element));
                break;

            case Rendertype::shadeWhite: {
                DEF_GLSL_COLOR(colors::White, White);
                colorShader.setUniform("amount", std::get<shaderIdx>(element));
                colorShader.setUniform("targetColor", White);
                lightingMap.draw(std::get<sprIdx>(element), &colorShader);
            } break;

            case Rendertype::shadeGldnGt: {
                DEF_GLSL_COLOR(colors::GldnGt, GldnGt);
                colorShader.setUniform("amount", std::get<shaderIdx>(element));
                colorShader.setUniform("targetColor", GldnGt);
                lightingMap.draw(std::get<sprIdx>(element), &colorShader);
            } break;

            case Rendertype::shadeRuby: {
                DEF_GLSL_COLOR(colors::Ruby, Ruby);
                colorShader.setUniform("amount", std::get<shaderIdx>(element));
                colorShader.setUniform("targetColor", Ruby);
                lightingMap.draw(std::get<sprIdx>(element), &colorShader);
            } break;

            case Rendertype::shadeElectric: {
                DEF_GLSL_COLOR(colors::Electric, Electric);
                colorShader.setUniform("amount", std::get<shaderIdx>(element));
                colorShader.setUniform("targetColor", Electric);
                lightingMap.draw(std::get<sprIdx>(element), &colorShader);
            } break;
            }
        }
        static const sf::Color blendAmount(185, 185, 185);
        sf::Sprite tempSprite;
        for (auto & element : gfxContext.glowSprs2) {
            element.setColor(blendAmount);
            lightingMap.draw(element,
                             sf::BlendMode(sf::BlendMode(
                                 sf::BlendMode::SrcAlpha, sf::BlendMode::One,
                                 sf::BlendMode::Add, sf::BlendMode::DstAlpha,
                                 sf::BlendMode::Zero, sf::BlendMode::Add)));
        }
        lightingMap.display();
        target.draw(sf::Sprite(lightingMap.getTexture()));
        target.setView(camera.getOverworldView());
        bkg.drawForeground(target);
        target.setView(worldView);
        sf::Vector2f fgMaskPos(
            viewPort.x * 0.115f + camera.getOffsetFromTarget().x * 0.75f,
            viewPort.y * 0.115f + camera.getOffsetFromTarget().y * 0.75f);
        vignetteSprite.setPosition(fgMaskPos);
        vignetteShadowSpr.setPosition(fgMaskPos);
        target.draw(vignetteSprite, sf::BlendMultiply);
        target.draw(vignetteShadowSpr);
        target.display();
    }
    const sf::Vector2u windowSize = window.getSize();
    const sf::Vector2f upscaleVec(windowSize.x / viewPort.x,
                                  windowSize.y / viewPort.y);
    if (UI.blurEnabled() && UI.desaturateEnabled()) {
        if (stashed) {
            sf::Sprite targetSprite(stash.getTexture());
            window.setView(camera.getWindowView());
            targetSprite.setScale(upscaleVec);
            window.draw(targetSprite);
        } else {
            sf::Shader & blurShader =
                getgResHandlerPtr()->getShader("shaders/blur.frag");
            sf::Shader & desaturateShader =
                getgResHandlerPtr()->getShader("shaders/desaturate.frag");
            secondPass.clear(sf::Color::Transparent);
            thirdPass.clear(sf::Color::Transparent);
            const sf::Vector2u textureSize = target.getSize();
            float blurAmount = UI.getBlurAmount();
            const sf::Glsl::Vec2 vBlur =
                sf::Glsl::Vec2(0.f, blurAmount / textureSize.y);
            blurShader.setUniform("blur_radius", vBlur);
            secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
            secondPass.display();
            const sf::Glsl::Vec2 hBlur =
                sf::Glsl::Vec2(blurAmount / textureSize.x, 0.f);
            blurShader.setUniform("blur_radius", hBlur);
            thirdPass.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
            thirdPass.display();
            desaturateShader.setUniform("amount", UI.getDesaturateAmount());
            sf::Sprite targetSprite(thirdPass.getTexture());
            window.setView(camera.getWindowView());
            targetSprite.setScale(upscaleVec);
            window.draw(targetSprite, &desaturateShader);
            if (!stashed && (UI.getState() == ui::Backend::State::statsScreen ||
                             UI.getState() == ui::Backend::State::menuScreen) &&
                !camera.moving()) {
                stash.clear(sf::Color::Black);
                stash.draw(sf::Sprite(thirdPass.getTexture()),
                           &desaturateShader);
                stash.display();
                stashed = true;
            }
        }
    } else if (UI.blurEnabled() && !UI.desaturateEnabled()) {
        if (stashed) {
            if (input.pausePressed()) {
                preload = true;
            }
            sf::Sprite targetSprite(stash.getTexture());
            window.setView(camera.getWindowView());
            targetSprite.setScale(upscaleVec);
            window.draw(targetSprite);
        } else {
            sf::Shader & blurShader =
                getgResHandlerPtr()->getShader("shaders/blur.frag");
            secondPass.clear(sf::Color::Transparent);
            sf::Vector2u textureSize = target.getSize();
            float blurAmount = UI.getBlurAmount();
            const sf::Glsl::Vec2 vBlur =
                sf::Glsl::Vec2(0.f, blurAmount / textureSize.y);
            blurShader.setUniform("blur_radius", vBlur);
            secondPass.draw(sf::Sprite(target.getTexture()), &blurShader);
            secondPass.display();
            const sf::Glsl::Vec2 hBlur =
                sf::Glsl::Vec2(blurAmount / textureSize.x, 0.f);
            blurShader.setUniform("blur_radius", hBlur);
            sf::Sprite targetSprite(secondPass.getTexture());
            window.setView(camera.getWindowView());
            targetSprite.setScale(upscaleVec);
            window.draw(targetSprite, &blurShader);
            if (!stashed && (UI.getState() == ui::Backend::State::statsScreen ||
                             UI.getState() == ui::Backend::State::menuScreen) &&
                !camera.moving()) {
                stash.clear(sf::Color::Black);
                stash.draw(sf::Sprite(secondPass.getTexture()), &blurShader);
                stash.display();
                stashed = true;
                preload = false;
            }
        }
    } else if (!UI.blurEnabled() && UI.desaturateEnabled()) {
        sf::Shader & desaturateShader =
            getgResHandlerPtr()->getShader("shaders/desaturate.frag");
        desaturateShader.setUniform("amount", UI.getDesaturateAmount());
        sf::Sprite targetSprite(target.getTexture());
        window.setView(camera.getWindowView());
        targetSprite.setScale(upscaleVec);
        window.draw(targetSprite, &desaturateShader);
    } else {
        sf::Sprite targetSprite(target.getTexture());
        window.setView(camera.getWindowView());
        targetSprite.setScale(upscaleVec);
        window.draw(targetSprite);
    }
    {
        std::lock_guard<std::mutex> UILock(UIMutex);
        if (transitionState == TransitionState::None) {
            UI.draw(window, uiFrontend);
        }
        uiFrontend.draw(window);
    }
    window.setView(worldView);
    drawTransitions(window);
    window.display();
}

void Game::updateLogic(LuaProvider & luaProv) {
    if (!hasFocus) {
        this->setSleep(std::chrono::microseconds(200));
        return;
    }
    // Blurring is graphics intensive, the game caches frames in a RenderTexture
    // when possible
    if (stashed && UI.getState() != ui::Backend::State::statsScreen &&
        UI.getState() != ui::Backend::State::menuScreen) {
        stashed = false;
    }
    if (!stashed || preload) {
        std::lock_guard<std::mutex> overworldLock(overworldMutex);
        if (level != 0) {
            const sf::Vector2f & cameraOffset = camera.getOffsetFromStart();
            bkg.setOffset(cameraOffset.x, cameraOffset.y);
        } else { // TODO: why is this necessary...?
            bkg.setOffset(0, 0);
        }
        tiles.update();
        luaProv.applyHook([this](lua_State * state) {
            lua_getglobal(state, "classes");
            if (!lua_istable(state, -1)) {
                throw std::runtime_error("Error: missing classtable");
            }
            for (auto & kvp : this->entityTable) {
                lua_getfield(state, -1, kvp.first.c_str());
                if (!lua_istable(state, -1)) {
                    const std::string err = "Error: classtable field " +
                                            kvp.first + " is not a table";
                    throw std::runtime_error(err);
                }
                for (auto it = kvp.second.begin(); it != kvp.second.end();) {
                    lua_getfield(state, -1, "onUpdate");
                    if (!lua_isfunction(state, -1)) {
                        const std::string err =
                            "Error: missing or malformed OnUpdate for class " +
                            kvp.first;
                        throw std::runtime_error(err);
                    }
                    lua_pushlightuserdata(state, (void *)(&(*it)));
                    if (lua_pcall(state, 1, 0, 0)) {
                        throw std::runtime_error(lua_tostring(state, -1));
                    }
                    if ((*it)->getKillFlag()) {
                        for (auto & member : (*it)->getMemberTable()) {
                            luaL_unref(state, LUA_REGISTRYINDEX, member.second);
                        }
                        it = kvp.second.erase(it);
                    } else {
                        ++it;
                    }
                }
                lua_pop(state, 1);
            }
            lua_pop(state, 1);
        });
        std::vector<sf::Vector2f> cameraTargets;
        camera.update(elapsedTime, cameraTargets);
    }
    {
        std::lock_guard<std::mutex> UILock(UIMutex);
        if (transitionState == TransitionState::None) {
            UI.update(this, elapsedTime);
        }
    }
    updateTransitions(elapsedTime);
}

void Game::drawTransitions(sf::RenderWindow & window) {
    std::lock_guard<std::mutex> grd(transitionMutex);
    switch (transitionState) {
    case TransitionState::None:
        break;

    case TransitionState::ExitBeamEnter:
        window.draw(beamShape);
        gfxContext.glowSprs1.push_back(beamGlowSpr);
        break;

    case TransitionState::ExitBeamInflate:
        window.draw(beamShape);
        gfxContext.glowSprs1.push_back(beamGlowSpr);
        break;

    case TransitionState::ExitBeamDeflate:
        window.draw(beamShape);
        gfxContext.glowSprs1.push_back(beamGlowSpr);
        break;

    // This isn't stateless, but only because it can't be. Reseting the level
    // involves texture creation, which is graphics code and therefore needs to
    // happen on the main thread for portability reasons.
    case TransitionState::TransitionOut:
        if (level != 0) {
            if (timer > 100000) {
                uint8_t alpha =
                    math::smoothstep(0.f, 900000, timer - 100000) * 255;
                transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
                window.draw(transitionShape);
            }
            if (timer > 1000000) {
                transitionState = TransitionState::TransitionIn;
                timer = 0;
                transitionShape.setFillColor(sf::Color(0, 0, 0, 255));
                beamGlowSpr.setColor(sf::Color::Black);
                this->nextLevel(); // Creates textures, needs to happen on main
                                   // thread
            }
        } else {
            if (timer > 1600000) {
                uint8_t alpha =
                    math::smoothstep(0.f, 1400000, timer - 1600000) * 255;
                transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
                window.draw(transitionShape);
                uint8_t textAlpha =
                    Easing::easeOut<1>(timer - 1600000,
                                       static_cast<int_fast64_t>(600000)) *
                    255;
                uiFrontend.drawTitle(textAlpha, window);
            } else {
                uiFrontend.drawTitle(255, window);
            }
            if (timer > 3000000) {
                transitionState = TransitionState::TransitionIn;
                beamGlowSpr.setColor(sf::Color::Black);
                timer = 0;
                transitionShape.setFillColor(sf::Color(0, 0, 0, 255));
                this->nextLevel();
            }
        }
        break;

    case TransitionState::TransitionIn:
        window.draw(transitionShape);
        break;

    case TransitionState::EntryBeamDrop:
        window.draw(beamShape);
        gfxContext.glowSprs1.push_back(beamGlowSpr);
        break;

    case TransitionState::EntryBeamFade:
        window.draw(beamShape);
        gfxContext.glowSprs1.push_back(beamGlowSpr);
        break;
    }
}

void Game::updateTransitions(const sf::Time & elapsedTime) {
    std::lock_guard<std::mutex> grd(transitionMutex);
    switch (transitionState) {
    case TransitionState::None: {
        beamShape.setPosition(viewPort.x / 2 - 1.5, viewPort.y / 2 + 48);
        beamShape.setFillColor(sf::Color(114, 255, 229, 6));
        beamShape.setSize(sf::Vector2f(2, 0));
        beamGlowSpr.setPosition(viewPort.x / 2 - 200,
                                viewPort.y / 2 - 200 + 36);
    } break;

    case TransitionState::ExitBeamEnter:
        timer += elapsedTime.asMicroseconds();
        {
            static const int_fast64_t transitionTime = 500000;
            static const int_fast64_t alphaTransitionTime = 450000;
            const int beamTargetY = -(viewPort.y / 2 + 48);
            float beamHeight =
                Easing::easeIn<1>(timer, transitionTime) * beamTargetY;
            uint8_t brightness = Easing::easeIn<1>(timer, transitionTime) * 255;
            uint_fast8_t alpha =
                Easing::easeIn<1>(timer, alphaTransitionTime) * 255;
            beamGlowSpr.setColor(
                sf::Color(brightness, brightness, brightness, 255));
            beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
            beamShape.setSize(sf::Vector2f(2, beamHeight));
            if (timer > transitionTime) {
                timer = 0;
                beamShape.setSize(sf::Vector2f(2, -(viewPort.y / 2 + 48)));
                beamShape.setFillColor(sf::Color(114, 255, 229, 255));
                transitionState = TransitionState::ExitBeamInflate;
            }
        }
        break;

    case TransitionState::ExitBeamInflate:
        timer += elapsedTime.asMicroseconds();
        {
            static const int64_t transitionTime = 250000;
            float beamWidth =
                std::max(2.f, Easing::easeIn<2>(timer, transitionTime) * 20.f);
            float beamHeight = beamShape.getSize().y;
            beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
            beamShape.setPosition(viewPort.x / 2 - 0.5f - beamWidth / 2.f,
                                  viewPort.y / 2 + 48);
            if (timer > transitionTime) {
                timer = 0;
                transitionState = TransitionState::ExitBeamDeflate;
            }
        }
        break;

    case TransitionState::ExitBeamDeflate:
        timer += elapsedTime.asMilliseconds();
        {
            // beamWidth is carefully calibrated, be sure to recalculate the
            // regression based on the timer if you change it...
            float beamWidth =
                0.9999995 *
                std::exp(-0.0050125355 * static_cast<double>(timer)) * 20.f;
            float beamHeight = beamShape.getSize().y;
            beamShape.setSize(sf::Vector2f(beamWidth, beamHeight));
            beamShape.setPosition(viewPort.x / 2 - 0.5f - beamWidth / 2.f,
                                  viewPort.y / 2 + 48);
            if (timer >= 640) {
                timer = 0;
                transitionState = TransitionState::TransitionOut;
            }
        }
        break;

    case TransitionState::TransitionOut:
        timer += elapsedTime.asMicroseconds();
        // Logic updates instead when drawing transitions, see above comment.
        break;

    case TransitionState::TransitionIn:
        timer += elapsedTime.asMicroseconds();
        {
            uint8_t alpha = 255 - math::smoothstep(0.f, 800000, timer) * 255;
            transitionShape.setFillColor(sf::Color(0, 0, 0, alpha));
        }
        if (timer >= 800000) {
            if (level != 0) {
                transitionState = TransitionState::EntryBeamDrop;
            } else {
                transitionState = TransitionState::None;
            }
            timer = 0;
            beamShape.setSize(sf::Vector2f(4, 0));
            beamShape.setPosition(viewPort.x / 2 - 2, 0);
            beamShape.setFillColor(sf::Color(114, 255, 229, 240));
        }
        break;

    case TransitionState::EntryBeamDrop:
        timer += elapsedTime.asMicroseconds();
        {
            static const int64_t transitionTime = 350000;
            float beamHeight = Easing::easeIn<2>(timer, transitionTime) *
                               (viewPort.y / 2 + 36);
            uint8_t brightness = Easing::easeIn<1>(timer, transitionTime) * 255;
            beamGlowSpr.setColor(
                sf::Color(brightness, brightness, brightness, 255));
            beamShape.setSize(sf::Vector2f(4, beamHeight));
            if (timer > transitionTime) {
                transitionState = TransitionState::EntryBeamFade;
                timer = 0;
                this->setSleep(std::chrono::microseconds(20000));
                camera.shake(0.19f);
            }
        }
        break;

    case TransitionState::EntryBeamFade:
        timer += elapsedTime.asMicroseconds();
        {
            static const int64_t transitionTime = 300000;
            uint8_t alpha = Easing::easeOut<1>(timer, transitionTime) * 240;
            beamShape.setFillColor(sf::Color(114, 255, 229, alpha));
            uint8_t brightness =
                Easing::easeOut<2>(timer, transitionTime) * 255;
            beamGlowSpr.setColor(
                sf::Color(brightness, brightness, brightness, 255));
            if (timer > transitionTime) {
                transitionState = TransitionState::None;
                timer = 0;
            }
        }
        break;
    }
}

void Game::nextLevel() {
    level += 1;
    uiFrontend.setWaypointText(level);
    tiles.clear();
    if (level == 0) {
        set = tileController::Tileset::intro;
    } else {
        camera.setOffset({1.f, 2.f});
        set = tileController::Tileset::regular;
    }
    if (set != tileController::Tileset::intro) {
        int count;
        do {
            count = generateMap(tiles.mapArray);
        } while (count < 150);
    }
    tiles.rebuild(set);
    bkg.setBkg(tiles.getWorkingSet());
    tiles.setPosition((viewPort.x / 2) - 16, (viewPort.y / 2));
    bkg.setPosition((tiles.posX / 2) + 206, tiles.posY / 2);
    auto pickLocation = [](std::vector<Coordinate> & emptyLocations)
        -> framework::option<Coordinate> {
        if (emptyLocations.size() > 0) {
            int locationSelect = rng::random(emptyLocations.size());
            Coordinate c = emptyLocations[locationSelect];
            emptyLocations[locationSelect] = emptyLocations.back();
            emptyLocations.pop_back();
            return framework::option<Coordinate>(c);
        } else {
            return {};
        }
    };
    if (level != 0) {
        Coordinate c = tiles.getTeleporterLoc();
        auto optCoord = pickLocation(tiles.emptyMapLocations);
        if (!rng::random<2>()) {
            auto pCoordVec = tiles.getEmptyLocations();
            const size_t vecSize = pCoordVec->size();
            const int locationSel = rng::random(vecSize / 3);
            const int xInit = (*pCoordVec)[vecSize - 1 - locationSel].x;
            const int yInit = (*pCoordVec)[vecSize - 1 - locationSel].y;
        }
        gfxContext.glowSprs1.clear();
        gfxContext.glowSprs2.clear();
        Circle teleporterFootprint;
        teleporterFootprint.x = tiles.getTeleporterLoc().x;
        teleporterFootprint.y = tiles.getTeleporterLoc().y;
        teleporterFootprint.r = 50;
    } else if (set == tileController::Tileset::intro) {
        static const int initTeleporterLocX = 8;
        static const int initTeleporterLocY = -7;
        tiles.teleporterLocation.x = initTeleporterLocX;
        tiles.teleporterLocation.y = initTeleporterLocY;
        for (auto it = ::levelZeroWalls.begin(); it != ::levelZeroWalls.end();
             ++it) {
            wall w;
            w.setXinit(it->first);
            w.setYinit(it->second);
            tiles.walls.push_back(w);
        }
    }
}

bool Game::hasSlept() const { return slept; }

void Game::clearSleptFlag() { slept = false; }

void Game::setSleep(const std::chrono::microseconds & time) {
    slept = true;
    std::this_thread::sleep_for(time);
}

tileController & Game::getTileController() { return tiles; }

Camera & Game::getCamera() { return camera; }

InputController & Game::getInputController() { return input; }

ui::Backend & Game::getUI() { return UI; }

ui::Frontend & Game::getUIFrontend() { return uiFrontend; }

SoundController & Game::getSounds() { return sounds; }

int Game::getLevel() { return level; }

sf::RenderWindow & Game::getWindow() { return window; }

ResHandler & Game::getResHandler() { return resHandler; }

const std::array<std::pair<float, float>, 59> levelZeroWalls{
    {std::make_pair(-20, 500), std::make_pair(-20, 526),
     std::make_pair(-20, 474), std::make_pair(-20, 448),
     std::make_pair(-20, 422), std::make_pair(-20, 396),
     std::make_pair(-20, 370), std::make_pair(-20, 552),
     std::make_pair(-20, 578), std::make_pair(196, 500),
     std::make_pair(196, 526), std::make_pair(196, 474),
     std::make_pair(196, 448), std::make_pair(196, 422),
     std::make_pair(196, 396), std::make_pair(196, 370),
     std::make_pair(196, 552), std::make_pair(196, 578),
     std::make_pair(12, 604),  std::make_pair(44, 604),
     std::make_pair(76, 604),  std::make_pair(108, 604),
     std::make_pair(140, 604), std::make_pair(172, 604),
     std::make_pair(12, 370),  std::make_pair(34, 370),
     std::make_pair(120, 370), std::make_pair(152, 370),
     std::make_pair(184, 370), std::make_pair(34, 344),
     std::make_pair(120, 344), std::make_pair(34, 318),
     std::make_pair(120, 318), std::make_pair(34, 292),
     std::make_pair(120, 292), std::make_pair(34, 266),
     std::make_pair(120, 266), std::make_pair(12, 266),
     std::make_pair(-20, 266), std::make_pair(152, 266),
     std::make_pair(-20, 240), std::make_pair(172, 240),
     std::make_pair(-20, 214), std::make_pair(172, 214),
     std::make_pair(-20, 188), std::make_pair(172, 188),
     std::make_pair(-20, 162), std::make_pair(172, 162),
     std::make_pair(-20, 136), std::make_pair(172, 136),
     std::make_pair(-20, 110), std::make_pair(172, 110),
     std::make_pair(-20, 84),  std::make_pair(172, 84),
     std::make_pair(12, 58),   std::make_pair(44, 58),
     std::make_pair(76, 58),   std::make_pair(108, 58),
     std::make_pair(140, 58)}};

const sf::Time & Game::getElapsedTime() { return elapsedTime; }

void Game::setElapsedTime(const sf::Time & elapsedTime) {
    this->elapsedTime = elapsedTime;
}

static Game * pGame;

void setgGamePtr(Game * pGame) { ::pGame = pGame; }

Game * getgGamePtr() { return ::pGame; }

EntityTable & Game::getEntityTable() { return entityTable; }

std::vector<Light> & Game::getLights() { return lights; }
