#version 330 core

#define MAX_KERNEL_SIZE 128

in vec2 uv;
out vec4 color;
uniform sampler2D RenderedTexture;
uniform vec2 sideSize;
uniform int kernerlSize;
uniform float offset[MAX_KERNEL_SIZE];// = float[](0.0, 1.3846153846, 3.2307692308);
uniform float weight[MAX_KERNEL_SIZE];// = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main()
{
  vec2 side = vec2(gl_FragCoord.x / sideSize.x, gl_FragCoord.y / sideSize.y);
  vec4 OutColor = texture(RenderedTexture, uv) * weight[0];
  for(int i = 1; i < kernerlSize; i++)
  {
    vec2 off = vec2(offset[i]/sideSize.x, 0);
    OutColor += texture(RenderedTexture, uv + off) * weight[i];
    OutColor += texture(RenderedTexture, uv - off) * weight[i];
  }
  color = vec4(OutColor.xyz,1);
}