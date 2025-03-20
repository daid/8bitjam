#pragma once

#include <sp2/graphics/gui/widget/widget.h>


class BorderPanel : public sp::gui::Widget
{
public:
    BorderPanel(sp::P<sp::gui::Widget> parent);
    
    virtual void updateRenderData() override;
    virtual bool onPointerMove(sp::Vector2d position, int id) override;
    virtual bool onPointerDown(sp::io::Pointer::Button button, sp::Vector2d position, int id) override;
};
