#pragma once

#include "renderer/render_push_buffer.h"
#include "renderer/application_opengl.h"
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
};

extern debug_application_render_commands* GlobalDebugRenderCommands;

u32 CreatePhongNoTexProgram(open_gl* OpenGL)
{
  u32 ProgramHandle = GlNewProgram(OpenGL,
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongVertexCameraViewNoTex.glsl"),
      1, LoadFileFromDisk("..\\jwin\\shaders\\PhongFragmentCameraViewNoTex.glsl"),
      "PhongShadingNoTex");

  GlDeclareUniform(OpenGL, ProgramHandle, "ProjectionMat", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "ModelView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "NormalView", GlUniformType::M4);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightDirection", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "LightColor", GlUniformType::V3);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialAmbient", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialDiffuse", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "MaterialSpecular", GlUniformType::V4);
  GlDeclareUniform(OpenGL, ProgramHandle, "Shininess", GlUniformType::R32);
  return ProgramHandle;
}

debug_application_render_commands DebugApplicationRenderCommands(application_render_commands* RenderCommands, camera* Camera)
{
  debug_application_render_commands Result = {};
  open_gl* OpenGL = &RenderCommands->OpenGL;
  Result.PhongProgramNoTex = CreatePhongNoTexProgram(OpenGL);
  Result.RenderCommands = RenderCommands;
  Result.Camera = Camera;
  Result.LightDirection = V3(1,2,1);

  obj_loaded_file* sphere = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\sphere.obj");
  Result.Sphere = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, sphere));

  obj_loaded_file* cylinder = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cylinder.obj");
  Result.Cylinder = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cylinder));
  
  obj_loaded_file* cone = ReadOBJFile(GlobalPersistentArena, GlobalTransientArena, "..\\data\\cone.obj");
  Result.Cone = GlLoadMesh(OpenGL, MapObjToOpenGLMesh(GlobalTransientArena, cone));
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
  Sphere->TextureHandle = U32Max;

  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ModelView"), ModelView);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "NormalView"), NormalView);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Sphere, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Sphere, {true,true});
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
  Vec->TextureHandle = U32Max;

  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Vec, {true,true});

}

void DrawVector(v3 From, v3 Direction, v3 Color, r32 scale)
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
  Vec->TextureHandle = U32Max;

  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ModelView"), ModelViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "NormalView"), NormalViewVec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(Vec, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(Vec, {true, true});

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
  VecTop->TextureHandle = U32Max;

  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ProjectionMat"), P);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "ModelView"), ModelViewVecTop);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "NormalView"), NormalViewVecTop);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightDirection"), LightDirection);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "LightColor"), V3(1,1,1));
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialAmbient"), Amb);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialDiffuse"), Diff);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "MaterialSpecular"),Spec);
  PushUniform(VecTop, GlGetUniformHandle(&RenderCommands->OpenGL, PhongProgramNoTex, "Shininess"), (r32) 20);
  PushRenderState(VecTop, {true, true});
}
