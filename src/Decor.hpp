// Вся математика автодекора. Здесь нет UI — только настройки, геометрия
// и создание объектов через LevelEditorLayer.
#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "Styles.hpp"

namespace autodecor {

using namespace geode::prelude;

// Метка на объектах, которые создал мод. Нужна, чтобы повторное нажатие
// кнопки не декорировало собственный декор (иначе — бесконечный рост).
inline constexpr char const* kMadeFlag = "auto-decor-made";

// Жёсткий предел на один клик: защита от зависания на слабом телефоне.
inline constexpr int kMaxCreatedPerRun = 6000;

struct Config {
    float detail = 1.f;   // множитель плотности (настройка 1..3)
    float spread = 1.f;   // множитель отступа
    float size = 1.f;     // множитель размера
    float alpha = 1.f;    // множитель прозрачности (0..1)
    bool tint = true;     // красить декор в цвета пресета
    bool skipTriggers = true;
    int maxTargets = 300;
};

inline Config readConfig() {
    auto mod = Mod::get();
    Config cfg;

    float detail = static_cast<float>(mod->getSettingValue<int64_t>("detail"));
    cfg.detail = std::clamp(detail, 1.f, 3.f) / 2.f;
    cfg.spread = std::clamp(static_cast<float>(mod->getSettingValue<double>("spread")), 0.f, 3.f);
    cfg.size = std::clamp(static_cast<float>(mod->getSettingValue<double>("size")), .2f, 3.f);
    float opacity = static_cast<float>(mod->getSettingValue<int64_t>("opacity"));
    cfg.alpha = std::clamp(opacity, 0.f, 255.f) / 255.f;
    cfg.tint = mod->getSettingValue<bool>("tint-presets");
    cfg.skipTriggers = mod->getSettingValue<bool>("skip-triggers");
    cfg.maxTargets = std::clamp(
        static_cast<int>(mod->getSettingValue<int64_t>("max-targets")), 1, 3000
    );
    return cfg;
}

class Decorator {
public:
    Decorator(LevelEditorLayer* editor, Config const& cfg, StyleDef const& style)
        : m_editor(editor),
          m_cfg(cfg),
          m_style(style),
          m_rng(std::random_device{ }()) {
        for (int i = 0; i < 5; ++i) {
            m_ids[i] = resolveSlot(static_cast<Slot>(i));
        }
    }

    int created() const { return m_created; }
    bool hitLimit() const { return m_hitLimit; }
    bool failed() const { return m_failed; }

    void decorate(GameObject* target) {
        if (!target || !m_editor || m_failed) return;

        auto rect = target->getObjectRect();
        float w = std::max(rect.size.width, 8.f);
        float h = std::max(rect.size.height, 8.f);
        cocos2d::CCPoint center = {
            rect.origin.x + rect.size.width * .5f,
            rect.origin.y + rect.size.height * .5f,
        };
        float unit = std::max(w, h);

        for (auto const& layer : m_style.layers) {
            if (m_failed || m_hitLimit) return;
            applyLayer(layer, center, w, h, unit);
        }
    }

private:
    LevelEditorLayer* m_editor;
    Config m_cfg;
    StyleDef const& m_style;
    std::mt19937 m_rng;
    int m_ids[5] = { -1, -1, -1, -1, -1 };
    int m_created = 0;
    bool m_hitLimit = false;
    bool m_failed = false;

    float rnd(float lo, float hi) {
        return std::uniform_real_distribution<float>(lo, hi)(m_rng);
    }

    // Создаёт один объект декора с центром в точке `at`.
    // Реальный размер считается от contentSize самого объекта, поэтому
    // работает с любым ID (у разных объектов разный «родной» размер).
    GameObject* place(int objectID, cocos2d::CCPoint at, float desiredPx,
                      float rotation, int opacity,
                      cocos2d::ccColor3B color, bool tint) {
        if (m_created >= kMaxCreatedPerRun) {
            m_hitLimit = true;
            return nullptr;
        }

        auto obj = m_editor->createObject(objectID, at, false);
        if (!obj) {
            // createObject вернул пустоту — дальше пробовать бессмысленно
            m_failed = true;
            return nullptr;
        }

        obj->setUserFlag(kMadeFlag, true);

        auto cs = obj->getContentSize();
        if (cs.width < 1.f || cs.height < 1.f) {
            // у некоторых объектов нет текстуры (партиклы и т.п.)
            cs = { 30.f, 30.f };
        }

        float native = std::max({ cs.width, cs.height, 1.f });
        float scale = std::clamp(desiredPx / native, .01f, 20.f);
        obj->setScale(scale);

        // Ставим объект так, чтобы его центр попал ровно в точку `at`.
        auto anchor = obj->getAnchorPoint();
        obj->setPosition({
            at.x + (anchor.x - .5f) * cs.width * scale,
            at.y + (anchor.y - .5f) * cs.height * scale,
        });

        obj->setRotation(rotation);

        int op = static_cast<int>(std::lround(opacity * m_cfg.alpha));
        obj->setOpacity(static_cast<GLubyte>(std::clamp(op, 0, 255)));

        if (tint && m_cfg.tint) {
            obj->setObjectColor(color);
        }

        ++m_created;
        return obj;
    }

    void applyLayer(Layer const& l, cocos2d::CCPoint c, float w, float h, float unit) {
        int const id = m_ids[static_cast<int>(l.slot)];
        if (id < 0) return;

        int count = std::max(1, static_cast<int>(std::lround(l.count * m_cfg.detail)));
        float const spread = l.spread * m_cfg.spread;

        switch (l.pattern) {
            case Pattern::Halo: {
                place(id, c, unit * l.radius * m_cfg.size, l.rotation,
                      l.opacity, l.color, true);
            } break;

            case Pattern::Ring: {
                float rx = (w * .5f + spread) * l.radius;
                float ry = (h * .5f + spread) * l.radius;
                float phase = rnd(0.f, 6.2831853f);

                for (int i = 0; i < count; ++i) {
                    float angle = phase + static_cast<float>(i) / count * 6.2831853f;
                    place(id,
                          { c.x + rx * std::cos(angle), c.y + ry * std::sin(angle) },
                          unit * rnd(l.sizeMin, l.sizeMax) * m_cfg.size,
                          l.randomRotation ? rnd(0.f, 360.f) : l.rotation,
                          l.opacity, l.color, true);
                }
            } break;

            case Pattern::Frame: {
                float margin = 2.f + spread * l.radius;
                float x0 = c.x - w * .5f - margin;
                float x1 = c.x + w * .5f + margin;
                float y0 = c.y - h * .5f - margin;
                float y1 = c.y + h * .5f + margin;

                float perimeter = 2.f * ((x1 - x0) + (y1 - y0));
                int n = std::clamp(
                    static_cast<int>(perimeter / 100.f * l.count), 8, 900
                );
                float step = perimeter / n;
                float dot = std::max(step * 1.4f * m_cfg.size,
                                     unit * l.sizeMin * m_cfg.size);

                for (int i = 0; i < n; ++i) {
                    place(id, pointOnRect(x0, y0, x1, y1, step * i), dot, 0.f,
                          l.opacity, l.color, true);
                }
            } break;

            case Pattern::CornerSpikes:
            case Pattern::SawCorners: {
                float margin = 4.f + spread;
                float x0 = c.x - w * .5f - margin;
                float x1 = c.x + w * .5f + margin;
                float y0 = c.y - h * .5f - margin;
                float y1 = c.y + h * .5f + margin;

                struct Corner { float x, y, angle; };
                Corner corners[4] = {
                    { x0, y0, 225.f },
                    { x1, y0, 315.f },
                    { x1, y1,  45.f },
                    { x0, y1, 135.f },
                };

                bool const saw = l.pattern == Pattern::SawCorners;
                for (auto const& corner : corners) {
                    float rot = (saw || l.randomRotation)
                        ? rnd(0.f, 360.f)
                        : corner.angle + l.rotation;
                    place(id, { corner.x, corner.y },
                          unit * rnd(l.sizeMin, l.sizeMax) * m_cfg.size,
                          rot, l.opacity, l.color, true);
                }
            } break;

            case Pattern::Wave: {
                float amp = l.amplitude * m_cfg.spread;
                int n = std::max(2, count);

                for (int side = -1; side <= 1; side += 2) {
                    float baseY = c.y + side * (h * .5f + spread);
                    float phase = rnd(0.f, 6.2831853f);

                    for (int i = 0; i < n; ++i) {
                        float t = static_cast<float>(i) / (n - 1);
                        float x = c.x - w * .5f - spread + t * (w + 2.f * spread);
                        float y = baseY + std::sin(t * 6.2831853f + phase) * amp;

                        place(id, { x, y },
                              unit * rnd(l.sizeMin, l.sizeMax) * m_cfg.size,
                              l.randomRotation ? rnd(0.f, 360.f) : l.rotation,
                              l.opacity, l.color, true);
                    }
                }
            } break;

            case Pattern::Sparks: {
                float rx = (w * .5f + 20.f * m_cfg.spread) * l.radius;
                float ry = (h * .5f + 20.f * m_cfg.spread) * l.radius;

                for (int i = 0; i < count; ++i) {
                    float angle = rnd(0.f, 6.2831853f);
                    float dist = std::sqrt(rnd(0.f, 1.f));
                    place(id,
                          { c.x + std::cos(angle) * rx * dist,
                            c.y + std::sin(angle) * ry * dist },
                          unit * rnd(l.sizeMin, l.sizeMax) * m_cfg.size,
                          l.randomRotation ? rnd(0.f, 360.f) : l.rotation,
                          l.opacity, l.color, true);
                }
            } break;

            case Pattern::Rays: {
                float phase = rnd(0.f, 6.2831853f);
                for (int i = 0; i < count; ++i) {
                    float angle = phase + static_cast<float>(i) / count * 6.2831853f;
                    float dx = std::cos(angle);
                    float dy = std::sin(angle);
                    float len = (unit * .55f + spread * 2.f) * rnd(.7f, 1.f);

                    for (int k = 0; k < 3; ++k) {
                        float t = (k + 1) / 3.f;
                        place(id,
                              { c.x + dx * len * t, c.y + dy * len * t },
                              unit * l.sizeMax * (1.f - .5f * t) * m_cfg.size,
                              l.randomRotation ? rnd(0.f, 360.f) : l.rotation,
                              l.opacity, l.color, true);
                    }
                }
            } break;
        }
    }

    // Точка на периметре прямоугольника, `t` — расстояние от левого нижнего
    // угла по часовой стрелке.
    static cocos2d::CCPoint pointOnRect(float x0, float y0,
                                        float x1, float y1, float t) {
        float w = x1 - x0;
        float h = y1 - y0;
        float perimeter = 2.f * (w + h);

        t = std::fmod(std::max(t, 0.f), perimeter);

        if (t < w)  return { x0 + t, y0 };
        t -= w;
        if (t < h)  return { x1, y0 + t };
        t -= h;
        if (t < w)  return { x1 - t, y1 };
        t -= w;
        return { x0, y1 - t };
    }
};

} // namespace autodecor
