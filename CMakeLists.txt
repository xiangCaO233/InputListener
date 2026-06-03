cmake_minimum_required(VERSION 3.16)

project(InputListener VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(INPUTLISTENER_BUILD_EXAMPLES "Build InputListener example programs" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(INPUTLISTENER_SOURCES
    src/GlobalScreen.cpp
    src/dispatcher/KeyEventDispatcher.cpp
    src/dispatcher/MouseEventDispatcher.cpp
    src/event/KeyEvent.cpp
    src/event/MouseEvent.cpp
    src/listener/KeyListener.cpp
    src/listener/MouseListener.cpp)

add_library(InputListener ${INPUTLISTENER_SOURCES})
add_library(InputListener::InputListener ALIAS InputListener)

target_compile_features(InputListener PUBLIC cxx_std_17)

target_include_directories(
  InputListener
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

find_package(Threads REQUIRED)
target_link_libraries(InputListener PUBLIC Threads::Threads)

if(APPLE)
  find_library(APPLICATION_SERVICES_LIBRARY ApplicationServices REQUIRED)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
  find_library(CARBON_LIBRARY Carbon REQUIRED)

  target_link_libraries(InputListener PUBLIC ${CARBON_LIBRARY}
                                             ${COREFOUNDATION_LIBRARY}
                                             ${APPLICATION_SERVICES_LIBRARY})
endif()

if(INPUTLISTENER_BUILD_EXAMPLES)
  add_executable(InputListenerBasicExample examples/basic.cpp)
  target_link_libraries(InputListenerBasicExample
                        PRIVATE InputListener::InputListener)
endif()

install(
  TARGETS InputListener
  EXPORT InputListenerTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT InputListenerTargets
  NAMESPACE InputListener::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/InputListener)
