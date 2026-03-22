#pragma once
#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class RLCreditsPopup : public geode::Popup {
public:
    static RLCreditsPopup* create();

private:
    bool init() override;
    void onAccountClicked(CCObject* sender);
    void onInfo(CCObject* sender);
    void onHeaderInfo(CCObject* sender);
    cue::ListNode* m_listNode = nullptr;
    LoadingSpinner* m_spinner = nullptr;
};
