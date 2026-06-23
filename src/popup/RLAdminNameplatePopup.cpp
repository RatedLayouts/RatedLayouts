#include "RLAdminNameplatePopup.hpp"
#include "../include/RLConstants.hpp"
#include "Geode/cocos/cocoa/CCGeometry.h"
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/ProfilePage.hpp>

using namespace geode::prelude;

RLAdminNameplatePopup* RLAdminNameplatePopup::create() {
    auto ret = new RLAdminNameplatePopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RLAdminNameplatePopup::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square01.png"))
        return false;

    this->setTitle("Pending Nameplates");

    m_listNode = cue::ListNode::create({400.f, 220.f}, {20, 25, 55, 255});
    m_listNode->setPosition(m_mainLayer->getContentSize() / 2.f - CCPoint(0.f, 5.f));
    m_mainLayer->addChild(m_listNode);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("RL_refresh01.png"_spr);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(RLAdminNameplatePopup::onRefresh));
    refreshBtn->setPosition({m_mainLayer->getContentSize().width - 5.f, 5.f});
    m_buttonMenu->addChild(refreshBtn);

    fetchPending();

    return true;
}

void RLAdminNameplatePopup::onRefresh(CCObject* sender) {
    fetchPending();
}

void RLAdminNameplatePopup::fetchPending() {
    m_listNode->clear();
    m_items.clear();

    if (!m_loadingCircle) {
        m_loadingCircle = LoadingSpinner::create(50.f);
        m_loadingCircle->setPosition(m_listNode->getContentSize() / 2.f);
        m_listNode->addChild(m_loadingCircle);
    }
    m_loadingCircle->setVisible(true);

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    if (token.empty()) {
        Notification::create("Argon token missing!", NotificationIcon::Warning)->show();
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = token;

    auto req = geode::utils::web::WebRequest();
    Ref<RLAdminNameplatePopup> self = this;

    m_fetchTask.spawn(
        req.bodyJSON(body).post(std::string(rl::BASE_API_URL) + "/getPendingNameplates"),
        [self](geode::utils::web::WebResponse response) {
            if (!self) return;
            if (self->m_loadingCircle) self->m_loadingCircle->setVisible(false);
            if (!response.ok()) {
                Notification::create("Failed to fetch pending nameplates", NotificationIcon::Error)->show();
                return;
            }
            auto jsonRes = response.json();
            if (!jsonRes) return;
            auto json = jsonRes.unwrap();

            if (!json["success"].asBool().unwrapOrDefault()) {
                Notification::create(json["message"].asString().unwrapOr("Error"), NotificationIcon::Error)->show();
                return;
            }

            std::vector<matjson::Value> arr;
            if (json.contains("nameplates") && json["nameplates"].isArray()) {
                arr = json["nameplates"].asArray().unwrap();
            }

            self->m_items.clear();
            for (auto& item : arr) {
                PendingItem pi;
                pi.id = item["id"].asInt().unwrapOrDefault();
                pi.accountId = item["accountId"].asInt().unwrapOrDefault();
                pi.username = item["username"].asString().unwrapOrDefault();
                pi.path = item["path"].asString().unwrapOrDefault();
                pi.price = item["price"].asInt().unwrapOrDefault();
                pi.createdAt = item["createdAt"].asString().unwrapOrDefault();
                self->m_items.push_back(pi);
            }

            int idx = 0;
            for (auto& item : self->m_items) {
                auto cell = CCLayer::create();
                cell->setContentSize({400.f, 40.f});

                auto bg = LazySprite::create({400.f, 40.f}, false);
                bg->setPosition(cell->getContentSize() / 2);
                bg->setZOrder(-1);
                bg->setAutoResize(true);
                bg->loadFromUrl(std::string(rl::BASE_API_URL) + "/" + item.path, CCImage::kFmtPng, true);
                cell->addChild(bg);

                auto nameLabel = CCLabelBMFont::create(fmt::format("{}", item.username).c_str(), "goldFont.fnt");
                nameLabel->limitLabelWidth(250.f, 0.5f, 0.15f);

                auto menu = CCMenu::create();
                menu->setPosition({0.f, 0.f});
                cell->addChild(menu);

                auto nameBtn = CCMenuItemSpriteExtra::create(nameLabel, self, menu_selector(RLAdminNameplatePopup::onOpenProfile));
                nameBtn->setTag(item.accountId);
                nameBtn->setAnchorPoint({0.f, 0.5f});
                nameBtn->setPosition({10.f, 20.f});
                menu->addChild(nameBtn);

                auto priceContainer = CCNode::create();
                priceContainer->setPosition({200.f, 20.f});
                cell->addChild(priceContainer);

                auto rubySpr = CCSprite::createWithSpriteFrameName("RL_rubiesIcon.png"_spr);
                rubySpr->setScale(0.4f);
                rubySpr->setPosition({0.f, 0.f});
                priceContainer->addChild(rubySpr);

                auto priceLabel = CCLabelBMFont::create(fmt::format("{}", item.price).c_str(), "bigFont.fnt");
                priceLabel->limitLabelWidth(45.f, 0.4f, 0.1f);
                priceLabel->setAnchorPoint({0.f, 0.5f});
                priceLabel->setPosition({rubySpr->getScaledContentSize().width / 2.f + 5.f, 0.f});
                priceContainer->addChild(priceLabel);

                auto viewSpr = CCSprite::createWithSpriteFrameName("backArrowPlain_01_001.png");
                auto viewBtn = CCMenuItemSpriteExtra::create(viewSpr, self, menu_selector(RLAdminNameplatePopup::onView));
                viewBtn->setTag(idx);
                viewBtn->setPosition({290.f, 20.f});
                menu->addChild(viewBtn);

                auto okSpr = CCSprite::createWithSpriteFrameName("RL_check.png"_spr);
                auto okBtn = CCMenuItemSpriteExtra::create(okSpr, self, menu_selector(RLAdminNameplatePopup::onApprove));
                okBtn->setTag(idx);
                okBtn->setPosition({330.f, 20.f});
                menu->addChild(okBtn);

                auto noSpr = CCSprite::createWithSpriteFrameName("RL_cross.png"_spr);
                auto noBtn = CCMenuItemSpriteExtra::create(noSpr, self, menu_selector(RLAdminNameplatePopup::onReject));
                noBtn->setTag(idx);
                noBtn->setPosition({370.f, 20.f});
                menu->addChild(noBtn);

                self->m_listNode->addCell(cell);
                idx++;
            }
        });
}

void RLAdminNameplatePopup::onView(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    int idx = btn->getTag();
    if (idx < 0 || idx >= m_items.size()) return;
    auto item = m_items[idx];

    geode::utils::web::openLinkInBrowser(std::string(rl::BASE_API_URL) + "/" + item.path);
}

void RLAdminNameplatePopup::onOpenProfile(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    int accountId = btn->getTag();
    ProfilePage::create(accountId, false)->show();
}

void RLAdminNameplatePopup::onApprove(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    int idx = btn->getTag();
    if (idx < 0 || idx >= m_items.size()) return;
    auto item = m_items[idx];

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = token;
    body["queueId"] = item.id;
    body["action"] = "accept";

    auto upopup = UploadActionPopup::create(nullptr, "Approving...");
    upopup->show();

    Ref<RLAdminNameplatePopup> self = this;
    Ref<UploadActionPopup> popupRef = upopup;
    m_actionTask.spawn(
        web::WebRequest().bodyJSON(body).post(std::string(rl::BASE_API_URL) + "/approveNameplate"),
        [self, popupRef](web::WebResponse response) {
            if (!self || !popupRef) return;
            if (response.ok()) {
                popupRef->showSuccessMessage("Nameplate Approved!");
                self->fetchPending();
            } else {
                popupRef->showFailMessage("Failed to approve");
            }
        });
}

void RLAdminNameplatePopup::onReject(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    int idx = btn->getTag();
    if (idx < 0 || idx >= m_items.size()) return;
    auto item = m_items[idx];

    auto token = Mod::get()->getSavedValue<std::string>("argon_token");
    matjson::Value body = matjson::Value::object();
    body["accountId"] = GJAccountManager::get()->m_accountID;
    body["argonToken"] = token;
    body["queueId"] = item.id;
    body["action"] = "deny";

    auto upopup = UploadActionPopup::create(nullptr, "Denying...");
    upopup->show();

    Ref<RLAdminNameplatePopup> self = this;
    Ref<UploadActionPopup> popupRef = upopup;
    m_actionTask.spawn(
        web::WebRequest().bodyJSON(body).post(std::string(rl::BASE_API_URL) + "/approveNameplate"),
        [self, popupRef](web::WebResponse response) {
            if (!self || !popupRef) return;
            if (response.ok()) {
                popupRef->showSuccessMessage("Nameplate Denied.");
                self->fetchPending();
            } else {
                popupRef->showFailMessage("Failed to deny");
            }
        });
}
