
uniform mat4 ProjectionMat;
uniform mat4 ModelView;
uniform mat4 NormalView;

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 n;
layout (location = 2)  in vec2 uv;
out vec2 tc;
out vec3 fn;
out vec3 vertPos;
void main()
{
  tc = vec2(uv.x, uv.y);
  fn = vec3(NormalView * vec4(n, 0.0));
  vec4 vertPos4 = ModelView * vec4(v, 1.0);
  vertPos = vec3(vertPos4) / vertPos4.w;
  gl_Position = ProjectionMat * vertPos4;
}
