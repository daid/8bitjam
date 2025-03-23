#pragma once

#include <sp2/scene/tilemap.h>

enum class TileType
{
    Void,
    Grass,
    Road,
    Water,
    Desert,
    Mountain,
    Forest,
    Town,
    Wall,

    MAX
};

void initTileInfo();
sp::P<sp::Tilemap> makeGroundTilemap(sp::P<sp::Node> parent);
TileType getTileType(int index);
sp::string getTileTypeName(TileType tt);
int tileIndexMax();
sp::Vector2i tilesetSize();
int getAutoTile(int index);

class TerrainClass
{
public:
    int move_cost[int(TileType::MAX)];
    int defense[int(TileType::MAX)];
};

const TerrainClass* getTerrainClass(const sp::string& key);