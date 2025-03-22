#include "combatlog.h"

static std::vector<sp::string> combatlog;

void addCombatLog(sp::string message)
{
    combatlog.push_back(message);
}

sp::string getCombatLog()
{
    if (combatlog.empty()) return "";
    auto res = combatlog.front();
    combatlog.erase(combatlog.begin());
    return res;
}