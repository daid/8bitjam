#include "unit.h"
#include "mainScene.h"
#include <sp2/tween.h>
#include <sp2/graphics/spriteAnimation.h>


static constexpr float move_time_per_cell = 0.06f;


Unit::Unit(sp::P<sp::Node> parent, sp::Vector2i position, const sp::string& unit_type, Team team)
: sp::Node(parent), pos(position), team(team)
{
    unit_info = UnitInfo::get(unit_type);
    if (!unit_info) return;
    setAnimation(sp::SpriteAnimation::load(unit_info->sprite[int(team)]));
    animationPlay("Ready");
    setPosition(sp::Vector2d(position) + sp::Vector2d(0.5, 0.5));
    hp = unit_info->max_hp;
    heart = unit_info->max_heart;
}

void Unit::onUpdate(float delta)
{
    if (move_timer.isRunning()) {
        if (move_timer.isExpired()) {
            move_path.erase(move_path.begin());
            if (move_path.size() > 1) {
                move_timer.start(move_time_per_cell);
            } else {
                setPosition(sp::Vector2d(move_path[0]) + sp::Vector2d(0.5, 0.5));
                move_path.clear();
                animationPlay("Ready");
            }
        } else {
            auto start = sp::Vector2d(move_path[0]) + sp::Vector2d(0.5, 0.5);
            auto end = sp::Vector2d(move_path[1]) + sp::Vector2d(0.5, 0.5);
            setPosition(sp::Tween<sp::Vector2d>::linear(move_timer.getProgress(), 0.0f, 1.0f, start, end));
            if (start.x < end.x)
                animationSetFlags(sp::SpriteAnimation::FlipFlag);
            if (start.x > end.x)
                animationSetFlags(0);
            animationPlay("Walk");
        }
    }
    render_data.order = 500 - getPosition2D().y * 10.0;
    if (destroy_me)
        delete this;
}

void Unit::teleport(sp::Vector2i p)
{
    pos = p;
    setPosition(sp::Vector2d(pos) + sp::Vector2d(0.5, 0.5));
}

void Unit::animateMovement(std::vector<sp::Vector2i> path)
{
    path.insert(path.begin(), pos);
    move_path = path;
    move_timer.start(move_time_per_cell);
}

void Unit::setReady(bool r)
{
    ready = r;
    if (ready)
        animationPlay("Ready");
    else
        animationPlay("Idle");
}

bool Unit::isEnemy(sp::P<Unit> other)
{
    return team != other->team;
}

void Unit::changeTeam(Team new_team)
{
    if (team == new_team) return;
    team = new_team;
    setAnimation(sp::SpriteAnimation::load(unit_info->sprite[int(team)]));
    animationPlay("Ready");
}

void Unit::luaTeleport(int x, int y)
{
    teleport({x, y});
    move_path.clear();
    move_timer.stop();
}

void Unit::luaMove(int x, int y)
{
    auto path = scene_instance->planMovePath(this, {x, y});
    if (path.size() > 0) {
        animateMovement(path);
        pos = {x, y};
    }
}

bool Unit::luaIsMoving()
{
    return move_path.size() > 0;
}

int Unit::luaGetX() const
{
    return pos.x;
}

void Unit::luaSetX(int x)
{
    pos.x = x;
    setPosition(sp::Vector2d(pos) + sp::Vector2d(0.5, 0.5));
}

int Unit::luaGetY() const
{
    return pos.y;
}

void Unit::luaSetY(int y)
{
    pos.y = y;
    setPosition(sp::Vector2d(pos) + sp::Vector2d(0.5, 0.5));
}

void Unit::luaDestroy()
{
    destroy_me = true;
}

bool Unit::luaIsPlayer()
{
    return team == Team::Player;
}

void Unit::onRegisterScriptBindings(sp::script::BindingClass& script_binding_class)
{
    script_binding_class.bind("teleport", &Unit::luaTeleport);
    script_binding_class.bind("move", &Unit::luaMove);
    script_binding_class.bind("isMoving", &Unit::luaIsMoving);
    script_binding_class.bind("isPlayer", &Unit::luaIsPlayer);
    script_binding_class.bind("destroy", &Unit::luaDestroy);

    script_binding_class.bindProperty("x", &Unit::luaGetX, &Unit::luaSetX);
    script_binding_class.bindProperty("y", &Unit::luaGetY, &Unit::luaSetY);
}