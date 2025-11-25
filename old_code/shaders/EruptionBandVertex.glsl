#version 330 core
uniform mat4 ProjectionMat;
uniform mat4 ModelView;

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 n;
layout (location = 2)  in vec2 uv;
layout (location = 3)  in vec4 Color;
layout (location = 4)  in vec3 Center;
layout (location = 5)  in float InnerRadii;
layout (location = 6)  in float OuterRadii;

out vec3 fv;
out vec4 fColor;
out vec3 fCenter;
out float fInnerRadii;
out float fOuterRadii;
void main()
{
  fv = v;
  fColor = Color;
  fCenter = Center;
  fInnerRadii = InnerRadii;
  fOuterRadii = OuterRadii;
  gl_Position = ProjectionMat * ModelView * vec4(v, 1.0);
}
