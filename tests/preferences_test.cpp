#include "shell/preferences.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace earthsiege;
void require(bool ok,std::string const &s){if(!ok)throw std::runtime_error{s};}
int main(int argc,char **argv)try{
  if(argc!=2)return 1;
  auto const data{read_file(argv[1])};binary_reader f{data};auto const magic{f.take(8)};require(std::string(magic.begin(),magic.end())=="ESPREF01","fixture version");
  auto const count{f.integer<uint32_t>()};std::unique_ptr<preferences> prefs;std::optional<preferences::values> disk;
  for(unsigned c{};c<count;++c){auto const exists{f.integer<uint8_t>()};auto const step{f.integer<uint32_t>()};auto const op{f.integer<uint8_t>()},index{f.integer<uint8_t>()},value{f.integer<uint8_t>()},live{f.integer<uint8_t>()};auto const selected{f.take(f.integer<uint8_t>())};
    if(!step){preferences::values init(17);if(exists)for(size_t i{};i<init.size();++i)init[i]=static_cast<uint8_t>(i);disk=exists?std::optional{init}:std::nullopt;prefs=std::make_unique<preferences>(exists?bytes{init}:bytes{});auto const initial{prefs->take_changes()};require(initial.size()==17,"load callbacks");for(size_t i{};i<17;++i)require(initial[i]==preference_change{static_cast<uint8_t>(i),init[i]},"load callback value");}
    switch(op){
      case 0:prefs->set(index,value,live);break;case 1:prefs->commit(live);break;case 2:prefs->cancel(live);break;case 3:prefs->restore(index,live);break;
      case 4:disk=prefs->current();break;case 5:disk=prefs->save_selected(disk?std::optional<bytes>{*disk}:std::nullopt,selected);break;
      case 6:prefs->toggle(index,live);break;case 7:prefs->increase(index,value,live);break;case 8:prefs->decrease(index,value,live);break;
      default:throw std::runtime_error{"fixture operation"};
    }
    for(auto const *actual:{&prefs->current(),&prefs->saved(),&prefs->previous()})require(std::ranges::equal(*actual,f.take(17)),"state mismatch case "+std::to_string(c));
    std::vector<preference_change> expected;auto const callbacks{f.integer<uint8_t>()};for(unsigned i{};i<callbacks;++i)expected.push_back({f.integer<uint8_t>(),f.integer<uint8_t>()});require(prefs->take_changes()==expected,"callback mismatch");
    auto const persisted{f.take(f.integer<uint8_t>())};require(std::ranges::equal(disk?bytes{*disk}:bytes{},persisted),"persistence mismatch");
  }
  f.require_end();std::cout<<count<<" original preference transactions, callbacks and saved files match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
