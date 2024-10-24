#include "../headers/GlobalScreen.h"
#include "listener/KeyListener.h"
#include <QApplication>
#include <iostream>
#include <ostream>
#include <unistd.h>

class MyKeyListener : public KeyListener {
  void onPress(const KeyEvent &e) { std::cout << e.getRawCode() << std::endl; }
  void onRelease(const KeyEvent &e) {}
  void onPressed(const KeyEvent &e) {}
};

class MyMouseListener : public MouseListener {
  void onPress(const MouseEvent &e) {
    std::cout << "鼠标:" << e.getButton() << "按下" << std::endl;
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onRelease(const MouseEvent &e) {}
  void onMove(const MouseEvent &e) {
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onDrag(const MouseEvent &e) {
    std::cout << "鼠标" << e.getButton() << "拖动---> " << std::endl;
    std::cout << "位置:[" << e.X2D() << "," << e.Y2D() << "]" << std::endl;
  }
  void onPressed(const MouseEvent &e) {}
  void onDragged(const MouseEvent &e) {}
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  GlobalScreen::registerScreenHook();
  auto l = new MyKeyListener;
  auto l2 = new MyMouseListener;
  GlobalScreen::addKeyListener(l);
  GlobalScreen::addMouseListener(l2);

  GlobalScreen::removeMouseListener(l2);

  return app.exec();
}
