#pragma once

#include "renderer/render_push_buffer/application_render_push_buffer.h"
#include "platform/obj_loader.h"
#include "utils.h"

struct debug_application_render_commands
{
  application_render_commands* RenderCommands;
  camera* Camera;
  v3 LightDirection;

  u32 PhongProgramNoTex;
  u32 Sphere;
  u32 Cylinder;
  u32 Cone;
  u32 MsaaFrameBuffer;
  u32 DefaultFrameBuffer;
};

extern debug_application_render_commands* GlobalDebugRenderCommands;

u32 CreatePhongNoTexProgram(render_group* RenderGroup)
{
  u32 ProgramHandle = NewShaderProgram(RenderGroup,
      "PhongShadingNoTex");

  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ProjectionMat");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "ModelView");
  AddUniform(RenderGroup, UniformType::M4, ProgramHandle, "NormalView");
  AddUniform(RenderGroup, UniformType::V3, ProgramHandle, "LightDirection");
  AddUniform(RenderGroup, UniformType::V3, ProgramHandle, "LightColor");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialAmbient");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialDiffuse");
  AddUniform(RenderGroup, UniformType::V4, ProgramHandle, "MaterialSpecular");
  AddUniform(RenderGroup, UniformType::R32, ProgramHandle, "Shininess");
  CompileShader(RenderGroup, ProgramHandle,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"));

  return ProgramHandle;
}

debug_application_render_commands DebugApplicationRenderCommands(application_render_commands* RenderCommands, camera* Camera)
{
  debug_application_render_commands Result = {};
  render_group* RenderGroup = RenderCommands->RenderGroup;
  Result.PhongProgramNoTex = CreatePhongNoTexProgram(RenderGroup);
  Result.RenderCommands = RenderCommands;
  Result.Camera = Camera;
  Result.LightDirection = V3(1,2,1);

  obj_loaded_file* sphere = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\sphere.obj");
  Result.Sphere = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, sphere));

  obj_loaded_file* cylinder = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cylinder.obj");
  Result.Cylinder = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
  
  obj_loaded_file* cone = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cone.obj");
  Result.Cone = PushNewMesh(RenderGroup, MapObjToOpenGLMesh(GlobalTransientArena, cone));
  return Result;
}


void DrawDebugDot(v3 Pos, v3 Color, r32 scale)
{
  application_render_commands* RenderCommands = GlobalDebugRenderCommands->RenderCommands;
  m4 P = GlobalDebugRenderCommands->Camera->P;
  m4 V = GlobalDebugRenderCommands->Camera->V;
  v3 LightDirection = GlobalDebugRenderCommands->LightDirection;
  u32 PhongProgramNoTex = GlobalDebugRenderCommands->PhongProgramNoTex;

  v4 Amb =  V4(Color.X * 0.5, Color.Y * 0.5, Color.Z * 0.5, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  m4 ModelMat = M4Identity();
  Scale(V4(scale,scale,scale,0),ModelMat);
  Translate(V4(Pos),ModelMat);

  m4 ModelView = V*ModelMat;
  m4 NormalView = V*Transpose(RigidInverse(ModelMat));

  render_object* Sphere = PushNewRenderObject(RenderCommands->RenderGroup);
  Sphere->ProgramHandle = PhongProgramNoTex;
  Sphere->MeshHandle = GlobalDebugRenderCommands->Sphere;
  Sphere->FrameBufferHandle = GlobalDebugRenderCommands->MsaaFrameBuffer;

  render_group* RenderGroup = RenderCommands->RenderGroup;
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ModelView"), ModelView);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "NormalView"), NormalView);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Sphere, GetUniformHandle(RenderGroup, PhongProgramNoTex, "Shininess"), (r32) 20);
}

void DrawDebugLine(v3 LineStart, v3 LineEnd, v3 Color, r32 scale)
{
  application_render_commands* RenderCommands = GlobalDebugRenderCommands->RenderCommands;
  m4 P = GlobalDebugRenderCommands->Camera->P;
  m4 V = GlobalDebugRenderCommands->Camera->V;
  v3 LightDirection = GlobalDebugRenderCommands->LightDirection;
  u32 PhongProgramNoTex = GlobalDebugRenderCommands->PhongProgramNoTex;

  v4 Amb =  V4(Color.X * 0.5, Color.Y * 0.5, Color.Z * 0.5, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  v4 RotQuat1 = GetRotation(V3(0,1,0), Normalize(LineEnd-LineStart));
  m4 ModelMatVec = M4Identity();
  r32 Length = Norm(LineEnd - LineStart);
  Scale(V4(0.1,0.5,0.1,0),ModelMatVec);
  Scale(V4(scale,Length,scale,0),ModelMatVec);
  Translate(V4(0,Length*0.5,0,0),ModelMatVec);

  ModelMatVec = GetRotationMatrix(RotQuat1) * ModelMatVec;
  Translate(V4(LineStart),ModelMatVec);

  m4 ModelViewVec = V*ModelMatVec;
  m4 NormalViewVec = V*Transpose(RigidInverse(ModelMatVec));

  render_object* Vec = PushNewRenderObject(RenderCommands->RenderGroup);
  Vec->ProgramHandle = PhongProgramNoTex;
  Vec->MeshHandle = GlobalDebugRenderCommands->Cylinder;
  Vec->FrameBufferHandle = GlobalDebugRenderCommands->MsaaFrameBuffer;

  render_group* RenderGroup = RenderCommands->RenderGroup;
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "Shininess"), (r32) 20);

}

void DebugDrawVector(v3 From, v3 Direction, v3 Color, r32 scale)
{
  application_render_commands* RenderCommands = GlobalDebugRenderCommands->RenderCommands;
  m4 P = GlobalDebugRenderCommands->Camera->P;
  m4 V = GlobalDebugRenderCommands->Camera->V;
  v3 LightDirection = GlobalDebugRenderCommands->LightDirection;
  u32 PhongProgramNoTex = GlobalDebugRenderCommands->PhongProgramNoTex;
  u32 Cylinder = GlobalDebugRenderCommands->Cylinder;
  u32 Cone = GlobalDebugRenderCommands->Cone;

  //v4 Amb =  V4(Color.X * 0.05, Color.Y * 0.05, Color.Z * 0.05, 1.0);
  //v4 Diff = V4(Color.X * 0.25, Color.Y * 0.25, Color.Z * 0.25, 1.0);
  //v4 Spec = V4(1, 1, 1, 1.0);
  v4 Amb =  V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Diff = V4(Color.X * 1, Color.Y * 1, Color.Z * 1, 1.0);
  v4 Spec = V4(1, 1, 1, 1.0);

  v4 RotQuat1 = GetRotation(V3(0,1,0), Normalize(Direction));
  m4 ModelMatVec = M4Identity();
  Scale(V4(0.1,0.5,0.1,0),ModelMatVec);
  Scale(V4(scale,scale,scale,0),ModelMatVec);
  Translate(V4(0,scale*0.5,0,0),ModelMatVec);

  ModelMatVec = GetRotationMatrix(RotQuat1) * ModelMatVec;
  Translate(V4(From),ModelMatVec);

  m4 ModelViewVec = V*ModelMatVec;
  m4 NormalViewVec = V*Transpose(RigidInverse(ModelMatVec));

  render_object* Vec = PushNewRenderObject(RenderCommands->RenderGroup);
  Vec->ProgramHandle = PhongProgramNoTex;
  Vec->MeshHandle = Cylinder;
  Vec->FrameBufferHandle = GlobalDebugRenderCommands->MsaaFrameBuffer;

  render_group* RenderGroup = RenderCommands->RenderGroup;
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GetUniformHandle(RenderGroup, PhongProgramNoTex, "Shininess"), (r32) 20);

  m4 ModelMatVecTop = M4Identity();
  Scale(V4(0.2,0.2,0.2,0),ModelMatVecTop);
  Scale(V4(scale,scale,scale,0),ModelMatVecTop);
  Translate(V4(0,scale*1,0,0),ModelMatVecTop);


  v4 RotQuat2 = GetRotation(V3(0,1,0), Normalize(Direction));
  ModelMatVecTop = GetRotationMatrix(RotQuat2) * ModelMatVecTop;
  Translate(V4(From), ModelMatVecTop);

  m4 ModelViewVecTop = V*ModelMatVecTop;
  m4 NormalViewVecTop = V*Transpose(RigidInverse(ModelMatVecTop));
  render_object* VecTop = PushNewRenderObject(RenderCommands->RenderGroup);
  VecTop->ProgramHandle = PhongProgramNoTex;
  VecTop->MeshHandle = Cone;
  VecTop->FrameBufferHandle = GlobalDebugRenderCommands->MsaaFrameBuffer;

  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "ModelView"), ModelViewVecTop);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "NormalView"), NormalViewVecTop);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(VecTop, GetUniformHandle(RenderGroup, PhongProgramNoTex, "Shininess"), (r32) 20);
}
