#include "textmenu.h"
#include <sp2/graphics/gui/theme.h>
#include <sp2/graphics/font.h>


BorderPanel::BorderPanel(sp::P<sp::gui::Widget> parent)
: Widget(parent)
{
    loadThemeStyle("borderpanel");
}

void BorderPanel::updateRenderData()
{
    const sp::gui::ThemeStyle::StateStyle& t = theme->states[int(getState())];

    sp::MeshData::Vertices vertices;
    sp::MeshData::Indices indices;
    sp::Vector2d tile_size{8, 8};
    int count_x = std::ceil(getRenderSize().x / tile_size.x);
    int count_y = std::ceil(getRenderSize().y / tile_size.y);
    int count = count_x * count_y;
    vertices.reserve(4 * count);
    indices.reserve(6 * count);

    for(int y=0; y<count_y; y++)
    {
        float y0 = float(y) * tile_size.x;
        for(int x=0; x<count_x; x++)
        {
            float x0 = float(x) * tile_size.x;
            float x1 = std::min(x0 + tile_size.x, getRenderSize().x);
            float y1 = std::min(y0 + tile_size.y, getRenderSize().y);
            sp::Rect2f uv{{0, 0}, {1.0f/4.0f, 1.0f/4.0f}};
            if (x == count_x - 1)
                uv.position.x = 3.0f/4.0f;
            else if (x > 0 && (x & 1))
                uv.position.x = 1.0f/4.0f;
            else if (x > 0)
                uv.position.x = 2.0f/4.0f;
            if (y == 0)
                uv.position.y = 3.0f/4.0f;
            else if (y < count_y - 1 && (y & 1))
                uv.position.y = 2.0f/4.0f;
            else if (y < count_y - 1)
                uv.position.y = 1.0f/4.0f;
            float u = uv.size.x * (x1 - x0) / tile_size.x;
            float v = uv.size.y * (y1 - y0) / tile_size.y;
            uint16_t idx = vertices.size();
            vertices.emplace_back(sp::Vector3f(x0, y0, 0.0f), sp::Vector2f(uv.position.x, uv.position.y + v));
            vertices.emplace_back(sp::Vector3f(x1, y0, 0.0f), sp::Vector2f(uv.position.x + u, uv.position.y + v));
            vertices.emplace_back(sp::Vector3f(x0, y1, 0.0f), sp::Vector2f(uv.position.x, uv.position.y));
            vertices.emplace_back(sp::Vector3f(x1, y1, 0.0f), sp::Vector2f(uv.position.x + u, uv.position.y));
            indices.push_back(idx+0);
            indices.push_back(idx+1);
            indices.push_back(idx+2);
            indices.push_back(idx+2);
            indices.push_back(idx+1);
            indices.push_back(idx+3);
        }
    }

    render_data.mesh = sp::MeshData::create(std::move(vertices), std::move(indices), sp::MeshData::Type::Dynamic);
    render_data.shader = sp::Shader::get("internal:basic.shader");
    render_data.texture = t.texture;
}

bool BorderPanel::onPointerMove(sp::Vector2d position, int id)
{
    return true;
}

bool BorderPanel::onPointerDown(sp::io::Pointer::Button button, sp::Vector2d position, int id)
{
    return true;
}