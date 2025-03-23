#include "mainScene.h"
#include "ingameMenu.h"
#include "main.h"
#include "tileinfo.h"
#include "unit.h"
#include "astar.h"
#include "dijkstra.h"
#include "combatlog.h"

#include <sp2/graphics/gui/loader.h>
#include <sp2/graphics/spriteAnimation.h>
#include <sp2/random.h>
#include <sp2/audio/sound.h>
#include <sp2/engine.h>
#include <sp2/scene/camera.h>
#include <sp2/scene/tilemap.h>
#include <sp2/graphics/textureManager.h>
#include <sp2/graphics/meshdata.h>
#include <sp2/io/keyValueTreeLoader.h>
#include <sp2/stringutil/convert.h>

Scene* scene_instance;


sp::P<Unit> astar_source;
std::vector<std::pair<sp::Vector2i, float>> getAStarNeighbors(sp::Vector2i p) {
    std::vector<std::pair<sp::Vector2i, float>> res;
    auto f = [&res](sp::Vector2i pp) {
        if (!scene_instance->level_size.contains(pp)) return;
        auto unit = scene_instance->getUnitAt(pp);
        if (unit && unit->team != astar_source->team)
            return;
        auto cost = scene_instance->moveCost(pp, astar_source);
        if (cost < 100)
            res.push_back({pp, cost});
    };
    f(p + sp::Vector2i( 1, 0));
    f(p + sp::Vector2i(-1, 0));
    f(p + sp::Vector2i( 0, 1));
    f(p + sp::Vector2i( 0,-1));
    return res;
}

float getAStarDistance(sp::Vector2i a, sp::Vector2i b)
{
    return (sp::Vector2f(a) - sp::Vector2f(b)).length() * 0.1f;
}

Scene::Scene(sp::string start_level)
: sp::Scene("MAIN")
{
    scene_instance = this;

    hud = sp::gui::Loader::load("gui/hud.gui", "HUD");

    loadLevel(start_level);
}

Scene::~Scene()
{
    script_env.destroy();
    hud.destroy();
    action_gui.destroy();
    unitinfo_gui.destroy();
    ingame_menu.destroy();
}

void Scene::onUpdate(float delta)
{
    int input = 0;
    int hold = 0;
    if (hud->getWidgetWithID("MESSAGE_WINDOW")->isVisible()) {
        if (controller.a.getDown()) hud->getWidgetWithID("MESSAGE_WINDOW")->hide();
    }
    if (ingame_menu) {
        if (escape_key.getDown())
            ingame_menu.destroy();
    } else if (unitinfo_gui) {
        if (controller.a.getDown()) unitinfo_gui.destroy();
        if (controller.b.getDown()) unitinfo_gui.destroy();
    } else if (!script_coroutine && !hud->getWidgetWithID("MESSAGE_WINDOW")->isVisible()) {
        if (controller.left.getDown()) input |= 0x01;
        if (controller.right.getDown()) input |= 0x02;
        if (controller.up.getDown()) input |= 0x04;
        if (controller.down.getDown()) input |= 0x08;

        if (controller.left.get()) hold |= 0x01;
        if (controller.right.get()) hold |= 0x02;
        if (controller.up.get()) hold |= 0x04;
        if (controller.down.get()) hold |= 0x08;
        if (hold) {
            hold_button_time += delta;
            if (hold_button_time > 0.2f) {
                hold_button_time -= 0.1f;
                input = hold;
            }
        } else {
            hold_button_time = 0.0f;
        }

        if (input & 0x01) moveCursor({-1, 0});
        if (input & 0x02) moveCursor({1, 0});
        if (input & 0x04) moveCursor({0, 1});
        if (input & 0x08) moveCursor({0, -1});

        if (controller.a.getDown()) activateCursor();
        if (controller.b.getDown()) deactivateCursor();
        if (controller.start.getDown() || escape_key.getDown()) {
            if (player_action_state == PlayerActionState::SelectAction)
                deactivateCursor();
            openIngameMenu();
        }
    }

    switch(player_action_state)
    {
    case PlayerActionState::SelectUnit: cursor->animationPlay("Idle"); break;
    case PlayerActionState::SelectMoveTarget:
        {
            astar_source = selected_unit;
            auto path = AStar<sp::Vector2i>(selected_unit->pos, cursor_pos, getAStarNeighbors, getAStarDistance);
            if (selected_unit->pos == cursor_pos || (path.size() > 0 && getUnitAt(cursor_pos) == nullptr && pathCost(path, selected_unit) <= selected_unit->unit_info->move)) {
                cursor->animationPlay("Idle");
            } else {
                cursor->animationPlay("Error");
            }
            auto tt = getTileType(ground_tilemap->getTileIndex(cursor_pos));
            auto tc = selected_unit->unit_info->terrain_class;
            if (tc->move_cost[int(tt)] < 1000)
                hud->getWidgetWithID("COMBAT_LOG")->setAttribute("caption", getTileTypeName(tt).capitalize() + " Move: " + sp::string(tc->move_cost[int(tt)]) + " Def: " + sp::string(tc->defense[int(tt)]));
            else
                hud->getWidgetWithID("COMBAT_LOG")->setAttribute("caption", getTileTypeName(tt).capitalize());
        }
        break;
    case PlayerActionState::WaitMoveDone:
        if (selected_unit->move_path.empty()) {
            player_action_state = PlayerActionState::SelectAction;
            auto gui_loader = sp::gui::Loader("gui/action.gui");
            action_gui = gui_loader.create("ACTION_MENU");
            for(auto& action : selected_unit->unit_info->actions) {
                if (action.requireTarget()) {
                    bool action_valid = false;
                    for(auto offset : action.targetOffsets()) {
                        if (action.isValidTarget(selected_unit, getUnitAt(cursor_pos + offset))) {
                            action_valid = true;
                            break;
                        }
                    }
                    if (!action_valid)
                        continue;
                }
                auto button = gui_loader.create("BUTTON", action_gui);
                button->setAttribute("caption", action.label);
                button->setEventCallback([this, &action](sp::Variant) {
                    selected_action = &action;
                    if (action.requireTarget()) {
                        player_action_state = PlayerActionState::SelectTarget;
                        buildTargetOverlay();
                        moveCursorToFirstTarget();
                    } else {
                        selected_unit->pos = move_target_position;
                        player_action_state = PlayerActionState::WaitActionDone;
                    }
                });
            }
            gui_loader.create("NAV", *action_gui->getChildren().begin());
        }
        break;
    case PlayerActionState::SelectAction: break;
    case PlayerActionState::SelectTarget:
        {
            auto diff = cursor_pos - move_target_position;
            auto dist = std::abs(diff.x) + std::abs(diff.y);
            auto target = getUnitAt(cursor_pos);
            if (dist >= selected_action->min_range && dist <= selected_action->max_range && selected_action->isValidTarget(selected_unit, target)) {
                cursor->animationPlay("Idle");
            } else {
                cursor->animationPlay("Error");
            }
        }
        action_gui.destroy();
        break;
    case PlayerActionState::WaitActionDone:
        action_gui.destroy();
        selected_unit->setReady(false);
        selection_cursor.destroy();
        move_overlay.destroy();
        player_action_state = PlayerActionState::SelectUnit;

        checkDeadUnits();

        {
            // Check for end of turn
            bool player_ready = false;
            for(sp::P<Unit> unit : getRoot()->getChildren()) {
                if (unit && unit->team == Team::Player && unit->ready)
                    player_ready = true;
            }
            if (!player_ready) {
                endPlayerTurn();
            }
        }
        break;
    case PlayerActionState::ExecuteAITurn:
        if ((ai_timer.isExpired() || !ai_timer.isRunning()) && !script_coroutine && !combat_log_timer.isRunning()) {
            checkDeadUnits();

            sp::P<Unit> ai_unit;
            for(sp::P<Unit> unit : getRoot()->getChildren()) {
                if (unit && unit->team != Team::Player && unit->ready)
                    ai_unit = unit;
            }
            if (ai_unit) {
                selected_unit = ai_unit;
                target_unit = nullptr;
                selected_action = nullptr;
                executeAITurnFor(ai_unit);
                player_action_state = PlayerActionState::WaitAIMove;
            } else {
                player_action_state = PlayerActionState::SelectUnit;
                for(sp::P<Unit> unit : getRoot()->getChildren()) {
                    if (unit && unit->team != Team::Player)
                        unit->setReady(true);
                }
                auto res = script_env->callCoroutine("onTurnStart");
                if (res.isErr()) {
                    LOG(Debug, "Lua error:", res.error());
                    hud->getWidgetWithID("LUA_ERROR")->setAttribute("caption", res.error());
                } else {
                    script_coroutine = res.value();
                }
            }
        }
        break;
    case PlayerActionState::WaitAIMove:
        if (selected_unit->move_path.empty()) {
            ai_timer.start(0.5);
            selected_unit->setReady(false);
            player_action_state = PlayerActionState::ExecuteAITurn;
            if (target_unit && selected_action) {
                selected_action->execute(selected_unit, target_unit, getTileType(ground_tilemap->getTileIndex(target_unit->pos)));
            }
        }
        break;
    case PlayerActionState::Defeat:
        if (!script_coroutine) {
            loadLevel(current_level);
        }
        break;
    case PlayerActionState::Victory:
        if (!script_coroutine) {
            has_heroes.clear();
            for(sp::P<Unit> unit : getRoot()->getChildren()) {
                if (unit && unit->team == Team::Player)
                    has_heroes.insert(unit->unit_info->key);
            }
            loadLevel(sp::string(sp::stringutil::convert::toInt(current_level) + 1));
        }
        break;
    }

    auto unit = getUnitAt(cursor_pos);
    if (unit) {
        hud->getWidgetWithID("HP")->setAttribute("caption", sp::string(unit->hp) + "/" + sp::string(unit->unit_info->max_hp) + " " + unit->getCharmInfo());
        hud->getWidgetWithID("NAME")->setAttribute("caption", unit->unit_info->name);
    } else {
        hud->getWidgetWithID("HP")->setAttribute("caption", "-");
        hud->getWidgetWithID("NAME")->setAttribute("caption", "");
    }

    if (combat_log_timer.isExpired() || !combat_log_timer.isRunning()) {
        auto log_entry = getCombatLog();
        if (log_entry != "") {
            hud->getWidgetWithID("COMBAT_LOG")->setAttribute("caption", log_entry);
            hud->getWidgetWithID("STATUS_LINE")->hide();
            combat_log_timer.start(1.5);
        } else {
            hud->getWidgetWithID("STATUS_LINE")->show();
        }
    }
    
    auto camera_pos = sp::Vector2d(cursor_pos);
    if (level_size.size.x < 17) {
        camera_pos.x = double(level_size.position.x) + double(level_size.size.x) * 0.5;
    } else {
        camera_pos.x += 0.5;
        camera_pos.x = std::max(camera_pos.x, double(level_size.position.x) + 8);
        camera_pos.x = std::min(camera_pos.x, double(level_size.position.x + level_size.size.x) - 8);
    }
    if (level_size.size.y < 14) {
        camera_pos.y = double(level_size.position.y) + double(level_size.size.y) * 0.5 - 0.5;
    } else {
        camera_pos.y = std::max(camera_pos.y, double(level_size.position.y) + 6);
        camera_pos.y = std::min(camera_pos.y, double(level_size.position.y + level_size.size.y) - 7);
    }
    getCamera()->setPosition(camera_pos);
    if (camera_pos.y < cursor->getPosition2D().y - 3.0)
        hud->getWidgetWithID("MESSAGE_WINDOW")->setAttribute("position", "0, 160");
    else
        hud->getWidgetWithID("MESSAGE_WINDOW")->setAttribute("position", "0, 0");
    if (exit_level) {
        delete this;
        openMainMenu();
    }
}

void Scene::onFixedUpdate()
{
    if (script_coroutine) {
        auto res = script_coroutine->resume();
        if (res.isErr()) {
            LOG(Debug, "Lua error:", res.error());
            hud->getWidgetWithID("LUA_ERROR")->setAttribute("caption", res.error());
            script_coroutine = nullptr;
        } else if (!res.value()) {
            script_coroutine = nullptr;
        }
    }
}

int luaYield(lua_State* L)
{
    return lua_yield(L, 0);
}

void luaMoveCursor(int x, int y)
{
    scene_instance->cursor_pos = {x, y};
    scene_instance->cursor->setPosition(sp::Vector2d(scene_instance->cursor_pos) + sp::Vector2d(0.5, 0.5));
}

void luaShowMessage(sp::string message, sp::string unit, int team)
{
    team = std::clamp(team, 0, 1);
    scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->show();
    scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->getWidgetWithID("TEXT")->setAttribute("caption", message);
    auto unitinfo = UnitInfo::get(unit);
    auto image = scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->getWidgetWithID("IMAGE");
    if (unitinfo) {
        scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->getWidgetWithID("IMAGE_CONTAINER")->show();
        image->render_data.scale = {16, 16, 16};
        image->setAnimation(sp::SpriteAnimation::load(unitinfo->sprite[team]));
        image->animationSetFlags(sp::SpriteAnimation::FlipFlag);
        image->animationPlay("Ready");
        image->show();
    } else {
        scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->getWidgetWithID("IMAGE_CONTAINER")->hide();
        image->hide();
    }
}

void luaHideMessage()
{
    scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->hide();
}

bool luaMessageOpen()
{
    return scene_instance->hud->getWidgetWithID("MESSAGE_WINDOW")->isVisible();
}

sp::P<Unit> luaCreateUnit(int x, int y, sp::string type, int team)
{
    team = std::clamp(team, 0, 1);
    if (!UnitInfo::get(type)) return nullptr;
    return new Unit(scene_instance->getRoot(), {x, y}, type, Team(team));
}

sp::P<Unit> luaGetUnit(int x, int y)
{
    return scene_instance->getUnitAt({x, y});
}

int luaAllUnits(lua_State* L)
{
    lua_newtable(L);
    int index = 1;
    int team_nr = luaL_checkinteger(L, 1);
    Team team = team_nr == 0 ? Team::Player : Team::AI;
    for(sp::P<Unit> unit : scene_instance->getRoot()->getChildren()) {
        if (!unit) continue;
        if (unit->team != team) continue;
        sp::script::pushToLua(L, unit);
        lua_seti(L, -2, index);
        index++;
    }
    return 1;
}

bool luaHasHero(sp::string name)
{
    return scene_instance->has_heroes.find(name) != scene_instance->has_heroes.end();
}

void luaDefeat()
{
    scene_instance->player_action_state = Scene::PlayerActionState::Defeat;
}

void luaVictory()
{
    scene_instance->player_action_state = Scene::PlayerActionState::Victory;
}

void Scene::loadLevel(const sp::string& name)
{
    current_level = name;
    player_action_state = PlayerActionState::SelectUnit;
    auto kvt = sp::io::KeyValueTreeLoader::loadFile("resources/level/" + name + ".txt");
    for(auto node : getRoot()->getChildren())
        node.destroy();
    script_env.destroy();
    // Create default nodes
    auto camera = new sp::Camera(getRoot());
    camera->setOrtographic({8, 7});
    setDefaultCamera(camera);
    ground_tilemap = makeGroundTilemap(getRoot());
    ground_tilemap->render_data.order = -10;
    cursor = new sp::Node(getRoot());
    cursor->setAnimation(sp::SpriteAnimation::load("cursor.txt"));
    cursor->animationPlay("Idle");
    cursor->render_data.order = 1000;

    //Load map
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
                new Unit(getRoot(), {sp::stringutil::convert::toInt(unit_node.items["x"]), sp::stringutil::convert::toInt(unit_node.items["y"])}, unit_node.id, Team(sp::stringutil::convert::toInt(unit_node.items["team"])));
            }
        }
    }

    level_size = ground_tilemap->getEnclosingRect();
    getCamera()->setPosition(sp::Vector2d(level_size.position) + sp::Vector2d(level_size.size) * 0.5);
    cursor_pos = level_size.center();
    cursor->setPosition(sp::Vector2d(cursor_pos) + sp::Vector2d(0.5, 0.5));

    script_env = new sp::script::Environment();
    script_env->setGlobal("yield", luaYield);
    script_env->setGlobal("moveCursor", luaMoveCursor);
    script_env->setGlobal("showMessage", luaShowMessage);
    script_env->setGlobal("hideMessage", luaHideMessage);
    script_env->setGlobal("messageOpen", luaMessageOpen);
    script_env->setGlobal("createUnit", luaCreateUnit);
    script_env->setGlobal("getUnitAt", luaGetUnit);
    script_env->setGlobal("allUnits", luaAllUnits);
    script_env->setGlobal("hasHero", luaHasHero);
    script_env->setGlobal("defeat", luaDefeat);
    script_env->setGlobal("victory", luaVictory);
    for(sp::P<Unit> unit : getRoot()->getChildren()) {
        if (unit && unit->team == Team::Player) script_env->setGlobal(unit->unit_info->key + "Player", unit);
        if (unit && unit->team == Team::AI) script_env->setGlobal(unit->unit_info->key + "AI", unit);
    }
    sp::string script_name = "level/" + name + ".lua";
    if (sp::io::ResourceProvider::get(script_name) == nullptr)
        script_name = "level/default.lua";
    auto res = script_env->loadCoroutine(script_name);
    if (res.isErr()) {
        LOG(Debug, "Lua error:", res.error());
        hud->getWidgetWithID("LUA_ERROR")->setAttribute("caption", res.error());
    } else {
        script_coroutine = res.value();
    }
    hud->getWidgetWithID("COMBAT_LOG")->setAttribute("caption", "");
}

void Scene::moveCursor(sp::Vector2i offset)
{
    if (player_action_state == PlayerActionState::SelectUnit || player_action_state == PlayerActionState::SelectMoveTarget || player_action_state == PlayerActionState::SelectTarget) {
        auto p = cursor_pos + offset;
        if (level_size.contains(p)) {
            //sp::audio::Sound::play("sfx/cursor-move.wav");
            cursor_pos = p;
            cursor->setPosition(sp::Vector2d(cursor_pos) + sp::Vector2d(0.5, 0.5));
        }
    }
}

void Scene::activateCursor()
{
    switch(player_action_state)
    {
    case PlayerActionState::SelectUnit:
        selected_unit = getUnitAt(cursor_pos);
        if (selected_unit && selected_unit->ready && selected_unit->team == Team::Player) {
            selection_cursor = new sp::Node(selected_unit);
            selection_cursor->setAnimation(sp::SpriteAnimation::load("cursor.txt"));
            selection_cursor->animationPlay("Selected");
            selection_cursor->render_data.order = 999;
            player_action_state = PlayerActionState::SelectMoveTarget;

            buildMoveOverlay();
        } else {
            selected_unit = nullptr;
            openIngameMenu();
        }
        break;
    case PlayerActionState::SelectMoveTarget:
        move_target_position = cursor_pos;
        if (selected_unit->pos == move_target_position) {
            player_action_state = PlayerActionState::WaitMoveDone;
        } else if (getUnitAt(move_target_position) == nullptr && selected_unit->ready && selected_unit->team == Team::Player) {
            astar_source = selected_unit;
            auto path = AStar<sp::Vector2i>(selected_unit->pos, move_target_position, getAStarNeighbors, getAStarDistance);
            if (path.size() > 0 && pathCost(path, selected_unit) <= selected_unit->unit_info->move) {
                selected_unit->animateMovement(path);
                player_action_state = PlayerActionState::WaitMoveDone;
            }
        }
        break;
    case PlayerActionState::WaitMoveDone: break;
    case PlayerActionState::SelectAction: break;
    case PlayerActionState::SelectTarget:
        {
            auto diff = cursor_pos - move_target_position;
            auto dist = std::abs(diff.x) + std::abs(diff.y);
            auto target = getUnitAt(cursor_pos);

            if (dist >= selected_action->min_range && dist <= selected_action->max_range && selected_action->isValidTarget(selected_unit, target)) {
                target_unit = target;
                selected_unit->pos = move_target_position;
                player_action_state = PlayerActionState::WaitActionDone;
                selected_action->execute(selected_unit, target, getTileType(ground_tilemap->getTileIndex(cursor_pos)));
            }
        }
        break;
    case PlayerActionState::WaitActionDone: break;
    case PlayerActionState::ExecuteAITurn: break;
    case PlayerActionState::WaitAIMove: break;
    case PlayerActionState::Defeat: break;
    case PlayerActionState::Victory: break;
    }
}

void Scene::deactivateCursor()
{
    switch(player_action_state)
    {
    case PlayerActionState::SelectUnit:
        if (auto unit = getUnitAt(cursor_pos)) {
            unitinfo_gui = sp::gui::Loader::load("gui/unitinfo.gui", "UNITINFO");
            unitinfo_gui->getWidgetWithID("NAME")->setAttribute("caption", unit->unit_info->name);
            sp::string hp_info = "[HEART]:" + sp::string(unit->hp) + "/" + sp::string(unit->unit_info->max_hp) + " ";
            hp_info += unit->getCharmInfo();
            unitinfo_gui->getWidgetWithID("HP")->setAttribute("caption", hp_info);
            unitinfo_gui->getWidgetWithID("DESCRIPTION")->setAttribute("caption", unit->unit_info->description);
            sp::string action_info;
            for(const auto& action : unit->unit_info->actions) {
                if (action.type == Action::Type::Attack) {
                    action_info += action.label + ": " + sp::string(action.damage) + "dmg";
                    if (action.max_range > 1) {
                        action_info += " " + sp::string(action.max_range) + "range";
                    }
                    action_info += "\n";
                } if (action.type == Action::Type::Charm) {
                    action_info += action.label + ": " + sp::string(action.damage) + "charm";
                    if (action.max_range > 1) {
                        action_info += " " + sp::string(action.max_range) + "range";
                    }
                    action_info += "\n";
                }
            }
            unitinfo_gui->getWidgetWithID("ACTIONS")->setAttribute("caption", action_info);

            auto image = unitinfo_gui->getWidgetWithID("IMAGE");
            image->render_data.scale = {16, 16, 16};
            image->setAnimation(sp::SpriteAnimation::load(unit->unit_info->sprite[int(unit->team)]));
            image->animationPlay("Ready");
            image->show();
        }
        break;
    case PlayerActionState::SelectMoveTarget:
        player_action_state = PlayerActionState::SelectUnit;
        selection_cursor.destroy();
        move_overlay.destroy();
        selected_unit = nullptr;
        hud->getWidgetWithID("COMBAT_LOG")->setAttribute("caption", "");
        break;
    case PlayerActionState::WaitMoveDone: break;
    case PlayerActionState::SelectAction:
        player_action_state = PlayerActionState::SelectMoveTarget;
        selected_unit->teleport(selected_unit->pos);
        action_gui.destroy();
        buildMoveOverlay();
        break;
    case PlayerActionState::SelectTarget:
        player_action_state = PlayerActionState::SelectMoveTarget;
        selected_unit->teleport(selected_unit->pos);
        action_gui.destroy();
        buildMoveOverlay();
        break;
    case PlayerActionState::WaitActionDone: break;
    case PlayerActionState::ExecuteAITurn: break;
    case PlayerActionState::WaitAIMove: break;
    case PlayerActionState::Defeat: break;
    case PlayerActionState::Victory: break;
    }
}

sp::P<Unit> Scene::getUnitAt(sp::Vector2i p)
{
    for(sp::P<Unit> unit : getRoot()->getChildren())
        if (unit && unit->pos == p)
            return unit;
    return nullptr;
}

int Scene::moveCost(sp::Vector2i pos, sp::P<Unit> for_unit)
{
    auto tt = getTileType(ground_tilemap->getTileIndex(pos));
    return for_unit->unit_info->terrain_class->move_cost[int(tt)];
}

int Scene::pathCost(const std::vector<sp::Vector2i>& path, sp::P<Unit> for_unit)
{
    int cost = 0;
    for(auto p : path)
        cost += moveCost(p, for_unit);
    return cost;
}

void Scene::buildMoveOverlay()
{
    move_overlay.destroy();
    move_overlay = new sp::Tilemap(getRoot(), "movetiles.png", 1.0f, 2);
    move_overlay->render_data.order = -9;
    astar_source = selected_unit;
    std::unordered_map<sp::Vector2i, int> move_type;
    for(auto [p, c] : Dijkstra(selected_unit->pos, getAStarNeighbors)) {
        if (c <= selected_unit->unit_info->move && getUnitAt(p) == nullptr) {
            move_type[p] = 3;
            for(const auto& action : selected_unit->unit_info->actions) {
                auto amt = action.type == Action::Type::Charm ? 1 : 2;
                for(auto offset : action.targetOffsets()) {
                    auto ap = p + offset;
                    if (level_size.contains(ap) && move_type[ap] < amt)
                        move_type[ap] = amt;
                }
            }
        }
    }
    for (auto [p, mt] : move_type) {
        if (mt == 3) move_overlay->setTile(p, 0);
        if (mt == 2) move_overlay->setTile(p, 1);
        if (mt == 1) move_overlay->setTile(p, 2);
    }
}

void Scene::buildTargetOverlay()
{
    move_overlay.destroy();
    move_overlay = new sp::Tilemap(getRoot(), "movetiles.png", 1.0f, 2);
    move_overlay->render_data.order = -9;
    for(auto offset : selected_action->targetOffsets()) {
        if (!level_size.contains(move_target_position + offset)) continue;
        if (selected_action->requireTarget()) {
            auto target = getUnitAt(move_target_position + offset);
            if (target && selected_action->isValidTarget(selected_unit, target))
                move_overlay->setTile(move_target_position + offset, 1);
        } else {
            move_overlay->setTile(move_target_position + offset, 1);
        }
    }
}

void Scene::moveCursorToFirstTarget()
{
    if (!selected_action->requireTarget()) return;
    for(auto offset : selected_action->targetOffsets()) {
        if (!level_size.contains(move_target_position + offset)) continue;

        auto target = getUnitAt(move_target_position + offset);
        if (target && selected_action->isValidTarget(selected_unit, target)) {
            cursor_pos = move_target_position + offset;
            cursor->setPosition(sp::Vector2d(cursor_pos) + sp::Vector2d(0.5, 0.5));
        }
    }
}

void Scene::openIngameMenu()
{
    ingame_menu = sp::gui::Loader::load("gui/ingame_menu.gui", "INGAME_MENU");
    ingame_menu->getWidgetWithID("END_TURN")->setEventCallback([this](sp::Variant) {
        ingame_menu.destroy();
        endPlayerTurn();
    });
    ingame_menu->getWidgetWithID("BACK")->setEventCallback([this](sp::Variant) {
        ingame_menu.destroy();
    });
    ingame_menu->getWidgetWithID("EXIT")->setEventCallback([this](sp::Variant) {
        exit_level = true;
    });
}

void Scene::endPlayerTurn()
{
    // Next turn
    for(sp::P<Unit> unit : getRoot()->getChildren()) {
        if (unit && unit->team == Team::Player)
            unit->setReady(true);
    }
    player_action_state = PlayerActionState::ExecuteAITurn;
    ai_timer.start(1.0);

    auto res = script_env->callCoroutine("onTurnEnd");
    if (res.isErr()) {
        LOG(Debug, "Lua error:", res.error());
        hud->getWidgetWithID("LUA_ERROR")->setAttribute("caption", res.error());
    } else {
        script_coroutine = res.value();
    }
}

void Scene::checkDeadUnits()
{
    for(sp::P<Unit> unit : getRoot()->getChildren()) {
        if (unit && unit->hp <= 0)
            unit.destroy();
    }
}

std::vector<sp::Vector2i> Scene::planMovePath(sp::P<Unit> source, sp::Vector2i target)
{
    astar_source = source;
    return AStar<sp::Vector2i>(source->pos, target, getAStarNeighbors, getAStarDistance);
}

void Scene::executeAITurnFor(sp::P<Unit> unit)
{
    struct Move {
        sp::Vector2i position;
        const Action* action;
        sp::P<Unit> target;
        float score;
    };
    std::vector<Move> possible_attack_moves;
    std::vector<Move> possible_charm_moves;
    std::vector<Move> possible_normal_moves;

    astar_source = unit;
    for(auto [p, c] : Dijkstra(unit->pos, getAStarNeighbors)) {
        if (c <= selected_unit->unit_info->move && (getUnitAt(p) == nullptr || getUnitAt(p) == unit)) {
            if (p != unit->pos) {
                possible_normal_moves.push_back({p, nullptr, nullptr, scorePosition(p, unit->team)});
            }
            for(const auto& action : selected_unit->unit_info->actions) {
                if (action.type == Action::Type::Wait) continue;
                for(auto offset : action.targetOffsets()) {
                    auto ap = p + offset;
                    auto target = getUnitAt(ap);
                    if (target && target->team == Team::Player) {
                        if (action.type == Action::Type::Attack)
                            possible_attack_moves.push_back({p, &action, target, 0.0f});
                        if (action.type == Action::Type::Charm)
                            possible_charm_moves.push_back({p, &action, target, -float(target->heart) + sp::random(0.0f, 0.8f)});
                    }
                }
            }
        }
    }
    if (!possible_attack_moves.empty()) {
        auto move = possible_attack_moves[sp::irandom(0, possible_attack_moves.size()-1)];
        if (move.position != unit->pos) {
            auto path = AStar<sp::Vector2i>(unit->pos, move.position, getAStarNeighbors, getAStarDistance);
            if (path.empty()) return;
            unit->animateMovement(path);
            unit->pos = move.position;
        }
        selected_action = move.action;
        target_unit = move.target;
    }
    else if (!possible_charm_moves.empty()) {
        std::sort(possible_charm_moves.begin(), possible_charm_moves.end(), [](const auto& a, const auto& b) { return a.score > b.score; });
        auto move = possible_charm_moves[0];
        if (move.position != unit->pos) {
            auto path = AStar<sp::Vector2i>(unit->pos, move.position, getAStarNeighbors, getAStarDistance);
            if (path.empty()) return;
            unit->animateMovement(path);
            unit->pos = move.position;
        }
        selected_action = move.action;
        target_unit = move.target;
    }
    else if (!possible_normal_moves.empty()) {
        std::sort(possible_normal_moves.begin(), possible_normal_moves.end(), [](const auto& a, const auto& b) { return a.score > b.score; });
        auto& move = possible_normal_moves.front();
        auto path = AStar<sp::Vector2i>(unit->pos, move.position, getAStarNeighbors, getAStarDistance);
        if (path.empty()) return;
        unit->animateMovement(path);
        unit->pos = move.position;
    }
}

float Scene::scorePosition(sp::Vector2i pos, Team team)
{
    float score = sp::random(0, 0.8);
    for(sp::P<Unit> unit : getRoot()->getChildren()) {
        if (unit && unit->team != team) {
            auto diff = pos - unit->pos;
            auto dist = std::abs(diff.x) + std::abs(diff.y);
            score -= dist;
        }
    }
    return score;
}
