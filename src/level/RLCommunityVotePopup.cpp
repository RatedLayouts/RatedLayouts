#include "RLCommunityVotePopup.hpp"
#include "../include/RLAchievements.hpp"
#include "../include/RLConstants.hpp"
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <algorithm>

using namespace geode::prelude;

RLCommunityVotePopup* RLCommunityVotePopup::create() {
    return RLCommunityVotePopup::create(0);
}

RLCommunityVotePopup* RLCommunityVotePopup::create(int levelId) {
    auto popup = new RLCommunityVotePopup();
    popup->m_levelId = levelId;
    if (popup && popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

void RLCommunityVotePopup::onSubmit(CCObject*) {
    // prepare payload (plays payload by dex)
    createQuickPopup(
        "Submit Vote",
        "Are you sure you want to submit your vote?\n<cy>You won't be able to "
        "change it later.</c>",
        "Cancel",
        "Submit",
        [this](auto, bool yes) {
            if (yes) {
                int accountId = GJAccountManager::get()->m_accountID;
                auto upopup =
                    UploadActionPopup::create(nullptr, "Submitting your vote...");
                upopup->show();
                Ref<UploadActionPopup> popupRef = upopup;
                auto argonToken =
                    Mod::get()->getSavedValue<std::string>("argon_token");
                if (argonToken.empty()) {
                    popupRef->showFailMessage("Auth required to submit vote");
                    return;
                }
                int gameplayVote = 0;
                int originalityVote = 0;
                int difficultyVote = 0;

                bool includeGameplay = false;
                bool includeOriginality = false;
                bool includeDifficulty = false;

                if (m_gameplayInput) {
                    auto s = m_gameplayInput->getString();
                    if (!s.empty()) {
                        gameplayVote = numFromString<int>(s).unwrapOr(0);
                        gameplayVote = std::clamp(gameplayVote, 1, 10);
                        includeGameplay = true;
                    }
                }
                if (m_originalityInput) {
                    auto s = m_originalityInput->getString();
                    if (!s.empty()) {
                        originalityVote = numFromString<int>(s).unwrapOr(0);
                        originalityVote = std::clamp(originalityVote, 1, 10);
                        includeOriginality = true;
                    }
                }
                if (m_difficultyInput) {
                    auto string = m_difficultyInput->getString();
                    if (!string.empty()) {
                        difficultyVote = numFromString<int>(string).unwrapOr(0);
                        difficultyVote = std::clamp(difficultyVote, 1, 30);
                        includeDifficulty = true;
                    }
                }

                if (!includeGameplay && !includeOriginality && !includeDifficulty) {
                    popupRef->showFailMessage("No votes provided");
                    return;
                }

                matjson::Value body = matjson::Value::object();
                body["accountId"] = accountId;
                body["argonToken"] = argonToken;
                body["levelId"] = m_levelId;
                if (includeGameplay)
                    body["gameplayScore"] = gameplayVote;
                if (includeOriginality)
                    body["originalityScore"] = originalityVote;
                if (includeDifficulty)
                    body["difficultyScore"] = difficultyVote;

                auto req = web::WebRequest();
                req.bodyJSON(body);
                Ref<RLCommunityVotePopup> self = this;
                self->m_submitVoteTask.spawn(
                    req.post(std::string(rl::BASE_API_URL) + "/submitScore"),
                    [self, popupRef, gameplayVote, originalityVote, difficultyVote](web::WebResponse res) {
                        if (!self || !popupRef)
                            return;
                        if (!res.ok()) {
                            popupRef->showFailMessage(
                                "Failed to submit vote!");
                            return;
                        }
                        auto jsonRes = res.json();
                        if (!jsonRes) {
                            popupRef->showFailMessage("Invalid response from server.");
                            return;
                        }
                        auto json = jsonRes.unwrap();
                        bool success = json["success"].asBool().unwrapOrDefault();
                        std::string message =
                            json["message"].asString().unwrapOrDefault();
                        if (success) {
                            if (!self->m_mainLayer || !self->getParent()) {
                                popupRef->showSuccessMessage("Vote submitted!");
                                // increment community vote achievements progress and notify
                                // achievements system
                                int oldVotes =
                                    Mod::get()->getSavedValue<int>("community_votes");
                                int votes = oldVotes + 1;
                                Mod::get()->setSavedValue<int>("community_votes", votes);
                                log::info("Community votes total submitted: {}", votes);
                                RLAchievements::onUpdated(
                                    RLAchievements::Collectable::Votes, oldVotes, votes);
                                return;
                            }

                            popupRef->showSuccessMessage("Vote submitted!");
                            // increment community vote achievements progress and notify
                            // achievements system
                            int oldVotes =
                                Mod::get()->getSavedValue<int>("community_votes");
                            int votes = oldVotes + 1;
                            Mod::get()->setSavedValue<int>("community_votes", votes);
                            log::info("Community votes total submitted: {}", votes);
                            RLAchievements::onUpdated(RLAchievements::Collectable::Votes,
                                oldVotes,
                                votes);

                            // Optimistically mark inputs as VOTED and disable
                            if (self->m_gameplayInput && gameplayVote > 0) {
                                self->m_gameplayInput->setString("VOTED");
                                self->m_gameplayInput->setEnabled(false);
                            }
                            if (self->m_originalityInput && originalityVote > 0) {
                                self->m_originalityInput->setString("VOTED");
                                self->m_originalityInput->setEnabled(false);
                            }
                            if (self->m_difficultyInput && difficultyVote > 0) {
                                self->m_difficultyInput->setString("VOTED");
                                self->m_difficultyInput->setEnabled(false);
                            }

                            // Refresh UI by re-fetching data
                            self->refreshFromServer();
                        } else {
                            popupRef->showFailMessage(message);
                        }
                    });
            }
        });
}

bool RLCommunityVotePopup::init() {
    if (!Popup::init(420.f, 240.f, "GJ_square02.png"))
        return false;
    setTitle("Rated Layouts Community Vote");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupBlue, false);
    // Use taller vertical rows so label, score and input stack vertically
    float rowW = 100.f;
    float rowH = 130.f;
    float rowSpacing = 8.f;

    // originality row (vertical layout)
    auto originalityRow = CCNode::create();
    originalityRow->setContentSize({rowW, rowH});
    originalityRow->setAnchorPoint({0.5f, 0.5f});
    originalityRow->setPosition({10.f + rowW / 2.f, 55.f + rowH / 2.f});
    m_mainLayer->addChild(originalityRow);

    auto originalityLabel = CCLabelBMFont::create("Originality", "chatFont.fnt");
    originalityRow->addChild(originalityLabel);

    m_originalityScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_originalityScoreLabel->limitLabelWidth(80.f, 0.6f, 0.3f);
    originalityRow->addChild(m_originalityScoreLabel);

    // numeric text input below score
    m_originalityInput = TextInput::create(80.f, "1-10");
    m_originalityInput->setID("originality-vote-input");
    m_originalityInput->setCommonFilter(CommonFilter::Int);
    originalityRow->addChild(m_originalityInput);

    m_originalityStatsLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_originalityStatsLabel->setScale(0.3f);
    m_originalityStatsLabel->setAlignment(kCCTextAlignmentCenter);
    m_originalityStatsLabel->setColor({200, 200, 200});
    originalityRow->addChild(m_originalityStatsLabel);

    originalityRow->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true)->setAutoScale(false));
    originalityRow->updateLayout();

    m_originalityVote =
        numFromString<int>(m_originalityInput->getString()).unwrapOr(0);

    // difficulty row (vertical layout)
    auto difficultyRow = CCNode::create();
    difficultyRow->setContentSize({rowW, rowH});
    difficultyRow->setAnchorPoint({0.5f, 0.5f});
    difficultyRow->setPosition(
        {originalityRow->getPositionX() + 100.f, originalityRow->getPositionY()});
    m_mainLayer->addChild(difficultyRow);

    auto diffLabel = CCLabelBMFont::create("Difficulty", "chatFont.fnt");
    difficultyRow->addChild(diffLabel);

    m_difficultyScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_difficultyScoreLabel->limitLabelWidth(80.f, 0.6f, 0.3f);
    difficultyRow->addChild(m_difficultyScoreLabel);

    // numeric input for difficulty vote
    m_difficultyInput = TextInput::create(80.f, "1-30");
    m_difficultyInput->setID("difficulty-vote-input");
    m_difficultyInput->setCommonFilter(CommonFilter::Int);
    difficultyRow->addChild(m_difficultyInput);

    m_difficultyStatsLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_difficultyStatsLabel->setScale(0.3f);
    m_difficultyStatsLabel->setAlignment(kCCTextAlignmentCenter);
    m_difficultyStatsLabel->setColor({200, 200, 200});
    difficultyRow->addChild(m_difficultyStatsLabel);

    difficultyRow->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true)->setAutoScale(false));
    difficultyRow->updateLayout();

    m_difficultyVote =
        numFromString<int>(m_difficultyInput->getString()).unwrapOr(0);

    // gameplay row (vertical layout)
    auto gameplayRow = CCNode::create();
    gameplayRow->setContentSize({rowW, rowH});
    gameplayRow->setAnchorPoint({0.5f, 0.5f});
    gameplayRow->setPosition(
        {difficultyRow->getPositionX() + 100.f, originalityRow->getPositionY()});
    m_mainLayer->addChild(gameplayRow);

    auto gpLabel = CCLabelBMFont::create("Gameplay", "chatFont.fnt");
    gameplayRow->addChild(gpLabel);

    m_gameplayScoreLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_gameplayScoreLabel->limitLabelWidth(80.f, 0.6f, 0.3f);
    gameplayRow->addChild(m_gameplayScoreLabel);

    // numeric input for gameplay vote
    m_gameplayInput = TextInput::create(80.f, "1-10");
    m_gameplayInput->setID("gameplay-vote-input");
    m_gameplayInput->setCommonFilter(CommonFilter::Int);
    gameplayRow->addChild(m_gameplayInput);

    m_gameplayStatsLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_gameplayStatsLabel->setScale(0.3f);
    m_gameplayStatsLabel->setAlignment(kCCTextAlignmentCenter);
    m_gameplayStatsLabel->setColor({200, 200, 200});
    gameplayRow->addChild(m_gameplayStatsLabel);

    gameplayRow->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true)->setAutoScale(false));
    gameplayRow->updateLayout();

    m_gameplayVote = numFromString<int>(m_gameplayInput->getString()).unwrapOr(0);

    // moderators' average difficulty (vertical layout)
    auto modRow = CCNode::create();
    modRow->setContentSize({rowW, rowH});
    modRow->setAnchorPoint({0.5f, 0.5f});
    modRow->setPosition(
        {gameplayRow->getPositionX() + 100.f, originalityRow->getPositionY()});
    m_mainLayer->addChild(modRow);

    auto modLabel =
        CCLabelBMFont::create("Layout Mods'\nAverage Difficulty", "chatFont.fnt");
    modLabel->setAlignment(kCCTextAlignmentCenter);
    modLabel->setScale(0.8f);
    modRow->addChild(modLabel);

    m_modDifficultyLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_modDifficultyLabel->limitLabelWidth(80.f, 0.6f, 0.3f);
    modRow->addChild(m_modDifficultyLabel);

    modRow->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisReverse(true)->setAutoScale(false));
    modRow->updateLayout();

    // submit button
    auto submitSpr =
        ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png");
    auto submitBtn = CCMenuItemSpriteExtra::create(
        submitSpr, this, menu_selector(RLCommunityVotePopup::onSubmit));
    submitBtn->setPosition({m_mainLayer->getContentSize().width / 2.f, 25.f});
    m_submitBtn = submitBtn;
    m_buttonMenu->addChild(submitBtn);

    // total votes label
    m_totalVotesLabel = CCLabelBMFont::create("Total votes: -", "goldFont.fnt");
    m_totalVotesLabel->setPosition({m_mainLayer->getContentSize().width / 2.f,
        m_mainLayer->getContentSize().height - 40.f});
    m_totalVotesLabel->setScale(0.7f);
    m_totalVotesLabel->setAlignment(kCCTextAlignmentCenter);
    m_mainLayer->addChild(m_totalVotesLabel, 3);

    // info button
    auto infoSpr = CCSprite::createWithSpriteFrameName("RL_info01.png"_spr);
    infoSpr->setScale(0.7f);
    auto infoBtn = CCMenuItemSpriteExtra::create(
        infoSpr, this, menu_selector(RLCommunityVotePopup::onInfo));
    infoBtn->setPosition({25.f, 25.f});
    m_buttonMenu->addChild(infoBtn, 3);

    // single toggle for moderators to show/hide all scores at once
    if (rl::isUserClassicRole() || rl::isUserPlatformerRole() || rl::isUserOwner() || rl::isUserDeveloper()) {
        auto allSpr = CCSprite::createWithSpriteFrameName("hideBtn_001.png");
        allSpr->setOpacity(120);
        m_toggleAllBtn = CCMenuItemSpriteExtra::create(
            allSpr, this, menu_selector(RLCommunityVotePopup::onToggleAll));
        m_toggleAllBtn->setPosition(
            {m_mainLayer->getScaledContentSize().width - 30.f, 25.f});
        m_buttonMenu->addChild(m_toggleAllBtn, 3);
    }

    // fetch current scores from server
    if (m_levelId > 0) {
        refreshFromServer();
    }

    return true;
}

void RLCommunityVotePopup::refreshFromServer() {
    if (!m_forceShowScores) {
        if (m_originalityScoreLabel)
            m_originalityScoreLabel->setVisible(false);
        if (m_difficultyScoreLabel)
            m_difficultyScoreLabel->setVisible(false);
        if (m_gameplayScoreLabel)
            m_gameplayScoreLabel->setVisible(false);
    } else {
        if (m_originalityScoreLabel)
            m_originalityScoreLabel->setVisible(true);
        if (m_difficultyScoreLabel)
            m_difficultyScoreLabel->setVisible(true);
        if (m_gameplayScoreLabel)
            m_gameplayScoreLabel->setVisible(true);
    }

    Ref<RLCommunityVotePopup> self = this;

    // Then query whether this account has already voted on this level and
    // show/hide score labels accordingly
    matjson::Value voteBody = matjson::Value::object();
    voteBody["accountId"] = GJAccountManager::get()->m_accountID;
    voteBody["argonToken"] =
        Mod::get()->getSavedValue<std::string>("argon_token");
    voteBody["levelId"] = m_levelId;

    web::WebRequest voteReq;
    voteReq.bodyJSON(voteBody);
    self->m_getVoteTask.spawn(
        voteReq.post(std::string(rl::BASE_API_URL) + "/getVote"),
        [self](web::WebResponse vres) {
            if (!self)
                return;
            if (!vres.ok()) {
                return;
            }
            auto vj = vres.json();
            if (!vj)
                return;
            auto vjson = vj.unwrap();

            if (!self->m_mainLayer || !self->getParent()) {
                return;
            }

            bool hasGameplay = vjson["hasGameplayVoted"].asBool().unwrapOrDefault();
            bool hasOriginality =
                vjson["hasOriginalityVoted"].asBool().unwrapOrDefault();
            bool hasDifficulty =
                vjson["hasDifficultyVoted"].asBool().unwrapOrDefault();
            int totalVotes = vjson["totalVotes"].asInt().unwrapOrDefault();
            if (self->m_totalVotesLabel) {
                self->m_totalVotesLabel->setString(
                    fmt::format("Total votes: {}", totalVotes).c_str());
            }

            double originalityScore = -1.0;
            if (vjson.contains("originalityScore")) {
                auto osRes = vjson["originalityScore"].asDouble();
                if (osRes)
                    originalityScore = osRes.unwrap();
            }
            double difficultyScore = -1.0;
            if (vjson.contains("difficultyScore")) {
                auto dsRes = vjson["difficultyScore"].asDouble();
                if (dsRes)
                    difficultyScore = dsRes.unwrap();
            }
            double gameplayScore = -1.0;
            if (vjson.contains("gameplayScore")) {
                auto gpRes = vjson["gameplayScore"].asDouble();
                if (gpRes)
                    gameplayScore = gpRes.unwrap();
            }

            if (self->m_originalityScoreLabel) {
                if (originalityScore >= 0.0) {
                    self->m_originalityScoreLabel->setString(
                        fmt::format("{:.2f}", originalityScore).c_str());
                }
            }
            if (self->m_difficultyScoreLabel) {
                if (difficultyScore >= 0.0) {
                    self->m_difficultyScoreLabel->setString(
                        fmt::format("{:.2f}", difficultyScore).c_str());
                }
            }
            if (self->m_gameplayScoreLabel) {
                if (gameplayScore >= 0.0) {
                    self->m_gameplayScoreLabel->setString(
                        fmt::format("{:.2f}", gameplayScore).c_str());
                }
            }

            auto updateStatsLabel = [&](CCLabelBMFont* label, std::string prefix) {
                if (!label) return;
                double minVal = -1.0, maxVal = -1.0, midVal = -1.0, noMinVal = -1.0, noMaxVal = -1.0;
                if (vjson.contains(prefix + "Min") && vjson[prefix + "Min"].asDouble())
                    minVal = vjson[prefix + "Min"].asDouble().unwrap();
                if (vjson.contains(prefix + "Max") && vjson[prefix + "Max"].asDouble())
                    maxVal = vjson[prefix + "Max"].asDouble().unwrap();
                if (vjson.contains(prefix + "Mid") && vjson[prefix + "Mid"].asDouble())
                    midVal = vjson[prefix + "Mid"].asDouble().unwrap();
                if (vjson.contains(prefix + "NoMin") && vjson[prefix + "NoMin"].asDouble())
                    noMinVal = vjson[prefix + "NoMin"].asDouble().unwrap();
                if (vjson.contains(prefix + "NoMax") && vjson[prefix + "NoMax"].asDouble())
                    noMaxVal = vjson[prefix + "NoMax"].asDouble().unwrap();

                auto formatVal = [](double val, bool isInt) -> std::string {
                    if (val < 0) return "-";
                    if (isInt) return fmt::format("{:.0f}", val);
                    return fmt::format("{:.1f}", val);
                };

                std::string statsStr = fmt::format("Min: {}\nMax: {}\nMid: {}\nNo Min: {}\nNo Max: {}",
                    formatVal(minVal, true),
                    formatVal(maxVal, true),
                    formatVal(midVal, false),
                    formatVal(noMinVal, false),
                    formatVal(noMaxVal, false));
                label->setString(statsStr.c_str());

                if (auto row = typeinfo_cast<CCNode*>(label->getParent())) {
                    if (auto layout = typeinfo_cast<ColumnLayout*>(row->getLayout())) {
                        row->updateLayout();
                    }
                }
            };

            updateStatsLabel(self->m_originalityStatsLabel, "originality");
            updateStatsLabel(self->m_difficultyStatsLabel, "difficulty");
            updateStatsLabel(self->m_gameplayStatsLabel, "gameplay");

            // moderator difficulty (read-only) - prefer averageDifficulty from
            // getVote
            double avg = -1.0;
            if (vjson.contains("averageDifficulty")) {
                auto avgRes = vjson["averageDifficulty"].asDouble();
                if (avgRes)
                    avg = avgRes.unwrap();
            }

            if (self->m_modDifficultyLabel) {
                if (avg >= 0.0) {
                    if (std::floor(avg) == avg) {
                        self->m_modDifficultyLabel->setString(
                            numToString(static_cast<int>(avg)).c_str());
                    } else {
                        self->m_modDifficultyLabel->setString(
                            fmt::format("{:.1f}", avg).c_str());
                    }
                } else {
                    self->m_modDifficultyLabel->setString("-");
                }
            }

            // If moderators forced show, keep labels visible regardless of 'hasX'
            // vote state
            if (self->m_forceShowScores) {
                if (self->m_gameplayInput && hasGameplay) {
                    self->m_gameplayInput->setVisible(false);
                }
                if (self->m_originalityInput && hasOriginality) {
                    self->m_originalityInput->setVisible(false);
                }
                if (self->m_difficultyInput && hasDifficulty) {
                    self->m_difficultyInput->setVisible(false);
                }

                if (self->m_gameplayScoreLabel)
                    self->m_gameplayScoreLabel->setVisible(true);
                if (self->m_originalityScoreLabel)
                    self->m_originalityScoreLabel->setVisible(true);
                if (self->m_difficultyScoreLabel)
                    self->m_difficultyScoreLabel->setVisible(true);

                if (self->m_gameplayStatsLabel) self->m_gameplayStatsLabel->setVisible(true);
                if (self->m_originalityStatsLabel) self->m_originalityStatsLabel->setVisible(true);
                if (self->m_difficultyStatsLabel) self->m_difficultyStatsLabel->setVisible(true);
            } else {
                if (hasGameplay) {
                    if (self->m_gameplayInput) {
                        self->m_gameplayInput->setVisible(false);
                    }
                    if (self->m_gameplayScoreLabel)
                        self->m_gameplayScoreLabel->setVisible(true);
                    if (self->m_gameplayStatsLabel)
                        self->m_gameplayStatsLabel->setVisible(true);
                } else {
                    if (self->m_gameplayScoreLabel)
                        self->m_gameplayScoreLabel->setVisible(false);
                    if (self->m_gameplayStatsLabel)
                        self->m_gameplayStatsLabel->setVisible(false);
                }

                if (hasOriginality) {
                    if (self->m_originalityInput) {
                        self->m_originalityInput->setVisible(false);
                    }
                    if (self->m_originalityScoreLabel)
                        self->m_originalityScoreLabel->setVisible(true);
                    if (self->m_originalityStatsLabel)
                        self->m_originalityStatsLabel->setVisible(true);
                } else {
                    if (self->m_originalityScoreLabel)
                        self->m_originalityScoreLabel->setVisible(false);
                    if (self->m_originalityStatsLabel)
                        self->m_originalityStatsLabel->setVisible(false);
                }

                if (hasDifficulty) {
                    if (self->m_difficultyInput) {
                        self->m_difficultyInput->setVisible(false);
                    }
                    if (self->m_difficultyScoreLabel)
                        self->m_difficultyScoreLabel->setVisible(true);
                    if (self->m_difficultyStatsLabel)
                        self->m_difficultyStatsLabel->setVisible(true);
                } else {
                    if (self->m_difficultyScoreLabel)
                        self->m_difficultyScoreLabel->setVisible(false);
                    if (self->m_difficultyStatsLabel)
                        self->m_difficultyStatsLabel->setVisible(false);
                }
            }

            // Update layout for columns to account for hidden TextInputs
            if (self->m_originalityScoreLabel && self->m_originalityScoreLabel->getParent()) {
                if (auto row = typeinfo_cast<CCNode*>(self->m_originalityScoreLabel->getParent())) {
                    row->updateLayout();
                }
            }
            if (self->m_difficultyScoreLabel && self->m_difficultyScoreLabel->getParent()) {
                if (auto row = typeinfo_cast<CCNode*>(self->m_difficultyScoreLabel->getParent())) {
                    row->updateLayout();
                }
            }
            if (self->m_gameplayScoreLabel && self->m_gameplayScoreLabel->getParent()) {
                if (auto row = typeinfo_cast<CCNode*>(self->m_gameplayScoreLabel->getParent())) {
                    row->updateLayout();
                }
            }

            // Ensure toggle buttons reflect current visibility state for moderators
            if (rl::isUserClassicRole() || rl::isUserPlatformerRole() || rl::isUserOwner() || rl::isUserDeveloper()) {
                if (self->m_toggleAllBtn)
                    self->m_toggleAllBtn->setVisible(true);
            } else {
                if (self->m_toggleAllBtn)
                    self->m_toggleAllBtn->setVisible(false);
            }

            // If all three categories have already been voted on, disable submit
            if (hasGameplay && hasOriginality && hasDifficulty) {
                if (self->m_submitBtn) {
                    self->m_submitBtn->setEnabled(false);
                    self->m_submitBtn->setNormalImage(ButtonSprite::create(
                        "Submit", "goldFont.fnt", "GJ_button_04.png"));
                }
            } else {
                if (self->m_submitBtn) {
                    self->m_submitBtn->setEnabled(true);
                    self->m_submitBtn->setNormalImage(ButtonSprite::create(
                        "Submit", "goldFont.fnt", "GJ_button_01.png"));
                }
            }

            // When moderators toggle, their actions should not be blocked by
            // 'already voted' state
            if (self->m_toggleAllBtn)
                self->m_toggleAllBtn->setEnabled(true);
        });
}

void RLCommunityVotePopup::onInfo(CCObject*) {
    MDPopup::create(
        "Community Votes & Layout Ratings",
        "Depending on the community scoring that a layout obtains on any of the three categories, it is very likely to affect an individual layout's rating drastically.\n"
        "Layout Admins will use these as references to further determine a layout's rating.\n\n"
        "\r\n\r\n---\r\n\r\n"
        "## Community Votes Specific Cases\n"
        "A layout's rating tier can be changed based on these cases around community votes:\n"
        "### Case 1: Lowering a Rating Tier\n"
        "- If <co>Originality</c> or <cg>Gameplay</c> aspects of the community votes have a score of _**5 or lower** (rounded to zero decimal places/one significant figure)_, the **rating tier will be decreased indefinitely** than its earlier intended rate when still being sent.\n"
        "### Case 2: Increase of a Rating Tier\n"
        "- If <co>Originality</c> or <cg>Gameplay</c> aspects of the community vote have a score of __**9 or higher** (rounded to zero decimal places/one significant figure)__, the **rating tier will potentially be increased** based on staffs and community's general opinions, as long as it still fully adheres to standards.\n"
        "### Case 3: Determining a Difficulty\n"
        "- The <cr>Difficulty</c> score will be rounded to one or two significant figures and will determine the difficulty tier for a layout. <cy>*(Example: score of 21-24 will determine the layout being a Hard Demon rate, whilist 25-28 is Insane Demon. Similar spreads works on other difficulties)*</c>\n"
        "\r\n\r\n---\r\n\r\n"
        "## Community Votes Categories\n\n"
        "All scores are rounded by nearest whole number/integer.\n"
        "### Originality Score\n\n"
        "<co>Originality Score</c> is used to determine how <cg>distinctive and recognizable</c> the work is done in said layouts, primarily how much the <cy>sync and song representation</c> is done in the gameplay, alongside the <cl>structuring/pattern style</c>. <co>Originality score</c> may be used to affect a rating tier, however standards still apply in the topic of <cy>sync and song representation</c>.\n"
        "### Difficulty Score\n\n"
        "Self explanatory. Ranges from <co>1-30</c> based on <cl>Sparks</c>/<co>Planets</c> difficulty ratings.\n\n"
        "For Demons, these are how scores determine the difficulty of any layout:\n\n"
        "- **10-14** = Easy Demon\n"
        "- **15-19** = Medium Demon\n"
        "- **20-24** = Hard Demon\n"
        "- **25-28** = Insane Demon\n"
        "- **29-30** = Extreme Demon\n"
        "### Gameplay Score **(most important)**\n\n"
        "This category determines whether a <cg>layout is enjoyable and engaging to play/buggy</c> or <cr>facing recurring and problematic issues around its playability</c>\n\n"
        "A score of _**8=>**_ will _potentially increase the layouts rating tier_, however a score of _**<=5**_ will _guarantee a rating tier decrease_. We track reports if a layout has recurring issues and is required to have a rating tier change.\n\n"
        "\r\n\r\n---\r\n\r\n"
        "### Extra note\n\n"
        "It is <cl>**highly recommended**</c> to <cg>beat a layout</c> before submitting a community vote for it for a much more <cy>precise average</c>.\n\n"
        "**3 or more votes** are <cg>required</c> for community votes to be <cc>accounted into changing a layout's rating</c>.",
        "OK")
        ->show();
}

void RLCommunityVotePopup::onToggleAll(CCObject* sender) {
    // toggle visibility for all three score labels
    if (!m_originalityScoreLabel || !m_difficultyScoreLabel ||
        !m_gameplayScoreLabel)
        return;

    bool anyVisible = m_originalityScoreLabel->isVisible() ||
                      m_difficultyScoreLabel->isVisible() ||
                      m_gameplayScoreLabel->isVisible();
    bool newVis = !anyVisible;
    m_forceShowScores = newVis;

    m_originalityScoreLabel->setVisible(newVis);
    m_difficultyScoreLabel->setVisible(newVis);
    m_gameplayScoreLabel->setVisible(newVis);

    if (m_originalityStatsLabel) m_originalityStatsLabel->setVisible(newVis);
    if (m_difficultyStatsLabel) m_difficultyStatsLabel->setVisible(newVis);
    if (m_gameplayStatsLabel) m_gameplayStatsLabel->setVisible(newVis);

    // update toggle visual state if possible
    if (m_toggleAllBtn) {
        auto normal = m_toggleAllBtn->getNormalImage();
        auto spr = static_cast<CCSprite*>(normal);
        if (spr)
            spr->setOpacity(newVis ? 255 : 120);
    }

    if (newVis) {
        // refresh scores when showing
        refreshFromServer();
    }

    // Update layout for columns
    if (m_originalityScoreLabel && m_originalityScoreLabel->getParent()) {
        if (auto row = typeinfo_cast<CCNode*>(m_originalityScoreLabel->getParent())) {
            row->updateLayout();
        }
    }
    if (m_difficultyScoreLabel && m_difficultyScoreLabel->getParent()) {
        if (auto row = typeinfo_cast<CCNode*>(m_difficultyScoreLabel->getParent())) {
            row->updateLayout();
        }
    }
    if (m_gameplayScoreLabel && m_gameplayScoreLabel->getParent()) {
        if (auto row = typeinfo_cast<CCNode*>(m_gameplayScoreLabel->getParent())) {
            row->updateLayout();
        }
    }
}
