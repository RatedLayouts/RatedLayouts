#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>
#include <vector>

using namespace geode::prelude;

class RLRubiesCodePopup : public geode::Popup {
public:
    static RLRubiesCodePopup* create();

private:
    struct CodeItem {
        int id;
        std::string code;
        std::string reward;
    };

    bool init() override;

    void onCheckCode(CCObject* sender);
    void onCrossCode(CCObject* sender);
    void onNewCode(CCObject* sender);

    cue::ListNode* m_listNode = nullptr;
    ScrollLayer* m_scrollLayer = nullptr;
    async::TaskHolder<web::WebResponse> m_fetchTask;
    async::TaskHolder<web::WebResponse> m_deleteTask;
    std::vector<CodeItem> m_codeItems;

    ~RLRubiesCodePopup() {
        m_fetchTask.cancel();
        m_scrollLayer = nullptr;
    }
};