#version 330 core

uniform mat4 ProjectionMat;
uniform mat4 ModelView;

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 n;
layout (location = 2)  in vec2 uv;
void main()
{
  gl_Position = ProjectionMat * ModelView * vec4(v, 1.0);

}
