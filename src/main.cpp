#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include "resources/resource_store.h"

auto main(int argc, char *argv[])->int try {
  std::filesystem::path data_dir{earthsiege::resource_store::locate(std::filesystem::absolute(argv[0]))};
  bool list{};
  for(int index{1}; index < argc; ++index) {
    std::string_view const argument{argv[index]};
    if(argument == "--help") {
      std::cout << "Earthsiege C++\nUsage: earthsiege [--data-dir PATH] [--list-resources]\n";
      return EXIT_SUCCESS;
    }
    if(argument == "--data-dir" && index + 1 < argc) data_dir = argv[++index];
    else if(argument == "--list-resources") list = true;
    else throw std::runtime_error{"unknown or incomplete option: " + std::string{argument}};
  }
  earthsiege::resource_store const resources{data_dir};
  unsigned int count{};
  for(auto const &[volume, archive] : resources.archives()) {
    for(auto const &[name, entry] : archive.entries()) {
      ++count;
      if(list) std::cout << volume << ':' << name << '\t' << entry.size << '\t' << std::hex << std::setw(8) << std::setfill('0') << earthsiege::crc32(archive.read(entry)) << std::dec << '\n';
    }
  }
  if(!list) std::cout << "Earthsiege: " << count << " original resources loaded from " << resources.root() << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
