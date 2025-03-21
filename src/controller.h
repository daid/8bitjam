#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <sp2/io/keybinding.h>

class Controller : sp::NonCopyable
{
public:
    Controller();

    sp::io::Keybinding up;
    sp::io::Keybinding down;
    sp::io::Keybinding left;
    sp::io::Keybinding right;

    sp::io::Keybinding b;
    sp::io::Keybinding a;
    sp::io::Keybinding start;
    sp::io::Keybinding select;

    sp::PList<sp::io::Keybinding> all;
};

#endif//CONTROLLER_H
