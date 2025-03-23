#include "tileinfo.h"
#include <sp2/io/keyValueTreeLoader.h>
#include <sp2/stringutil/convert.h>


static sp::Vector2i tileset_size;
static std::vector<TileType> tile_types;
static std::unordered_map<sp::string, TerrainClass> terrain_classes;
static std::vector<int> auto_tiles;

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
    LOG(Debug, "tileset_size:", tileset_size);
    tile_types.resize(tileIndexMax(), TileType::Void);
    auto_tiles.resize(tileIndexMax(), -1);

    for(auto [p, t] : type_mapping) {
        int idx = p.x+p.y*tileset_size.x;
        switch(std::tolower(t)) {
            case 'g': tile_types[idx] = TileType::Grass; break;
            case 'r': tile_types[idx] = TileType::Road; break;
            case 'w': tile_types[idx] = TileType::Water; break;
            case 'd': tile_types[idx] = TileType::Desert; break;
            case 'm': tile_types[idx] = TileType::Mountain; break;
            case 'f': tile_types[idx] = TileType::Forest; break;
            case 't': tile_types[idx] = TileType::Town; break;
            case '#': tile_types[idx] = TileType::Wall; break;
            case ' ': break;
            case '_': break;
            default: LOG(Debug, "Unknown tile type:", sp::string(char(t)));
        }
        if (std::isupper(t)) {
            for(int x=-1;x<5; x++) {
                for(int y=-1;y<3; y++) {
                    if (x < 3 || y < 1)
                        auto_tiles[idx+x+y*tileset_size.x] = idx;
                }
            }
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

int getAutoTile(int index)
{
    if (index >= 0 && index < tileIndexMax())
        return auto_tiles[index];
    return -1;
}

sp::string getTileTypeName(TileType tt)
{
    switch(tt) {
    case TileType::Void: return "void";
    case TileType::Grass: return "grass";
    case TileType::Road: return "road";
    case TileType::Water: return "water";
    case TileType::Desert: return "desert";
    case TileType::Mountain: return "mountain";
    case TileType::Forest: return "forest";
    case TileType::Town: return "town";
    case TileType::Wall: return "wall";
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
