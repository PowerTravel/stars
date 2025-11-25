#version 330 core

in vec2 uv;
in vec4 TextColor;
out vec4 color;
uniform sampler2D SDFMap;
uniform float OnEdgeValue;
uniform float PixelDistanceScale;

// Maps x from range [a0,a1] to [b0,b1]
float LinearRemap(float x, float a0, float a1, float b0, float b1){
  float Slope = (b1-b0) / (a1-a0);
  float Result = Slope * (x - a0) + b0;
  return Result;
}
float Lerp(float t, float a, float b) {
  return ( a + t *(b-a) );
}
float Unlerp(float t, float a, float b){
 return ( t - a) / (b - a);
}
void main()
{
  float PixelDistanceScale2 = 0.6;
  float OnEdgeValue2 = 0.4;
  #if 1
  float strokeWeight = 0.3;
  float smoothing = 0.4;
  float data = texture(SDFMap, uv).r;
  float value = LinearRemap(data, OnEdgeValue2, OnEdgeValue2 + PixelDistanceScale2, 0, 1);
  value = smoothstep(-0.5, 0.5, value);
  
  if(value <= 0){
    discard;
  }
  vec4 MainColor = vec4(TextColor.xyz,value*TextColor.w);

  vec4 strokeColor = vec4(0.2,0,0.6,value);
  float u_weight = 0.5;
  float outlineFactor = smoothstep(u_weight - PixelDistanceScale2, u_weight + PixelDistanceScale2, value);
  vec4 tmpColor = mix(strokeColor,  MainColor, outlineFactor);

  color = tmpColor;//vec4(tmpColor,value*TextColor.w);//vec4(TextColor.xyz,value*TextColor.w);//tmpColor;// vec4(TextColor.xyz,value*TextColor.w);
  #else
  float x = Lerp(uv.x, TexCoord.x, TexCoord.z);
  float y = Lerp(uv.y, TexCoord.y, TexCoord.w);
  
  float tint = 0.8;
  float u_pxrange = 0.4;
  float u_fontSize = 4;
  //float smoothing = clamp(2.0 * u_pxrange / u_fontSize, 0.0, 0.5);
  float smoothing = 0.4;//PixelDistanceScale;
  vec3 strokeColor = vec3(1,0,1);
  float u_weight = 0.1;
  float u_alpha = 1.1;
  float strokeWeight = 0.3;
  float dist = texture(SDFMap, vec2(x,y)).r;
  
  // Shadows
  bool hasShadow = false;
  vec3 shadowColor = vec3(0,0,0);
  float shadowAlpha = 0.2;
  float shadowSmoothing = 0.1;
  float shadowOffset = 0.00001;

  float alpha;
  vec3 tmpColor;

  // dirty if statment, will change soon
  if (strokeWeight > 0.0) {
      alpha = smoothstep(strokeWeight - smoothing, strokeWeight + smoothing, dist);
      float outlineFactor = smoothstep(u_weight - smoothing, u_weight + smoothing, dist);
      tmpColor = mix(strokeColor, TextColor.xyz, outlineFactor) * alpha;
  } else {
      alpha = smoothstep(u_weight - smoothing, u_weight + smoothing, dist);
      tmpColor = TextColor.xyz * alpha;
  }
  vec4 text = vec4(tmpColor * tint, alpha) * u_alpha;
  if (hasShadow == false) {
      color = text;
  } else {
      float shadowDist = texture(SDFMap, vec2(x,y) - shadowOffset).r;
      float distAlpha = smoothstep(0.5 - shadowSmoothing, 0.5 + shadowSmoothing, shadowDist);
      vec4 shadow = vec4(shadowColor, shadowAlpha * distAlpha);
      color = mix(shadow, text, text.a);
  }
  #endif
}
