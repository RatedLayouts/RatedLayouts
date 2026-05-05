#include "RLBadgeRequestPopup.hpp"
#include "../include/RLConstants.hpp"
#include "../include/RLNetworkUtils.hpp"

using namespace geode::prelude;

RLBadgeRequestPopup* RLBadgeRequestPopup::create() {
    auto ret = new RLBadgeRequestPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RLBadgeRequestPopup::init() {
    if (!Popup::init(440.f, 280.f, "GJ_square04.png"))
        return false;

    setTitle("Claim Layout Supporter Badge");

    auto cs = m_mainLayer->getContentSize();

    m_emailInput = TextInput::create(370.f, "Email Address");
    m_emailInput->setCommonFilter(CommonFilter::Any);
    m_emailInput->setPosition({cs.width / 2.f, cs.height - 50.f});
    m_mainLayer->addChild(m_emailInput);

    // info text
    auto infoText = MDTextArea::create(
        "Enter your <co>Email Address</c> that is linked "
        "to your <cp>Ko-fi account</c> to receive a <cp>Layout Supporter Badge</c>.\n\n"
        "After you <cg>successfuly get your badge</c>, you can click the <co>Request</c> button to request access to supporter-only features in Rated Layouts!\n\n"
        "Make sure that you got the <cd>Rated Layouts Supporter Membership</c> and have already <cg>linked</c> your <cb>Email</c> through <cp>Ko-fi.</c> beforehand!\n\n"
        "### If you encounter any <cr>issues</c> during this process, please contact <cf>[ArcticWoof](user:7689052)</c> via GD DMs or through <cp>[Ko-Fi](https://ko-fi.com/arcticwoof) messages</c>.",
        {cs.width - 40.f, 150.f});
    infoText->setPosition({cs.width / 2.f, cs.height - 150.f});
    infoText->setAnchorPoint({0.5f, 0.5f});
    infoText->setID("rl-badge-request-info");
    m_mainLayer->addChild(infoText);

    // claim button
    auto submitSpr =
        ButtonSprite::create("Claim", "goldFont.fnt", "GJ_button_01.png");
    auto submitBtn = CCMenuItemSpriteExtra::create(
        submitSpr, this, menu_selector(RLBadgeRequestPopup::onSubmit));
    submitBtn->setPosition({cs.width / 2.f, 25.f});
    m_buttonMenu->addChild(submitBtn);

    return true;
}

void RLBadgeRequestPopup::onSubmit(CCObject* sender) {
    if (!m_emailInput)
        return;
    auto upopup =
        UploadActionPopup::create(nullptr, "Submitting Badge Request...");
    upopup->show();
    auto email = m_emailInput->getString();
    if (email.empty()) {
        upopup->showFailMessage("Please enter your email address");
        return;
    }

    matjson::Value body = matjson::Value::object();
    body["email"] = std::string(email);
    body["argonToken"] = Mod::get()->getSavedValue<std::string>("argon_token");
    body["accountId"] = GJAccountManager::get()->m_accountID;

    auto req = web::WebRequest();
    req.bodyJSON(body);
    Ref<RLBadgeRequestPopup> self = this;
    self->m_getSupporterTask.spawn(
        req.post(std::string(rl::BASE_API_URL) + "/getSupporter"),
        [self, upopup](web::WebResponse res) {
            if (!self)
                return;
            if (!res.ok()) {
                upopup->showFailMessage(rl::getResponseFailMessage(res, "Email address doesn't exist."));
                return;
            }

            auto str = res.string().unwrapOrDefault();
            if (!str.empty()) {
                if (!res.ok()) {
                    upopup->showFailMessage(str);
                    return;
                }
                Notification::create(str, NotificationIcon::Success)->show();
                self->removeFromParent();
                return;
            }

            if (!res.ok()) {
                // show status code
                upopup->showFailMessage(
                    fmt::format("Failed to submit request (code {})", res.code()));
                return;
            }

            upopup->showSuccessMessage("Supporter Badge acquired!");
            self->removeFromParent();
        });
}
