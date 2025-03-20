#pragma once

#include <sp2/scene/tilemap.h>

enum class TileType
{
    Void,
    Solid,
    Grass,
    Water,
    Mountain,

    MAX
};

void initTileInfo();
sp::P<sp::Tilemap> makeGroundTilemap(sp::P<sp::Node> parent);
TileType getTileType(int index);
sp::string getTileTypeName(TileType tt);
int tileIndexMax();
sp::Vector2i tilesetSize();

class TerrainClass
{
public:
    int move_cost[int(TileType::MAX)];
    int defense[int(TileType::MAX)];
};

const TerrainClass* getTerrainClass(const sp::string& key);