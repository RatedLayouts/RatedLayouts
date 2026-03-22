#pragma once
#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include <functional>

using namespace geode::prelude;

class RLAddCodePopup : public geode::Popup {
public:
    static RLAddCodePopup* create(const std::string& code = "", const std::string& reward = "", long long id = -1, std::function<void()> onSuccess = nullptr);

private:
    bool init(const std::string& code, const std::string& reward, long long id, std::function<void()> onSuccess);
    void onAdd(CCObject* sender);

    TextInput* m_codeInput = nullptr;
    TextInput* m_rewardInput = nullptr;
    CCLabelBMFont* m_editingLabel = nullptr;
    long long m_id = -1;
    std::function<void()> m_onSuccess;
    async::TaskHolder<web::WebResponse> m_submitTask;

    ~RLAddCodePopup() {
        m_submitTask.cancel();
    }
};