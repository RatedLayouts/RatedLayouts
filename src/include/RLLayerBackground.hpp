#pragma once

#include <Geode/Geode.hpp>
#include <cue/RepeatingBackground.hpp>

using namespace geode::prelude;

namespace rl {
    // Adds the user-configured background to a layer. When the
    // "disableBackground" setting is on a plain solid-colour background is
    // used; otherwise a scrolling tiled game background is applied.
    // The node is added at z-order -1 so it sits behind all other content.
    inline void addLayerBackground(cocos2d::CCNode* parent) {
        auto color = Mod::get()->getSettingValue<cocos2d::ccColor3B>("rgbBackground");

        if (Mod::get()->getSettingValue<bool>("disableBackground")) {
            auto bg = createLayerBG();
            bg->setColor(color);
            parent->addChild(bg, -1);
            return;
        }

        auto value = Mod::get()->getSettingValue<int>("backgroundType");
        std::string bgIndex = (value >= 1 && value <= 9)
                                  ? ("0" + numToString(value))
                                  : numToString(value);
        std::string bgName = "game_bg_" + bgIndex + "_001.png";
        auto bg = cue::RepeatingBackground::create(bgName.c_str(), 1.f, cue::RepeatMode::X);
        bg->setColor(color);
        parent->addChild(bg, -1);
    }
}  // namespace rl
