#include <InputListener/InputListener.h>

#include <cstdio>
#include <iostream>

/**
 * @brief 示例键盘监听器，打印按下、释放和重复事件。
 */
class MyKeyListener : public InputListener::KeyListener {
  /**
   * @brief 打印键盘按下事件。
   */
  void onPress(const InputListener::KeyEvent &event) override {
    std::cout << "key press: " << event.getRawCode() << " "
              << event.getKey() << std::endl;
  }

  /**
   * @brief 打印键盘释放事件。
   */
  void onRelease(const InputListener::KeyEvent &event) override {
    std::cout << "key release: " << event.getRawCode() << std::endl;
  }

  /**
   * @brief 打印键盘重复事件。
   */
  void onRepeat(const InputListener::KeyEvent &event) override {
    std::cout << "key repeat: " << event.getRawCode() << " "
              << event.getKey() << std::endl;
  }
};

/**
 * @brief 示例鼠标监听器，打印按下、移动和拖动事件。
 */
class MyMouseListener : public InputListener::MouseListener {
  /**
   * @brief 打印鼠标按下事件。
   */
  void onPress(const InputListener::MouseEvent &event) override {
    std::cout << "mouse press: " << event.getButton() << " ["
              << event.X2D() << "," << event.Y2D() << "]" << std::endl;
  }

  /**
   * @brief 打印鼠标移动事件。
   */
  void onMove(const InputListener::MouseEvent &event) override {
    std::cout << "mouse move: [" << event.X2D() << "," << event.Y2D()
              << "]" << std::endl;
  }

  /**
   * @brief 打印鼠标拖动事件。
   */
  void onDrag(const InputListener::MouseEvent &event) override {
    std::cout << "mouse drag: " << event.getButton() << " ["
              << event.X2D() << "," << event.Y2D() << "]" << std::endl;
  }
};

/**
 * @brief 示例程序入口，注册监听器并启动全局输入监听。
 */
int main() {
  MyKeyListener keyListener;
  MyMouseListener mouseListener;

  auto keyConnection =
      InputListener::GlobalScreen::addKeyListener(keyListener);
  auto mouseConnection =
      InputListener::GlobalScreen::addMouseListener(mouseListener);

  InputListener::GlobalScreen::registerScreenHook();

  while (true) {
    getchar();
  }
}
