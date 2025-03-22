#include "action.h"
#include "unit.h"
#include "healthbareffect.h"
#include "combatlog.h"

//AAAAAAAAAAAAAAAAHHHHHHHHHHHHH
void luaShowMessage(sp::string message, sp::string unit, int team);

std::vector<sp::Vector2i> Action::targetOffsets() const
{
    std::vector<sp::Vector2i> res;
    for(int range=min_range; range<=max_range; range++) {
        for(int n=0; n<range; n++)
            res.push_back({range-n, n});
        for(int n=0; n<range; n++)
            res.push_back({-n, range-n});
        for(int n=0; n<range; n++)
            res.push_back({-range+n, -n});
        for(int n=0; n<range; n++)
            res.push_back({n, -range+n});
    }
    return res;
}

bool Action::requireTarget() const
{
    return type != Type::Wait;
}

bool Action::isValidTarget(sp::P<Unit> source, sp::P<Unit> target) const
{
    if (source == target) return false;
    switch(type) {
    case Type::Attack: return target && target->isEnemy(source);
    case Type::Charm:  return target != nullptr;
    case Type::Wait:   return true;
    }
    return false;
}

void Action::execute(sp::P<Unit> source, sp::P<Unit> target, TileType tt) const
{
    int pre = target->hp;
    int actual_damage = damage;
    switch(type) {
    case Type::Attack:
        actual_damage = std::max(0, actual_damage - target->unit_info->terrain_class->defense[int(tt)]);
        target->hp -= actual_damage;
        if (actual_damage == 0)
            addCombatLog(source->unit_info->name + " misses " + target->unit_info->name);
        else
            addCombatLog(source->unit_info->name + " hits " + target->unit_info->name + " for " + sp::string(actual_damage) + " damage");
        if (target->hp < 1)
            addCombatLog(target->unit_info->name + " died");
        break;
    case Type::Charm:
        if (source->team == target->team) {
            target->hp = std::min(target->hp + damage * 2, target->unit_info->max_hp);
            target->heart = std::min(target->heart + damage, target->unit_info->max_heart);
            addCombatLog(target->unit_info->name + " healed for " + sp::string(damage * 2));
        } else {
            target->heart = std::max(target->heart - damage, 0);
            if (target->heart == 0) {
                target->changeTeam(source->team);
                target->setReady(false);
                target->heart = target->unit_info->max_heart;
                luaShowMessage(target->unit_info->name + ":\n" + target->unit_info->getCharmLine(), target->unit_info->key, int(target->team));
            } else {
                addCombatLog(target->unit_info->name + " remains fighting");
            }
        }
        break;
    case Type::Wait:   break;
    }
    if (pre != target->hp)
        (new HealthBarEffect(target->getParent(), target->unit_info->max_hp, pre, target->hp))->setPosition(target->getPosition2D());
}