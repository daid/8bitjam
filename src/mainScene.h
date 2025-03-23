#ifndef MAIN_SCENE_H
#define MAIN_SCENE_H

#include <sp2/scene/scene.h>
#include <sp2/scene/tilemap.h>
#include <sp2/graphics/gui/widget/widget.h>
#include <sp2/script/environment.h>
#include "unit.h"
#include <unordered_set>


class Scene;
extern Scene* scene_instance;
class Scene : public sp::Scene
{
public:
    Scene(sp::string start_level);
    ~Scene();

    void onUpdate(float delta) override;
    void onFixedUpdate() override;

    void loadLevel(const sp::string& name);
    void moveCursor(sp::Vector2i);
    void activateCursor();
    void deactivateCursor();
    void buildMoveOverlay();
    void buildTargetOverlay();
    void moveCursorToFirstTarget();
    void endPlayerTurn();
    void openIngameMenu();

    sp::P<Unit> getUnitAt(sp::Vector2i);
    int moveCost(sp::Vector2i pos, sp::P<Unit> for_unit);
    int pathCost(const std::vector<sp::Vector2i>& path, sp::P<Unit> for_unit);
    void checkDeadUnits();

    std::vector<sp::Vector2i> planMovePath(sp::P<Unit> source, sp::Vector2i target);

    void executeAITurnFor(sp::P<Unit> unit);
    float scorePosition(sp::Vector2i pos, Team team);

    float hold_button_time = 0.0f;

    sp::P<sp::Tilemap> ground_tilemap;
    sp::P<sp::Tilemap> move_overlay;
    sp::Rect2i level_size;
    sp::string current_level;
    sp::Vector2i cursor_pos;
    sp::P<sp::Node> cursor;
    sp::P<sp::Node> selection_cursor;
    sp::P<sp::gui::Widget> action_gui;
    sp::P<sp::gui::Widget> hud;
    sp::P<sp::gui::Widget> unitinfo_gui;
    sp::P<sp::gui::Widget> ingame_menu;
    bool exit_level = false;
    sp::Timer combat_log_timer;

    sp::P<Unit> selected_unit;
    sp::Vector2i move_target_position;
    const Action* selected_action;
    sp::Timer ai_timer;
    sp::P<Unit> target_unit;

    enum class PlayerActionState
    {
        SelectUnit,
        SelectMoveTarget,
        WaitMoveDone,
        SelectAction,
        SelectTarget,
        WaitActionDone,
        ExecuteAITurn,
        WaitAIMove,
        Defeat,
        Victory,
    } player_action_state = PlayerActionState::SelectUnit;

    sp::P<sp::script::Environment> script_env;
    sp::script::CoroutinePtr script_coroutine;

    std::unordered_set<sp::string> has_heroes;
};

#endif//MAIN_SCENE_H
