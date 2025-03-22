#pragma once

#include <sp2/string.h>
#include <sp2/math/vector2.h>
#include <sp2/pointer.h>
#include "tileinfo.h"


class Unit;
class Action
{
public:
    enum class Type
    {
        Attack,
        Charm,
        Wait,
    } type = Type::Wait;
    sp::string label;
    int min_range = 0;
    int max_range = 0;
    int damage = 0;
    sp::string damage_class;

    std::vector<sp::Vector2i> targetOffsets() const;
    bool requireTarget() const;
    bool isValidTarget(sp::P<Unit> source, sp::P<Unit> target) const;

    void execute(sp::P<Unit> source, sp::P<Unit> target, TileType tt) const;
};