#pragma once

#include <sp2/string.h>
#include "action.h"
#include "tileinfo.h"

class UnitInfo
{
public:
    sp::string key;
    sp::string name;
    sp::string description;
    sp::string sprite[2];
    std::vector<sp::string> charm_lines;
    int move = 5;
    int max_hp = 5;
    int max_heart = 3;
    std::vector<Action> actions;
    const TerrainClass* terrain_class = nullptr;

    sp::string getCharmLine() const;

    static void init();
    static const UnitInfo* get(const sp::string& name);
    static std::vector<UnitInfo> getAll();
};
