#pragma once
#include "audio/hmi_pcm.h"
#include <map>
#include <vector>
struct recording_backend:earthsiege::pcm_midi_backend {
  struct event{uint8_t action{},handle{};std::vector<uint32_t> values;bool operator==(event const&)const=default;};
  std::map<int32_t,earthsiege::midi_sample> voices;std::vector<event> events;
  auto start(earthsiege::midi_sample const&s)->int32_t override{int32_t h{};while(h<32&&voices.contains(h))++h;if(h==32)return -1;voices[h]=s;events.push_back({0,static_cast<uint8_t>(h),{s.key,static_cast<uint32_t>(s.samples.size()),s.volume,s.pan,s.flags,s.offset}});return h;}
  void stop(int32_t h)override{if(voices.erase(h))events.push_back({1,static_cast<uint8_t>(h),{}});}
  void volume(int32_t h,uint32_t value)override{if(voices.contains(h)){voices[h].volume=value;events.push_back({2,static_cast<uint8_t>(h),{value}});}}
  void pan(int32_t h,uint32_t value)override{if(voices.contains(h)){voices[h].pan=value;events.push_back({3,static_cast<uint8_t>(h),{value}});}}
};
