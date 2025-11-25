#version 330 core
// PlaneStarVert
uniform mat4 ProjectionMat;
uniform mat4 ViewMat;

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 n;
layout (location = 2)  in vec2 uv;
layout (location = 3)  in vec4 ModelMatV0;
layout (location = 4)  in vec4 ModelMatV1;
layout (location = 5)  in vec4 ModelMatV2;
layout (location = 6)  in vec4 ModelMatV3;
layout (location = 7)  in vec4 Color;
layout (location = 8)  in float Radius;
layout (location = 9)  in float FadeDist;
layout (location = 10) in vec3 Center;

out vec3 vertPos;
out vec4 fColor;
out float fRadius;
out float fFadeDist;
out vec3 fCenter;
out float depth;

void main()
{
  fColor = Color;
  fRadius = Radius;
  fFadeDist = FadeDist;
  fCenter = Center;
  mat4 ModelMat;
  ModelMat[0] = ModelMatV0;
  ModelMat[1] = ModelMatV1;
  ModelMat[2] = ModelMatV2;
  ModelMat[3] = ModelMatV3;
  ModelMat = transpose(ModelMat);
  vec4 vertPos4 =  ModelMat * vec4(v, 1.0);
  vertPos = vec3(vertPos4.xyz);
  vec4 vertPosCamSpace = ViewMat * vertPos4;
  depth = length(vertPosCamSpace.xyz);
  gl_Position = ProjectionMat * vertPosCamSpace;
}
