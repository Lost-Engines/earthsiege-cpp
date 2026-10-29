#include "hmi_fm.h"
#include "hmi_fm_tables.h"
#include <stdexcept>
#include <utility>

namespace earthsiege {
namespace {
constexpr std::array<unsigned int,9> mod{0,1,2,8,9,10,16,17,18};
constexpr auto car(unsigned int v)->unsigned int{return mod[v]+3;}
}
auto hmi_fm::read_bank(bytes source)->bank {
  bank b;b.raw.assign(source.begin(),source.end());binary_reader r{source,"HMI BNK"};r.seek(8);
  auto const count{r.integer<int16_t>()};r.integer<uint16_t>();b.names=r.integer<uint32_t>();b.patches=r.integer<uint32_t>();
  if(count<2 || b.patches+static_cast<size_t>(count)*30>source.size() || b.names+static_cast<size_t>(count)*12>source.size())r.fail("bank bounds");
  for(int i{};i<count-2;++i){auto *p{b.raw.data()+b.patches+i*30};
    p[11]=static_cast<uint8_t>(p[11]<<7|p[12]<<6|p[7]<<5|p[13]<<4|p[3]);
    p[2]=static_cast<uint8_t>(p[2]<<6|p[10]);p[5]=static_cast<uint8_t>(p[5]<<4|p[8]);p[6]=static_cast<uint8_t>(p[6]<<4|p[9]);p[14]=static_cast<uint8_t>(p[4]*2|p[14]);
    p[24]=static_cast<uint8_t>(p[24]<<7|p[25]<<6|p[20]<<5|p[26]<<4|p[16]);
    p[15]=static_cast<uint8_t>(p[15]<<6|p[23]);p[18]=static_cast<uint8_t>(p[18]<<4|p[21]);p[19]=static_cast<uint8_t>(p[19]<<4|p[22]);
  }return b;
}
hmi_fm::hmi_fm(bool opl3,bytes m,bytes d):stereo{opl3},melodic{read_bank(m)},drums{read_bank(d)} {
  // The original drum loader takes these offsets from the melodic header.
  drums.names=melodic.names;drums.patches=melodic.patches;
  state_.bend.fill(64);state_.range.fill(2);state_.volume.fill(127);state_.velocity.fill(127);state_.pan.fill(64);
  if(stereo){write(0x105,1);write(0x104,0);}else write(1,32);
  for(int repeat{};repeat<2;++repeat){for(unsigned int v{};v<9;++v)both(0xb0+v,0);write(0xbd,0);}
}
void hmi_fm::write(unsigned int reg,unsigned int value){writes.push_back({static_cast<uint16_t>(reg),static_cast<uint8_t>(value)});}
void hmi_fm::both(unsigned int reg,unsigned int value){write(reg,value);if(stereo)write(reg+256,value);}
auto hmi_fm::take_writes()->std::vector<opl_write>{return std::exchange(writes,{});}
auto hmi_fm::patch(bool percussion,unsigned int id)const->bytes {
  auto const &b{percussion?drums:melodic};size_t const start{b.patches+id*30};
  if(start+30>b.raw.size())throw std::runtime_error{"HMI instrument outside bank"};
  return bytes{b.raw}.subspan(start,30);
}
auto hmi_fm::allocate(unsigned int ch)const->unsigned int {
  for(unsigned int v{};v<9;++v)if(!state_.note[v])return v;
  for(unsigned int c{};c<16;++c)if(!state_.bent[c])for(unsigned int v{};v<9;++v)if(state_.channel[v]==c)return v;
  return ch>8?ch-9:ch;
}
void hmi_fm::voice_off(unsigned int v){if(state_.note[v]){release[25+v]&=0xdf;both(0xb0+v,release[25+v]);state_.note[v]=0;}}
void hmi_fm::set_patch(unsigned int v,bytes p){
  auto const m{mod[v]},c{car(v)};release[m]=p[6];release[c]=p[19];
  both(m+0x20,p[11]);both(m+0x40,p[2]);level[m]=p[2];both(m+0x60,p[5]);both(m+0x80,p[6]);
  if(stereo){write(v+0xc0,(p[14]&31)|32);write(v+0x1c0,(p[14]&31)|16);}else write(v+0xc0,p[14]);
  both(m+0xe0,p[28]);both(c+0x20,p[24]);level[c]=p[15];both(c+0x60,p[18]);both(c+0x80,p[19]);both(c+0xe0,p[29]);
}
void hmi_fm::frequency(unsigned int v,uint32_t value,bool start){
  release[16+v]=static_cast<uint8_t>(value);release[25+v]=static_cast<uint8_t>((value>>8)|32);both(0xa0+v,value);
  if(start)both(0xb0+v,release[25+v]&0xdf);
  both(0xb0+v,release[25+v]);
}
auto hmi_fm::amplitude(unsigned int v)const->unsigned int {return (state_.velocity[v]*((state_.volume[state_.channel[v]]<<7)/127)>>7)&255;}
auto hmi_fm::attenuation(unsigned int v,unsigned int amp)const->uint8_t {
  return static_cast<uint8_t>((8192-(64-fm_volume_curve[(amp&255)>>1])*2*(64-(level[car(v)]&63)))>>7);
}
auto hmi_fm::bent_frequency(unsigned int v)const->uint32_t {
  auto const ch{state_.channel[v]},note{static_cast<uint32_t>(state_.note[v])},bend{state_.bend[ch]},range{state_.range[ch]};
  auto lookup=[&](int64_t index){auto const &table{stereo?fm_frequency_opl3:fm_frequency_opl2};return index>=0&&static_cast<size_t>(index)<table.size()?table[static_cast<size_t>(index)]:0u;};
  auto base{lookup(note)};uint32_t diff{};
  if(bend<64){
    // OPL3's downward path indexes the range by voice, not MIDI channel.
    diff=base-lookup(static_cast<int64_t>(note)-(stereo?state_.range[v]:range));
    if(diff>0x2cf)diff=((base&0x3ff)-lookup(114+range))&0x3ff;
    base-=diff*((63-bend)*1000>>6)/1000;
  }else{
    diff=lookup(note+range)-base;
    if(diff>0x2cf){base=((base&0x1c00)+0x400)|lookup(115+(11-(note-12)%12));diff=lookup(note+range)-base;}
    base+=diff*((bend-64)*1000>>6)/1000;
  }return base;
}
void hmi_fm::note_on(uint8_t ch,uint8_t note,uint8_t velocity){
  auto const v{allocate(ch)},m{mod[v]},c{car(v)};voice_off(v);state_.channel[v]=ch;
  for(unsigned int i{};i<5;++i){both(c+0x80,release[c]|15);both(m+0x80,release[m]|15);}
  bool const percussion{ch==9};auto const p{patch(percussion,percussion?note:state_.program[ch])};set_patch(v,p);state_.velocity[v]=velocity;
  auto const a{attenuation(v,amplitude(v))};
  auto set_level=[&](unsigned int op){auto const val{a|(level[op]&0xc0)};if(stereo&&!percussion){write(op+0x140,val);write(op+0x40,val);}else both(op+0x40,val);};
  if(!percussion && !p[14])set_level(m);
  set_level(c);
  if(stereo){state_.note[v]=note;pan(ch,static_cast<uint8_t>(state_.pan[ch]));}
  auto const pitch{percussion?drums.raw.at(drums.names+note*12+2):note};
  frequency(v,(stereo?fm_frequency_opl3:fm_frequency_opl2)[pitch],true);state_.note[v]=note;
  if(!percussion && state_.bent[ch])frequency(v,bent_frequency(v),false);
}
void hmi_fm::note_off(uint8_t ch,uint8_t note,uint8_t velocity){
  if(!state_.sustain[ch] || state_.deferred[ch].size()>31){for(unsigned int v{};v<9;++v)if(state_.note[v]==note&&state_.channel[v]==ch)voice_off(v);}
  else state_.deferred[ch].push_back({note,velocity,ch});
}
void hmi_fm::volume(uint8_t ch,uint8_t value){
  state_.volume[ch]=value;
  for(unsigned int v{};v<9;++v)if(state_.note[v]&&state_.channel[v]==ch){
    auto const a{attenuation(v,amplitude(v))};auto set=[&](unsigned int op){both(op+0x40,a|(level[op]&0xc0));};
    bool const additive{!patch(false,state_.program[ch])[14]};
    if(stereo){set(car(v));if(additive)set(mod[v]);}else{if(additive)set(mod[v]);set(car(v));}
  }
}
void hmi_fm::pan(uint8_t ch,uint8_t value){
  state_.pan[ch]=value;auto const amount{static_cast<uint8_t>((value>63?127-value:value)<<1)};
  for(unsigned int v{};v<9;++v)if(state_.note[v]&&state_.channel[v]==ch){
    auto const amp{(amplitude(v)<<7)/127*amount>>7};auto const a{attenuation(v,amp)};
    auto set=[&](unsigned int op){write(op+0x40+(value<64?256:0),a|(level[op]&0xc0));};
    set(car(v));if(!patch(false,state_.program[ch])[14])set(mod[v]);
  }
}
void hmi_fm::reset(uint8_t ch,bool all_notes){
  state_.volume[ch]=127;state_.sustain[ch]=0;
  // The driver's channel-indexed velocity reset overwrites unrelated flag
  // storage for channels >= 9; those flags have no consumer affecting sound.
  if(ch<9)state_.velocity[ch]=127;
  if(all_notes){for(unsigned int v{};v<9;++v)if(state_.channel[v]==ch)voice_off(v);}
  else{state_.bend[ch]=64;state_.range[ch]=2;}
}
void hmi_fm::midi(bytes msg){
  if(msg.empty())return;
  auto const ch{static_cast<uint8_t>(msg[0]&15)};auto const type{msg[0]&0xf0};
  if(msg.size()<static_cast<size_t>(type==0xc0||type==0xd0?2:3))throw std::runtime_error{"Truncated FM MIDI packet"};
  switch(type){
    case 0x80:note_off(ch,msg[1],msg[2]);break;
    case 0x90:if(msg[2])note_on(ch,msg[1],msg[2]);else note_off(ch,msg[1],0);break;
    case 0xc0:state_.program[ch]=msg[1];break;
    case 0xe0:if(ch!=9){state_.bend[ch]=msg[2];state_.bent[ch]=1;for(unsigned int v{};v<9;++v)if(state_.note[v]&&state_.channel[v]==ch)frequency(v,bent_frequency(v),false);}break;
    case 0xb0:switch(msg[1]){
      case 7:volume(ch,msg[2]);break;case 10:if(stereo)pan(ch,msg[2]);break;
      case 64:state_.sustain[ch]=msg[2];if(!msg[2]){auto &queue{state_.deferred[ch]};while(!queue.empty()){auto const entry{queue.back()};queue.pop_back();note_off(entry[2],entry[0],entry[1]);}}break;
      case 102:state_.range[ch]=msg[2];break;case 121:reset(ch,false);break;case 123:reset(ch,true);break;default:break;
    }break;default:break;
  }
}
}
