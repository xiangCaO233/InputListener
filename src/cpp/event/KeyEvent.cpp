#include "../../headers/event/KeyEvent.h"

KeyEvent::KeyEvent(int r, const char *k, Modifiers &m, KeyEventType t)
    : rawCode(r), key(k), modifiers(m), type(t){};

KeyEvent::~KeyEvent() { delete[] key; };

int KeyEvent::getRawCode() const { return rawCode; }

const char *KeyEvent::getKey() const { return key; }

Modifiers KeyEvent::getModifiers() const { return modifiers; }

KeyEventType KeyEvent::getType() const { return type; }
