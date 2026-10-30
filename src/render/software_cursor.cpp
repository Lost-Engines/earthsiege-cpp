#include "software_cursor.h"
namespace earthsiege {
software_cursor::software_cursor(bytes data){
  binary_reader file{data,"cursor resource"};
  if(file.integer<uint32_t>()!=0x28000b)file.fail("expected DCI cursor");
  file.take(4);hot_x=file.integer<int32_t>();hot_y=file.integer<int32_t>();
  image=read_bitmaps(data).front();
}
void software_cursor::draw(framebuffer &target,int x,int y)const{
  scoped_framebuffer_clip clip{target,nullptr};target.image(image,x-hot_x,y-hot_y,true);
}
}
