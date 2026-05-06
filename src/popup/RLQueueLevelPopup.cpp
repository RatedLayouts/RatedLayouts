#include <Geode/Geode.hpp>
#include "RLQueueLevelPopup.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"

using namespace geode::prelude;

RLQueueLevelPopup* RLQueueLevelPopup::create() {
    auto ret = new RLQueueLevelPopup();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool RLQueueLevelPopup::init() {
    if (!Popup::init(400.f, 250.f, "GJ_square02.png"))
        return false;

    this->setTitle("Submit your Layout into the Queue!");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    // level id input
    m_levelIDInput = TextInput::create(180.f, "Provide Level ID", "bigFont.fnt");
    m_levelIDInput->setCommonFilter(CommonFilter::Name);
    m_levelIDInput->setTextAlign(TextInputAlign::Left);
    m_levelIDInput->setLabel("Level ID");
    m_mainLayer->addChildAtPosition(m_levelIDInput, Anchor::Center, {-25.f, 55.f}, false);

    // search button to look up based of the provided level id
    auto findBtnSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    auto findBtnBtn = CCMenuItemSpriteExtra::create(findBtnSpr, this, menu_selector(RLQueueLevelPopup::checkLevelID));
    findBtnBtn->setPosition({m_levelIDInput->getPositionX() + 120.f, m_levelIDInput->getPositionY()});
    m_buttonMenu->addChild(findBtnBtn);

    // info button
    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.75f);
    auto infoBtn = CCMenuItemSpriteExtra::create(
        infoSpr, this, menu_selector(RLQueueLevelPopup::onInfo));
    m_buttonMenu->addChildAtPosition(infoBtn, Anchor::TopRight, {-3.f, -3.f});

    // level cell
    auto bgLevelCell = NineSlice::create("GJ_square01.png");
    bgLevelCell->setContentSize({320, 90});
    m_mainLayer->addChildAtPosition(bgLevelCell, Anchor::Center, {0.f, -25.f});

    m_noLevelLabel = CCLabelBMFont::create("No level loaded", "goldFont.fnt");
    m_noLevelLabel->setScale(0.5f);
    bgLevelCell->addChildAtPosition(m_noLevelLabel, Anchor::Center, {0.f, 0.f}, false);

    m_levelCell = LevelCell::create(356, 100);
    m_levelCell->setScale(.85f);
    m_levelCell->setContentSize({356, 100});
    m_levelCell->setAnchorPoint({0.16f, 0.2f});
    m_levelCell->setPosition({0, 0});

    bgLevelCell->addChild(m_levelCell, 2);

    // submit Button
    auto submitBtnSpr = ButtonSprite::create("Submit to Queue", "goldFont.fnt", "GJ_button_04.png");
    auto submitBtn = CCMenuItemSpriteExtra::create(submitBtnSpr, this, menu_selector(RLQueueLevelPopup::submitToQueue));
    submitBtn->setEnabled(false);
    m_submitButton = submitBtn;
    m_buttonMenu->addChildAtPosition(submitBtn, Anchor::Bottom, {0.f, 25.f});

    return true;
}

void RLQueueLevelPopup::checkLevelID(CCObject* sender) {
    std::string levelIDStr = m_levelIDInput ? m_levelIDInput->getString() : "";
    auto levelIdResult = numFromString<int>(levelIDStr);
    int levelId = levelIdResult ? levelIdResult.unwrap() : 0;
    if (levelId <= 0) {
        Notification::create("Invalid Level ID", NotificationIcon::Warning)->show();
        return;
    }

    m_selectedLevelId = levelId;
    m_levelExists = false;
    if (m_submitButton) {
        m_submitButton->setEnabled(false);
        m_submitButton->setNormalImage(ButtonSprite::create("Submit to Queue", "goldFont.fnt", "GJ_button_04.png"));
    }

    if (m_levelCell) {
        auto parent = m_levelCell->getParent();
        m_levelCell->removeFromParentAndCleanup(true);
        m_levelCell = nullptr;
        if (parent) {
            auto newLevelCell = LevelCell::create(356, 100);
            newLevelCell->setScale(.85f);
            newLevelCell->setContentSize({356, 100});
            newLevelCell->setAnchorPoint({0.16f, 0.2f});
            newLevelCell->setPosition({0, 0});
            parent->addChild(newLevelCell, 2);
            m_levelCell = newLevelCell;
            m_noLevelLabel->setVisible(true);
        }
    }

    auto glm = GameLevelManager::sharedState();
    if (!glm) {
        Notification::create("Level manager unavailable", NotificationIcon::Error)->show();
        return;
    }

    auto searchObj = GJSearchObject::create(SearchType::Search, numToString(levelId));
    auto key = std::string(searchObj->getKey());
    auto stored = glm->getStoredOnlineLevels(key.c_str());
    if (stored && stored->count() > 0) {
        GJGameLevel* foundLevel = nullptr;
        for (auto level : CCArrayExt<GJGameLevel*>(stored)) {
            if (level && static_cast<int>(level->m_levelID) == levelId) {
                foundLevel = level;
                break;
            }
        }
        if (foundLevel) {
            if (m_levelCell) {
                m_levelCell->loadFromLevel(foundLevel);
                m_noLevelLabel->setVisible(false);
            }
            m_levelExists = true;
            if (m_submitButton) {
                m_submitButton->setEnabled(true);
                m_submitButton->setNormalImage(ButtonSprite::create("Submit to Queue", "goldFont.fnt", "GJ_button_01.png"));
            }

            if (m_levelCell->m_mainMenu) {
                m_levelCell->m_mainMenu->setPosition({0, 0});
                auto viewButton = m_levelCell->m_mainMenu->getChildByID("view-button");
                auto creatorButton = m_levelCell->m_mainMenu->getChildByID("creator-name");
                creatorButton->setPosition({50, 57});
                creatorButton->setAnchorPoint({0.f, 0.5f});
                viewButton->removeFromParent();
            }
            Notification::create("Level loaded", NotificationIcon::Success)->show();
            return;
        }
    }

    if (glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(searchObj);
}

void RLQueueLevelPopup::submitToQueue(CCObject* sender) {
    if (!m_levelExists || m_selectedLevelId <= 0) {
        Notification::create("Load a valid level first", NotificationIcon::Warning)->show();
        return;
    }

    createQuickPopup("Submit to Queue",
        "Are you sure you want to <co>submit</c> this <cg>level to the queue</c>?\n<cy>Any non-layout related levels will result in blacklist from using this feature.</c>",
        "No",
        "Yes",
        [this](auto, bool yes) {
            if (!yes)
                return;

            auto upopup = UploadActionPopup::create(nullptr, "Submitting level to queue...");
            upopup->show();
            matjson::Value jsonBody = matjson::Value::object();
            jsonBody["accountId"] = GJAccountManager::get()->m_accountID;
            jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
            jsonBody["levelId"] = m_selectedLevelId;

            auto req = web::WebRequest();
            req.bodyJSON(jsonBody);

            Ref<RLQueueLevelPopup> self = this;
            async::spawn(req.post(std::string(rl::BASE_API_URL) + "/submitQueue"),
                [self, upopup](web::WebResponse const& response) {
                    if (!self) {
                        return;
                    }

                    if (!response.ok()) {
                        auto message = rl::getResponseFailMessage(response, "Failed to submit queue");
                        upopup->showFailMessage(message);
                        return;
                    }

                    upopup->showSuccessMessage("Level submitted successfully!");
                    self->onClose(nullptr);
                });
        });
}

void RLQueueLevelPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2) {
    auto glm = GameLevelManager::sharedState();
    if (glm && glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }

    if (!levels || levels->count() == 0) {
        Notification::create("Level not found", NotificationIcon::Warning)->show();
        return;
    }

    GJGameLevel* foundLevel = nullptr;
    for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
        if (level && static_cast<int>(level->m_levelID) == m_selectedLevelId) {
            foundLevel = level;
            break;
        }
    }

    if (!foundLevel) {
        Notification::create("Level not found", NotificationIcon::Warning)->show();
        return;
    }

    if (m_levelCell) {
        m_levelCell->loadFromLevel(foundLevel);
        m_noLevelLabel->setVisible(false);
    }
    m_levelExists = true;
    if (m_submitButton) {
        m_submitButton->setEnabled(true);
        m_submitButton->setNormalImage(ButtonSprite::create("Submit to Queue", "goldFont.fnt", "GJ_button_01.png"));
    }

    if (m_levelCell->m_mainMenu) {
        m_levelCell->m_mainMenu->setPosition({0, 0});
        auto viewButton = m_levelCell->m_mainMenu->getChildByID("view-button");
        auto creatorButton = m_levelCell->m_mainMenu->getChildByID("creator-name");
        creatorButton->setPosition({50, 57});
        creatorButton->setAnchorPoint({0.f, 0.5f});
        viewButton->removeFromParent();
    }
    Notification::create("Level loaded", NotificationIcon::Success)->show();
}

void RLQueueLevelPopup::loadLevelsFailed(char const* key, int p1) {
    auto glm = GameLevelManager::sharedState();
    if (glm && glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }
    Notification::create("Failed to fetch level", NotificationIcon::Error)->show();
}

void RLQueueLevelPopup::onInfo(CCObject* sender) {
    MDPopup::create("How to submit your layout into the queue",
        "As a <cp>Layout Supporter</c>, you get the ability to submit your layouts directly into an <cg>in-game queue system</c>!\n\n"
        "To use this feature, simply enter your Level ID then click the <cc>Search Button</c>, this will load your level into the level cell below.\n\n"
        "Once you have your level loaded, click the <cc>Submit to Queue</c> button to submit your layout into the queue.\n\n"
        "From there, your layout will be reviewed by <cl>Layout Moderators</c> and/or <cr>Layout Admins</c> to be viewed and potentially be sent/rejected depending if it meets the <cg>rating standards</c>\n\n"
        "To prevent spamming the queue, you can only submit a layout once per <co>certain amount of time</c>, and submitting a <cy>non-layout related level</c> will result in a <cr>blacklist from using this feature</c>.\n\n",
        "OK")
        ->show();
}