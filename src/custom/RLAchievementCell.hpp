#pragma once

#include <Geode/Geode.hpp>
#include "../include/RLAchievements.hpp"

using namespace geode::prelude;

TableViewCell* RLAchievementCell(RLAchievements::Achievement const& ach, bool unlocked);