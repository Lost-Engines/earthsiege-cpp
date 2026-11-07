#include "resources/resource_store.h"
#include "simulation/development_controls.h"
#include "simulation/locomotion.h"
#include <iostream>
using namespace earthsiege;
namespace {
void check(bool ok,std::string const &message){if(!ok)throw std::runtime_error{message};}
template<class T>vec3<T> point(binary_reader &r){return{r.integer<T>(),r.integer<T>(),r.integer<T>()};}
}
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto bytes{read_file(argv[1])};binary_reader r{bytes};auto magic{r.take(8)};
 check(std::string(magic.begin(),magic.end())=="ESDEV001","fixture version");
 development_controls defaults;check(defaults.translation_step==2000&&defaults.turn_step==2000,"default steps");
 auto moves{r.integer<uint32_t>()};
 for(unsigned i{};i<moves;++i){
  development_controls controls;auto step{r.integer<uint8_t>()};auto key{r.integer<uint16_t>()};
  auto pos{point<int32_t>(r)};auto angles{point<uint16_t>(r)};
  check(controls.reposition(static_cast<uint16_t>(0x601+step),pos,angles),"step command");
  check(controls.reposition(key,pos,angles),"position command");
  auto expected_pos{point<int32_t>(r)};auto expected_angles{point<uint16_t>(r)};
  check(pos==expected_pos,"original position "+std::to_string(i));check(angles==expected_angles,"original angles "+std::to_string(i));
 }
 auto throttles{r.integer<uint32_t>()};
 for(unsigned i{};i<throttles;++i){
  auto mode{r.integer<int16_t>()},value{r.integer<int16_t>()},input{r.integer<int16_t>()},steer{r.integer<int16_t>()};
  bool changed{update_player_throttle(value,input,steer,mode)};
  auto expected{r.integer<int16_t>()},expected_steer{r.integer<int16_t>()};bool expected_changed{r.integer<uint8_t>()!=0};
  check(value==expected,"original throttle "+std::to_string(i));check(steer==expected_steer,"original steering "+std::to_string(i));check(changed==expected_changed,"original ownership "+std::to_string(i));
 }
 auto budgets{r.integer<uint32_t>()};
 for(unsigned i{};i<budgets;++i){
  bool safe{r.integer<uint8_t>()!=0};auto side{r.integer<uint8_t>()};auto count{r.integer<uint16_t>()};std::vector<development_contact> contacts;
  for(unsigned j{};j<count;++j){development_contact c;c.position=point<int32_t>(r);c.radius=r.integer<int32_t>();c.strength=r.integer<int16_t>();c.side=r.integer<uint8_t>();c.eligible=r.integer<uint8_t>()!=0;contacts.push_back(c);}
  auto expected{r.integer<uint32_t>()};auto actual{development_damage_passes(contacts,{0,0,1000},side,safe)};
  check(actual==static_cast<int>(expected),"original damage passes "+std::to_string(i)+" expected="+std::to_string(expected)+" got="+std::to_string(actual));
 }
 r.require_end();std::cout<<moves<<" repositionings, "<<throttles<<" throttle cases and "<<budgets<<" damage budgets match both original editions\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
