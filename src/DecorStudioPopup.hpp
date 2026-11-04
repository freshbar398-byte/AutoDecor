// Попап выбора стиля: сетка пресетов + кнопка DECORATE.
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <vector>

class EditorUI;
class CCMenuItemToggler;

class DecorStudioPopup : public geode::Popup {
protected:
    EditorUI* m_ui = nullptr;
    int m_styleIndex = 0;
    bool m_wholeLevel = false;
    std::vector<CCMenuItemToggler*> m_styleToggles;
    CCMenuItemToggler* m_modeToggle = nullptr;

    bool init(EditorUI* ui);
    void onStyle(cocos2d::CCObject* sender);
    void onMode(cocos2d::CCObject* sender);
    void onApply(cocos2d::CCObject* sender);

public:
    static DecorStudioPopup* create(EditorUI* ui);
};
