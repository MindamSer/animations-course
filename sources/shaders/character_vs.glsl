#version 460

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
  vec3 BoneColor;
};

uniform mat4 ViewProjection;
uniform mat4 SkinningMatrices[255];


layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec4 BoneWeights;
layout(location = 4) in uvec4 BoneIndex;

out VsOutput vsOutput;


vec3 get_random_color(uint x)
{
  x += 1u;
  vec3 col = vec3(1.61803398875);
  col = fract(col) * vec3(x,x,x);
  col = fract(col) * vec3(1,x,x);
  col = fract(col) * vec3(1,1,x);
  //col = vec3(phi*i, phi*i*i, phi*i*i*i); // has precision issues
  return fract(col);
}


void main()
{

  mat4 SkinningTransform =
    SkinningMatrices[BoneIndex.x] * BoneWeights.x +
    SkinningMatrices[BoneIndex.y] * BoneWeights.y +
    SkinningMatrices[BoneIndex.z] * BoneWeights.z +
    SkinningMatrices[BoneIndex.w] * BoneWeights.w;

  vec3 VertexPosition = (SkinningTransform * vec4(Position, 1)).xyz;
  vsOutput.EyespaceNormal = (SkinningTransform * vec4(Normal, 0)).xyz;

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;

  vsOutput.BoneColor =
    get_random_color(BoneIndex.x) * BoneWeights.x +
    get_random_color(BoneIndex.y) * BoneWeights.y +
    get_random_color(BoneIndex.z) * BoneWeights.z +
    get_random_color(BoneIndex.w) * BoneWeights.w;

}