#pragma once

#include <string_view>

namespace rl {
    // Account ID of the mod owner / primary developer (ArcticWoof).
    constexpr int DEV_ACCOUNT_ID = 7689052;

    // Base URL for all Rated Layouts API endpoints.
    // Use this constant when constructing new endpoint URLs to avoid
    // scattering the server address across many files.
    constexpr std::string_view BASE_API_URL = "https://gdrate.arcticwoof.xyz";
}  // namespace rl
