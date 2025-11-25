#version 330 core

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 vn;
layout (location = 2)  in vec2 vt;
layout (location = 3)  in vec4 TextColor_in;
layout (location = 4)  in vec4 TexCoord_in;
layout (location = 5)  in mat4 Model;
out vec4 TextColor;
out vec2 uv;
uniform mat4 Projection;

void main()
{
  uv.x = mix(TexCoord_in.x, TexCoord_in.z, vt.x);
  uv.y = mix(TexCoord_in.y, TexCoord_in.w, vt.y);
  TextColor = TextColor_in;
  gl_Position = Projection * Model * vec4(v,1);
}