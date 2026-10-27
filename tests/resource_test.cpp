#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"

namespace {

void require(bool value, std::string const &message) {
  if(!value) throw std::runtime_error{message};
}

} // namespace

auto main(int argc, char *argv[])->int try {
  require(argc == 3, "expected assets and independent fixture paths");
  earthsiege::resource_store const store{argv[1]};
  std::ifstream fixture{argv[2]};
  require(static_cast<bool>(fixture), "cannot read resource fixture");
  std::string volume;
  std::string name;
  uint32_t expected_size{};
  uint32_t expected_crc{};
  unsigned int checked{};
  while(fixture >> volume >> name >> expected_size >> std::hex >> expected_crc >> std::dec) {
    auto const payload{store.read(volume, name)};
    require(payload.size() == expected_size && earthsiege::crc32(payload) == expected_crc, "payload differs from independent extraction: " + volume + ':' + name);
    ++checked;
  }
  require(checked == 2484, "incomplete original asset fixture");
  for(auto const &[volume_name, archive] : store.archives()) {
    for(auto const &[path, entry] : archive.entries()) {
      require(archive.find(path) == &entry, "unstable archive entry");
    }
  }
  bool rejected{};
  try { earthsiege::binary_reader{earthsiege::bytes{}}.integer<uint32_t>(); }
  catch(std::runtime_error const &) { rejected = true; }
  require(rejected, "truncated integer was accepted");
  std::cout << checked << " original resource payloads agree with independent extraction" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
