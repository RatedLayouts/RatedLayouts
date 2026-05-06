#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLQueueLevelPopup : public Popup, public LevelManagerDelegate {
private:
    bool init() override;
    void checkLevelID(CCObject* sender);
    void submitToQueue(CCObject* sender);
    void onInfo(CCObject* sender);
    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2) override;
    void loadLevelsFailed(char const* key, int p1) override;

    TextInput* m_levelIDInput;
    LevelCell* m_levelCell;
    CCMenuItemSpriteExtra* m_submitButton;
    CCLabelBMFont* m_noLevelLabel;
    bool m_levelExists = false;
    int m_selectedLevelId = 0;

public:
    static RLQueueLevelPopup* create();
};