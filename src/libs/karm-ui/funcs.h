#pragma once

#include "node.h"

namespace Karm::Ui {

inline void shouldRepaint(Node &n) {
    Events::PaintEvent e{n.bound()};
    n.bubble(e);
}

inline void shouldRepaint(Node &n, Math::Recti bound) {
    Events::PaintEvent e{bound};
    n.bubble(e);
}

inline void shouldLayout(Node &n) {
    Events::LayoutEvent e;
    n.bubble(e);
}

inline void shouldRebuild(Node &n) {
    Events::BuildEvent e;
    n.bubble(e);
}

inline void shouldAnimate(Node &n) {
    Events::AnimateEvent e;
    n.bubble(e);
}

inline void mouseLeave(Node &n) {
    Events::MouseLeaveEvent e{};
    n.event(e);
}

inline void mouseLeave(Children &children) {
    for (auto &c : children) {
        mouseLeave(*c);
    }
}

} // namespace Karm::Ui
