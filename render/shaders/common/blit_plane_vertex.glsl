#version 330 core

layout (location = 0) in vec3 v;
out vec2 uv;
void main()
{
  gl_Position = vec4(v,1);
  uv = (v.xy+vec2(1,1))/2.0; // Map from [(-1,-1),(1,1)] to [(0,0),(1,1)]
}
