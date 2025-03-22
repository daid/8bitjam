#include "editor.h"
#include "main.h"
#include "unitinfo.h"
#include "tileinfo.h"
#include "mainScene.h"
#include <sp2/scene/scene.h>
#include <sp2/scene/tilemap.h>
#include <sp2/scene/camera.h>
#include <sp2/math/plane.h>
#include <sp2/graphics/gui/loader.h>
#include <sp2/graphics/gui/widget/togglebutton.h>
#include <sp2/graphics/gui/widget/listbox.h>
#include <sp2/graphics/spriteAnimation.h>
#include <sp2/stringutil/convert.h>
#include <sp2/io/keyValueTreeSaver.h>
#include <sp2/io/keyValueTreeLoader.h>
#include <sp2/window.h>

extern sp::P<sp::Window> window;


struct AutoTileData
{
    sp::string info;
    sp::Vector2i offset;
};

AutoTileData auto_tile_data[] = {
    {"?0? 0.1 ?1?", {-1, -1}},
    {"?0? 1.1 ?1?", { 0, -1}},
    {"?0? 1.0 ?1?", { 1, -1}},
    
    {"?1? 0.1 ?1?", {-1,  0}},
    {"?1? 1.1 ?1?", { 0,  0}},
    {"?1? 1.0 ?1?", { 1,  0}},

    {"?1? 0.1 ?0?", {-1,  1}},
    {"?1? 1.1 ?0?", { 0,  1}},
    {"?1? 1.0 ?0?", { 1,  1}},

    {"?0? 0.0 ?1?", { 2, -1}},
    {"?1? 0.0 ?1?", { 2,  0}},
    {"?1? 0.0 ?0?", { 2,  1}},

    {"?0? 0.1 ?0?", {-1,  2}},
    {"?0? 1.1 ?0?", { 0,  2}},
    {"?0? 1.0 ?0?", { 1,  2}},

    {"111 1.1 110", { 3, -1}},
    {"111 1.1 011", { 4, -1}},
    {"110 1.1 111", { 3,  0}},
    {"011 1.1 111", { 4,  0}},

    {"?0? 0.0 ?0?", {2, 2}},
};
int auto_tile_lookup[0x100];


class EditorUnit : public sp::Node
{
public:
    EditorUnit(sp::P<sp::Node> parent, sp::string unit_type, int team, sp::Vector2i position)
    : sp::Node(parent), unit_type(unit_type), pos(position), team(team)
    {
        auto unit_info = UnitInfo::get(unit_type);
        if (!unit_info) return;
        setAnimation(sp::SpriteAnimation::load(unit_info->sprite[team]));
        animationPlay("Ready");
        setPosition(sp::Vector2d(position) + sp::Vector2d(0.5, 0.5));
    }

    sp::string unit_type;
    sp::Vector2i pos;
    int team;
};


class EditorScene : public sp::Scene
{
public:
    EditorScene() : sp::Scene("EDITOR") {
        for(auto& n : auto_tile_lookup) n = 0xFFFFFF;
        for(auto& atd : auto_tile_data) {
            int mask = 0;
            int match = 0;
            auto f = [&mask, &match](char c, int shift) {
                if (c == '0') { mask |= 1 << shift; }
                if (c == '1') { mask |= 1 << shift; match |= 1 << shift; }
            };
            f(atd.info[0], 5);
            f(atd.info[1], 6);
            f(atd.info[2], 7);
            f(atd.info[4], 3);
            f(atd.info[6], 4);
            f(atd.info[8], 0);
            f(atd.info[9], 1);
            f(atd.info[10], 2);
            for(int n=0; n<0x100; n++) {
                if ((n & mask) == match) {
                    if (auto_tile_lookup[n] != 0xFFFFFF) LOG(Debug, "autotile overlap...");
                    auto_tile_lookup[n] = atd.offset.x + atd.offset.y * tilesetSize().x;
                }
            }
        }
        for(int n=0; n<0x100; n++) {
            if (n == 0xFFFFFF) LOG(Debug, "Autotile lookup missing for:", sp::string::hex(n));
        }
        ground_tilemap = makeGroundTilemap(getRoot());

        auto camera = new sp::Camera(getRoot());
        camera->setOrtographic({8, 7});
        setDefaultCamera(camera);

        auto loader = sp::gui::Loader("gui/editor.gui");
        gui = loader.create("EDITOR");
        gui->getWidgetWithID("PLAY")->setEventCallback([this](sp::Variant) {
            start_level = true;
        });
        auto left_button = loader.create("BTN", gui->getWidgetWithID("PALETTE"));
        left_button->setAttribute("caption", "<");
        left_button->setEventCallback([this](sp::Variant) {
            if (palette_offset < 1) return;
            palette_offset -= 1;
            updateVisiblePalette();
        });
        float fx = 1.0f / float(tilesetSize().x);
        float fy = 1.0f / float(tilesetSize().y);
        for(int id=0; id<tileIndexMax(); id++) {
            if (getTileType(id) == TileType::Void) continue;
            if (getAutoTile(id) != -1 && getAutoTile(id) != id) continue;
            float x = float(id % tilesetSize().x) / float(tilesetSize().x);
            float y = float(id / tilesetSize().x) / float(tilesetSize().y);

            auto tile = loader.create("TILE", gui->getWidgetWithID("PALETTE"));
            tile->getWidgetWithID("IMAGE")->setAttribute("uv", sp::string(x, 4) + "," + sp::string(y, 4) + "," + sp::string(fx, 4) + "," + sp::string(fy, 4));
            tile->setEventCallback([this, tile, id](sp::Variant v) {
                for(sp::P<sp::gui::ToggleButton> w : gui->getWidgetWithID("PALETTE")->getChildren()) {
                    if (!w) continue;
                    w->setActive(w == tile && v.getInteger());
                }
                if (v.getInteger())
                    draw_tile = id;
                else
                    draw_tile = -1;
                draw_unit = "";
            });
        }
        for(auto unitinfo : UnitInfo::getAll()) {
            for(int team=0; team<2; team++) {
                auto tile = loader.create("TILE", gui->getWidgetWithID("PALETTE"));
                tile->getWidgetWithID("IMAGE")->setAttribute("size", "1, 1");
                tile->getWidgetWithID("IMAGE")->setAttribute("alignment", "center");
                tile->getWidgetWithID("IMAGE")->setAnimation(sp::SpriteAnimation::load(unitinfo.sprite[team]));
                tile->getWidgetWithID("IMAGE")->animationPlay("Ready");
                tile->getWidgetWithID("IMAGE")->render_data.scale = {16, 16, 16};
                tile->setEventCallback([this, tile, unitinfo, team](sp::Variant v) {
                    for(sp::P<sp::gui::ToggleButton> w : gui->getWidgetWithID("PALETTE")->getChildren()) {
                        if (!w) continue;
                        w->setActive(w == tile && v.getInteger());
                    }
                    if (v.getInteger()) {
                        draw_unit = unitinfo.key;
                        draw_unit_team = team;
                    } else {
                        draw_unit = "";
                    }
                    draw_tile = -1;
                });
            }
        }
        auto right_button = loader.create("BTN", gui->getWidgetWithID("PALETTE"));
        right_button->setAttribute("caption", ">");
        right_button->setEventCallback([this](sp::Variant) {
            if (palette_offset >= gui->getWidgetWithID("PALETTE")->getChildren().size() - palette_size - 2) return;
            palette_offset += 1;
            updateVisiblePalette();
        });
        updateVisiblePalette();

        sp::P<sp::gui::Listbox> files = gui->getWidgetWithID("FILES");
        for(int n=0; n<20; n++)
            files->addItem(sp::string(n));
        files->setEventCallback([this](sp::Variant v) {
            saveLevel(current_level_index);
            current_level_index = v.getString();
            loadLevel(current_level_index);
        });
        loadLevel("0");
    }

    void updateVisiblePalette()
    {
        int index = 0;
        for(sp::P<sp::gui::ToggleButton> btn : gui->getWidgetWithID("PALETTE")->getChildren()) {
            if (!btn) continue;
            btn->setVisible(index >= palette_offset && index < palette_offset + palette_size);
            index++;
        }
    }

    virtual ~EditorScene()
    {
        saveLevel(current_level_index);
        gui.destroy();
    }

    bool onPointerMove(sp::Ray3d ray, int id) override
    {
        auto p3 = sp::Plane3d({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}).intersect(ray);
        auto p = sp::Vector2i(std::floor(p3.x), std::floor(p3.y));
        gui->getWidgetWithID("TOOLTIP")->setAttribute("caption", sp::string(p.x) + "," + sp::string(p.y) + "\n" + draw_unit);
        return true;
    }

    bool onPointerDown(sp::io::Pointer::Button button, sp::Ray3d ray, int id) override
    {
        drag_button = button;
        drag_start = sp::Plane3d({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}).intersect(ray);
        onPointerDrag(ray, id);
        return true;
    }

    void onPointerDrag(sp::Ray3d ray, int id) override
    {
        onPointerMove(ray, id);
        auto p3 = sp::Plane3d({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}).intersect(ray);
        if (drag_button == sp::io::Pointer::Button::Right) {
            auto diff = drag_start - p3;
            getCamera()->setPosition(getCamera()->getPosition2D() + sp::Vector2d(diff.x, diff.y));
        } else {
            auto p = sp::Vector2i(std::floor(p3.x), std::floor(p3.y));
            if (draw_unit == "") {
                ground_tilemap->setTile(p, draw_tile);
                updateAutoTiles();
            } else {
                bool destroyed = false;
                for(sp::P<EditorUnit> eu : getRoot()->getChildren()) {
                    if (eu && eu->pos == p) {
                        eu.destroy();
                        destroyed = true;
                    }
                }
                if (!destroyed)
                    new EditorUnit(getRoot(), draw_unit, draw_unit_team, p);
            }
        }
    }

    void saveLevel(sp::string name)
    {
        LOG(Debug, "Saving:", name);
        sp::KeyValueTree kvt;
        kvt.root_nodes.emplace_back();
        auto& node = kvt.root_nodes.back();
        node.id = "TILES";
        for(auto p : ground_tilemap->getEnclosingRect()) {
            auto t = ground_tilemap->getTileIndex(p);
            if (t >= 0) {
                node.items[sp::string(p.x) + "," + sp::string(p.y)] = sp::string(t);
            }
        }
        kvt.root_nodes.emplace_back();
        auto& unit_root = kvt.root_nodes.back();
        unit_root.id = "UNITS";
        for(sp::P<EditorUnit> eu : getRoot()->getChildren()) if (eu) {
            unit_root.child_nodes.emplace_back();
            auto& unit_node = unit_root.child_nodes.back();
            unit_node.id = eu->unit_type;
            unit_node.items["team"] = sp::string(eu->team);
            unit_node.items["x"] = sp::string(eu->pos.x);
            unit_node.items["y"] = sp::string(eu->pos.y);
        }
        sp::io::KeyValueTreeSaver::save("resources/level/" + name + ".txt", kvt);
    }

    void loadLevel(sp::string name)
    {
        auto kvt = sp::io::KeyValueTreeLoader::loadFile("resources/level/" + name + ".txt");
        ground_tilemap.destroy();
        for(sp::P<EditorUnit> eu : getRoot()->getChildren())
            eu.destroy();
        ground_tilemap = makeGroundTilemap(getRoot());
        if (!kvt) return;
        if (auto tiles = kvt->findId("TILES")) {
            for(auto [k, v] : tiles->items) {
                auto [xs, ys] = k.partition(",");
                ground_tilemap->setTile({sp::stringutil::convert::toInt(xs), sp::stringutil::convert::toInt(ys)}, sp::stringutil::convert::toInt(v));
            }
        }
        if (auto units = kvt->findId("UNITS")) {
            for(auto& unit_node : units->child_nodes) {
                if (UnitInfo::get(unit_node.id)) {
                    new EditorUnit(getRoot(), unit_node.id, sp::stringutil::convert::toInt(unit_node.items["team"]), {sp::stringutil::convert::toInt(unit_node.items["x"]), sp::stringutil::convert::toInt(unit_node.items["y"])});
                }
            }
        }
        auto r = ground_tilemap->getEnclosingRect();
        getCamera()->setPosition(sp::Vector2d(r.position) + sp::Vector2d(r.size) * 0.5);
        updateAutoTiles();
    }

    void onUpdate(float delta) override
    {
        if (controller.start.getUp()) {
            openMainMenu();
            delete this;
            return;
        }
        if (start_level) {
            saveLevel(current_level_index);
            new ::Scene(current_level_index);
            delete this;
        }
    }

    void updateAutoTiles()
    {
        for(auto p : ground_tilemap->getEnclosingRect()) {
            auto t = ground_tilemap->getTileIndex(p);
            auto att = getAutoTile(t);
            if (att == -1) continue;
            int mask = 0;
            for(auto p2 : sp::Rect2i({-1, -1}, {3, 3})) {
                if (getAutoTile(ground_tilemap->getTileIndex(p + p2)) == att) {
                    mask |= 1 << ((p2.x + 1)  + (p2.y + 1) * 3);
                }
            }
            mask = (mask & 0x0F) | ((mask >> 1) & 0xF0);
            auto offset = auto_tile_lookup[mask];
            if (offset != 0xFFFFFF)
                ground_tilemap->setTile(p, att + offset);
        }
    }

    sp::P<sp::Tilemap> ground_tilemap;
    sp::P<sp::gui::Widget> gui;
    int draw_tile = -1;
    sp::string draw_unit;
    int draw_unit_team = 0;
    int palette_offset = 0;
    static constexpr int palette_size = 12;
    sp::io::Pointer::Button drag_button;
    sp::Vector3d drag_start;
    sp::string current_level_index = "0";
    bool start_level = false;
};

void openEditor()
{
    new EditorScene();
}