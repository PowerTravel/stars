uniform mat4 ProjectionMat;
uniform mat4 View;
uniform mat4 Model;
uniform mat4 NormalModel;

layout (location = 0)  in vec3 v;
layout (location = 1)  in vec3 n;
layout (location = 2)  in vec2 uv;

out vec2 tc;   // Texture Coordinate
out vec3 pw;   // World position
out vec3 nw;   // surface normal world space

void main()
{
  tc = uv;
  pw = (Model * vec4(v, 1.0)).xyz;
  nw = (NormalModel * vec4(n, 0.0)).xyz;
  
  gl_Position = ProjectionMat * View * Model * vec4(v, 1.0);
}
