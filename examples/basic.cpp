#include <InputListener/GlobalScreen.h>
#include <InputListener/listener/KeyListener.h>
#include <InputListener/listener/MouseListener.h>

#include <cstdio>
#include <iostream>

class MyKeyListener : public InputListener::KeyListener {
  void onPress(const InputListener::KeyEvent &e) override {
    std::cout << e.getRawCode() << std::endl;
  }
  void onRelease(const InputListener::KeyEvent &e) override {}
  void onPressed(const InputListener::KeyEvent &e) override {}
};

class MyMouseListener : public InputListener::MouseListener {
  void onPress(const InputListener::MouseEvent &e) override {
    std::cout << "鼠标:" << e.getButton() << "按下" << std::endl;
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onRelease(const InputListener::MouseEvent &e) override {}
  void onMove(const InputListener::MouseEvent &e) override {
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onDrag(const InputListener::MouseEvent &e) override {
    std::cout << "鼠标" << e.getButton() << "拖动---> " << std::endl;
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onPressed(const InputListener::MouseEvent &e) override {}
  void onDragged(const InputListener::MouseEvent &e) override {}
};

int main(int argc, char *argv[]) {
  InputListener::GlobalScreen::registerScreenHook();
  auto l = new MyKeyListener;
  auto l2 = new MyMouseListener;
  InputListener::GlobalScreen::addKeyListener(l);
  InputListener::GlobalScreen::addMouseListener(l2);

  InputListener::GlobalScreen::removeMouseListener(l2);

  while (true) {
    getchar();
  }
}
