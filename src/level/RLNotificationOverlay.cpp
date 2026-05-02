#include "RLNotificationOverlay.hpp"
#include "RLNotificationAlert.hpp"
#include "../include/RLConstants.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

RLNotificationOverlay* RLNotificationOverlay::create() {
    auto ret = new RLNotificationOverlay();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

RLNotificationOverlay::~RLNotificationOverlay() {
    if (m_alertQueue) {
        m_alertQueue->release();
        m_alertQueue = nullptr;
    }
}

void RLNotificationOverlay::pushAlert(CCNode* alert) {
    if (!alert || !m_alertQueue)
        return;

    m_alertQueue->addObject(alert);

    if (!m_isShowingAlert) {
        showNextAlert();
    }
}

void RLNotificationOverlay::showNextAlert() {
    if (!m_alertQueue || m_alertQueue->count() == 0) {
        m_isShowingAlert = false;
        return;
    }

    m_isShowingAlert = true;
    FMODAudioEngine::sharedEngine()->playEffect("geode.loader/newNotif00.ogg");

    auto alert = static_cast<CCNode*>(m_alertQueue->objectAtIndex(0));
    m_alertQueue->removeObjectAtIndex(0);

    this->addChild(alert);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto finalPos = alert->getPosition();
    alert->setPosition(ccp(-winSize.width, finalPos.y));
    alert->runAction(CCSequence::create(
        CCEaseIn::create(CCMoveTo::create(0.30f, finalPos), 2.0f), nullptr));

    if (auto notifAlert = typeinfo_cast<RLNotificationAlert*>(alert)) {
        notifAlert->setOnCloseCallback([this]() { this->showNextAlert(); });
    }
}

bool RLNotificationOverlay::init() {
    if (!CCLayer::init())
        return false;

    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    m_alertQueue = CCArray::create();
    m_alertQueue->retain();
    m_isShowingAlert = false;

    float pollingInterval = Mod::get()->getSettingValue<float>("pollingInterval");

    if (pollingInterval <= 0) {
        return true;
    }

    if (Mod::get()->getSettingValue<bool>("disableNewRate")) {
        log::info("notifications are disabled");
        return true;
    }

    this->schedule(schedule_selector(RLNotificationOverlay::callRateNotification),
        static_cast<float>(pollingInterval));

    return true;
}

void RLNotificationOverlay::callRateNotification(float dt) {
    if (Mod::get()->getSettingValue<bool>("disableNewRate")) {
        log::info("notifications are disabled");
        return;
    }

    if ((PlayLayer::get() || LevelEditorLayer::get()) && Mod::get()->getSettingValue<bool>("disableNewRateInLevel")) {
        log::info("notifications are disabled in level/editor");
        return;
    }

    auto req = web::WebRequest();

    log::debug("Polling for new rate notifications...");
    async::spawn(
        req.get(std::string(rl::BASE_API_URL) + "/getNewRate"),
        [this](web::WebResponse response) {
            if (!response.ok()) {
                log::warn("Rate notification fetch failed: {}", response.code());
                return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("Failed to parse JSON response for rate notification");
                return;
            }

            auto json = jsonRes.unwrap();

            // Helper struct and parser for nested objects
            struct RateInfo {
                bool present = false;
                int levelId = 0;
                std::string levelName;
                int difficulty = 0;
                int featured = 0;
                int featuredScore = 0;
                int accountId = 0;
                std::string accountName;
                bool isSuggested = false;
                bool coinVerified = false;
                bool isPlatformer = false;
                std::string eventType;
            };

            auto parseObj = [](const matjson::Value& obj) {
                RateInfo r;
                if (!obj.isObject())
                    return r;
                r.present = true;
                r.levelId = obj["levelId"].asInt().unwrapOr(0);
                r.levelName = obj["levelName"].asString().unwrapOr(std::string(""));
                r.difficulty = obj["difficulty"].asInt().unwrapOr(0);
                r.featured = obj["featured"].asInt().unwrapOr(0);
                r.featuredScore = obj["featuredScore"].asInt().unwrapOr(0);
                r.accountId = obj["accountId"].asInt().unwrapOr(0);
                r.accountName =
                    obj["accountName"].asString().unwrapOr(std::string(""));
                r.isSuggested = obj["isSuggested"].asBool().unwrapOr(false);
                r.coinVerified = obj["coinVerified"].asBool().unwrapOr(false);
                r.isPlatformer = obj["isPlatformer"].asBool().unwrapOr(false);
                r.eventType = obj["eventType"].asString().unwrapOr(std::string(""));
                return r;
            };

            int latestLevelId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateLevelId");
            int latestDailyClassicEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateDailyClassicEventId");
            int latestDailyPlatformerEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateDailyPlatformerEventId");
            int latestWeeklyClassicEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateWeeklyClassicEventId");
            int latestWeeklyPlatformerEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateWeeklyPlatformerEventId");
            int latestMonthlyClassicEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateMonthlyClassicEventId");
            int latestMonthlyPlatformerEventId =
                Mod::get()->getSavedValue<int>("latestNotifiedRateMonthlyPlatformerEventId");

            RateInfo newRate = parseObj(json["newRate"]);
            RateInfo newDailyClassic = parseObj(json["newDailyClassic"]);
            RateInfo newDailyPlatformer = parseObj(json["newDailyPlatformer"]);
            RateInfo newWeeklyClassic = parseObj(json["newWeeklyClassic"]);
            RateInfo newWeeklyPlatformer = parseObj(json["newWeeklyPlatformer"]);
            RateInfo newMonthlyClassic = parseObj(json["newMonthlyClassic"]);
            RateInfo newMonthlyPlatformer = parseObj(json["newMonthlyPlatformer"]);
            RateInfo newEvent = parseObj(json["newEvent"]);  // fallback for compatibility

            // store new rate levelid and event id to avoid duplicate notifications
            if (!newRate.present && !newDailyClassic.present && !newDailyPlatformer.present && !newWeeklyClassic.present && !newWeeklyPlatformer.present && !newMonthlyClassic.present && !newMonthlyPlatformer.present && !newEvent.present) {
                log::debug("No new rate or event notifications");
                return;
            }

            if (newRate.present) {
                if (newRate.levelId != latestLevelId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Rated Layout", newRate.levelName, newRate.difficulty, newRate.featured, newRate.levelId, newRate.accountName, newRate.isPlatformer, "rate");
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateLevelId",
                                newRate.levelId);
                        }
                    }
                } else {
                    log::debug("No new rate notifications");
                }
            }

            if (newDailyClassic.present) {
                if (newDailyClassic.levelId != latestDailyClassicEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Daily Classic Layout",
                            newDailyClassic.levelName,
                            newDailyClassic.difficulty,
                            newDailyClassic.featured,
                            newDailyClassic.levelId,
                            newDailyClassic.accountName,
                            newDailyClassic.isPlatformer,
                            newDailyClassic.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateDailyClassicEventId",
                                newDailyClassic.levelId);
                        }
                    }
                } else {
                    log::debug("No new daily classic event notifications");
                }
            }

            if (newDailyPlatformer.present) {
                if (newDailyPlatformer.levelId != latestDailyPlatformerEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Daily Platformer Layout",
                            newDailyPlatformer.levelName,
                            newDailyPlatformer.difficulty,
                            newDailyPlatformer.featured,
                            newDailyPlatformer.levelId,
                            newDailyPlatformer.accountName,
                            newDailyPlatformer.isPlatformer,
                            newDailyPlatformer.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateDailyPlatformerEventId",
                                newDailyPlatformer.levelId);
                        }
                    }
                } else {
                    log::debug("No new daily platformer event notifications");
                }
            }

            if (newWeeklyClassic.present) {
                if (newWeeklyClassic.levelId != latestWeeklyClassicEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Weekly Classic Layout",
                            newWeeklyClassic.levelName,
                            newWeeklyClassic.difficulty,
                            newWeeklyClassic.featured,
                            newWeeklyClassic.levelId,
                            newWeeklyClassic.accountName,
                            newWeeklyClassic.isPlatformer,
                            newWeeklyClassic.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateWeeklyClassicEventId",
                                newWeeklyClassic.levelId);
                        }
                    }
                } else {
                    log::debug("No new weekly classic event notifications");
                }
            }

            if (newWeeklyPlatformer.present) {
                if (newWeeklyPlatformer.levelId != latestWeeklyPlatformerEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Weekly Platformer Layout",
                            newWeeklyPlatformer.levelName,
                            newWeeklyPlatformer.difficulty,
                            newWeeklyPlatformer.featured,
                            newWeeklyPlatformer.levelId,
                            newWeeklyPlatformer.accountName,
                            newWeeklyPlatformer.isPlatformer,
                            newWeeklyPlatformer.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateWeeklyPlatformerEventId",
                                newWeeklyPlatformer.levelId);
                        }
                    }
                } else {
                    log::debug("No new weekly platformer event notifications");
                }
            }

            if (newMonthlyClassic.present) {
                if (newMonthlyClassic.levelId != latestMonthlyClassicEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Monthly Classic Layout",
                            newMonthlyClassic.levelName,
                            newMonthlyClassic.difficulty,
                            newMonthlyClassic.featured,
                            newMonthlyClassic.levelId,
                            newMonthlyClassic.accountName,
                            newMonthlyClassic.isPlatformer,
                            newMonthlyClassic.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateMonthlyClassicEventId",
                                newMonthlyClassic.levelId);
                        }
                    }
                } else {
                    log::debug("No new monthly classic event notifications");
                }
            }

            if (newMonthlyPlatformer.present) {
                if (newMonthlyPlatformer.levelId != latestMonthlyPlatformerEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            "New Monthly Platformer Layout",
                            newMonthlyPlatformer.levelName,
                            newMonthlyPlatformer.difficulty,
                            newMonthlyPlatformer.featured,
                            newMonthlyPlatformer.levelId,
                            newMonthlyPlatformer.accountName,
                            newMonthlyPlatformer.isPlatformer,
                            newMonthlyPlatformer.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateMonthlyPlatformerEventId",
                                newMonthlyPlatformer.levelId);
                        }
                    }
                } else {
                    log::info("No new monthly platformer event notifications");
                }
            }

            if (!newDailyClassic.present && !newDailyPlatformer.present && !newWeeklyClassic.present && !newWeeklyPlatformer.present && !newMonthlyClassic.present && !newMonthlyPlatformer.present && newEvent.present) {
                int currentLatestEventId =
                    Mod::get()->getSavedValue<int>("latestNotifiedRateEventId");
                if (newEvent.levelId != currentLatestEventId) {
                    Ref<RLNotificationOverlay> self = this;
                    if (self) {
                        auto alert = RLNotificationAlert::create(
                            fmt::format("New {} Layout", newEvent.eventType),
                            newEvent.levelName,
                            newEvent.difficulty,
                            newEvent.featured,
                            newEvent.levelId,
                            newEvent.accountName,
                            newEvent.isPlatformer,
                            newEvent.eventType);
                        if (alert) {
                            self->pushAlert(alert);
                            Mod::get()->setSavedValue<int>("latestNotifiedRateEventId",
                                newEvent.levelId);
                        }
                    }
                } else {
                    log::info("No new event notifications");
                }
            }
        });
}
