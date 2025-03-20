#include "controller.h"


Controller::Controller()
: up("UP"), down("DOWN"), left("LEFT"), right("RIGHT")
, b("B"), a("A"), start("START"), select("SELECT")
{
    up.setKeys({"up", "keypad 8", "gamecontroller:0:button:dpup", "gamecontroller:0:axis:lefty"});
    down.setKeys({"down", "keypad 2", "gamecontroller:0:button:dpdown"});
    left.setKeys({"left", "keypad 4", "gamecontroller:0:button:dpleft"});
    right.setKeys({"right", "keypad 6", "gamecontroller:0:button:dpright", "gamecontroller:0:axis:leftx"});

    b.setKeys({"z", "gamecontroller:0:button:a"});
    a.setKeys({"space", "x", "gamecontroller:0:button:x"});
    start.setKeys({"return", "escape", "gamecontroller:0:button:start"});
    select.setKeys({"backspace", "gamecontroller:0:button:back"});

    all.add(&up);
    all.add(&down);
    all.add(&left);
    all.add(&right);

    all.add(&b);
    all.add(&a);
    all.add(&start);
    all.add(&select);
}
