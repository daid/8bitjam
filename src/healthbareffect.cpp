#include "healthbareffect.h"
#include <sp2/graphics/meshdata.h>
#include <sp2/tween.h>


HealthBarEffect::HealthBarEffect(sp::P<sp::Node> parent, int max, int start, int end)
: sp::Node(parent), max(max), start(start), end(end)
{
    render_data.type = sp::RenderData::Type::Normal;
    render_data.shader = sp::Shader::get("internal:color.shader");
    render_data.order = 2000;
    render_data.color = sp::Color(0xFF89D900);
    timer.start(1.0);
}

void HealthBarEffect::onUpdate(float delta)
{
    if (timer.isExpired()) {
        delete this;
        return;
    }

    float hp = sp::Tween<float>::linear(std::min(0.5f, timer.getProgress()), 0.0f, 0.5f, float(start), float(end));
    float w = hp / float(max);

    sp::MeshData::Vertices vertices;
    sp::MeshData::Indices indices{0,1,2,2,1,3};
    vertices.reserve(4);
    
    float x0 = -0.5f;
    float x1 = x0 + w;
    float y0 = 0.5f - 1.0f / 8.0f;
    float y1 = y0 + 1.0f / 8.0f;
    vertices.emplace_back(sp::Vector3f(x0, y0, 0.0f));
    vertices.emplace_back(sp::Vector3f(x1, y0, 0.0f));
    vertices.emplace_back(sp::Vector3f(x0, y1, 0.0f));
    vertices.emplace_back(sp::Vector3f(x1, y1, 0.0f));
    
    render_data.mesh = sp::MeshData::create(std::move(vertices), std::move(indices));
}
