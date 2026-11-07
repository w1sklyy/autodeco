// AutoDeco - automatic level decoration for the Geometry Dash editor (Geode).
// All generated objects are tagged with group MARK so they can be told apart
// from original objects. Original objects are never modified or removed.
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

namespace ad {

constexpr int MARK = 9999;                    // group id used to tag generated objects
constexpr int CH_SOFT = 994, CH_C1 = 995, CH_C2 = 996, CH_C3 = 997, CH_TXT = 998;
constexpr float FINE = 30.f, COARSE = 60.f;

// Decoration object IDs (no hitbox): 50 pulse circle, 51 pulse ring, 54 pulse star,
// 41 long chain, 131 cloud, 134 diamond rod, 406-408 grass, 461-466 bushes, 725/1764 dots, 914 text.
struct Style {
    const char* name;
    std::vector<int> top, hang, haz, bgL, bgS, line, star;
    ccColor3B soft, c1, c2, c3, txt;
    bool planets, vines;
};

static const std::vector<Style>& styles() {
    static const std::vector<Style> S = {
        {"Modern", {406, 407}, {41}, {51}, {51, 50}, {1764, 725}, {41}, {},
         {70, 90, 140}, {120, 170, 255}, {170, 190, 220}, {255, 255, 255}, {200, 220, 255}, false, false},
        {"Glow", {407, 408, 461}, {41}, {51, 50}, {50, 51}, {1764, 54}, {41}, {54},
         {40, 120, 160}, {60, 230, 255}, {255, 240, 120}, {255, 255, 255}, {170, 255, 255}, false, false},
        {"Minimal", {406}, {}, {51}, {51}, {1764}, {41}, {},
         {90, 90, 100}, {210, 210, 220}, {170, 170, 180}, {255, 255, 255}, {230, 230, 235}, false, false},
        {"Tech", {134, 1764}, {41, 134}, {51}, {51, 50}, {134, 1764}, {41, 134}, {},
         {20, 90, 70}, {50, 255, 170}, {40, 200, 255}, {200, 255, 230}, {120, 255, 200}, false, false},
        {"Space", {406, 1764}, {41}, {51}, {50, 51}, {54, 1764, 725}, {41}, {54},
         {40, 30, 120}, {110, 120, 255}, {170, 120, 255}, {255, 255, 255}, {190, 190, 255}, true, false},
        {"Nature", {406, 407, 408, 461, 462, 463, 464, 465, 466}, {41}, {461, 462}, {131}, {131, 407, 461}, {41}, {},
         {30, 80, 40}, {90, 200, 80}, {50, 150, 60}, {230, 255, 150}, {170, 255, 150}, false, true},
        {"Hell", {408, 134}, {41, 134}, {51, 134}, {50}, {134, 54}, {41, 134}, {54},
         {90, 10, 10}, {255, 60, 20}, {200, 20, 20}, {255, 190, 60}, {255, 110, 60}, false, false},
        {"Fantasy", {461, 462, 463, 464, 465, 466}, {41}, {51, 54}, {50, 51, 131}, {54, 131, 1764}, {41}, {54},
         {70, 40, 120}, {190, 120, 255}, {80, 230, 200}, {255, 200, 255}, {230, 190, 255}, false, true},
        {"Neon", {407, 408, 134}, {41}, {51, 50}, {50, 51}, {54, 1764, 134}, {41}, {54},
         {120, 0, 120}, {255, 0, 200}, {0, 255, 255}, {255, 255, 0}, {255, 120, 255}, false, false},
        {"Space Glow", {461, 462, 463, 464, 465, 466}, {41}, {51, 50}, {50, 51}, {54, 1764, 725}, {41}, {54},
         {110, 15, 100}, {255, 60, 170}, {170, 230, 90}, {190, 90, 255}, {255, 170, 230}, true, true},
    };
    return S;
}

struct Cfg {
    int style = 9;
    float amount = 0.5f, glow = 0.6f, density = 0.5f;
    bool lyrics = false;
    float textSize = 0.5f, textOpacity = 0.5f;
    std::string lyricsText;
    void load() {
        auto m = Mod::get();
        style = std::clamp(m->getSavedValue<int>("style", 9), 0, 9);
        amount = m->getSavedValue<float>("amount", 0.5f);
        glow = m->getSavedValue<float>("glow", 0.6f);
        density = m->getSavedValue<float>("density", 0.5f);
        lyrics = m->getSavedValue<bool>("lyrics", false);
        textSize = m->getSavedValue<float>("textSize", 0.5f);
        textOpacity = m->getSavedValue<float>("textOpacity", 0.5f);
        lyricsText = m->getSavedValue<std::string>("lyricsText", "");
    }
    void save() const {
        auto m = Mod::get();
        m->setSavedValue("style", style);
        m->setSavedValue("amount", amount);
        m->setSavedValue("glow", glow);
        m->setSavedValue("density", density);
        m->setSavedValue("lyrics", lyrics);
        m->setSavedValue("textSize", textSize);
        m->setSavedValue("textOpacity", textOpacity);
        m->setSavedValue("lyricsText", lyricsText);
    }
};
static Cfg g_cfg;
static std::vector<Ref<GameObject>> g_lastBatch;

// ---- version-tolerant helpers (if constexpr + requires: discarded branches are not instantiated) ----
template <class T>
static bool isMarkedT(T* o) {
    if constexpr (requires { (*o->m_groups)[0]; }) {
        if (!o->m_groups) return false;
        for (size_t i = 0; i < 10; ++i)
            if ((*o->m_groups)[i] == MARK) return true;
        return false;
    } else if constexpr (requires { o->m_groups[0]; o->m_groupCount; }) {
        if (!o->m_groups) return false;
        for (int i = 0; i < static_cast<int>(o->m_groupCount); ++i)
            if (o->m_groups[i] == MARK) return true;
        return false;
    } else {
        return false;
    }
}
static bool isMarked(GameObject* o) { return isMarkedT(o); }

template <class T>
static void setChannelT(T* o, int ch) {
    if constexpr (requires { o->m_baseColor->m_colorID = ch; }) {
        if (o->m_baseColor) o->m_baseColor->m_colorID = ch;
    }
    if constexpr (requires { o->m_detailColor->m_colorID = ch; }) {
        if (o->m_detailColor) o->m_detailColor->m_colorID = ch;
    }
}

template <class E>
static void setColorT(E* em, int ch, ccColor3B c, float opacity) {
    if constexpr (requires { em->getColorAction(ch)->m_color = c; }) {
        if (auto a = em->getColorAction(ch)) {
            a->m_color = c;
            if constexpr (requires { a->m_opacity = opacity; }) a->m_opacity = opacity;
        }
    }
}

template <class T>
static void setTextT(T* o, const std::string& s) {
    if constexpr (requires { o->updateTextObject(gd::string(s), false); }) {
        o->updateTextObject(gd::string(s), false);
    }
}

static int64_t key(int x, int y) { return (static_cast<int64_t>(x) << 32) ^ static_cast<uint32_t>(y); }
static int fl(float v, float s) { return static_cast<int>(std::floor(v / s)); }

struct Cand {
    int id;
    float x, y, sx, sy, rot;
    int ch;
    std::string text;
};

static std::vector<std::string> lyricLines() {
    std::string raw = g_cfg.lyricsText;
    std::ifstream f(Mod::get()->getSaveDir() / "lyrics.txt");
    if (f) {
        std::stringstream ss;
        ss << f.rdbuf();
        if (!ss.str().empty()) raw = ss.str() + "\n" + raw;
    }
    for (auto& c : raw)
        if (c == '|' || c == '\r') c = '\n';
    std::vector<std::string> out;
    std::stringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;
        line = line.substr(b);
        if (line.size() > 56) line = line.substr(0, 56);
        out.push_back(line);
        if (out.size() >= 60) break;
    }
    return out;
}

static std::string detectLang(const std::vector<std::string>& lines) {
    bool cyr = false, cjk = false, kr = false, jp = false, lat = false;
    for (auto& l : lines) {
        for (size_t i = 0; i < l.size();) {
            unsigned char c = l[i];
            uint32_t cp = c;
            int n = 1;
            if (c >= 0xF0) { n = 4; cp = c & 0x07; }
            else if (c >= 0xE0) { n = 3; cp = c & 0x0F; }
            else if (c >= 0xC0) { n = 2; cp = c & 0x1F; }
            for (int k = 1; k < n && i + k < l.size(); ++k) cp = (cp << 6) | (l[i + k] & 0x3F);
            i += n;
            if (cp >= 0x400 && cp <= 0x52F) cyr = true;
            else if (cp >= 0x3040 && cp <= 0x30FF) jp = true;
            else if (cp >= 0x4E00 && cp <= 0x9FFF) cjk = true;
            else if (cp >= 0xAC00 && cp <= 0xD7AF) kr = true;
            else if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) lat = true;
        }
    }
    if (jp) return "Japanese";
    if (kr) return "Korean";
    if (cjk) return "Chinese";
    if (cyr) return "Cyrillic (RU/UA/...)";
    if (lat) return "Latin";
    return "unknown";
}

static void removeObjs(LevelEditorLayer* lel, const std::vector<GameObject*>& v) {
    for (auto o : v) lel->removeObject(o, true);
}

static std::vector<GameObject*> markedObjects(LevelEditorLayer* lel) {
    std::vector<GameObject*> v;
    if (!lel->m_objects) return v;
    CCObject* it;
    CCARRAY_FOREACH(lel->m_objects, it) {
        auto o = static_cast<GameObject*>(it);
        if (o && isMarked(o)) v.push_back(o);
    }
    return v;
}

static int removeGenerated(LevelEditorLayer* lel) {
    auto v = markedObjects(lel);
    removeObjs(lel, v);
    g_lastBatch.clear();
    return static_cast<int>(v.size());
}

static int undoLast(LevelEditorLayer* lel) {
    std::vector<GameObject*> v;
    for (auto& r : g_lastBatch)
        if (r && lel->m_objects && lel->m_objects->containsObject(r) && isMarked(r)) v.push_back(r);
    removeObjs(lel, v);
    g_lastBatch.clear();
    return static_cast<int>(v.size());
}

// Core generator. Returns number of objects created.
static int generate(LevelEditorLayer* lel, bool onlyMissing, std::string& report) {
    auto& st = styles()[g_cfg.style];
    if (!onlyMissing) removeGenerated(lel);

    // 1. analyse level: originals only
    std::unordered_set<int64_t> solid, haz, occ, solidCoarse;
    std::vector<CCPoint> hazPts;
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    int origCount = 0;
    CCObject* it;
    CCARRAY_FOREACH(lel->m_objects, it) {
        auto o = static_cast<GameObject*>(it);
        if (!o) continue;
        auto p = o->getPosition();
        if (isMarked(o)) {
            occ.insert(key(fl(p.x, COARSE), fl(p.y, COARSE)));
            continue;
        }
        ++origCount;
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        auto t = o->m_objectType;
        if (t == GameObjectType::Solid || t == GameObjectType::Slope) {
            solid.insert(key(fl(p.x, FINE), fl(p.y, FINE)));
            solidCoarse.insert(key(fl(p.x, COARSE), fl(p.y, COARSE)));
        } else if (t == GameObjectType::Hazard) {
            haz.insert(key(fl(p.x, FINE), fl(p.y, FINE)));
            hazPts.push_back(p);
            solidCoarse.insert(key(fl(p.x, COARSE), fl(p.y, COARSE)));
        }
    }
    if (origCount == 0 || minX > maxX) {
        report = "Level is empty - nothing to decorate.";
        return 0;
    }

    std::mt19937 rng(std::random_device{}());
    auto rnd = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };
    auto chance = [&](float p) { return rnd(0.f, 1.f) < p; };
    auto pick = [&](const std::vector<int>& v) { return v[rng() % v.size()]; };
    float dens = std::clamp(g_cfg.density, 0.f, 1.f);
    float gl = std::clamp(g_cfg.glow, 0.f, 1.f);
    float gscale = 0.8f + gl * 0.8f;

    std::vector<Cand> cands;
    auto add = [&](int id, float x, float y, float sx, float sy, float rot, int ch) {
        cands.push_back({id, x, y, sx, sy, rot, ch, ""});
    };

    // 2. surface details: tops (grass / vines), undersides (hanging chains), hazards (glow rings)
    CCARRAY_FOREACH(lel->m_objects, it) {
        auto o = static_cast<GameObject*>(it);
        if (!o || isMarked(o)) continue;
        auto p = o->getPosition();
        auto t = o->m_objectType;
        int fx = fl(p.x, FINE), fy = fl(p.y, FINE);
        if (t == GameObjectType::Solid) {
            if (!st.top.empty() && !solid.count(key(fx, fy + 1)) && !haz.count(key(fx, fy + 1)) &&
                chance(0.10f + 0.45f * dens)) {
                float s = rnd(0.8f, 1.25f) * (st.vines ? 1.1f : 1.f);
                add(pick(st.top), p.x + rnd(-8, 8), p.y + 14.f * s, s, s, rnd(-8, 8), CH_C2);
            }
            if (!st.hang.empty() && !solid.count(key(fx, fy - 1)) && chance(0.03f + 0.12f * dens)) {
                float s = rnd(0.5f, 1.f);
                add(pick(st.hang), p.x, p.y - 15.f - 30.f * s, 0.6f, s, 0, CH_C1);
            }
        } else if (t == GameObjectType::Hazard && chance(0.35f + 0.3f * dens)) {
            auto ck = key(fl(p.x, COARSE), fl(p.y, COARSE));
            if (!occ.count(ck)) {
                occ.insert(ck);
                float s = rnd(1.1f, 1.7f) * gscale;
                add(pick(st.haz), p.x, p.y, s, s, rnd(0, 360), chance(0.5f) ? CH_C3 : CH_C1);
                if (chance(0.5f)) add(50, p.x, p.y, 0.5f * gscale, 0.5f * gscale, 0, CH_SOFT);
            }
        }
    }

    // 3. composite background motifs on a coarse grid; fills big empty areas
    float x0 = minX - 60.f, x1 = maxX + 120.f;
    float y0 = std::max(30.f, minY - 30.f), y1 = maxY + 240.f;
    float fill = 0.25f + 0.55f * dens;
    for (float x = x0; x < x1; x += COARSE) {
        for (float y = y0; y < y1; y += COARSE) {
            int cx = fl(x, COARSE), cy = fl(y, COARSE);
            auto ck = key(cx, cy);
            if (occ.count(ck) || solidCoarse.count(ck)) continue;
            // keep motifs away from direct neighbours of gameplay objects
            bool near = false;
            for (int dx = -1; dx <= 1 && !near; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    if (solidCoarse.count(key(cx + dx, cy + dy))) { near = true; break; }
            float p = near ? fill * 0.35f : fill * 0.55f;
            if (!chance(p)) continue;
            occ.insert(ck);
            float px = x + rnd(-20, 20), py = y + rnd(-20, 20);
            float r = rnd(0.f, 1.f);
            if (st.planets && r < 0.30f && !near) {
                // planet on a rod: chain up + body + ring + satellite (Space / Space Glow look)
                float s = rnd(0.9f, 1.8f) * gscale;
                float rodLen = rnd(1.5f, 4.f);
                add(41, px, py + 30.f * rodLen, 0.5f, rodLen, 0, CH_C1);
                add(50, px, py, s, s, 0, CH_C1);
                add(50, px, py, s * 1.5f, s * 1.5f, 0, CH_SOFT);
                add(51, px, py, s * 1.35f, s * 1.35f, rnd(-25, 25), CH_C3);
                add(50, px + 30.f * s, py + 20.f * s, 0.35f, 0.35f, 0, CH_C3);
            } else if (r < 0.50f && !st.bgL.empty()) {
                float s = rnd(1.5f, 3.2f) * gscale;
                add(pick(st.bgL), px, py, s, s, rnd(0, 360), CH_SOFT);
                if (chance(0.5f)) add(pick(st.bgS), px + rnd(-30, 30), py + rnd(-30, 30), rnd(0.5f, 1.f), rnd(0.5f, 1.f), rnd(0, 360), CH_C3);
            } else if (r < 0.68f && !st.line.empty()) {
                float len = rnd(2.f, 6.f);
                add(pick(st.line), px, py, rnd(0.4f, 0.7f), len, 0, chance(0.5f) ? CH_C1 : CH_C3);
            } else if (r < 0.88f && !st.star.empty()) {
                float s = rnd(0.5f, 1.3f) * gscale;
                add(pick(st.star), px, py, s, s, rnd(0, 90), chance(0.5f) ? CH_C3 : CH_C1);
            } else if (!st.bgS.empty()) {
                float s = rnd(0.5f, 1.2f);
                add(pick(st.bgS), px, py, s, s, rnd(0, 360), chance(0.5f) ? CH_C2 : CH_C1);
            }
        }
    }

    // 4. safety cap: sample evenly if over budget (prevents lag)
    size_t budget = static_cast<size_t>(300 + 4700 * std::clamp(g_cfg.amount, 0.f, 1.f));
    size_t before = cands.size();
    if (cands.size() > budget) {
        std::shuffle(cands.begin(), cands.end(), rng);
        cands.resize(budget);
    }

    // 5. lyrics (manual text, optional lyrics.txt) placed in the biggest free vertical gap
    std::string lang;
    if (g_cfg.lyrics) {
        auto lines = lyricLines();
        lang = detectLang(lines);
        float span = std::max(300.f, maxX - minX);
        for (size_t i = 0; i < lines.size(); ++i) {
            float tx = minX + (static_cast<float>(i) + 0.5f) * span / static_cast<float>(lines.size());
            int bestLen = -1; float bestY = maxY + 120.f;
            int run = 0; float runStart = y0;
            for (float y = y0; y <= y1; y += FINE) {
                bool blocked = false;
                for (int dx = -3; dx <= 3 && !blocked; ++dx)
                    if (solid.count(key(fl(tx, FINE) + dx, fl(y, FINE))) || haz.count(key(fl(tx, FINE) + dx, fl(y, FINE)))) blocked = true;
                if (blocked) { run = 0; runStart = y + FINE; }
                else { ++run; if (run > bestLen) { bestLen = run; bestY = runStart + run * FINE * 0.5f; } }
            }
            cands.push_back({914, tx, bestY, 0, 0, 0, CH_TXT, lines[i]});
        }
    }

    // 6. create objects in the editor
    if (auto em = lel->m_effectManager) {
        setColorT(em, CH_SOFT, st.soft, 0.25f + 0.35f * gl);
        setColorT(em, CH_C1, st.c1, 1.f);
        setColorT(em, CH_C2, st.c2, 1.f);
        setColorT(em, CH_C3, st.c3, 1.f);
        setColorT(em, CH_TXT, st.txt, std::clamp(g_cfg.textOpacity, 0.05f, 1.f));
    }
    g_lastBatch.clear();
    int created = 0;
    for (auto& c : cands) {
        auto o = lel->createObject(c.id, ccp(c.x, c.y), false);
        if (!o) continue;
        o->addToGroup(MARK);
        setChannelT(o, c.ch);
        if (c.id == 914) {
            float s = 0.3f + std::clamp(g_cfg.textSize, 0.f, 1.f) * 1.7f;
            setTextT(static_cast<TextGameObject*>(o), c.text);
            o->updateCustomScaleX(s);
            o->updateCustomScaleY(s);
        } else {
            if (c.sx != 1.f) o->updateCustomScaleX(c.sx);
            if (c.sy != 1.f) o->updateCustomScaleY(c.sy);
            if (c.rot != 0.f) o->setRotation(c.rot);
        }
        g_lastBatch.push_back(o);
        ++created;
    }

    std::ostringstream r;
    r << st.name << ": +" << created << " objects";
    if (before > budget) r << " (capped from " << before << ")";
    if (!lang.empty()) {
        r << ", lyrics: " << lang;
        if (lang == "Japanese" || lang == "Korean" || lang == "Chinese")
            r << " (GD font may lack these glyphs)";
    }
    report = r.str();
    return created;
}

} // namespace ad

// ---------------------------------------------------------------- UI
class DecoPopup : public Popup<LevelEditorLayer*> {
protected:
    LevelEditorLayer* m_lel = nullptr;
    CCLabelBMFont* m_styleLbl = nullptr;
    CCLabelBMFont* m_lbl[5] = {};
    TextInput* m_input = nullptr;

    CCLabelBMFont* label(const char* t, float x, float y, float sc, const char* font = "bigFont.fnt") {
        auto l = CCLabelBMFont::create(t, font);
        l->setScale(sc);
        l->setPosition(x, y);
        m_mainLayer->addChild(l);
        return l;
    }

    void addSlider(int tag, const char* name, float val, float y) {
        auto s = Slider::create(this, menu_selector(DecoPopup::onSlider), 0.7f);
        s->setPosition(m_size.width / 2 + 40.f, y);
        s->setValue(val);
        s->getThumb()->setTag(tag);
        m_mainLayer->addChild(s);
        m_lbl[tag] = label(name, 70.f, y, 0.4f);
    }

    bool setup(LevelEditorLayer* lel) override {
        m_lel = lel;
        ad::g_cfg.load();
        setTitle("AutoDeco");
        float cx = m_size.width / 2;
        m_styleLbl = label(ad::styles()[ad::g_cfg.style].name, cx, m_size.height - 55.f, 0.7f);

        auto la = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto ra = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        ra->setFlipX(true);
        la->setScale(0.7f); ra->setScale(0.7f);
        auto lb = CCMenuItemSpriteExtra::create(la, this, menu_selector(DecoPopup::onPrev));
        auto rb = CCMenuItemSpriteExtra::create(ra, this, menu_selector(DecoPopup::onNext));
        lb->setPosition(cx - 110.f, m_size.height - 55.f);
        rb->setPosition(cx + 110.f, m_size.height - 55.f);
        m_buttonMenu->addChild(lb);
        m_buttonMenu->addChild(rb);

        float y = m_size.height - 90.f;
        addSlider(0, "Amount", ad::g_cfg.amount, y);
        addSlider(1, "Glow", ad::g_cfg.glow, y - 25.f);
        addSlider(2, "Density", ad::g_cfg.density, y - 50.f);
        addSlider(3, "Text size", ad::g_cfg.textSize, y - 75.f);
        addSlider(4, "Text alpha", ad::g_cfg.textOpacity, y - 100.f);

        auto tog = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(DecoPopup::onLyricsToggle), 0.6f);
        tog->toggle(ad::g_cfg.lyrics);
        tog->setPosition(30.f, y - 130.f);
        m_buttonMenu->addChild(tog);
        label("Lyrics", 62.f, y - 130.f, 0.4f);
        m_input = TextInput::create(260.f, "line 1 | line 2 | ... (or lyrics.txt)", "chatFont.fnt");
        m_input->setMaxCharCount(2000);
        m_input->setString(ad::g_cfg.lyricsText);
        m_input->setPosition(cx + 40.f, y - 130.f);
        m_mainLayer->addChild(m_input);

        auto mk = [&](const char* t, SEL_MenuHandler h, float x, float yy, int tag) {
            auto b = CCMenuItemSpriteExtra::create(ButtonSprite::create(t, 90, true, "bigFont.fnt", "GJ_button_01.png", 28.f, 0.5f), this, h);
            b->setTag(tag);
            b->setPosition(x, yy);
            m_buttonMenu->addChild(b);
        };
        float by = y - 170.f;
        mk("Generate", menu_selector(DecoPopup::onGen), cx - 100.f, by, 0);
        mk("Add Missing", menu_selector(DecoPopup::onGen), cx, by, 1);
        mk("Regenerate", menu_selector(DecoPopup::onGen), cx + 100.f, by, 0);
        mk("Remove Deco", menu_selector(DecoPopup::onRemove), cx - 50.f, by - 35.f, 0);
        mk("Undo Last", menu_selector(DecoPopup::onUndo), cx + 50.f, by - 35.f, 0);
        return true;
    }

    void refreshStyle() {
        m_styleLbl->setString(ad::styles()[ad::g_cfg.style].name);
        ad::g_cfg.save();
    }
    void onPrev(CCObject*) { ad::g_cfg.style = (ad::g_cfg.style + 9) % 10; refreshStyle(); }
    void onNext(CCObject*) { ad::g_cfg.style = (ad::g_cfg.style + 1) % 10; refreshStyle(); }
    void onLyricsToggle(CCObject*) { ad::g_cfg.lyrics = !ad::g_cfg.lyrics; ad::g_cfg.save(); }
    void onSlider(CCObject* s) {
        auto t = static_cast<SliderThumb*>(s);
        float v = t->getValue();
        switch (t->getTag()) {
            case 0: ad::g_cfg.amount = v; break;
            case 1: ad::g_cfg.glow = v; break;
            case 2: ad::g_cfg.density = v; break;
            case 3: ad::g_cfg.textSize = v; break;
            case 4: ad::g_cfg.textOpacity = v; break;
        }
        ad::g_cfg.save();
    }
    void onGen(CCObject* s) {
        ad::g_cfg.lyricsText = m_input->getString();
        ad::g_cfg.save();
        std::string rep;
        bool onlyMissing = static_cast<CCNode*>(s)->getTag() == 1;
        ad::generate(m_lel, onlyMissing, rep);
        Notification::create(rep, NotificationIcon::Success)->show();
    }
    void onRemove(CCObject*) {
        int n = ad::removeGenerated(m_lel);
        Notification::create("Removed " + std::to_string(n) + " generated objects", NotificationIcon::Success)->show();
    }
    void onUndo(CCObject*) {
        int n = ad::undoLast(m_lel);
        Notification::create("Undid last generation: " + std::to_string(n) + " objects", NotificationIcon::Success)->show();
    }

public:
    static DecoPopup* create(LevelEditorLayer* lel) {
        auto r = new DecoPopup();
        if (r->initAnchored(380.f, 330.f, lel)) {
            r->autorelease();
            return r;
        }
        delete r;
        return nullptr;
    }
};

class $modify(AutoDecoEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;
        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition(0, 0);
        auto btn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Deco", 40, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.6f),
                                                 this, menu_selector(AutoDecoEditorUI::onAutoDeco));
        btn->setPosition(40.f, win.height * 0.62f);
        menu->addChild(btn);
        this->addChild(menu, 100);
        return true;
    }
    void onAutoDeco(CCObject*) { DecoPopup::create(m_editorLayer)->show(); }
};
