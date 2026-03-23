#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace rl {
    // Returns the server's response body as the error message when it is
    // non-empty, otherwise falls back to the provided fallback string.
    inline std::string getResponseFailMessage(web::WebResponse const& response,
        std::string const& fallback) {
        auto message = response.string().unwrapOrDefault();
        if (!message.empty())
            return message;
        return fallback;
    }
}  // namespace rl
