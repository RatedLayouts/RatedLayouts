#pragma once
#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class RLModsNotesPopup : public geode::Popup {
public:
    static RLModsNotesPopup* create(GJGameLevel* level);

private:
    bool init() override;

    GJGameLevel* m_level = nullptr;
    cue::ListNode* m_listNode = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    async::TaskHolder<web::WebResponse> m_getNotesTask;

    ~RLModsNotesPopup() {
        m_getNotesTask.cancel();
        m_scrollLayer = nullptr;
    }
};
