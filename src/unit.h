#pragma once

#include <sp2/scene/node.h>
#include <sp2/timer.h>
#include "unitinfo.h"

enum class Team
{
    Player,
    AI,
};

class Unit : public sp::Node
{
public:
    Unit(sp::P<sp::Node> parent, sp::Vector2i position, const sp::string& type, Team team);

    void onUpdate(float delta) override;

    void teleport(sp::Vector2i p);
    void animateMovement(std::vector<sp::Vector2i> path);
    void setReady(bool r);

    bool isEnemy(sp::P<Unit> other);
    void changeTeam(Team new_team);

    sp::string getCharmInfo();

    bool ready = true;
    const UnitInfo* unit_info = nullptr;
    sp::Vector2i pos;

    int hp = 5;
    int heart = 3;
    Team team = Team::Player;
    bool allow_carry_over = true;
    bool destroy_me = false;

    std::vector<sp::Vector2i> move_path;
    sp::Timer move_timer;

    void luaTeleport(int x, int y);
    void luaMove(int x, int y);
    bool luaIsMoving();
    int luaGetX() const;
    void luaSetX(int x);
    int luaGetY() const;
    void luaSetY(int y);
    void luaDestroy();
    bool luaIsPlayer();
    void luaSetGhost();
protected:
    void onRegisterScriptBindings(sp::script::BindingClass& script_binding_class) override;
};