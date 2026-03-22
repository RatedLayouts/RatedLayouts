#include "RLModRatePayloadBuilder.hpp"

using namespace geode::prelude;

RLModRatePayloadBuilder::RLModRatePayloadBuilder(RLModRatePopup* owner,
    UploadActionPopup* popup,
    bool suggest)
    : m_owner(owner), m_popup(popup), m_isSuggest(suggest) {}

bool RLModRatePayloadBuilder::build(matjson::Value& outBody) {
    if (!m_owner->ensureToken(m_token, m_popup, "Token not found!"))
        return false;

    if (!m_owner->validateDifficultyOrRating(m_popup))
        return false;

    outBody = matjson::Value::object();
    outBody["accountId"] = GJAccountManager::get()->m_accountID;
    outBody["argonToken"] = m_token;
    outBody["levelId"] = m_owner->m_levelId;
    outBody["levelOwnerId"] = m_owner->m_accountId;
    outBody["isPlat"] = (m_owner->m_level ? m_owner->m_level->isPlatformer() : false);
    outBody["featured"] = m_owner->determineFeaturedValue();

    if (m_isSuggest) {
        outBody["suggest"] = true;
    }

    if (m_owner->m_notesInput) {
        auto noteStr = m_owner->m_notesInput->getString();
        if (!noteStr.empty())
            outBody["note"] = std::string(noteStr);
    }

    m_owner->applyDifficultyField(outBody);
    m_owner->applyFeaturedScore(outBody);

    if (!m_isSuggest) {
        m_owner->applyVerifiedFlag(outBody);
    }

    return true;
}
