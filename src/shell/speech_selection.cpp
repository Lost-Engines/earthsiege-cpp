#include "speech_selection.h"
#include <array>
#include <stdexcept>
namespace earthsiege {
int chapter_commander(int chapter){constexpr std::array<int,8> table{0,1,2,3,1,2,3,1};return table.at(static_cast<size_t>(chapter));}
std::vector<uint16_t> chapter_introduction(int chapter){
 static const std::array<std::vector<uint16_t>,8> table{{
 {0,1,2,3,5,54},{6,8,9,54},{10,11,12,13,14,15,16,17,18,54},
 {19,20,21,22,23,24,25,26,54},{27,28,29,30,31,32,54},
 {33,34,35,36,37,38,39,40,41,54},{42,43,44,45,46,47,54},{48,49,50,51,52,53,54}}};
 return table.at(static_cast<size_t>(chapter));
}
speech_selection select_shell_speech(int mode,int chapter,int entry,int grade,int kind,bool experienced,random_state &random){
 constexpr std::array<unsigned,8> destinations{1501,1500,1700,1510,1520,1490,9999,1520};
 speech_selection out;out.destination=destinations.at(static_cast<size_t>(mode));
 std::string stem;
 switch(mode){
 case 0:case 1:stem="C"+std::to_string(mode==1?chapter_commander(chapter):0);out.region=3000;break;
 case 2:{
  constexpr std::array<int,8> songs{15,16,16,17,17,18,18,18};
  stem="G0";out.region=7000;out.song=songs.at(static_cast<size_t>(chapter));
  entry=chapter_introduction(chapter).at(static_cast<size_t>(entry));break;
 }
 case 3:case 4:{
  constexpr std::array<int,9> route{0,0,1,1,1,2,2,3,3};
  constexpr std::array<int,8> other{0,0,1,1,2,2,3,3};
  auto const quality{kind==0||kind==1||kind==2||kind==6?route.at(static_cast<size_t>(grade)):other.at(static_cast<size_t>(grade))};
  auto variant{random.bounded(2)};if(quality==1)variant=0;
  entry=quality*2+variant;stem="D"+std::to_string(mode==4?chapter_commander(chapter):0);out.region=5000;break;
 }
 case 5:case 6:case 7:{
  if(mode==5)entry=2;
  if(mode==6)entry=3+random.bounded(experienced?6:4);
  constexpr std::array<uint8_t,41> mechanic{1,1,1,1,0,0,1,0,0,1,0,0,0,1,1,0,1,0,0,0,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,0,0,0,0,1};
  stem="T"+std::to_string(mechanic.at(static_cast<size_t>(entry)));out.region=4000;break;
 }
 default:throw std::out_of_range{"unsupported shell speech mode"};
 }
 out.entry=static_cast<uint16_t>(entry);out.list_file="snc/"+stem+"_FILES.DAT";
 out.text_file="eng/"+(out.region==4000?std::string{"T1"}:stem)+"_text.bin";return out;
}

std::array<int16_t,3> select_mechanic_return(int condition,bool technology,int32_t recovery,int deaths,random_state &random){
 std::array<int16_t,3> result{-1,-1,-1};
 if(technology)result[1]=static_cast<int16_t>(32+random.bounded(1));
 if(condition>=96){
  result[0]=static_cast<int16_t>(10+random.bounded(4));
  if(result[1]==-1&&recovery>20000)result[1]=34;
 }else if(condition>=71)result[0]=static_cast<int16_t>(15+random.bounded(4));
 else if(condition>=51)result[0]=static_cast<int16_t>(20+random.bounded(3));
 else if(condition>0){
  result[0]=static_cast<int16_t>(24+random.bounded(4));
  if(result[1]==-1&&deaths>1)result[1]=30;
 }
 return result;
}
}
