#version 460

struct VsOutput
{
  vec3 WorldPosition;
  vec3 EyespaceNormal;
};

uniform mat4 ViewProjection;


layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;

out VsOutput vsOutput;

void main()
{
  gl_Position = ViewProjection * vec4(Position, 1);

  vsOutput.WorldPosition = Position;
  vsOutput.EyespaceNormal = Normal;
}