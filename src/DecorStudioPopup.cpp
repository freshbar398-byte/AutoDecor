#include "DecorStudioPopup.hpp"

#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "Decor.hpp"
#include "Styles.hpp"

using namespace geode::prelude;

DecorStudioPopup* DecorStudioPopup::create(EditorUI* ui) {
    auto ret = new DecorStudioPopup();
    if (ret && ret->init(ui)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool DecorStudioPopup::init(EditorUI* ui) {
    if (!Popup::init(390.f, 300.f)) return false;

    m_ui = ui;
    this->setTitle("AUTO DECOR");

    auto const& styles = autodecor::builtinStyles();
    m_styleIndex = std::clamp(
        Mod::get()->getSavedValue<int>("style", 0), 0,
        static_cast<int>(styles.size()) - 1
    );
    m_wholeLevel = Mod::get()->getSavedValue<bool>("whole-level", false);

    // Координаты ниже — смещения от центра попапа. m_buttonMenu всегда стоит
    // в центре, поэтому ничего сдвигать не нужно.

    // --- сетка стилей ----------------------------------------------------
    constexpr float startY = 92.f;
    constexpr float stepY = 40.f;

    for (int i = 0; i < static_cast<int>(styles.size()); ++i) {
        auto off = ButtonSprite::create(
            styles[i].name, "goldFont.fnt", "GJ_button_01.png", .5f
        );
        auto on = ButtonSprite::create(
            styles[i].name, "goldFont.fnt", "GJ_button_02.png", .5f
        );

        auto toggle = CCMenuItemToggler::create(
            off, on, this, menu_selector(DecorStudioPopup::onStyle)
        );
        toggle->setTag(i);
        toggle->toggle(i == m_styleIndex);
        toggle->setPosition({
            (i % 2 == 0) ? -84.f : 84.f,
            startY - (i / 2) * stepY,
        });
        m_buttonMenu->addChild(toggle);
        m_styleToggles.push_back(toggle);
    }

    // --- режим: выделение / весь уровень ---------------------------------
    auto modeOff = ButtonSprite::create(
        "SELECTION ONLY", "goldFont.fnt", "GJ_button_01.png", .5f
    );
    auto modeOn = ButtonSprite::create(
        "WHOLE LEVEL", "goldFont.fnt", "GJ_button_02.png", .5f
    );
    m_modeToggle = CCMenuItemToggler::create(
        modeOff, modeOn, this, menu_selector(DecorStudioPopup::onMode)
    );
    m_modeToggle->toggle(m_wholeLevel);
    m_modeToggle->setPosition({ 0.f, -70.f });
    m_buttonMenu->addChild(m_modeToggle);

    // --- применить --------------------------------------------------------
    auto applyCaption = ButtonSprite::create(
        "DECORATE", "goldFont.fnt", "GJ_button_01.png", .7f
    );
    auto apply = CCMenuItemSpriteExtra::create(
        applyCaption, this, menu_selector(DecorStudioPopup::onApply)
    );
    apply->setID("decorate-button"_spr);
    apply->setPosition({ 0.f, -114.f });
    m_buttonMenu->addChild(apply);

    auto hint = CCLabelBMFont::create(
        "pick a style, then hit DECORATE", "chatFont.fnt"
    );
    hint->setScale(.55f);
    hint->setOpacity(150);
    auto center = m_buttonMenu->getPosition();
    hint->setPosition({ center.x, center.y - 138.f });
    m_mainLayer->addChild(hint);

    return true;
}

void DecorStudioPopup::onStyle(cocos2d::CCObject* sender) {
    auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
    if (!toggle) return;

    for (auto* other : m_styleToggles) {
        if (other != toggle) other->toggle(false);
    }
    toggle->toggle(true);

    m_styleIndex = std::clamp(
        toggle->getTag(), 0,
        static_cast<int>(autodecor::builtinStyles().size()) - 1
    );
    Mod::get()->setSavedValue<int>("style", m_styleIndex);
}

void DecorStudioPopup::onMode(cocos2d::CCObject* sender) {
    m_wholeLevel = !m_wholeLevel;
    if (m_modeToggle) m_modeToggle->toggle(m_wholeLevel);
    Mod::get()->setSavedValue<bool>("whole-level", m_wholeLevel);
}

void DecorStudioPopup::onApply(cocos2d::CCObject*) {
    auto editor = LevelEditorLayer::get();
    if (!editor) {
        Notification::create("Editor not found", NotificationIcon::Error)->show();
        return;
    }

    auto cfg = autodecor::readConfig();
    auto const& styles = autodecor::builtinStyles();
    auto const& style = styles[m_styleIndex];

    std::vector<GameObject*> targets;
    auto collect = [&targets, &cfg](cocos2d::CCArray* array) {
        if (!array) return;
        for (unsigned int i = 0; i < array->count(); ++i) {
            auto obj = typeinfo_cast<GameObject*>(array->objectAtIndex(i));
            if (!obj) continue;
            // не декорируем то, что уже создал мод
            if (obj->getUserFlag(autodecor::kMadeFlag)) continue;
            if (cfg.skipTriggers && obj->m_isTrigger) continue;
            targets.push_back(obj);
        }
    };

    cocos2d::CCArray* selection = m_ui ? m_ui->getSelectedObjects() : nullptr;
    bool const hasSelection = selection && selection->count() > 0;

    if (!m_wholeLevel) {
        if (!hasSelection) {
            Notification::create(
                "Select objects first, or switch to WHOLE LEVEL",
                NotificationIcon::Warning
            )->show();
            return;
        }
        collect(selection);
    } else {
        collect(editor->getAllObjects());
    }

    if (targets.empty()) {
        Notification::create(
            "Nothing to decorate", NotificationIcon::Warning
        )->show();
        return;
    }

    bool truncated = false;
    if (static_cast<int>(targets.size()) > cfg.maxTargets) {
        targets.resize(cfg.maxTargets);
        truncated = true;
    }

    log::info(
        "Auto Decor: style '{}', ids dot={} glow={} spike={} saw={}",
        style.id,
        autodecor::resolveSlot(autodecor::Slot::Dot),
        autodecor::resolveSlot(autodecor::Slot::Glow),
        autodecor::resolveSlot(autodecor::Slot::Spike),
        autodecor::resolveSlot(autodecor::Slot::Saw)
    );

    autodecor::Decorator decorator(editor, cfg, style);
    for (auto obj : targets) {
        decorator.decorate(obj);
    }

    int const made = decorator.created();
    log::info("Auto Decor: created {} objects", made);

    if (made == 0) {
        Notification::create(
            "Could not create decor - check object IDs in Styles.hpp",
            NotificationIcon::Error
        )->show();
        return;
    }

    std::string message = "Auto Decor: +" + std::to_string(made) + " objects";
    if (truncated) message += ", some targets skipped";
    if (decorator.hitLimit()) message += " (per-click limit)";

    Notification::create(message, NotificationIcon::Success)->show();
    this->onClose(nullptr);
}
