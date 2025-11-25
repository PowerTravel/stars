#version 330 core
out vec4 fragColor;

in vec3 fv;
in vec3 fCenter;
in vec4 fColor;
in float fInnerRadii;
in float fOuterRadii;

void main() 
{
  float len = acos(dot(normalize(fv), normalize(fCenter)));  // Gives Sliiiightly smoother edge
  if(len < fInnerRadii || 
     len > fOuterRadii ) 
  {
    discard;
  }
 fragColor = fColor;
}

