#include "shell/game_state.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
bytes blob(binary_reader &f){return f.take(f.integer<uint32_t>());}
random_state::state rng(binary_reader &f){random_state::state s;for(auto &v:s.words)v=f.integer<uint16_t>();s.first=f.integer<uint8_t>();s.second=f.integer<uint8_t>();return s;}
void compare(random_state::state const &a,random_state::state const &b){require(a.words==b.words&&a.first==b.first&&a.second==b.second,"native RNG state");}
void compare(bytes a,bytes b,std::string const &description){if(std::ranges::equal(a,b))return;auto mismatch{std::ranges::mismatch(a,b)};std::cerr<<description<<" sizes "<<a.size()<<'/'<<b.size()<<" mismatch "<<mismatch.in1-a.begin();if(mismatch.in1!=a.end()&&mismatch.in2!=b.end())std::cerr<<" values "<<int(*mismatch.in1)<<'/'<<int(*mismatch.in2);std::cerr<<'\n';throw std::runtime_error{"campaign cycle mismatch"};}
std::vector<uint8_t> hangars(campaign_start const &c){binary_writer f;for(auto const &h:c.hangars)f.append(write_hangar(h));return std::move(f).finish();}
campaign_start saved(bytes c,bytes h){campaign_start out;out.career=read_career(c);binary_reader f{h};for(auto &v:out.hangars)v=read_hangar(f);f.require_end();return out;}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};require(std::string(magic.begin(),magic.end())=="ESCYC001","fixture version");auto cases{f.integer<uint32_t>()};unsigned total{};
 for(unsigned c{};c<cases;++c){auto seed{f.integer<int16_t>()},battle{f.integer<int16_t>()};auto steps{f.integer<uint32_t>()};game_state game{resources,seed};game.new_campaign(battle);
  for(unsigned step{};step<steps;++step){auto return_seed{f.integer<int16_t>()};auto before{blob(f)},hangar_before{blob(f)},results{blob(f)},after{blob(f)},hangar_after{blob(f)},script{blob(f)},player{blob(f)},flags{blob(f)};
   mission_counters counters;for(auto &v:counters)v=f.integer<int16_t>();auto next_name{f.cstring()};auto rng_return{rng(f)},rng_next{rng(f)};auto label{std::to_string(c)+"/"+std::to_string(step)};
   game.update_roster();game.checkpoint();compare(write_career(game.career().career),before,label+" launch career");compare(hangars(game.career()),hangar_before,label+" launch hangar");
   game.resume_shell(return_seed,saved(before,hangar_before));game.complete(read_mission_results(results),counters);
   compare(write_career(game.career().career),after,label+" return career");compare(hangars(game.career()),hangar_after,label+" return hangar");compare(game.random_snapshot(),rng_return);
   game.prepare_campaign(true);require(normalise_path("msn/"+game.mission_name()+".msn")==normalise_path(next_name),"next mission filename");compare(write_mission_script(game.prepared().compiled.script),script,label+" next script");compare(write_player_roster(game.player()),player,label+" next roster");compare(write_mission_counters(game.prepared().counters),flags,label+" next counters");compare(game.random_snapshot(),rng_next);++total;
  }
 }
 f.require_end();std::cout<<total<<" consecutive native launch, reload, return, repair and next-mission transactions match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
