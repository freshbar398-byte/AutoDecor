// Auto Decor: кнопка в редакторе, которая открывает выбор стиля декора.
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "DecorStudioPopup.hpp"

using namespace geode::prelude;

class $modify(AutoDecorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->setID("auto-decor-menu"_spr);
        this->addChild(menu, 999);

        auto caption = ButtonSprite::create(
            "DECOR", "goldFont.fnt", "GJ_button_01.png", .6f
        );
        auto button = CCMenuItemSpriteExtra::create(
            caption, this, menu_selector(AutoDecorUI::onAutoDecor)
        );
        button->setID("auto-decor-button"_spr);
        button->setPosition({ 36.f, winSize.height * .45f });
        menu->addChild(button);

        return true;
    }

    // Прячем кнопку вместе с остальным UI редактора.
    void showUI(bool show) {
        EditorUI::showUI(show);
        if (auto menu = this->getChildByID("auto-decor-menu"_spr)) {
            menu->setVisible(show);
        }
    }

    void onAutoDecor(CCObject*) {
        if (auto popup = DecorStudioPopup::create(this)) {
            popup->show();
        }
    }
};