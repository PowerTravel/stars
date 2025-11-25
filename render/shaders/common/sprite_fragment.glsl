
in vec3 uv;
in vec4 Color;
out vec4 frag_color;
//uniform sampler2DArray SpriteMap;
uniform sampler2D SpriteMap;

void main()
{
#if TEXTURE_COMPONENT == 4
  vec4 Sample = texture(SpriteMap, uv.xy);
  frag_color  = Sample;
#elif TEXTURE_COMPONENT == 3
  vec4 Sample = texture(SpriteMap, uv.xy);
  frag_color  = vec4(Sample.rgb, 1);
#elif TEXTURE_COMPONENT == 1
  vec4 tex    = texture(SpriteMap, uv.xy);
  frag_color  = vec4(Color.rgb*tex.a, tex.a);
#else
  frag_color  = Color;
#endif
}