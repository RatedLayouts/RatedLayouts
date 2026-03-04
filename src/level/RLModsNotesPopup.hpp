#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLModsNotesPopup : public geode::Popup {
public:
    static RLModsNotesPopup* create(GJGameLevel* level);

private:
    bool init() override;

    GJGameLevel* m_level = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    async::TaskHolder<web::WebResponse> m_getNotesTask;

    ~RLModsNotesPopup() {
        m_getNotesTask.cancel();
    }
};
