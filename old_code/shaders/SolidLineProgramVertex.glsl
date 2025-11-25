#version 330 core

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 vn;
layout (location = 2)  in vec2 vt;
layout (location = 3)  in vec3 P0;
layout (location = 4)  in vec3 P1;
layout (location = 5)  in vec4 Color_in;
layout (location = 6)  in float Thickness;
out vec4 Color;
uniform mat4 ProjectionMat;
void main()
{
  Color = Color_in;
  vec3 Pos = (P0 + P1)*0.5;
  vec3 Length = (P1 - P0)*0.5;

  gl_Position = ProjectionMat * vec4(v,1);
}