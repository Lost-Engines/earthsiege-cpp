#include <algorithm>
#include <iostream>
#include "resources/resource_store.h"
#include "resources/combat.h"
#include "simulation/world_effects.h"
using namespace earthsiege;
namespace {
template<typename T> vec3<T> vector(binary_reader &f){return {f.integer<T>(),f.integer<T>(),f.integer<T>()};}
model_transform matrix(binary_reader &f){model_transform r;for(unsigned int i:{0U,1U,3U,4U,2U,5U,6U,7U,8U})r.basis.data[i]=f.integer<int16_t>();r.type=f.integer<uint8_t>();r.flags=f.integer<uint8_t>();r.translation=vector<int32_t>(f);return r;}
random_state::state rng(binary_reader &f){random_state::state r;for(auto &w:r.words)w=f.integer<uint16_t>();r.first=f.integer<uint8_t>();r.second=f.integer<uint8_t>();return r;}
void check_rng(binary_reader &f,random_state::state const &a){auto e=rng(f);if(a.words!=e.words||a.first!=e.first||a.second!=e.second)f.fail("visual RNG");}
void check_emitter(binary_reader &f,smoke_emitter_state const &a){auto timer=f.integer<int16_t>(),remaining=f.integer<int16_t>(),root=f.integer<int16_t>(),family=f.integer<int16_t>();if(a.timer!=timer||a.remaining!=remaining||a.root!=root||a.family!=family)f.fail("smoke controller");}
void header(binary_reader &f,std::string const &expected){auto b=f.take(8);if(std::string(b.begin(),b.end())!=expected)f.fail("fixture version");}
void check_debris(binary_reader &f,debris_particle const &a){auto p=vector<int32_t>(f);auto ang=vector<uint16_t>(f);auto v=vector<int16_t>(f);if(a.position!=p||a.angles!=ang||a.velocity!=v){std::cerr<<"actual "<<a.position<<" "<<a.angles<<" "<<a.velocity<<" expected "<<p<<" "<<ang<<" "<<v<<'\n';f.fail("detached weapon state");}}
}
int main(int argc,char **argv)try{
 if(argc!=4)return 1;
 resource_store resources{argv[1]};auto smoke=read_file(argv[2]);binary_reader f{smoke};header(f,"ESSMK001");auto cases=f.integer<uint16_t>();size_t states{},frames{};
 for(unsigned int row{};row<cases;++row){
  auto family=f.integer<int16_t>(),ratio=f.integer<int16_t>();auto mode=f.integer<uint8_t>();auto offset=vector<int16_t>(f);random_state random{rng(f)};model_transform owner,part;
  world_effects world{resources};auto controller=world.add_smoke(family,[&]{return smoke_attachment_position(owner,mode?std::optional<vec3<int16_t>>{offset}:std::nullopt,mode==2?&part:nullptr);},random).value();
  check_rng(f,random.snapshot());check_emitter(f,world.smoke_controllers().at(controller).state);auto count=f.integer<uint16_t>();
  for(unsigned int step{};step<count;++step){owner=matrix(f);part=matrix(f);world.advance(ratio);bool alive=f.integer<uint8_t>()!=0;
   if(world.smoke_controllers().size()){auto const &c=world.smoke_controllers().at(controller);check_emitter(f,c.state);if(alive==world.smoke_controllers().retiring(controller))f.fail("smoke controller retirement");}
   else {f.take(8);if(alive)f.fail("missing emitter");}
   world.collect();auto expected=f.integer<uint16_t>();if(world.live_smoke().size()!=expected)f.fail("puff count");
   std::vector<size_t> slots;world.live_smoke().for_each([&](size_t slot,auto const &){slots.push_back(slot);});std::ranges::reverse(slots);
   for(auto slot:slots){auto const &p=world.live_smoke().at(slot);auto s=f.integer<uint16_t>();auto root=f.integer<int16_t>();auto pos=vector<int32_t>(f);auto velocity=vector<int16_t>(f);auto timer=f.integer<int32_t>();auto cell=f.integer<uint16_t>();auto length=f.integer<uint16_t>();
    if(slot!=s||p.state.root!=root||p.state.position!=pos||p.state.velocity!=velocity||p.state.timer!=timer||p.state.cell!=cell||p.model.cell_frames().front()!=cell||length!=9)f.fail("puff state row "+std::to_string(row)+" step "+std::to_string(step));
    ++states;
   }
   if(step==0){std::vector<scene_model> models;world.append_models(models);if(models.size()!=1||models.front().native_class!=4||models.front().textures.empty())f.fail("smoke drawing attachment");}
   ++frames;
  }check_rng(f,random.snapshot());if(world.smoke_controllers().size()||world.live_smoke().size())f.fail("smoke leaked after expiry");
 }
 f.require_end();auto data=read_file(argv[3]);binary_reader d{data};header(d,"ESDETW01");auto detachments=d.integer<uint16_t>();auto weapons=read_weapon_catalogue(resources.read("simvol0","dat/WEAPONS.DAT"));size_t updates{};
 for(unsigned int row{};row<detachments;++row){auto definition=d.integer<int16_t>(),model=d.integer<int16_t>();auto raw=d.take(26);std::vector<uint8_t> mount_bytes{1,0};mount_bytes.insert(mount_bytes.end(),raw.begin(),raw.end());auto mount=read_weapon_mounts(mount_bytes).front();auto transform=matrix(d);auto centre=vector<int32_t>(d);auto state=create_detached_weapon(weapons.weapons.at(static_cast<size_t>(definition)),mount,transform,centre);check_debris(d,state);if(state.model!=model)d.fail("detached model");world_effects world{resources};auto slot=world.add_detached_weapon(std::move(state)).value();auto count=d.integer<uint16_t>();
  std::vector<scene_model> models;world.append_models(models);if(models.size()!=1||models.front().native_class!=1||models.front().textures.empty())d.fail("weapon drawing attachment");
  for(unsigned int step{};step<count;++step){world.advance(256);check_debris(d,world.live_particles().at(slot).state);bool done=d.integer<uint8_t>()!=0;if(world.live_particles().retiring(slot)!=done)d.fail("weapon retirement");world.collect();++updates;}
  if(world.live_particles().size())d.fail("weapon body leaked");
 }
 d.require_end();if(cases!=18||frames!=12906||states!=113400||detachments!=28||updates!=808)throw std::runtime_error{"persistent effect coverage"};
 std::cout<<frames<<" original smoke frames / "<<states<<" puff states, and "<<detachments<<" weapon detachments / "<<updates<<" trajectories agree\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
