#include <Geode/Geode.hpp>
#include <Geode/modify/DialogLayer.hpp>
#include "../include/RLDialogIcons.hpp"

using namespace geode::prelude;

class $modify(RLDialogLayerHook, DialogLayer) {
    void displayDialogObject(DialogObject* dialogObject) {
        DialogLayer::displayDialogObject(dialogObject);
        if (!dialogObject || !this->m_mainLayer) {
            return;
        }

        if (this->m_characterSprite) {
            this->m_characterSprite->setVisible(false);
        } else {
            return;
        }

        if (dialogObject->m_characterFrame >= 0 &&
            dialogObject->m_characterFrame < rl::DialogIconCount) {
            rl::setDialogObjectIcon(this, dialogObject->m_characterFrame);
            return;
        }

        rl::setDialogObjectIcon(this, 0);
    }
};
