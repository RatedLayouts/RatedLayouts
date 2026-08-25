#include "RLConstants.hpp"
#include "utils/CachedSettings.hpp"
#include <capeling.garage-stats-menu/include/stats_api.hpp>

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;
using namespace rl;
using namespace stats_api;

void fetchAndRegisterProfile() {
    if (CachedSettings::get()->disableGarageStats)
        return;

    int accountId = GJAccountManager::get()->m_accountID;

    matjson::Value jsonBody = matjson::Value::object();
    jsonBody["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
    jsonBody["accountId"] = accountId;

    auto postReq = web::WebRequest();
    postReq.bodyJSON(jsonBody);

    async::spawn(
        postReq.post(std::string(rl::BASE_API_URL) + "/profile"),
        [](web::WebResponse response) {
            log::info("Received response from server");

            if (!response.ok()) {
                return;
            }

            auto jsonRes = response.json();
            if (!jsonRes) {
                log::warn("Failed to parse JSON response");
                return;
            }

            auto json = jsonRes.unwrap();

            int planets = json["planets"].asInt().unwrapOrDefault();
            int stars = json["stars"].asInt().unwrapOrDefault();
            int coins = json["coins"].asInt().unwrapOrDefault();
            int votes = json["votes"].asInt().unwrapOrDefault();
            int points = json["points"].asInt().unwrapOrDefault();

            log::info("Profile data - points: {}, stars: {}", points, stars);

            setDisplayedNumber("rl-stars-value"_spr, stars);
            setDisplayedNumber("planets-collected"_spr, planets);
            setDisplayedNumber("coins-collected"_spr, coins);
            setDisplayedNumber("votes-collected"_spr, votes);
            setDisplayedNumber("rl-points-value"_spr, points);
        }
    );
}

$execute {
    registerStatItem(
        "rl-stars-value"_spr,
        []() {
            return CCSprite::createWithSpriteFrameName("RL_starMed.png"_spr);
        },
        0,
        0.54f
    );

    registerStatItem(
        "planets-collected"_spr,
        []() {
            return CCSprite::createWithSpriteFrameName("RL_planetMed.png"_spr);
        },
        0,
        0.54f
    );

    registerStatItem(
        "coins-collected"_spr,
        []() {
            return CCSprite::createWithSpriteFrameName("RL_BlueCoinSmall.png"_spr);
        },
        0,
        0.54f
    );

    registerStatItem(
        "votes-collected"_spr,
        []() {
            return CCSprite::createWithSpriteFrameName("RL_commVote01.png"_spr);
        },
        0,
        0.54f
    );

    registerStatItem(
        "rl-points-value"_spr,
        []() {
            return CCSprite::createWithSpriteFrameName("RL_blueprintPoint01.png"_spr);
        },
        0,
        0.54f
    );

    fetchAndRegisterProfile();
}