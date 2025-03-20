#pragma once

#include <sp2/scene/node.h>
#include <sp2/timer.h>

class HealthBarEffect : public sp::Node
{
public:
    HealthBarEffect(sp::P<sp::Node> parent, int max, int start, int end);

    void onUpdate(float delta) override;

    int max;
    int start;
    int end;
    sp::Timer timer;
};