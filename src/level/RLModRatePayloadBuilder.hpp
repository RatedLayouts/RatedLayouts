#pragma once

#include <Geode/Geode.hpp>
#include "RLModRatePopup.hpp"

class RLModRatePayloadBuilder {
public:
    RLModRatePayloadBuilder(RLModRatePopup* owner, UploadActionPopup* popup, bool suggest);
    bool build(matjson::Value& outBody);

private:
    RLModRatePopup* m_owner;
    UploadActionPopup* m_popup;
    bool m_isSuggest;
    std::string m_token;
};
