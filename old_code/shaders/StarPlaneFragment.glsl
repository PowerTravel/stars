#version 330 core
// PlaneStarFrag
layout(location = 0) out vec4 AccumTexOut;
layout(location = 1) out vec4 RevealTexOut;
in vec3 vertPos;
in vec4 fColor;
in float fRadius;
in float fFadeDist;
in vec3 fCenter;
in float depth;

float w7(float depth, float alpha)
{
  float normDepth = length(depth);

  float denom = 
    (1.f/100000.f) +
    (normDepth/5.f)*(normDepth/5.f) + 
    (normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f);

  return alpha * max(1.f/100.f, min(3000, 10.f / denom));
}

float w8(float depth, float alpha)
{
  float normDepth = length(depth);

  float denom = 
    (1.f/100000.f) +
    (normDepth/10.f)*(normDepth/10.f)*(normDepth/10.f) + 
    (normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f);

  return alpha * max(1.f/100.f, min(3000, 10.f / denom));
}

float w9(float depth, float alpha)
{
  float normDepth = length(depth);

  float denom = 
    (1.f/100000.f) +
    (normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f)*(normDepth/200.f);

  return alpha * max(1.f/100.f, min(3000, 0.03f / denom));
}

float w10(float depth, float alpha)
{
  float normDepth = length(depth);
  float zNear = 0.1f;
  float zFar = 500.f;
  float d = 1.f - (zNear*zFar/depth-zFar) / (zNear - zFar);

  float denom = 
    3000 * d*d*d;

  return alpha * max(1.f/100.f, min(3000, 0.03f / denom));
}

void main() 
{
  vec2 uv = vec2(gl_FragCoord.x /2048, gl_FragCoord.y/ 1456);
  float mDepth = depth;
  vec3 vp = vertPos - fCenter;
  float Dist = sqrt(dot(vp, vp));
  float alpha = fColor.a * smoothstep(fRadius, fRadius-fFadeDist, Dist);
  if(alpha <= 0)
  {
    discard;
  }
  AccumTexOut = vec4(fColor.xyz * alpha, alpha ) * w7(mDepth, alpha);
  RevealTexOut = vec4(alpha);

}
