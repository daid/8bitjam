#include "tileinfo.h"
#include <sp2/io/keyValueTreeLoader.h>
#include <sp2/stringutil/convert.h>


static sp::Vector2i tileset_size;
static std::vector<TileType> tile_types;
static std::unordered_map<sp::string, TerrainClass> terrain_classes;

void initTileInfo()
{
    auto s = sp::io::ResourceProvider::get("tiletype.txt");
    std::unordered_map<sp::Vector2i, int> type_mapping;
    while(s->tell() != s->getSize()) {
        auto line = s->readLine().strip();
        for(int x=0; x<int(line.size()); x++) {
            type_mapping[{x, tileset_size.y}] = line[x];
            tileset_size.x = std::max(tileset_size.x, x + 1);
        }
        if (!line.empty())
            tileset_size.y += 1;
    }
    tile_types.resize(tileIndexMax(), TileType::Void);
    for(auto [p, t] : type_mapping) {
        switch(t) {
            case 'g': tile_types[p.x+p.y*tileset_size.x] = TileType::Grass; break;
            case 'w': tile_types[p.x+p.y*tileset_size.x] = TileType::Water; break;
            case 'S': tile_types[p.x+p.y*tileset_size.x] = TileType::Solid; break;
            case 'M': tile_types[p.x+p.y*tileset_size.x] = TileType::Mountain; break;
        }
    }

    auto kvt = sp::io::KeyValueTreeLoader::loadResource("terrainclasses.txt");
    if (!kvt) return;
    for(auto [key, data] : kvt->getFlattenNodesByIds()) {
        for(auto [k, v] : data) { LOG(Debug, k, v); }
        auto& ti = terrain_classes[key];
        for(int tt=0; tt<int(TileType::MAX); tt++) {
            auto v = sp::stringutil::convert::toVector2i(data[getTileTypeName(TileType(tt))]);
            ti.move_cost[tt] = std::clamp(v.x, 1, 1000);
            ti.defense[tt] = std::clamp(v.y, 0, 1000);
        }
    }
}

TileType getTileType(int index)
{
    if (index >= 0 && index < tileIndexMax())
        return tile_types[index];
    return TileType::Void;
}

sp::string getTileTypeName(TileType tt)
{
    switch(tt) {
    case TileType::Void: return "void";
    case TileType::Solid: return "solid";
    case TileType::Grass: return "grass";
    case TileType::Water: return "water";
    case TileType::Mountain: return "mountain";
    case TileType::MAX: return "";
    }
    return "";
}

int tileIndexMax()
{
    return tileset_size.x * tileset_size.y;
}

sp::P<sp::Tilemap> makeGroundTilemap(sp::P<sp::Node> parent)
{
    return new sp::Tilemap(parent, "tileset.png", 1.0, 1.0, tileset_size.x, tileset_size.y);
}

sp::Vector2i tilesetSize()
{
    return tileset_size;
}

const TerrainClass* getTerrainClass(const sp::string& key)
{
    auto it = terrain_classes.find(key);
    if (it != terrain_classes.end())
        return &it->second;
    LOG(Debug, "Terrain class:", key, "not found");
    return &terrain_classes["Basic"];
}