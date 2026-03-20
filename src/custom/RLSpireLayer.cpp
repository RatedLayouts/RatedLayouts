#include <Geode/Geode.hpp>
#include "RLSpireLayer.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <cue/RepeatingBackground.hpp>
#include "Geode/cocos/cocoa/CCObject.h"

using namespace geode::prelude;

RLSpireLayer* RLSpireLayer::create() {
    auto layer = new RLSpireLayer();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool RLSpireLayer::init() {
    if (!CCLayer::init())
        return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto bg = cue::RepeatingBackground::create("game_bg_19_001.png", 1.f, cue::RepeatMode::X);
    bg->setSpeed(0.0f);
    bg->setColor({0, 70, 150});
    addChild(bg, -5);

    addBackButton(this, BackButtonStyle::Pink);

    //floating spire
    m_spireSpr = CCSprite::createWithSpriteFrameName("theSpire_01.png"_spr);
    m_spireSpr->setPosition({winSize.width / 2, winSize.height / 2 - 10});
    m_spireSpr->setScale(0.95f);
    this->addChild(m_spireSpr);

    //float up and down forever with easing
    auto moveUp = CCMoveBy::create(2.0f, {0, 5});
    auto moveDown = CCMoveBy::create(2.0f, {0, -5});
    auto easeUp = CCEaseSineInOut::create(moveUp);
    auto easeDown = CCEaseSineInOut::create(moveDown);
    auto floatSeq = CCSequence::create(easeUp, easeDown, nullptr);
    auto floatForever = CCRepeatForever::create(floatSeq);
    m_spireSpr->runAction(floatForever);

    //menu thing to enter the spire lol
    auto entryMenu = CCMenu::create();
    entryMenu->setPosition({0, 0});
    m_spireSpr->addChild(entryMenu, -1);

    auto whiteSquareSpr = CCSprite::create("geode.loader/white-square.png");
    whiteSquareSpr->setOpacity(0);
    whiteSquareSpr->setScale(2.f);
    auto whiteSquareBtn = CCMenuItemSpriteExtra::create(whiteSquareSpr, this, menu_selector(RLSpireLayer::onEnterSpire));
    whiteSquareBtn->setPosition({50, 100});
    entryMenu->addChild(whiteSquareBtn);

    this->setKeypadEnabled(true);

    return true;
}

void RLSpireLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}

void RLSpireLayer::onEnterSpire(CCObject* sender) {
    if (!Mod::get()->getSavedValue<bool>("hasCode")) {
        FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");
        DialogObject* dialogObj = nullptr;
        std::string response = "";
        switch (m_indexDia) {
            case 0:
                response = "<cf>The Spire</c> isn't opening.";
                m_indexDia++;
                break;
            case 1:
                response = "Yep... still <co>closed</c>.";
                m_indexDia++;
                break;
            case 2:
                response = "Do you think that speaking to me will open it <cg>magically</c>?";
                m_indexDia++;
                break;
            case 3:
                response = "I heard <cp>The Oracle</c> is pretty <cl>knowledgeable</c>, maybe you should ask it about this <cf>Spire</c>.";
            default:
                m_indexDia = 0;
                break;
        }

        dialogObj = DialogObject::create("ArcticWoof", response.c_str(), 1, 1.f, false, ccWHITE);

        auto dialog = DialogLayer::createDialogLayer(dialogObj, nullptr, 2);
        dialog->addToMainScene();
        dialog->animateInRandomSide();

        auto awSprite =
            CCSprite::createWithSpriteFrameName("RL_dialogIconAW.png"_spr);
        awSprite->setPosition(dialog->m_characterSprite->getPosition());
        dialog->m_mainLayer->addChild(awSprite, 1);
        dialog->m_characterSprite->removeFromParent();
        return;
    }
    log::debug("Entering the spire");
}