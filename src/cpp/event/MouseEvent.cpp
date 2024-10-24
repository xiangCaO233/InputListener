#include "../../headers/event/MouseEvent.h"
#include "event/KeyEvent.h"

MouseEvent::MouseEvent(int b, Modifiers &m, MouseEventType t, int x, int y,
                       double x2d, double y2d)
    : button(b), modifiers(m), type(t), xv(x), yv(y), xd(x2d), yd(y2d) {}

MouseEvent::~MouseEvent() {}
int MouseEvent::getButton() const { return button; }

Modifiers MouseEvent::getModifiers() const { return modifiers; }

MouseEventType MouseEvent::getType() const { return type; }

int MouseEvent::X() const { return xv; }

int MouseEvent::Y() const { return yv; }

double MouseEvent::X2D() const { return xd; }

double MouseEvent::Y2D() const { return yd; }
