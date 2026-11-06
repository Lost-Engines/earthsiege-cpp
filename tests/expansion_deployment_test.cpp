#include "resources/resource_store.h"
#include "simulation/deployment.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
std::string context;
void check(bool value,char const *field){if(!value)throw std::runtime_error{context+": "+field};}
template<class T>void equal(binary_reader &r,T actual,char const *field){auto expected{r.integer<T>()};if(actual!=expected)throw std::runtime_error{context+": "+field+" actual="+std::to_string(actual)+" expected="+std::to_string(expected)};}
template<class T>vec3<T> point(binary_reader &r){return{r.integer<T>(),r.integer<T>(),r.integer<T>()};}
void state(binary_reader &r,deployment_state const &s,bool released,vec3<int32_t> const &member){
 check(s.position==point<int32_t>(r),"position");check(s.angles==point<uint16_t>(r),"angles");check(s.velocity==point<int16_t>(r),"velocity");equal(r,uint8_t(s.landed),"landed");equal(r,uint8_t(s.obstructed),"obstructed");equal(r,s.clock,"clock");equal(r,int32_t(s.warned),"warned");equal(r,s.cell,"cell");equal(r,uint8_t(released),"released");check(member==point<int32_t>(r),"member position");
}
}
int main(int argc,char **argv)try{
 if(argc!=4)return 1;resource_store resources{argv[1]};auto model{read_model(resources.read("simvol0","dts/METEOR.DTS"))};auto frames{std::get<shape_part>(model.roots.at(1).content).cell_frame_counts.front()};
 auto data{read_file(argv[2])};binary_reader r{data};auto magic{r.take(8)};check(std::string(magic.begin(),magic.end())=="ESEXPOD1","magic");auto cases{r.integer<uint16_t>()};size_t updates{};
 for(unsigned c{};c<cases;++c){auto seed{r.integer<uint16_t>()};auto ratio{r.integer<int16_t>()};bool const obstruction{r.integer<uint8_t>()!=0};random_state random{static_cast<int16_t>(seed)};auto s{create_deployment({10000,20000,0},random)};bool released{};vec3<int32_t> member{-1,-2,-3};context="case "+std::to_string(c);state(r,s,released,member);auto rng{random.snapshot()};for(auto v:rng.words)equal(r,v,"random");equal(r,uint8_t(rng.first),"random first");equal(r,uint8_t(rng.second),"random second");
  auto steps{r.integer<uint16_t>()};for(unsigned i{};i<steps;++i){context="case "+std::to_string(c)+" frame "+std::to_string(i);std::vector<uint16_t> sounds;std::vector<vec3<int32_t>> hits;
   bool const done{advance_deployment(s,static_cast<uint16_t>(frames),ratio,{.sound{[&](uint16_t id){sounds.push_back(id);}},.impact{[&](auto const &p){hits.push_back(p);return obstruction;}},.finish{[&](auto const &p,bool clear){if(clear){released=true;member=p;}}}})};
   state(r,s,released,member);equal(r,uint16_t(sounds.size()),"sounds");equal(r,uint8_t(done),"done");for(auto v:sounds)equal(r,v,"sound id");equal(r,uint16_t(hits.size()),"hits");for(auto const &p:hits)check(p==point<int32_t>(r),"hit position");++updates;
  }
 }
 r.require_end();
 auto approach_data{read_file(argv[3])};binary_reader a{approach_data};auto am{a.take(8)};check(std::string(am.begin(),am.end())=="ESEXAPR1","approach magic");auto searches{a.integer<uint16_t>()};
 for(unsigned c{};c<searches;++c){context="approach "+std::to_string(c);auto mode{a.integer<int16_t>()},seed{a.integer<int16_t>()};auto heading{a.integer<uint16_t>()},rejected{a.integer<uint16_t>()},count{a.integer<uint16_t>()};random_state random{seed};auto choice{choose_deployment(mode,random)};unsigned attempts{};
  auto destination{deployment_destination(choice,{1000,20000,500},heading,[&](auto const &p){check(p==point<int32_t>(a),"search position");return attempts++<rejected;})};
  check(attempts==count,"search attempts");check(destination==point<int32_t>(a),"destination");equal(a,choice.pod?uint16_t{100}:static_cast<uint16_t>(choice.heading-0x8000),"actor heading");equal(a,uint8_t(choice.pod),"started");equal(a,uint8_t(choice.pod),"waiting");for(auto word:random.snapshot().words)equal(a,word,"approach random");equal(a,uint8_t(random.snapshot().first),"first tap");equal(a,uint8_t(random.snapshot().second),"second tap");
 }
 a.require_end();std::cout<<searches<<" native deployment approach searches agree\n";
 r.require_end();std::cout<<updates<<" native deployment frames agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
