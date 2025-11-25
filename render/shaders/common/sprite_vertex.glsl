
layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 vn;
layout (location = 2)  in vec2 vt;
layout (location = 3)  in vec4 Color_in;
layout (location = 4)  in vec4 TexCoord;
layout (location = 5)  in int TexDepth;
layout (location = 6)  in mat4 Model;
uniform mat4 Projection;
out vec4 Color;

out vec3 uv;

void main()
{
  uv.x = mix(TexCoord.x, TexCoord.z, vt.x);
  uv.y = mix(TexCoord.y, TexCoord.w, vt.y);
  uv.z = TexDepth;
  Color = Color_in;
  gl_Position = Projection * Model * vec4(v,1);
}