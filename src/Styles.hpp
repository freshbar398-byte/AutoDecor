// Пресеты декора. Здесь только данные: какой узор, каким объектом рисовать,
// сколько, какого размера и цвета. Вся геометрия — в Decor.hpp.
#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

namespace autodecor {

using namespace geode::prelude;

// "Роль" объекта в декоре. Для каждой роли есть список ID-кандидатов:
// мод берёт первый, который реально существует в этой версии игры.
enum class Slot {
    Dot,    // точка / маленький квадратик
    Glow,   // большая мягкая подложка
    Spike,  // шип
    Saw,    // пила
    Bar,    // тонкая полоска
};

enum class Pattern {
    Halo,          // одна большая подложка позади объекта
    Ring,          // точки по эллипсу вокруг объекта
    Frame,         // точки по контуру прямоугольника
    CornerSpikes,  // шипы в четыре угла, наружу
    Wave,          // синусоида над и под объектом
    Sparks,        // случайные точки в эллипсе
    Rays,          // короткие лучи из центра
    SawCorners,    // пилы по углам
};

struct Layer {
    Pattern pattern = Pattern::Ring;
    Slot slot = Slot::Dot;
    int count = 8;
    float radius = 1.f;     // множитель радиуса/размера
    float spread = 8.f;     // отступ от объекта, px
    float amplitude = 8.f;  // только для Wave
    float sizeMin = .2f;
    float sizeMax = .35f;
    float rotation = 0.f;
    bool randomRotation = true;
    int opacity = 220;
    cocos2d::ccColor3B color = { 255, 255, 255 };
};

inline Layer mk(Pattern pattern, Slot slot, int count, float radius, float spread,
                float sizeMin, float sizeMax, int opacity,
                cocos2d::ccColor3B color, bool randomRotation = true,
                float rotation = 0.f, float amplitude = 8.f) {
    Layer l;
    l.pattern = pattern;
    l.slot = slot;
    l.count = count;
    l.radius = radius;
    l.spread = spread;
    l.sizeMin = sizeMin;
    l.sizeMax = sizeMax;
    l.opacity = opacity;
    l.color = color;
    l.randomRotation = randomRotation;
    l.rotation = rotation;
    l.amplitude = amplitude;
    return l;
}

struct StyleDef {
    char const* id;
    char const* name;
    std::vector<Layer> layers;
};

// --- объекты -------------------------------------------------------------
// Порядок = приоритет. Если ID нет в игре, берётся следующий.
// Если после установки декор выглядит "не теми объектами" — правь здесь,
// больше нигде ID не зашиты.

inline std::vector<int> const& slotCandidates(Slot slot) {
    static std::vector<int> const dot   = { 1764, 1777, 725, 211, 1 };
    static std::vector<int> const glow  = { 211, 1764, 725, 1 };
    static std::vector<int> const spike = { 8, 9, 39, 1 };
    static std::vector<int> const saw   = { 88, 89, 1705, 8 };
    static std::vector<int> const bar   = { 1, 2, 211 };

    switch (slot) {
        case Slot::Glow:  return glow;
        case Slot::Spike: return spike;
        case Slot::Saw:   return saw;
        case Slot::Bar:   return bar;
        case Slot::Dot:
        default:          return dot;
    }
}

inline bool isValidObjectID(int id) {
    if (id <= 0) return false;
    auto toolbox = ObjectToolbox::sharedState();
    if (!toolbox) return false;
    auto frame = toolbox->intKeyToFrame(id);
    return frame && frame[0] != '\0';
}

inline int resolveSlot(Slot slot) {
    for (int id : slotCandidates(slot)) {
        if (isValidObjectID(id)) return id;
    }
    return -1;
}

// --- пресеты -------------------------------------------------------------

inline std::vector<StyleDef> const& builtinStyles() {
    using P = Pattern;
    using S = Slot;

    static std::vector<StyleDef> const styles = [] {
        std::vector<StyleDef> v;

        // Slaughterhouse: глубокая синева, двойное кольцо точек, дрейфующие искры.
        v.push_back({ "slaughterhouse", "SLAUGHTERHOUSE", {
            mk(P::Halo,   S::Glow,  1, 1.95f, 0.f,  1.f,   1.f,   80,  { 55, 125, 255 }, false),
            mk(P::Ring,   S::Dot,  12, 1.15f, 12.f, .16f,  .28f,  210, { 150, 210, 255 }),
            mk(P::Ring,   S::Dot,  18, 1.45f, 22.f, .10f,  .18f,  150, { 90, 165, 255 }),
            mk(P::Sparks, S::Dot,  10, 1.80f, 26.f, .08f,  .16f,  120, { 205, 235, 255 }),
        }});

        // Tidal Wave: бирюзовые синусоиды над и под объектом + редкие капли.
        v.push_back({ "tidal-wave", "TIDAL WAVE", {
            mk(P::Halo,   S::Glow,  1, 1.70f, 0.f,  1.f,   1.f,   70,  { 0, 200, 255 }, false),
            mk(P::Wave,   S::Dot,  14, 1.f,   12.f, .12f,  .20f,  190, { 130, 255, 255 }, true, 0.f, 10.f),
            mk(P::Wave,   S::Bar,  10, 1.f,   22.f, .10f,  .16f,  150, { 0, 230, 255 },  true, 0.f, 6.f),
            mk(P::Sparks, S::Dot,   8, 1.60f, 24.f, .08f,  .16f,  110, { 180, 255, 255 }),
        }});

        // Acheron: багровое свечение, шипы наружу и пилы по углам.
        v.push_back({ "acheron", "ACHERON", {
            mk(P::Halo,        S::Glow,  1, 1.65f, 0.f,  1.f,  1.f,  85,  { 170, 15, 25 }, false),
            mk(P::CornerSpikes,S::Spike, 4, 1.f,   6.f,  .55f, .85f, 235, { 255, 70, 70 }, false),
            mk(P::SawCorners,  S::Saw,   4, 1.f,   18.f, .32f, .48f, 200, { 255, 95, 95 }),
            mk(P::Frame,       S::Dot,   6, 1.f,   5.f,  .06f, .12f, 170, { 255, 130, 130 }),
        }});

        // Своя вариация: неоновое пульсирующее кольцо и лучи.
        v.push_back({ "neon-pulse", "NEON PULSE", {
            mk(P::Halo,   S::Glow,  1, 1.60f, 0.f,  1.f,  1.f,   60,  { 255, 40, 190 }, false),
            mk(P::Ring,   S::Dot,  16, 1.10f, 10.f, .14f, .24f,  220, { 255, 110, 220 }),
            mk(P::Ring,   S::Dot,  24, 1.38f, 20.f, .08f, .16f,  150, { 255, 60, 200 }),
            mk(P::Rays,   S::Dot,   8, 1.f,   10.f, .10f, .18f,  180, { 255, 190, 250 }),
        }});

        // Своя вариация: мороз — много мелких точек, холодный ореол.
        v.push_back({ "frost", "FROST", {
            mk(P::Halo,   S::Glow,  1, 1.75f, 0.f,  1.f,  1.f,   65,  { 150, 215, 255 }, false),
            mk(P::Sparks, S::Dot,  22, 1.45f, 26.f, .06f, .15f,  200, { 235, 250, 255 }),
            mk(P::Ring,   S::Dot,  12, 1.20f, 14.f, .08f, .15f,  140, { 190, 235, 255 }),
        }});

        // Своя вариация: инферно — шипы по углам и плотные оранжевые искры.
        v.push_back({ "inferno", "INFERNO", {
            mk(P::Halo,        S::Glow,  1, 1.60f, 0.f,  1.f,  1.f,  75,  { 255, 90, 20 }, false),
            mk(P::CornerSpikes,S::Spike, 4, 1.f,   8.f,  .45f, .8f,  230, { 255, 140, 40 }, true),
            mk(P::Sparks,      S::Dot,  16, 1.55f, 24.f, .08f, .18f, 190, { 255, 190, 90 }),
        }});

        // Своя вариация: минимализм — только аккуратная рамка.
        v.push_back({ "minimal", "MINIMAL", {
            mk(P::Frame, S::Dot, 7, 1.f, 4.f, .05f, .1f, 200, { 235, 235, 235 }),
        }});

        // Своя вариация: космос — лучи, кольцо, звёздная пыль.
        v.push_back({ "cosmic", "COSMIC", {
            mk(P::Halo,   S::Glow,  1, 1.80f, 0.f,  1.f,  1.f,   70,  { 120, 60, 255 }, false),
            mk(P::Rays,   S::Dot,  10, 1.f,   12.f, .08f, .16f,  190, { 190, 150, 255 }),
            mk(P::Ring,   S::Dot,  14, 1.25f, 16.f, .08f, .16f,  150, { 150, 200, 255 }),
            mk(P::Sparks, S::Dot,  14, 1.70f, 28.f, .06f, .13f,  130, { 240, 240, 255 }),
        }});

        return v;
    }();

    return styles;
}

inline int styleIndexById(std::string const& id) {
    auto const& styles = builtinStyles();
    for (int i = 0; i < static_cast<int>(styles.size()); ++i) {
        if (id == styles[i].id) return i;
    }
    return 0;
}

} // namespace autodecor
