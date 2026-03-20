#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLSpireLayer : public CCLayer {
protected:
    bool init() override;
    void keyBackClicked() override;

public:
    static RLSpireLayer* create();

private:
    void onEnterSpire(CCObject* sender);
    int m_indexDia = 0;
    CCSprite* m_spireSpr = nullptr;
};