#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include "Geode/ui/Popup.hpp"
using namespace geode::prelude;

class RLNameplateTestPopup : public Popup {
public:
    static RLNameplateTestPopup* create();
private:
    bool init() override;
    void onPickImage(CCObject* sender);
    arc::Future<void> pickAndLoadPng();

    cue::ListNode* m_leaderboardListNode;
    LazySprite* m_nameplateLazy = nullptr;
};