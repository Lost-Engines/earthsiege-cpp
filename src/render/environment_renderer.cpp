#include "environment_renderer.h"
namespace earthsiege {
environment_renderer::environment_renderer(resource_store const &resources,world_environment const &env)
 :environment{env},
  clouds{env,read_bitmaps(resources.read("simvol0","dba/"+env.resource_names[1]+".DBA"))},
  marks{env},
  texture{env,resources.read("simvol0","dba/"+env.resource_names[3]+".64K"),resources.read("simvol0","dat/"+env.resource_names[4]+".DAT")},
  reflection{env}{
 if(!env.resource_names[0].empty())horizon=read_bitmaps(resources.read("simvol0","dba/"+env.resource_names[0]+".DBA"));
}
void environment_renderer::draw(framebuffer &target,vec3<int32_t> const &position,vec3<uint16_t> const &angles,camera_projection const &projection,
                                rectangle bounds,random_state &random,palette &colours,environment_detail const &detail,vec2<int32_t> const &offset){
 background_view const view{.pitch{angles.x},.roll{angles.y},.projection_shift{projection.perspective_shift},.centre{vec2<int32_t>{projection.centre}},.offset{vec2<int32_t>{offset}},.bounds{bounds}};
 bool const textured{detail.ground>240&&detail.ground_texture};
 auto bands{make_background_bands(environment,view,!textured)};
 draw_background_bands(target,bands,bounds);
 if(textured)texture.draw(target,position,angles,view,bands);
 if(detail.panorama)draw_panorama(target,horizon,place_panorama(horizon,bands,view,angles.z,environment.background.horizon_offset),bounds);
 clouds.draw(target,position,angles,projection,bounds,detail.clouds,random);
 marks.draw(target,position,angles,projection,bounds,detail.ground,random);
 reflection.update(colours,angles.x);
}
}
