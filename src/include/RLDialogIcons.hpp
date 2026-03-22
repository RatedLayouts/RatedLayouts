#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace rl {
    constexpr int DialogIconTagBase = 10100;
    constexpr int DialogIconCount = 5;

    void setDialogObjectIcon(DialogLayer* dialog, int characterFrame);
    void setDialogObjectCustomIcon(DialogLayer* dialog, const std::string& frameName);
}
