#version 330 core

in vec2 uv;
layout(location = 0) out vec4 color;
uniform sampler2D AccumTex;
uniform sampler2D RevealTex;

void main()
{
  vec4 Accum = texelFetch(AccumTex, ivec2(gl_FragCoord.xy),0);
  float Reveal = texelFetch(RevealTex, ivec2(gl_FragCoord.xy),0 ).r;
  color = vec4(Accum.rgb/clamp(Accum.a, 0.0001, 50000), Reveal);
}
