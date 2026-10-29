#include "shell_animation.h"
#include "simulation/fixed_math.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
shell_animation::shell_animation(presentation_program value,bool loop):program{std::move(value)},repeat{loop}{}
void shell_animation::select_page(uint16_t page){
 if(page>=program.pages.size())return;
 current.page=page;current.remaining=current.period;current.wait_latched=current.fade_latched=current.executed=false;current.mode=0;
}
void shell_animation::restart(){
 // GO 0x4b3c6 leaves the weighted branch and selected shape bank untouched.
 current.mode=0;current.page=0;current.wait_latched=current.fade_latched=false;
 current.jump=-1;current.depth=current.x=current.y=current.remaining=current.period=0;
 output.push_back({0x80,{-1},{}});
}
void shell_animation::next_page(){
 if(current.jump!=-1){current.page=static_cast<uint16_t>(current.jump);current.jump=-1;}
 else if(current.branch!=-1){current.page=static_cast<uint16_t>(current.branch);current.branch=-1;}
 else if(current.mode!=3)++current.page;
 current.wait_latched=current.fade_latched=current.executed=false;
 if(current.page>=program.pages.size()){if(repeat)restart();else current.mode=2;}
}
void shell_animation::advance(int16_t elapsed,random_state &random,shell_animation_sound *sound){
 output.clear();current.remaining=wrap_word(current.remaining-elapsed);
 if(current.remaining>0||!current.executed){
  if(current.mode!=2){execute(current.page,random,sound);current.executed=true;
   if(current.remaining<1){current.remaining=current.period;next_page();}
  }
  return;
 }
 current.remaining=current.period;
 switch(current.mode){
  case 0:execute(current.page,random,sound);next_page();break;
  case 1:execute(current.page,random,sound);break;
  case 3:if(repeat)restart();else current.mode=2;break;
  default:break;
 }
}
void shell_animation::execute(uint16_t page,random_state &random,shell_animation_sound *sound,unsigned recursion){
 if(page>=program.pages.size()||recursion>32)throw std::runtime_error{"invalid shell TTM label recursion"};
 bool have_weights{};std::array<int16_t,5> cumulative{};
 for(auto const &instruction:program.pages[page]){
  auto const &a{instruction.arguments};
  switch(instruction.opcode){
   case 0xff0:return;
   case 0xc102:break; // GO 0x4a0e0 ignores the authored volume operation.
   case 0xc21f:
    if(sound&&!current.fade_latched)sound_loaded=sound->load(instruction.text);
    break;
   case 0xc220:
    if(sound&&sound_loaded&&!current.fade_latched){
     sound->volume(static_cast<uint8_t>(random.bounded(25)+10));sound->play();current.fade_latched=true;
    }
    break;
   case 0x1101:case 0x1111:case 0x1071:case 0xf04f:case 0xf10f:break; // GO ignores font operations and retained text-section labels.
   case 0x110:current.mode=3;break;
   case 0x1051:shape_bank=static_cast<uint8_t>(a.at(0));break;
   case 0xf02f:
    if(!current.executed)output.push_back({instruction.opcode,{shape_bank},instruction.text});
    break;
   case 0x80:output.push_back({0x80,{shape_bank},{}});break;
   case 0x2002:output.push_back({instruction.opcode,{a.begin(),a.end()},{}});break;
   case 0x1061:case 0xf01f:case 0xf05f:case 0x4114:case 0x4124:
    if(!current.executed)output.push_back({instruction.opcode,{a.begin(),a.end()},instruction.text});
    break;
   case 0x20:
    if(!current.executed)output.push_back({0x4204,{0,0,319,199},{}});
    break;
   case 0x1021:case 0x2022:
    if(!current.wait_latched){
     current.wait_latched=true;
     current.remaining=current.period=instruction.opcode==0x1021?a.at(0):
      wrap_word(a.at(0)+random.bounded(wrap_word(a.at(1)-a.at(0)+1)));
    }
    break;
   case 0x1201:current.jump=program.label_page(a.at(0));break;
   case 0x3003:
    ++current.depth;current.x=a.at(0);current.y=a.at(1);
    execute(program.label_page(a.at(2)),random,sound,recursion+1);--current.depth;break;
   case 0x5005:
    if(have_weights&&current.branch==-1){
     auto const choice{random.bounded(cumulative.back())};size_t index{};
     while(index<5&&choice>=cumulative[index])++index;
     current.branch=program.label_page(a.at(index==5?0:index));
    }else{
     int16_t sum{};for(size_t i{};i<5;++i){sum=wrap_word(sum+a.at(i));cumulative[i]=sum;}
     have_weights=true;
    }
    break;
   case 0x4204:
    if(!current.executed){
     int32_t const x{a.at(0)},y{a.at(1)},right{x+a.at(2)},bottom{y+a.at(3)};
     output.push_back({instruction.opcode,{std::min(x,right),std::min(y,bottom),std::max(x,right),std::max(y,bottom)},{}});
    }
    break;
   case 0xa504:case 0xa524:case 0xa104:case 0xa0a4:{
    std::vector<int32_t> args(a.begin(),a.end());
    if(current.depth){args[0]=wrap_word(args[0]+current.x);args[1]=wrap_word(args[1]+current.y);}
    output.push_back({instruction.opcode,std::move(args),{}});break;
   }
   default:throw std::runtime_error{"unsupported GO TTM operation "+std::to_string(instruction.opcode)};
  }
 }
}
}
