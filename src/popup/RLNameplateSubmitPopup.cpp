#include "RLNameplateSubmitPopup.hpp"
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/utils/file.hpp>
#include <algorithm>
#include "Geode/ui/General.hpp"
#include "Geode/ui/TextInput.hpp"
#include "Geode/utils/general.hpp"
#include "../include/RLConstants.hpp"
#include "RLAdminNameplatePopup.hpp"

using namespace geode::prelude;

arc::Future<void> RLNameplateSubmitPopup::pickAndLoadPng() {
    auto popup = WeakRef(this);
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({"PNG Image", {"*.png"}});

    auto notify = [&](std::string message) {
        Loader::get()->queueInMainThread([message = std::move(message)]() {
            Notification::create(message.c_str(), NotificationIcon::Warning)->show();
        });
    };

    auto result = co_await geode::utils::file::pick(
        geode::utils::file::PickMode::OpenFile,
        options);

    if (!result) {
        notify("Failed to open file picker");
        co_return;
    }

    auto maybePath = result.unwrap();
    if (!maybePath) {
        co_return;
    }

    auto path = *maybePath;

    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension != ".png") {
        notify("Please select a PNG file");
        co_return;
    }

    auto binaryResult = geode::utils::file::readBinary(path);
    if (!binaryResult) {
        notify("Failed to read file");
        co_return;
    }

    CCImage image;
    if (!image.initWithImageFile(path.string().c_str())) {
        notify("Unable to decode image");
        co_return;
    }

    if (image.getWidth() != 1500 || image.getHeight() != 150) {
        notify("Image must be 1500x150, got " + numToString(image.getWidth()) + "x" + numToString(image.getHeight()));
        co_return;
    }

    // imma goog this an create a new lazysprite with this one :speak:
    Loader::get()->queueInMainThread([popup, path = std::move(path)]() {
        if (auto self = popup.lock()) {
            if (auto existing = self->m_nameplateLazy) {
                auto parent = existing->getParent();
                auto zorder = existing->getZOrder();
                auto position = existing->getPosition();
                auto anchor = existing->getAnchorPoint();
                auto size = existing->getContentSize();

                existing->removeFromParent();

                auto replacement = LazySprite::create(size, false);
                replacement->setAutoResize(true);
                replacement->setPosition(position);
                replacement->setAnchorPoint(anchor);

                if (parent) {
                    parent->addChild(replacement, zorder);
                }

                self->m_nameplateLazy = replacement;
                self->m_nameplateLazy->loadFromFile(path, CCImage::kFmtPng, true);
                self->m_pickedPath = path;
            }
        }
    });

    co_return;
}

RLNameplateSubmitPopup* RLNameplateSubmitPopup::create() {
    auto popup = new RLNameplateSubmitPopup();
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool RLNameplateSubmitPopup::init() {
    if (!Popup::init(440, 200, "GJ_square01.png"))
        return false;

    this->setTitle("Nameplate Submission");
    addSideArt(m_mainLayer, SideArt::Bottom, SideArtStyle::PopupBlue);

    m_leaderboardListNode = cue::ListNode::create({355.f, 40.f});
    m_leaderboardListNode->setPosition({m_mainLayer->getContentSize().width / 2.f, m_mainLayer->getContentSize().height / 2.f + 35.f});
    m_mainLayer->addChild(m_leaderboardListNode);

    // leaderboard item
    auto rowContainer = CCLayer::create();
    rowContainer->setContentSize({356.f, 40.f});

    m_nameplateLazy = LazySprite::create({356.f, 40.f}, false);
    m_nameplateLazy->setAutoResize(true);
    m_nameplateLazy->setPosition({rowContainer->getContentWidth() / 2.f, 20.f});
    rowContainer->addChild(m_nameplateLazy, -1);

    // Rank label
    auto rankLabel =
        CCLabelBMFont::create("1", "goldFont.fnt");
    rankLabel->setScale(0.5f);
    rankLabel->setPosition({15.f, 20.f});
    rankLabel->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(rankLabel, 2);

    auto accountLabel = CCLabelBMFont::create("Player", "goldFont.fnt");
    accountLabel->setAnchorPoint({0.f, 0.5f});
    accountLabel->setScale(0.7f);
    accountLabel->setPosition({80.f, 20.f});
    accountLabel->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(accountLabel, 2);

    auto gm = GameManager::sharedState();
    auto player = SimplePlayer::create(1);
    player->updatePlayerFrame(1, IconType::Cube);
    player->setColors(gm->colorForIdx(1), gm->colorForIdx(2));
    player->setPosition({55.f, 20.f});
    player->setScale(0.75f);
    rowContainer->addChild(player, 2);

    auto scoreLabelText = CCLabelBMFont::create(
        fmt::format("{}", GameToolbox::pointsToString(69420)).c_str(),
        "bigFont.fnt");
    scoreLabelText->setScale(0.5f);
    scoreLabelText->setPosition({320.f, 20.f});
    scoreLabelText->setAnchorPoint({1.f, 0.5f});
    rowContainer->addChild(scoreLabelText, 2);

    auto iconSprite = CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
    iconSprite->setScale(0.65f);
    iconSprite->setPosition({325.f, 20.f});
    iconSprite->setAnchorPoint({0.f, 0.5f});
    rowContainer->addChild(iconSprite, 2);

    m_leaderboardListNode->addCell(rowContainer);
    m_leaderboardListNode->getScrollLayer()->m_disableMovement = true;

    m_priceInput = geode::TextInput::create(180.f, "Rubies Price", "bigFont.fnt");
    m_priceInput->setCommonFilter(CommonFilter::Int);
    m_priceInput->setLabel("Price");
    m_priceInput->setPosition({m_mainLayer->getContentSize().width / 2.f + 15.f, 75.f});
    m_mainLayer->addChild(m_priceInput);

    auto rubiesSpr = CCSprite::createWithSpriteFrameName("RL_rubiesIcon.png"_spr);
    rubiesSpr->setPosition(m_priceInput->getPosition() - CCPoint(120, 0));
    m_mainLayer->addChild(rubiesSpr);

    auto pickSprite = ButtonSprite::create("Pick Nameplate", 120, true, "goldFont.fnt", "GJ_button_01.png", 25.f, .7f);
    auto pickMenuItem = CCMenuItemSpriteExtra::create(
        pickSprite, this, menu_selector(RLNameplateSubmitPopup::onPickImage));
    pickMenuItem->setPosition({m_mainLayer->getContentSize().width / 2 - 80.f, 25.f});
    m_buttonMenu->addChild(pickMenuItem);

    auto submitSprite = ButtonSprite::create("Submit", 120, true, "goldFont.fnt", "GJ_button_01.png", 25.f, .7f);
    auto submitMenuItem = CCMenuItemSpriteExtra::create(
        submitSprite, this, menu_selector(RLNameplateSubmitPopup::onSubmit));
    submitMenuItem->setPosition({m_mainLayer->getContentSize().width / 2 + 80.f, 25.f});
    m_buttonMenu->addChild(submitMenuItem);

    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.7f);
    auto infoBtn = CCMenuItemSpriteExtra::create(
        infoSpr, this, menu_selector(RLNameplateSubmitPopup::onInfo));
    infoBtn->setPosition({m_mainLayer->getContentSize().width, m_mainLayer->getContentSize().height - 3.f});
    m_buttonMenu->addChild(infoBtn);

    if (rl::isUserOwner() || rl::isUserDeveloper()) {
        auto pendingSprite = ButtonSprite::create("Pending", 60, true, "goldFont.fnt", "GJ_button_01.png", 25.f, .7f);
        auto pendingMenuItem = CCMenuItemSpriteExtra::create(
            pendingSprite, this, menu_selector(RLNameplateSubmitPopup::onViewPending));
        pendingMenuItem->setPosition({60.f, m_mainLayer->getContentSize().height - 25.f});
        m_buttonMenu->addChild(pendingMenuItem);
    }

    return true;
}

void RLNameplateSubmitPopup::onInfo(CCObject* sender) {
    MDPopup::create(
        "Nameplate Submission Rules",
        "There's a strict rules you MUST follow to be added in-game\n"
        "- Image must be a <cg>PNG</c> image format! <co>(Other image format will be an instant rejection)</c>\n"
        "- Must be appropriate <co>(No NSFW or anything bad)</c>\n"
        "- Must be in a <cg>good image quality</c>\n"
        "- Must be an <cy>effort made in the nameplate</c>, don't be lazy about it and be creative with it :)\n"
        "- No heavy <cr>self-promotion</c> <co>(eg. Making your username as the main focus on the banner)</c>\n"
        "- It must be <cl>universal for everyone</c> to use. <co>(Don't make it only exclusive to you)</c>\n"
        "- <cr>AI-slop is an instant rejection</c>.\n"
        "----\n"
        "Base Price must be <cr>10k</c> or higher as different difficulty provides different <cr>Rubies rewards</c>.\n"
        "<co>Demon Levels</c> rewards more <cr>rubies</c> so if you beaten lot's of levels, <co>then you already have lots of rubies</c>.",
        "OK")
        ->show();
}

void RLNameplateSubmitPopup::onViewPending(CCObject* sender) {
    RLAdminNameplatePopup::create()->show();
}

void RLNameplateSubmitPopup::onPickImage(CCObject* sender) {
    async::spawn(this->pickAndLoadPng());
}

void RLNameplateSubmitPopup::onSubmit(CCObject* sender) {
    auto upopup = UploadActionPopup::create(nullptr, "Submitting Nameplate...");
    upopup->show();
    Ref<UploadActionPopup> popupRef = upopup;
    if (m_pickedPath.empty()) {
        popupRef->showFailMessage("Please pick an image first!");
        return;
    }

    auto priceStr = m_priceInput->getString();
    if (priceStr.empty()) {
        popupRef->showFailMessage("Please set a base price!");
        return;
    }

    auto priceRes = geode::utils::numFromString<int>(priceStr);
    if (!priceRes) {
        popupRef->showFailMessage("Invalid price!");
        return;
    }
    int price = priceRes.unwrap();

    if (price < 10000) {
        popupRef->showFailMessage("Price must be at least 10000 rubies");
        return;
    }

    auto accountId = GJAccountManager::get()->m_accountID;
    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        popupRef->showFailMessage("Argon token missing!");
        return;
    }

    auto req = geode::utils::web::WebRequest();
    geode::utils::web::MultipartForm form;
    form.param("accountId", numToString(accountId));
    form.param("argonToken", token);
    form.param("price", numToString(price));

    auto res = form.file("banner", m_pickedPath, "image/png");
    if (!res) {
        popupRef->showFailMessage("Failed to attach file");
        return;
    }
    req.bodyMultipart(std::move(form));

    Ref<RLNameplateSubmitPopup> self = this;
    m_fetchTask.spawn(
        req.post(std::string(rl::BASE_API_URL) + "/submitNameplate"),
        [self, popupRef](geode::utils::web::WebResponse response) {
            if (!self || !popupRef) return;

            if (response.ok()) {
                popupRef->showSuccessMessage("Nameplate submitted!");
                self->onClose(nullptr);
            } else {
                auto msg = response.string().unwrapOr("Unknown error");
                popupRef->showFailMessage("Failed to submit");
                log::warn("submitNameplate error {}: {}", response.code(), msg);
            }
        });
}