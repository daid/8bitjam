#include "combatlog.h"

static std::vector<sp::string> combatlog;

void addCombatLog(sp::string message)
{
    combatlog.push_back(message);
}