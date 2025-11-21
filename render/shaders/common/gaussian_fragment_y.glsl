#version 330 core

in vec2 uv;
out vec4 color;
uniform sampler2D RenderedTexture;
// This is a 9 texel kernel, see https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// for why it has 3 numbers.
// Short story is:
//   1 Original weights come from the binomial distribution koefficients,
//   2 For this 9-texel kernel we took the 12th degree because the outer 2 coefficients 
//     contribute so little to the final pixel.
//   3 Normalize the coefficients so their sum add up to 1.
//   4 Apply the kernel twice, once in y-direction and once in x-direction
//   5 Utilize the linear interprolation circuits graphic card has and get two
//     texel lookups for the price of one.

// So ->  12 binomial coefficients:              1 12 66 220 495 792 924 792 495 220 66 12 1
//                              Or:              924 +- 792 495 220 66 12 1
// Remove outer two coefficients:                924 +- 792 495 220 66
// Normalize them:                               0.2270270270 +- 0.1945945946 0.1216216216 0.0540540541 0.0162162162
// Scale them due to the linear interprolation:  Weight_l(t_1, t_2) = weigth_d(t_1) +  weigth_d(t_2)
//                                               offset_l(t_1, t_2) = ( offset_d(t_1) * weigth_d(t_1) + offset_d(t_2) * weigth_d(t_2) ) / Weight_l(t_1, t_2);
// Offset_d = [0,1,2,3,4]
// Weight_d = [0.2270270270, 0.1945945946,  0.1216216216, 0.0540540541,  0.0162162162]
// Weight_l = [0.2270270270, 0.1945945946 + 0.1216216216, 0.0540540541 + 0.0162162162] = [0.2270270270, 0.3162162162, 0.0702702703]
// Offset_l = [0, (1*0.1945945946 + 2*0.1216216216) /(0.1945945946 + 0.1216216216), (3*0.0540540541 + 4*0.0162162162)/(0.0540540541 + 0.0162162162)]
//          = [0.0, 1.3846153846, 3.2307692308]

#define MAX_KERNEL_SIZE 128
uniform vec2 sideSize;
uniform int kernerlSize;
uniform float offset[MAX_KERNEL_SIZE];// = float[](0.0, 1.3846153846, 3.2307692308);
uniform float weight[MAX_KERNEL_SIZE];// = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main()
{
  vec2 side = vec2(gl_FragCoord.x / sideSize.x, gl_FragCoord.y / sideSize.y);
  vec4 OutColor = texture(RenderedTexture, uv) * weight[0];
  for(int i = 1; i<kernerlSize; i++)
  {
    vec2 off = vec2(0, offset[i]/sideSize.y);
    OutColor += texture(RenderedTexture, uv + off) * weight[i];
    OutColor += texture(RenderedTexture, uv - off) * weight[i];
  }
  color = vec4(OutColor.xyz,1);
}
