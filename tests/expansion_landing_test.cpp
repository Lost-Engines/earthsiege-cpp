#include "resources/resource_store.h"
#include "simulation/ground_actor.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
std::string context;
void check(bool b,char const *s){if(!b)throw std::runtime_error{context+": "+s};}
template<class T>void equal(binary_reader &r,T v,char const *s){auto e{r.integer<T>()};if(v!=e)throw std::runtime_error{context+": "+s+" actual="+std::to_string(v)+" expected="+std::to_string(e)};}
template<class T>vec3<T> point(binary_reader &r){return{r.integer<T>(),r.integer<T>(),r.integer<T>()};}
void matrix(binary_reader &r,model_transform const &m){std::array<int16_t,9> a{m.basis.at(0,0),m.basis.at(0,1),m.basis.at(1,0),m.basis.at(1,1),m.basis.at(0,2),m.basis.at(1,2),m.basis.at(2,0),m.basis.at(2,1),m.basis.at(2,2)};for(auto v:a)equal(r,v,"beam matrix");r.take(2);check(m.translation==point<int32_t>(r),"beam translation");}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};ground_catalogue ground{resources};auto weapons{read_weapon_catalogue(resources)};
 auto data{read_file(argv[2])};binary_reader r{data};auto magic{r.take(8)};check(std::string(magic.begin(),magic.end())=="ESEXLND1","fixture magic");auto cases{r.integer<uint16_t>()};size_t updates{},shots{};
 for(unsigned c{};c<cases;++c){auto ratio{r.integer<int16_t>()};auto damage{r.integer<uint8_t>()};auto steps{r.integer<uint16_t>()};ground_actor actor{ground,10,1,100,{1000,20000,0},{0,0,12345}};random_state random{17};ground_damage_events events;
  for(auto section:damage==1?std::vector<size_t>{1}:damage==2?std::vector<size_t>{2,3,5,6}:std::vector<size_t>{})actor.section_hit(section,actor.profile().expansion_sections.at(section-1),random,events);
  for(unsigned s{};s<steps;++s){context="case "+std::to_string(c)+" step "+std::to_string(s);auto target{point<int32_t>(r)};bool present{r.integer<uint8_t>()!=0};std::vector<uint16_t> selections;std::vector<weapon_shot> fired;
   actor.advance_landing_craft(ratio,weapons,random,{
    .select{[&](uint16_t heading)->std::optional<size_t>{selections.push_back(heading);return present?std::optional<size_t>{0}:std::nullopt;}},
    .target{[&](size_t){return turret_target{vec3<int32_t>{target},{100,200,300},{},0};}},
    .fire{[&](weapon_shot const &shot,size_t){fired.push_back(shot);}},
   });
   equal(r,static_cast<uint16_t>(selections.size()),"target selection count");for(auto v:selections)equal(r,v,"acquisition heading");
   for(auto const &b:actor.landing_banks()){equal(r,b.target_timer,"target timer");equal(r,static_cast<uint8_t>(b.target.has_value()),"target pointer");for(auto const &w:b.weapons){equal(r,w.refire,"refire");equal(r,w.phase_timer,"phase timer");equal(r,static_cast<int16_t>(w.phase),"burst phase");}}
   equal(r,static_cast<uint16_t>(actor.instance().cell_frames().size()),"cell count");for(auto v:actor.instance().cell_frames())equal(r,v,"cell frame");
   equal(r,static_cast<uint16_t>(fired.size()),"shot count");for(auto const &shot:fired){equal(r,static_cast<uint8_t>(shot.kind),"shot kind");equal(r,weapons.projectiles.at(shot.projectile).subtype,"projectile subtype");equal(r,shot.kind==shot_kind::rocket?int16_t{0}:shot.charge,"charge");equal(r,static_cast<uint32_t>(shot.range),"range");check(shot.muzzle.position==point<int32_t>(r),"muzzle position");if(shot.kind==shot_kind::rocket)check(shot.muzzle.angles==point<uint16_t>(r),"missile orientation");else matrix(r,shot.muzzle.transform);++shots;}
   auto rng{random.snapshot()};for(auto v:rng.words)equal(r,v,"random word");equal(r,static_cast<uint8_t>(rng.first),"random first");equal(r,static_cast<uint8_t>(rng.second),"random second");++updates;
  }
 }
 r.require_end();std::cout<<updates<<" landing craft frames and "<<shots<<" original projectile launches agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
