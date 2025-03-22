#include "unitinfo.h"

#include <sp2/io/resourceProvider.h>
#include <sp2/io/keyValueTreeLoader.h>
#include <sp2/stringutil/convert.h>
#include <sp2/random.h>


static std::vector<UnitInfo*> unit_vector;
static std::unordered_map<sp::string, UnitInfo*> unit_mapping;

void UnitInfo::init()
{
    auto kvt = sp::io::KeyValueTreeLoader::loadResource("units.txt");
    if (!kvt) return;
    for(auto [key, data] : kvt->getFlattenNodesByIds()) {
        auto info = new UnitInfo();
        info->key = key;
        info->name = key;
        if (data.find("name") != data.end()) info->name = data["name"];
        info->description = data["description"];
        info->sprite[0] = data["sprite.red"];
        info->sprite[1] = data["sprite.blue"];
        info->move = sp::stringutil::convert::toInt(data["move"]);
        info->max_hp = sp::stringutil::convert::toInt(data["hp"]);
        info->max_heart = sp::stringutil::convert::toInt(data["heart"]);
        info->terrain_class = getTerrainClass(data["terrain_class"]);
        for(int n=0; data.find("charmline." + sp::string(n)) != data.end(); n++) {
            info->charm_lines.push_back(data["charmline." + sp::string(n)]);
        }
        for(int n=0; data.find("action." + sp::string(n)) != data.end(); n++) {
            info->actions.emplace_back();
            auto& action = info->actions.back();
            for(auto kv : data["action." + sp::string(n)].split(" ")) {
                auto [key, value] = kv.partition("=");
                if (key == "label") action.label = value;
                if (key == "type" && value == "attack") action.type = Action::Type::Attack;
                if (key == "type" && value == "charm") action.type = Action::Type::Charm;
                if (key == "range") {
                    auto [min, max] = value.partition("-");
                    action.min_range = sp::stringutil::convert::toInt(min);
                    action.max_range = std::max(sp::stringutil::convert::toInt(max), action.min_range);
                }
                if (key == "damage") action.damage = sp::stringutil::convert::toInt(value);
                if (key == "class") action.damage_class = value;
            }
            if (action.label == "") action.label = action.type == Action::Type::Attack ? "Attack" : "Charm";
            action.label = (action.label + "        ").substr(0, 6);
        }
        info->actions.push_back({Action::Type::Wait, "Wait  "});
        unit_vector.push_back(info);
        unit_mapping[key] = info;
    }
}

const UnitInfo* UnitInfo::get(const sp::string& name)
{
    auto it = unit_mapping.find(name);
    if (it != unit_mapping.end()) return it->second;
    return nullptr;
}

sp::string UnitInfo::getCharmLine() const
{
    if (charm_lines.empty()) return "I [HEART] you";
    return charm_lines[sp::irandom(0, charm_lines.size()-1)];
}

std::vector<UnitInfo> UnitInfo::getAll()
{
    std::vector<UnitInfo> res;
    for(auto u : unit_vector)
        res.push_back(*u);
    return res;
}

