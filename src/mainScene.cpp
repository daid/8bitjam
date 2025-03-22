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

Scene::Scene()
: sp::Scene("MAIN")
{
    scene_instance = this;
    sp::Scene::get("INGAME_MENU")->enable();

    hud = sp::gui::Loader::load("gui/hud.gui", "HUD");

    loadLevel("0");
}

Scene::~Scene()
{
    hud.destroy();
    action_gui.destroy();
    unitinfo_gui.destroy();
    sp::Scene::get("INGAME_MENU")->disable();
}

void Scene::onUpdate(float delta)
{
    int input = 0;
    int hold = 0;
    if (unitinfo_gui) {
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
    }
    if (hud->getWidgetWithID("MESSAGE_WINDOW")->isVisible()) {
        if (controller.a.getDown()) hud->getWidgetWithID("MESSAGE_WINDOW")->hide();
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
                // Next turn
                for(sp::P<Unit> unit : getRoot()->getChildren()) {
                    if (unit && unit->team == Team::Player)
                        unit->setReady(true);
                }
                player_action_state = PlayerActionState::ExecuteAITurn;
                ai_timer.start(0.1);
            }
        }
        break;
    case PlayerActionState::ExecuteAITurn:
        if (ai_timer.isExpired()) {
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
            }
        }
        break;
    case PlayerActionState::WaitAIMove:
        if (selected_unit->move_path.empty()) {
            ai_timer.start(0.5);
            selected_unit->setReady(false);
            player_action_state = PlayerActionState::ExecuteAITurn;
            if (target_unit && selected_action) {
                selected_action->execute(selected_unit, target_unit);
            }
        }
        break;
    }

    auto unit = getUnitAt(cursor_pos);
    if (unit) {
        hud->getWidgetWithID("HP")->setAttribute("caption", sp::string(unit->hp) + "/" + sp::string(unit->unit_info->max_hp));
        hud->getWidgetWithID("NAME")->setAttribute("caption", unit->unit_info->name);
    } else {
        hud->getWidgetWithID("HP")->setAttribute("caption", "-");
        hud->getWidgetWithID("NAME")->setAttribute("caption", "");
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
        image->render_data.scale = {16, 16, 16};
        image->setAnimation(sp::SpriteAnimation::load(unitinfo->sprite[team]));
        image->animationPlay("Ready");
        image->show();
    } else {
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

void Scene::loadLevel(const sp::string& name)
{
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
    for(sp::P<Unit> unit : getRoot()->getChildren()) {
        if (unit && unit->team == Team::Player) script_env->setGlobal(unit->unit_info->key + "Player", unit);
        if (unit && unit->team == Team::AI) script_env->setGlobal(unit->unit_info->key + "AI", unit);
    }
    auto res = script_env->loadCoroutine("level/" + name + ".lua");
    if (res.isErr()) {
        LOG(Debug, "Lua error:", res.error());
        hud->getWidgetWithID("LUA_ERROR")->setAttribute("caption", res.error());
    } else {
        script_coroutine = res.value();
    }
}

void Scene::moveCursor(sp::Vector2i offset)
{
    if (player_action_state == PlayerActionState::SelectUnit || player_action_state == PlayerActionState::SelectMoveTarget || player_action_state == PlayerActionState::SelectTarget) {
        auto p = cursor_pos + offset;
        if (level_size.contains(p)) {
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
                selected_action->execute(selected_unit, target);
            }
        }
        break;
    case PlayerActionState::WaitActionDone: break;
    case PlayerActionState::ExecuteAITurn: break;
    case PlayerActionState::WaitAIMove: break;
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
            hp_info += "[CHARM]:";
            sp::string hp0 = "[HP0]";
            sp::string hp1 = "[HP1]";
            if (unit->team == Team::AI) std::swap(hp0, hp1);
            for(int n=0; n<unit->unit_info->max_heart; n++) {
                if (n < unit->heart)
                    hp_info += hp0;
                else
                    hp_info += hp1;
            }
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
    std::vector<Move> possible_normal_moves;

    astar_source = unit;
    for(auto [p, c] : Dijkstra(unit->pos, getAStarNeighbors)) {
        if (c <= selected_unit->unit_info->move && (getUnitAt(p) == nullptr || getUnitAt(p) == unit)) {
            if (p != unit->pos) {
                possible_normal_moves.push_back({p, nullptr, nullptr, scorePosition(p, unit->team)});
            }
            for(const auto& action : selected_unit->unit_info->actions) {
                if (action.type != Action::Type::Attack) continue;
                for(auto offset : action.targetOffsets()) {
                    auto ap = p + offset;
                    auto target = getUnitAt(ap);
                    if (target && target->team == Team::Player) {
                        possible_attack_moves.push_back({p, &action, target, 0});
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
