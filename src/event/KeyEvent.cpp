#include <InputListener/event/KeyEvent.h>

namespace InputListener {

KeyEvent::KeyEvent(int r, const char *k, Modifiers &m, KeyEventType t)
    : rawCode(r), key(k), modifiers(m), type(t){};

KeyEvent::~KeyEvent() = default;

int KeyEvent::getRawCode() const { return rawCode; }

const char *KeyEvent::getKey() const { return key; }

Modifiers KeyEvent::getModifiers() const { return modifiers; }

KeyEventType KeyEvent::getType() const { return type; }

} // namespace InputListener
