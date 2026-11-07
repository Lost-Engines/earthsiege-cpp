#include "platform/keyboard.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}}
int main()try{
  struct key_pair{int alias,key;uint16_t scan;};
  for(auto const pair:{key_pair{GLFW_KEY_HOME,GLFW_KEY_KP_7,71},{GLFW_KEY_UP,GLFW_KEY_KP_8,72},
    {GLFW_KEY_PAGE_UP,GLFW_KEY_KP_9,73},{GLFW_KEY_LEFT,GLFW_KEY_KP_4,75},{GLFW_KEY_RIGHT,GLFW_KEY_KP_6,77},
    {GLFW_KEY_END,GLFW_KEY_KP_1,79},{GLFW_KEY_DOWN,GLFW_KEY_KP_2,80},{GLFW_KEY_PAGE_DOWN,GLFW_KEY_KP_3,81},
    {GLFW_KEY_ENTER,GLFW_KEY_KP_ENTER,28}}){
    require(keyboard_held_key(pair.key).has_value(),"held alias exists");
    require(keyboard_held_key(pair.key)==keyboard_held_key(pair.alias),"held aliases agree");
    for(int modifiers{};modifiers<4;++modifiers)for(auto key:{pair.key,pair.alias})
      require(keyboard_command(key,modifiers&1,modifiers&2)==(pair.scan|((modifiers&1)?0x200:0)|((modifiers&2)?0x400:0)),"DOS command/modifier aliases");
  }
  require(keyboard_command(GLFW_KEY_KP_0,false,false)==82,"keypad zero aliases Insert");
  require(keyboard_command(GLFW_KEY_N,false,false)==49,"N reaches squad camera");
  require(keyboard_command(GLFW_KEY_F11,false,false)==87,"F11 reaches objectives");
  require(keyboard_command(GLFW_KEY_L,false,false)==38,"L reaches link control");
  keyboard_input input;input.event(*keyboard_held_key(GLFW_KEY_I),true,false);
  require(input.sample().aiming.y==64,"ordinary aim speed");
  input.modifiers(true);require(input.sample().aiming.y==256,"Alt pressed after aim key");
  input.modifiers(false);require(input.sample().aiming.y==64,"Alt released before aim key");
  input.clear();input.event(*keyboard_held_key(GLFW_KEY_HOME),true,false);
  auto a{input.sample()};require(a.movement.x==-256&&a.movement.y==-256,"Home combines forward and left");
  input.clear();input.event(*keyboard_held_key(GLFW_KEY_KP_ENTER),true,false);require(input.sample().enter,"keypad Enter held action");
  std::cout<<"Host keyboard aliases, DOS command bits and modifier transitions pass\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
