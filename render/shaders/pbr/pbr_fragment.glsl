// Reflectance Equation: L_0 (P,ω_0) = Integral_Ω [ f_r(P, ω_i, ω_0) * L_i(p, ω_i) * dot(n, ω_i) ] dω_i

// L_i(p, ω_i) = Radiance of some point p and some infinitely small solid angle ω_i (or incoming light direction vector also denoted ω_i)

// Omega (Ω) = the visible hemisphere
// omega (ω) = solid angle. Integral_Ω ω = Ω.

// Radiant Flux (Phi Φ) is the transmitted energy of a light source measured in watts.
// Radiant Intensity (I) measures the amount of Radiant Flux per solid angle (ω) 
//   Thus (I = dΦ / dω).

// Radiance the total observed energy in an area A over the solid angle ω of a light of radiant flux Φ: 
// Radiance equation: L = (d^2Φ) / (dA dω cos(θ)). θ is the angle between the surface normal and the light direction cos(θ) = dot(n,L).

out vec4 fragColor;
in vec2 tc; // Texture Coordinate
in vec3 pw; // Position world space
in vec3 nw; // surface normal world space

uniform vec3 CamPos; // WorldSpace 
uniform vec3 LightPos; // WorldSpace

#if METALLIC_ROUGHNESS_MAP
uniform sampler2D MetallicRoughnessMap;
#else
uniform float Metalness;
uniform float Roughness;
#endif

#if ALBEDO_MAP
uniform sampler2D AlbedoMap;
#else
uniform vec3  Albedo;
#endif
//uniform sampler2D MetalnessMap;
//uniform sampler2D DisplacementMap;
//uniform sampler2D NormalMap;
//uniform sampler2D RoughnessMap;
//uniform sampler2D AmbientOcclusionMap;

#define PI        3.1415926538
#define OneOverPI 0.3183098862

struct BrdfData
{
  // Material properties
  vec3 specularF0;
  vec3 diffuseReflectance;

  // Roughnesses
  float roughness;    //< perceptively linear roughness (artist's input)
  float alpha;        //< linear roughness - often 'alpha' in specular BRDF equations
  float alphaSquared; //< alpha squared - pre-calculated value commonly used in BRDF equations

  // Commonly used terms for BRDF evaluation
  vec3 F; //< Fresnel term

  // Vectors
  vec3 V; //< Direction to viewer (or opposite direction of incident ray)
  vec3 N; //< Shading normal
  vec3 H; //< Half vector (microfacet normal)
  vec3 L; //< Direction to light (or direction of reflecting ray)

  float NdotL;
  float NdotV;

  float LdotH;
  float NdotH;
  float VdotH;

  // True when V/L is backfacing wrt. shading normal N
  bool Vbackfacing;
  bool Lbackfacing;
};


// The three DFG Functions
// 1 D: Normal Distribution Function -> Area of microfacets aligned with Halfway vector
// 2 F: Fresnel funciton -> The fraction of incoming light which gets reflected versus absorbed into material.
//                          1 = All light is reflected, 0 = All light gets absorbed
// 3 G: Geometry funciton -> How much self-shadowing happens between microfacets.


// ---=== 1: Normal Distribution Function ===---
// Estimates the average surface area of microfacets aligned with the halfway vector.

// N = Normal Vector
// H = Halfway Vector between surface normal and Light direction
// Alpha = Parameter related to the roughness of the surface. [0?,1]
// NDF_GGX_TR = Normal Distribution Function GGX by Trowbridge-Reitz
//    It estimates the relative surface area of microfacets exactly alighted with the halfway vector V 
float NDF_GGX_TR(vec3 N, vec3 H, float Alpha)
{
  float Alpha2 = Alpha*Alpha;
  float NDotH = max(dot(N,H), 0.0);

  float DenomIntermediate = NDotH*NDotH*(Alpha2 - 1.0) + 1.0;
  float Denominator  = PI * DenomIntermediate*DenomIntermediate;
  float Result = Alpha2 / Denominator;
  return Result;
}

// For direct lightning roughness relates to Alpha via Alpha = roughness^2
float DistributionGGX_Direct(vec3 N, vec3 H, float roughness) {
  float Alpha = roughness*roughness;
  float Result = NDF_GGX_TR(N,H,Alpha);
  return Result;
}



// ---=== 2: Fresnel Function ===---
// Describes the ratio of light that gets reflected over the light that gets refracted


vec3 F0_Function(vec3 SurfaceColor, float metalness)
{
  vec3 F0_Dielectric = vec3(0.04); // Reflectance of dielectrics when viewed straigt on for R-G-B Triplet. (Wavelength dependant)
  vec3 F0 = mix(F0_Dielectric, SurfaceColor.rgb, metalness);
  return F0;
}


// This is the Schlick approximation:
// H = HalfwayVector
// V = ViewDirection
// F0 is the Fresnel when viewed straight on or "Surface reflection at zero incidence".
// Fresnel approaches 1 for grazing angles of light
vec3 Fresnel_Schlick(float HDotV, vec3 F0){
  float f =  clamp(1 - HDotV, 0.0, 1.0);
  vec3 Result = F0 + (1-F0) * pow(f,5);
  return Result;
}

// The Fresnel equation describes the ratio of light that gets reflected over the light that gets refracted.
vec3 FresnelFunction(vec3 H, vec3 V, vec3 SurfaceColor, float metalness) {
  float HDotV = max(dot(H,V),0.0);
  vec3 F0 = F0_Function(SurfaceColor, metalness);
  vec3 Result = Fresnel_Schlick(HDotV, F0);
  return Result;
}


// ---=== 3: Geometry Function ===---
// Estimates the relative surface area of microfacets being blocked by incoming or outgoing light

// NDotS is the dot between N and S, N = Normal Vector, S = View/Light Vector, (For incoming light S = L, For reflected light S = V)
// k = A paramterer related to alpha related to roughness.
//      Some material-engines relate k to alpha in different ways
//      One Example: k_direct = (alpha + 1)^2 / 8
//      Another:     k_ibl    = alpha^2 / 2   (image-based lighting which involves capturing an omnidirectional representation of real-world light information as an image)
float Geometry_Schlick_GXX(float NDotS, float k)
{
  float Denominator = NDotS * (1-k) + k;
  float Result = NDotS/Denominator;
  return Result;
}

// Combined Gemotetry using Smith method,
float Geometry_Smith_GXX(vec3 N, vec3 L, vec3 V, float k)
{
  float NDotV = max(dot(N,V), 0.0);
  float NDotL = max(dot(N,L), 0.0);
  float ggx1 = Geometry_Schlick_GXX(NDotV, k);
  float ggx2 = Geometry_Schlick_GXX(NDotL, k);
  float Result = ggx1*ggx2;
  return Result;
}

// Direct Lighting
float Geometry_Smith_Direct(vec3 N, vec3 L, vec3 V, float roughness)
{
  float r = roughness+1.0;
  float k = (r*r) / 8.0;
  float Result = Geometry_Smith_GXX(N, L, V, k);
  return Result;
}

// F = Fresnel function
vec3 Cook_Torrance_BRDF(vec3 F, vec3 N, vec3 L, vec3 V, vec3 H, float roughness) {
  
  float NDF = DistributionGGX_Direct(N, H, roughness);
  float G = Geometry_Smith_Direct(N, L, V, roughness);

  vec3 Numerator = NDF * G * F;
  // note 0.0001 is to avoid dividing by 0 if any of the dot products are 0
  float Denominator = 4.0 * max(dot(N,V), 0.0) * max(dot(N,L), 0.0) + 0.0001;
  vec3 Specular = Numerator / Denominator;
  return Specular;
}

vec3 CalcRadiance(vec3 LightColor, vec3 LightPos, vec3 WorldPos)
{
  float Distance    = length(LightPos - WorldPos);
  float Attenuation = 1.0 / (Distance * Distance);
  vec3 Radiance     = LightColor * Attenuation; 
  //return Radiance;
  return LightColor;
}

vec3 LightColor = vec3(1.0);
vec3 lp = vec3(4,4,4);
void main() {
  // All in world space (for now, ive read its better in object-space, well see, start simple)
  vec3 N = normalize(nw);            // Normal of fragment
  vec3 V = normalize(CamPos - pw);   // Direction of camera from Fragment
  
  #if ALBEDO_MAP
  vec3 albedo = texture(AlbedoMap,tc).bgr;
  #else
  vec3 albedo = Albedo;
  #endif
  
  vec3 Lo = vec3(0.0); // Light out
  /// for each Light LightPos:
  {

    vec3 L = normalize(lp - pw); // Direction of light from Fragment
    vec3 H = normalize(V + L);         // Vector halfway between L and V
    vec3 R = L - 2*dot(L,N)*N;  // L reflected around N;

    vec3 Radiance = CalcRadiance(LightColor, lp, pw);

    //float NDotV = max(dot(N,V), 0.0);
    float NDotL = max(dot(N,L), 0.0);
    //float HDotV = max(dot(H,V), 0.0);

    vec3 F = FresnelFunction(H, V, albedo, Metalness);
    vec3 Specular = Cook_Torrance_BRDF(F,N,L,V,H, Roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0-Metalness;

    Lo += (kD * albedo * OneOverPI + Specular) * Radiance * NDotL;
  }

  ///vec4 Displacement = texture(DisplacementMap, tc);
  ///vec3 Normal = texture(NormalMap, tc).rgb;
  ///float Roughness = texture(RoughnessMap,tc).r;

  fragColor = vec4(Lo,1);
}
