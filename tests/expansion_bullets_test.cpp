#include "resources/resource_store.h"
#include "simulation/projectile_instance.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
std::string context;
void check(bool b,char const *s){if(!b)throw std::runtime_error{context+": "+s};}
template<class T>void equal(binary_reader &r,T v,char const *s){auto e{r.integer<T>()};if(v!=e)throw std::runtime_error{context+": "+s+" actual="+std::to_string(v)+" expected="+std::to_string(e)};}
template<class T>vec3<T> point(binary_reader &r){return{r.integer<T>(),r.integer<T>(),r.integer<T>()};}
random_state::state rng(binary_reader &r){random_state::state s;for(auto &v:s.words)v=r.integer<uint16_t>();s.first=r.integer<uint8_t>();s.second=r.integer<uint8_t>();return s;}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto definitions{read_weapon_catalogue(resources)};
 projectile_motions motions{read_bullet_motions(resources.read("simvol0","dat/bullets.dat"),resources.edition()),read_rocket_motions(resources.read("simvol0","dat/rockets.dat")),resources.edition()};
 auto bullets{read_model(resources.read("simvol0","dts/bullets.dts"))},rockets{read_model(resources.read("simvol0","dts/rockets.dts"))};
 auto data{read_file(argv[2])};binary_reader r{data};auto magic{r.take(8)};check(std::string(magic.begin(),magic.end())=="ESEXBUL1","fixture magic");auto cases{r.integer<uint16_t>()};size_t updates{};
 for(unsigned c{};c<cases;++c){
  context="case "+std::to_string(c);weapon_shot shot;shot.projectile=static_cast<size_t>(r.integer<int16_t>());auto ratio{r.integer<int16_t>()};shot.charge=r.integer<int16_t>();shot.kind=shot.charge?shot_kind::scaled:shot_kind::bullet;
  bool falloff{r.integer<uint8_t>()!=0},target{r.integer<uint8_t>()!=0},hit{r.integer<uint8_t>()!=0};shot.muzzle.position=point<int32_t>(r);shot.muzzle.angles=point<uint16_t>(r);
  auto angles{point<uint16_t>(r)};auto strength{r.integer<int16_t>()},decay{r.integer<int16_t>()},multiplier{r.integer<int16_t>()};random_state random{rng(r)};auto expected_rng{rng(r)};
  projectile_instance projectile{shot,definitions,motions,bullets,rockets,target,0,&random};check(projectile.flight().angles==angles,"spawn spread");check(projectile.flight().strength==strength,"initial strength");
  if(motions.bullets[projectile.flight().subtype].damage_falloff)check(projectile.flight().falloff_step==decay,"falloff step");
  auto state{random.snapshot()};check(state.words==expected_rng.words&&state.first==expected_rng.first&&state.second==expected_rng.second,"spawn random draws");
  auto steps{r.integer<uint16_t>()};
  for(unsigned s{};s<steps;++s){context="case "+std::to_string(c)+" step "+std::to_string(s);projectile_guidance guidance;guidance.target_position=point<int32_t>(r);guidance.damage_falloff=falloff;guidance.area_damage_multiplier=multiplier;
   std::vector<impact_request> packets,areas;
   auto result{projectile.advance(ratio,guidance,[&](impact_request const &p)->std::optional<int32_t>{if(p.area_radius){areas.push_back(p);return 1;}packets.push_back(p);return hit?std::optional<int32_t>{101}:std::nullopt;})};
   equal(r,static_cast<uint8_t>(result!=projectile_result::flying),"termination");check(projectile.flight().position==point<int32_t>(r),"position");check(projectile.flight().angles==point<uint16_t>(r),"angles");equal(r,projectile.flight().age,"age");equal(r,projectile.flight().strength,"strength");equal(r,projectile.flight().animation_remaining,"animation timer");equal(r,static_cast<uint16_t>(projectile.flight().cells.size()),"cell count");for(auto v:projectile.flight().cells)equal(r,v,"cell frame");
   equal(r,static_cast<uint16_t>(packets.size()),"hit dispatch count");for(auto const &p:packets){equal(r,p.reach,"ray reach");equal(r,p.radius,"ray radius");equal(r,p.damage.armour,"armour");equal(r,p.damage.shield,"shield");equal(r,p.damage.splash_q10,"splash");}
   equal(r,static_cast<uint16_t>(areas.size()),"area damage count");for(auto const &p:areas){equal(r,static_cast<int32_t>(p.damage.armour),"area damage");equal(r,static_cast<int32_t>(p.area_radius),"area radius");check(p.transform.translation==point<int32_t>(r),"area origin");}++updates;
  }
 }
 r.require_end();std::cout<<cases<<" expansion bullet traces and "<<updates<<" original updates agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
