#include "input_options.h"
#include <cctype>
#include <string>
namespace earthsiege {
keyjoy_options read_keyjoy(std::string_view text){
 keyjoy_options result;int selected{};bool assignment{};size_t cursor{};
 while(cursor<text.size()){
  while(cursor<text.size()&&std::isspace(static_cast<unsigned char>(text[cursor])))++cursor;
  if(cursor==text.size()||text[cursor]==26)break;
  auto start{cursor};while(cursor<text.size()&&!std::isspace(static_cast<unsigned char>(text[cursor])))++cursor;
  std::string token{text.substr(start,cursor-start)};
  for(auto &c:token)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if(token=="#"){while(cursor<text.size()&&text[cursor]!='\n')++cursor;}
  else if(token=="=")assignment=true;
  else if(token=="TILT")selected=1;
  else if(token=="BACKTURN")selected=2;
  else if(token=="MISSILE")selected=3;
  else if(token=="DEFAULT"||token=="REVERSE"){
   if(!assignment)break;
   if(token=="REVERSE"){
    if(selected==1)result.tilt_reverse=true;
    else if(selected==2)result.backturn_reverse=true;
    else if(selected==3)result.missile_reverse=true;
    else break;
   }
   assignment=false;
  }else{result.invalid_token=true;break;}
 }
 return result;
}
int cockpit_hat_request(unsigned view,std::array<uint8_t,4> const &hat){
 constexpr std::array<int,4> actions{1,0,4,5};
 for(size_t i{};i<4;++i)if(hat[i]){
  auto const action{actions[i]};
  if((action==0&&view==0)||(action==1&&view==1))return action;
  if(action==4){if(view==0)return 4;if(view==3)return 6;}
  if(action==5){if(view==0)return 5;if(view==2)return 6;}
 }
 return -1;
}
}
