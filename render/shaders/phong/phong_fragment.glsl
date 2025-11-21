
#if TRANSPARENT
layout(location = 0) out vec4 AccumTexOut;
layout(location = 1) out vec4 RevealTexOut;
#else
layout(location = 0) out vec4 fragColor;
#endif
uniform vec4 MaterialAmbient;

#if DIFFUSE_TEXTURE
uniform sampler2D DiffuseTexture;
#else
uniform vec4 MaterialDiffuse;
#endif

uniform vec4 MaterialSpecular;
uniform float Shininess;
uniform vec3 LightColor;
uniform vec3 LightDirection;


in vec2 tc;
in vec3 fn;
in vec3 vertPos;

const float irradiPerp = 1.0;

vec3 reflect(vec3 I, vec3 N)
{
  // I is the direction from the light towards the material
  // N is the normal vector at the material point
  // Return is the incoming light reflection direction.
  return I - 2.0 * dot(N, I) * N;
}

// L = LightDir
// V = ViewDir
// N = Surface Normal
// R = Reflection of L around N
//  phongBRDF = DiffuseLambertianBRDF + SpecularLambertianBRDF = ( phongDiffuseCol ) + ( phongSpecularCol * dot(R,V)^shininess * dot (N,L) )
vec3 phongBRDF(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 phongDiffuseCol, vec3 phongSpecularCol, float phongShininess) {
  vec3 color = phongDiffuseCol;
  vec3 reflectDir = reflect(-lightDir, normal);
  float specDot = max(dot(reflectDir, viewDir), 0.0);
  color += pow(specDot, phongShininess) * phongSpecularCol;
  return color;
}

#if TRANSPARENT
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
#endif

void main() 
{
  vec4 AmbientColor = MaterialAmbient;
  vec4 SpecularColor = MaterialSpecular;

  #if DIFFUSE_TEXTURE
  vec4 DiffuseColor = vec4(1);
  vec4 DiffuseSample = texture(DiffuseTexture, tc);
  #else
  vec4 DiffuseColor = MaterialDiffuse;
  vec4 DiffuseSample = vec4(1);
  #endif

  

  vec3 lightDir = normalize(LightDirection);
  vec3 viewDir = normalize(-vertPos);
  vec3 n = normalize(fn);
  vec3 radiance = AmbientColor.rgb;
  
  float irradiance = max(dot(lightDir, n), 0.0) * irradiPerp;
  if(irradiance > 0.0) {
    vec3 brdf = phongBRDF(lightDir, viewDir, n, DiffuseColor.rgb, SpecularColor.rgb, Shininess);
    radiance += brdf * irradiance * LightColor.rgb;
  }

  radiance = pow(radiance, vec3(1.0 / 2.2) ); // gamma correction
  
  vec3 fColor = radiance;
#if TRANSPARENT
  float alpha = AmbientColor.a;
  AccumTexOut  = vec4(fColor.xyz * alpha, alpha) * w8(length(vertPos), alpha);
  RevealTexOut = vec4(alpha);
#else 
  fragColor.rgb = radiance * DiffuseSample.rgb;
  fragColor.a = AmbientColor.a;
#endif
}
