#pragma once

struct skybox_edge;
struct skybox_plane;

enum skybox_side{
  X_MINUS,
  Z_MINUS,
  X_PLUS,
  Y_PLUS,
  Z_PLUS,
  Y_MINUS,
  SIDE_COUNT
};

struct bitmap {
  u32   BPP; // Bits Per Pixel
  u32   Width;
  u32   Height;
  void* Pixels;
};

struct sky_vectors {
  v3 TopLeft;
  skybox_side TopLeftSide;
  v3 TopRight;
  skybox_side TopRightSide;
  v3 BotLeft;
  skybox_side BotLeftSide;
  v3 BotRight;
  skybox_side BotRightSide;
};


struct skybox_edge
{
  v3 A;
  v3 B;
  skybox_plane* RightPlane;
  skybox_plane* LeftPlane;
};

skybox_edge SkyboxEdge(v3 A, v3 B)
{
  skybox_edge Result = {};
  Result.A = A;
  Result.B = B;
  return Result;
}

skybox_edge SkyboxEdge(v3 A, v3 B, skybox_plane* RightPlane, skybox_plane* LeftPlane)
{
  skybox_edge Result = {};
  Result.A = A;
  Result.B = B;
  Result.RightPlane = RightPlane;
  Result.LeftPlane = LeftPlane;
  return Result;
}

struct skybox_point_list
{
  v3 Point;
  skybox_edge* Edge;
  skybox_plane* Plane;
  skybox_point_list* Previous;
  skybox_point_list* Next;
};
skybox_point_list* SkyboxPointList(v3 Point)
{
  skybox_point_list* Result = PushStruct(GlobalTransientArena, skybox_point_list);
  Result->Point = Point;
  return Result;
}

struct skybox_plane
{
  v3 Normal;
  skybox_point_list* PointsOnPlane;
  skybox_point_list* CornerPoint;
  skybox_side Side;
  v3 P[4];
};

skybox_plane SkyboxPlane(v3 P0, v3 P1, v3 P2, v3 P3, skybox_side Side)
{
  skybox_plane Result = {};
  Result.P[0] = P0;
  Result.P[1] = P1;
  Result.P[2] = P2;
  Result.P[3] = P3;
  Result.Normal = GetTriangleNormal(P0, P1, P3);
  Result.Side = Side;
  Result.PointsOnPlane = PushStruct(GlobalTransientArena, skybox_point_list);
  ListInitiate(Result.PointsOnPlane);
  return Result;
}

skybox_point_list* GetPointAtShortestDistanceFrom(v3 PointToMeasureFrom, skybox_point_list* PointList)
{
  skybox_point_list* Result = 0;
  r32 ShortestDistance = R32Max;
  for(skybox_point_list* Element = PointList->Next;
      Element != PointList;
      Element = Element->Next)
  {
    r32 Distance = Norm(Element->Point - PointToMeasureFrom);
    if(Distance < ShortestDistance)
    {
      Result = Element;
      ShortestDistance = Distance;
    }
  }
  return Result;
}

skybox_point_list* SortPointsAlongTriangleEdge(skybox_point_list* PointListToSort, v3 TrianglePointOrigin, v3 PlaneNormal, v3 PointOnPlane)
{
  skybox_point_list* SortedList = PushStruct(GlobalTransientArena, skybox_point_list);
  ListInitiate(SortedList);
  while(PointListToSort->Next != PointListToSort)
  {
    v3 ProjectedPoint = ProjectRayOntoPlane(PlaneNormal, PointOnPlane, TrianglePointOrigin, V3(0,0,0));
    skybox_point_list* ElementToMove = GetPointAtShortestDistanceFrom(ProjectedPoint, PointListToSort);
    ListRemove(ElementToMove);
    ListInsertBefore(SortedList, ElementToMove);
  }
  return SortedList;
}

void AppendPointsTo(skybox_point_list* ListToInsert, skybox_point_list* ListToMove)
{
  while(ListToMove->Next != ListToMove)
  {
    skybox_point_list* ElementToMove = ListToMove->Next;
    ListRemove(ElementToMove);
    ListInsertAfter(ListToInsert->Previous, ElementToMove);
  } 
}

b32 GetIntersectionPoint( v3 LineOrigin, v3 LineEnd, v3 SkyboxLineOrigin, v3 SkyboxLineEnd, v3 ForwardDirection, v3* IntersectionPoint)
{
  skybox_point_list* Result;
  v3 ProjectionOrigin = V3(0,0,0);
  v3 SkyboxEdge = Normalize(SkyboxLineEnd - SkyboxLineOrigin);
  v3 EdgeMidPoint = (SkyboxLineOrigin+SkyboxLineEnd) * 0.5f;
  v3 PerpendicularLine = CrossProduct(SkyboxEdge, Normalize(ForwardDirection));
  // Note PlaneNormal is perpendicular to the SkyboxEdge and aligned with ForwardDirection
  v3 PlaneNormal = CrossProduct(PerpendicularLine, SkyboxEdge);
  v3 LineOriginProjection = ProjectRayOntoPlane( PlaneNormal, EdgeMidPoint, LineOrigin, ProjectionOrigin);
  v3 LineEndProjection = ProjectRayOntoPlane( PlaneNormal, EdgeMidPoint, LineEnd, ProjectionOrigin);
  
  b32 TriangleEdgeIntersection = LineLineIntersection(
    LineOriginProjection,
    LineEndProjection,
    SkyboxLineOrigin,
    SkyboxLineEnd, NULL, IntersectionPoint);

  return TriangleEdgeIntersection;
}

b32 IsLineInFront(v3 LineOrigin, v3 LineEnd, v3 ForwardPoint)
{
  r32 LineOriginCosAngle = ForwardPoint * LineEnd;
  r32 LineEndCosAngle = ForwardPoint * LineOrigin;
  return (LineOriginCosAngle > 0 || LineEndCosAngle >= 0);
}

b32 IntersectionPointIsWithinAngle(r32 ViewAngle, v3 Forward, v3 IntersectionPoint)
{
  r32 IntersectionPointAngle = ACos(Normalize(Forward) * Normalize(IntersectionPoint));
  return IntersectionPointAngle <= ViewAngle;
}

// Projected points need to be CCW
b32 PointInTrinagle(v3 EdgePoint, v3 ProjectionNormal, v3* TrianglePoints)
{
  r32 lambda0 = RayPlaneIntersection( ProjectionNormal, EdgePoint, TrianglePoints[0], V3(0,0,0));
  r32 lambda1 = RayPlaneIntersection( ProjectionNormal, EdgePoint, TrianglePoints[1], V3(0,0,0));
  r32 lambda2 = RayPlaneIntersection( ProjectionNormal, EdgePoint, TrianglePoints[2], V3(0,0,0));
  v3 ProjectedPoint0 = lambda0 * TrianglePoints[0];
  v3 ProjectedPoint1 = lambda1 * TrianglePoints[1];
  v3 ProjectedPoint2 = lambda2 * TrianglePoints[2];
  r32 PointInRange0 = EdgeFunction(ProjectedPoint0, ProjectedPoint1, EdgePoint);
  r32 PointInRange1 = EdgeFunction(ProjectedPoint1, ProjectedPoint2, EdgePoint);
  r32 PointInRange2 = EdgeFunction(ProjectedPoint2, ProjectedPoint0, EdgePoint);
  r32 PointInRange = PointInRange0 >= 0 && PointInRange1 >= 0 && PointInRange2 >= 0;
  return PointInRange;
}

b32 FindCornerPointInTriangle(v3* CornerPoints, v3* TrianglePoints, v3* Result)
{
  u32 SkyboxLineCount = 4;
  v3 ProjectionNormal = GetTriangleNormal(TrianglePoints[0], TrianglePoints[1], TrianglePoints[2]);
  for (u32 SkyboxLineIndex = 0; SkyboxLineIndex < SkyboxLineCount; ++SkyboxLineIndex)
  {
    v3 CornerPoint = CornerPoints[SkyboxLineIndex];
    
    if(PointInTrinagle(CornerPoint, ProjectionNormal, TrianglePoints))
    {
      *Result = CornerPoint;
      return true;
    }
  }

  return false;
}

skybox_point_list* FindClosestPointInList(skybox_point_list* PointListSentinel, v3 CornerPoint)
{
  skybox_point_list* Result = 0;
  r32 Distance = R32Max;
  for(skybox_point_list* Element = PointListSentinel->Next; Element != PointListSentinel; Element = Element->Next)
  {
    r32 NewDistance = Norm(Element->Point - CornerPoint);
    if(NewDistance < Distance)
    {
      Distance = NewDistance;
      Result = Element;
    }
  }

  return Result;
}

void InsertPointBeforeOrAfter(skybox_plane* Plane, skybox_point_list* ClosestPointOnPlane, v3 CornerPoint)
{
  skybox_point_list* NextElement = ClosestPointOnPlane->Next != Plane->PointsOnPlane ? ClosestPointOnPlane->Next : ClosestPointOnPlane->Next->Next;
  skybox_point_list* PreviousElement = ClosestPointOnPlane->Previous != Plane->PointsOnPlane ? ClosestPointOnPlane->Previous : ClosestPointOnPlane->Previous->Previous;

  r32 DotAfter  = Normalize(ClosestPointOnPlane->Point - CornerPoint) * Normalize(NextElement->Point - CornerPoint);
  r32 DotBefore = Normalize(ClosestPointOnPlane->Point - CornerPoint) * Normalize(PreviousElement->Point - CornerPoint);

  skybox_point_list* Element = SkyboxPointList(CornerPoint);
  if(DotAfter < DotBefore)
  {
    ListInsertAfter(ClosestPointOnPlane, Element);
  }else{
    ListInsertBefore(ClosestPointOnPlane, Element);
  }
  Plane->CornerPoint = Element;
}

void AlignTriangleCCW(v3* Triangle, skybox_side* Sides)
{
  r32 Area = EdgeFunction(Triangle[0], Triangle[1], Triangle[2]);
  if(Area < 0)
  {
    v3 TmpPoint = Triangle[1];
    Triangle[1] = Triangle[2];
    Triangle[2] = TmpPoint;

    skybox_side TmpSide = Sides[1];
    Sides[1] = Sides[2];
    Sides[2] = TmpSide;  
  }
}

b32 FindCornerPoint(skybox_plane* Plane, sky_vectors SkyVectors, v3* Result)
{
  v3 BotLeftDstTriangle[] = {SkyVectors.BotLeft, SkyVectors.BotRight, SkyVectors.TopLeft};
  skybox_side BotLeftDstTriangleSide[] = {SkyVectors.BotLeftSide, SkyVectors.BotRightSide, SkyVectors.TopLeftSide};
  AlignTriangleCCW(BotLeftDstTriangle, BotLeftDstTriangleSide);

  v3 TopRightDstTriangle[] = {SkyVectors.TopLeft, SkyVectors.BotRight, SkyVectors.TopRight};
  skybox_side TopRightDstTriangleSide[] = {SkyVectors.TopLeftSide, SkyVectors.BotRightSide, SkyVectors.TopRightSide};
  AlignTriangleCCW(TopRightDstTriangle, TopRightDstTriangleSide);
  b32 CornerPointFound = false;
  CornerPointFound = FindCornerPointInTriangle(Plane->P, TopRightDstTriangle, Result) ? true : FindCornerPointInTriangle(Plane->P, BotLeftDstTriangle, Result);
  return CornerPointFound;
}

void InsertCornerPoint(skybox_plane* Plane, sky_vectors SkyVectors)
{
  v3 CornerPoint = {};
  b32 CornerPointFound = FindCornerPoint(Plane, SkyVectors, &CornerPoint);
  if(CornerPointFound)
  {
    skybox_point_list* ClosestPointOnPlane = FindClosestPointInList(Plane->PointsOnPlane, CornerPoint);
    if(ClosestPointOnPlane)
    {
      // Inserts Point where the angle is most shallow
      InsertPointBeforeOrAfter(Plane, ClosestPointOnPlane, CornerPoint);
    }
  }  
}

void AddEdgeIntersectionPoints(skybox_point_list* PointsSentinel, v3 LineOrigin, v3 LineEnd, v3* SkyboxPlaneCorners, v3 Forward)
{
  // Check intersections for the current Triangle Line against all skybox lines
  u32 SkyboxLineCount = 4;
  r32 ViewAngle = ACos(Normalize(Forward) * Normalize(LineOrigin));
  for(u32 SkyboxLineIndex = 0; SkyboxLineIndex < SkyboxLineCount; SkyboxLineIndex++)
  {
    v3 SkyboxLineOrigin = SkyboxPlaneCorners[SkyboxLineIndex];
    v3 SkyboxLineEnd    = SkyboxPlaneCorners[(SkyboxLineIndex+1)%SkyboxLineCount];
    if(!IsLineInFront(SkyboxLineOrigin, SkyboxLineEnd, Forward))
    {
      continue;
    }
    
    v3 IntersectionPoint = {};
    b32 TriangleEdgeIntersection = GetIntersectionPoint(LineOrigin, LineEnd, SkyboxLineOrigin, SkyboxLineEnd, Forward, &IntersectionPoint);
    if(TriangleEdgeIntersection)
    {
      if(IntersectionPointIsWithinAngle(ViewAngle, Forward, IntersectionPoint))
      {
        skybox_point_list* P = SkyboxPointList(IntersectionPoint);
        ListInsertBefore(PointsSentinel, P);
      }  
    }
  }
}

void InsertPointsAlongBorder(skybox_plane* Plane, sky_vectors SkyVectors)
{
  v3 Square[4] = {SkyVectors.BotLeft, SkyVectors.TopLeft, SkyVectors.TopRight, SkyVectors.BotRight};
  skybox_side SquareSide[4] = {SkyVectors.BotLeftSide, SkyVectors.TopLeftSide, SkyVectors.TopRightSide, SkyVectors.BotRightSide};
  v3 SquareCenter = Normalize((Square[0]+Square[1]+Square[2]+Square[3]) * 0.25f);
  u32 SquarePointCount = ArrayCount(Square);
  for(u32 SquareIndex = 0; SquareIndex < SquarePointCount; SquareIndex++)
  {
    skybox_point_list* SkyboxPointsSentinel = PushStruct(GlobalTransientArena, skybox_point_list);
    ListInitiate(SkyboxPointsSentinel);
    v3 SquareLineOrigin = Square[SquareIndex];
    v3 SquareLineEnd    = Square[(SquareIndex+1) % SquarePointCount];

    if(SquareSide[SquareIndex] == Plane->Side)
    {
      v3 ProjectedPoint = ProjectRayOntoPlane(Plane->Normal, Plane->P[0], SquareLineOrigin, V3(0,0,0));
      skybox_point_list* Point = SkyboxPointList(ProjectedPoint);
      ListInsertBefore( SkyboxPointsSentinel,  Point);
    }
    
    AddEdgeIntersectionPoints(SkyboxPointsSentinel, SquareLineOrigin, SquareLineEnd, Plane->P, SquareCenter);
    SkyboxPointsSentinel = SortPointsAlongTriangleEdge(SkyboxPointsSentinel, SquareLineOrigin, Plane->Normal, Plane->P[0]);
    AppendPointsTo(Plane->PointsOnPlane, SkyboxPointsSentinel);
  }
}

u32 GetTriangleCount(u32 PlaneCount, skybox_plane* Planes)
{
  u32 TriangleCount = 0;
  for (int SkyboxPlaneIndex = 0; SkyboxPlaneIndex < PlaneCount; ++SkyboxPlaneIndex)
  {
    u32 PointCount = 0;
    ListCount( Planes[SkyboxPlaneIndex].PointsOnPlane, skybox_point_list, PointCount );
    if(PointCount >= 3)
    {
      TriangleCount += PointCount - 2;
    }
  }  
  return TriangleCount;
}

u32 TriangulatePlanes(u32 PlaneCount, skybox_plane* Planes, sky_vectors SkyVectors, v3 CamForward, m4 CamToWorld,
  v3** TrianglesResult, 
  v2** TextureCoordinatesResult, 
  skybox_side** SkyboxSidesResult)
{
  u32 TriangleCount = GetTriangleCount(PlaneCount, Planes);

  u32 PointCount = TriangleCount*3;
  v3* Triangles = PushArray(GlobalTransientArena, PointCount, v3);
  *TrianglesResult = Triangles;
  v2* TextureCoordinates = PushArray(GlobalTransientArena, PointCount, v2);
  *TextureCoordinatesResult = TextureCoordinates;
  skybox_side* SkyboxSides = PushArray(GlobalTransientArena, PointCount, skybox_side);
  *SkyboxSidesResult = SkyboxSides;
  u32 PointIndex = 0;
  for (int SkyboxPlaneIndex = 0; SkyboxPlaneIndex < PlaneCount; ++SkyboxPlaneIndex)
  {
    skybox_plane Plane = Planes[SkyboxPlaneIndex];
    skybox_point_list* StartingPoint = Plane.CornerPoint ? Plane.CornerPoint : Plane.PointsOnPlane->Next;
    skybox_point_list* Element = StartingPoint->Next == Plane.PointsOnPlane ? Plane.PointsOnPlane->Next : StartingPoint->Next;
    skybox_point_list* NextElement = Element->Next == Plane.PointsOnPlane ? Plane.PointsOnPlane->Next : Element->Next;
    while(NextElement != StartingPoint)
    {
      Triangles[PointIndex] = StartingPoint->Point;
      SkyboxSides[PointIndex] = Plane.Side;
      PointIndex++;
      Triangles[PointIndex] = Element->Point;
      SkyboxSides[PointIndex] = Plane.Side;
      PointIndex++;
      Triangles[PointIndex] = NextElement->Point;
      SkyboxSides[PointIndex] = Plane.Side;
      PointIndex++;
      Element =  NextElement;
      NextElement = Element->Next == Plane.PointsOnPlane ? Plane.PointsOnPlane->Next : Element->Next;
    }
  }

  v3 C2 = (SkyVectors.BotLeft + SkyVectors.TopLeft + SkyVectors.TopRight + SkyVectors.BotRight)/4.f;
  m4 RotationMat = CamToWorld;

  v3 a = V3(RotationMat * V4(SkyVectors.BotLeft  - C2,1));
  v3 b = V3(RotationMat * V4(SkyVectors.TopLeft  - C2,1));
  v3 c = V3(RotationMat * V4(SkyVectors.TopRight - C2,1));
  v3 d = V3(RotationMat * V4(SkyVectors.BotRight - C2,1));

  r32 Width =  c.X - a.X;
  r32 Height = b.Y - a.Y;

  for (int TriangleIndex = 0; TriangleIndex < TriangleCount; TriangleIndex++)
  {
    v3 t0 = Triangles[3*TriangleIndex];
    v3 t1 = Triangles[3*TriangleIndex+1];
    v3 t2 = Triangles[3*TriangleIndex+2];

    v4 A = V4(ProjectRayOntoPlane(-CamForward, C2, t0, V3(0,0,0)),1);
    v4 B = V4(ProjectRayOntoPlane(-CamForward, C2, t1, V3(0,0,0)),1);
    v4 C = V4(ProjectRayOntoPlane(-CamForward, C2, t2, V3(0,0,0)),1);
    
    {
      v3 AA = V3(RotationMat*(A-V4(C2,0))) + V3(Width/2.f, Height/2.f,0);
      v3 BB = V3(RotationMat*(B-V4(C2,0))) + V3(Width/2.f, Height/2.f,0);
      v3 CC = V3(RotationMat*(C-V4(C2,0))) + V3(Width/2.f, Height/2.f,0);

      DrawDebugDot(AA,  V3(1,0,0), 0.012);
      DrawDebugDot(BB,  V3(0,1,0), 0.012);
      DrawDebugDot(CC,  V3(0,0,1), 0.012);
      DrawDebugLine(AA, BB, V3(1,1,1), 0.012);
      DrawDebugLine(BB, CC, V3(1,1,1), 0.012);
      DrawDebugLine(CC, AA, V3(1,1,1), 0.012);

      if( AA.X < 0 || AA.Y < 0 || AA.X > Width || AA.Y > Height ||
          BB.X < 0 || BB.Y < 0 || BB.X > Width || BB.Y > Height ||
          CC.X < 0 || CC.Y < 0 || CC.X > Width || CC.Y > Height )
      {
        int a = 10;
      }

      AA.X = Clamp( Unlerp(AA.X,0,Width), 0, 1);
      AA.Y = Clamp( Unlerp(AA.Y,0,Height), 0, 1);
      BB.X = Clamp( Unlerp(BB.X,0,Width), 0, 1);
      BB.Y = Clamp( Unlerp(BB.Y,0,Height), 0, 1);
      CC.X = Clamp( Unlerp(CC.X,0,Width), 0, 1);
      CC.Y = Clamp( Unlerp(CC.Y,0,Height), 0, 1);

      if( AA.X < 0 || AA.Y < 0 || AA.X > 1 || AA. Y > 1 ||
          BB.X < 0 || BB.Y < 0 || BB.X > 1 || BB. Y > 1 ||
          CC.X < 0 || CC.Y < 0 || CC.X > 1 || CC. Y > 1 )
      {
        int a = 10;
      }

      TextureCoordinates[3*TriangleIndex  ] = V2(AA);
      TextureCoordinates[3*TriangleIndex+1] = V2(BB);
      TextureCoordinates[3*TriangleIndex+2] = V2(CC);

    }

  }
  return TriangleCount;
}

skybox_side GetSkyboxSide(v3 Direction)
{
  r32 AbsX = Abs(Direction.X);
  r32 AbsY = Abs(Direction.Y);
  r32 AbsZ = Abs(Direction.Z);
  if(AbsX >= AbsY && AbsX >= AbsZ ) 
  {
    return Direction.X > 0 ? skybox_side::X_PLUS : skybox_side::X_MINUS;
  } else if(AbsY >= AbsZ && AbsY > AbsX) {
    return Direction.Y > 0 ? skybox_side::Y_PLUS : skybox_side::Y_MINUS;
  } else if(AbsZ > AbsX && AbsZ > AbsY) {
    return Direction.Z > 0 ? skybox_side::Z_PLUS : skybox_side::Z_MINUS;
  }

  // Not specifically a bug to end up here,
  // Just not knowing when it could happen
  INVALID_CODE_PATH
  return skybox_side::SIDE_COUNT;
}

const char* SkyboxSideToText(int i)
{
  switch(i)
  {
    case skybox_side::X_MINUS: return "X_MINUS"; break;
    case skybox_side::Z_MINUS: return "Z_MINUS"; break;
    case skybox_side::X_PLUS:  return "X_PLUS"; break;
    case skybox_side::Y_PLUS:  return "Y_PLUS"; break;
    case skybox_side::Z_PLUS:  return "Z_PLUS"; break;
    case skybox_side::Y_MINUS: return "Y_MINUS"; break;
    default: return "All Sides"; break;
  }

  return "All Sides";
}

v3 GetSkyNormal(skybox_side SkyboxSide)
{
  switch(SkyboxSide)
  {
    case skybox_side::X_MINUS: return V3(-1, 0, 0); break;
    case skybox_side::Z_MINUS: return V3( 0, 0,-1); break;
    case skybox_side::X_PLUS:  return V3( 1, 0, 0); break;
    case skybox_side::Y_PLUS:  return V3( 0, 1, 0); break;
    case skybox_side::Z_PLUS:  return V3( 0, 0, 1); break;
    case skybox_side::Y_MINUS: return V3( 0,-1, 0); break;
    default: INVALID_CODE_PATH;
  }

  return {};
}

sky_vectors GetSkyVectors(v3 TexForward, v3 TexRight, v3 TexUp, r32 SkyAngle)
{
  r32 TexWidth = Sin(SkyAngle);
  r32 TexHeight = Sin(SkyAngle);
  sky_vectors Result = {};
  Result.TopLeft   = Normalize( TexForward - TexRight * TexWidth + TexUp * TexHeight);
  Result.TopRight  = Normalize( TexForward + TexRight * TexWidth + TexUp * TexHeight);
  Result.BotLeft   = Normalize( TexForward - TexRight * TexWidth - TexUp * TexHeight);
  Result.BotRight  = Normalize( TexForward + TexRight * TexWidth - TexUp * TexHeight);
  Result.TopLeftSide = GetSkyboxSide(Result.TopLeft);
  Result.TopRightSide = GetSkyboxSide(Result.TopRight);
  Result.BotLeftSide = GetSkyboxSide(Result.BotLeft);
  Result.BotRightSide = GetSkyboxSide(Result.BotRight);
  return Result;
}


v2 GetTextureCoordinateFromUnitSphereCoordinate(v3 UnitSphere, skybox_side Side, bitmap* Bitmap)
{
  Assert(Bitmap->Width / 3.f == Bitmap->Height / 2.f);

  u32 TextureStartX = 0;
  u32 TextureStartY = 0;
  u32 TextureSideSize = Bitmap->Height / 2.f;
  v2 PlaneCoordinate = V2(0,0);
  r32 Alpha = 0;

  r32 AbsX = Abs(UnitSphere.X);
  r32 AbsY = Abs(UnitSphere.Y);
  r32 AbsZ = Abs(UnitSphere.Z);
  Assert(AbsX <= 1 && AbsY <= 1 && AbsZ <= 1);

  switch(Side)
  {
    case skybox_side::X_MINUS:
    {
      Alpha = RayPlaneIntersection( V3(-1,0,0), V3(-1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 0;
      TextureStartY = 0;
      PlaneCoordinate.X = -PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::Z_MINUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,0,-1), V3(0,0,-1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      
      TextureStartX = Bitmap->Width/3;
      TextureStartY = 0;
      PlaneCoordinate.X =  PointOnPlane.X;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::X_PLUS:
    {
      TextureStartX = 2*Bitmap->Width / 3.f;
      TextureStartY = 0;
      Alpha = RayPlaneIntersection( V3(1,0,0), V3(1,0,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;
      PlaneCoordinate.X = PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.Y;
    }break;
    case skybox_side::Y_PLUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,1,0), V3(0,1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 0;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X =  PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
    case skybox_side::Z_PLUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,0,1), V3(0,0,1), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = Bitmap->Width/3;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X = -PointOnPlane.Y;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
    case skybox_side::Y_MINUS:
    {
      r32 Alpha = RayPlaneIntersection( V3(0,-1,0), V3(0,-1,0), UnitSphere, V3(0,0,0) );
      v3 PointOnPlane = Alpha * UnitSphere;

      TextureStartX = 2*Bitmap->Width/3.f;
      TextureStartY = Bitmap->Height / 2.f;
      PlaneCoordinate.X = -PointOnPlane.Z;
      PlaneCoordinate.Y = -PointOnPlane.X;
    }break;
  }

  r32 Tx = Clamp(LinearRemap(PlaneCoordinate.X, -1, 1, TextureStartX, TextureStartX+TextureSideSize), TextureStartX, TextureStartX+TextureSideSize-1);
  r32 Ty = Clamp(LinearRemap(PlaneCoordinate.Y, -1, 1, TextureStartY, TextureStartY+TextureSideSize), TextureStartY, TextureStartY+TextureSideSize-1);

  return {Tx, Ty};
}


v2 GetTextureCoordinateFromUnitSphereCoordinate(v3 UnitSphere, bitmap* Bitmap)
{
  return GetTextureCoordinateFromUnitSphereCoordinate(UnitSphere, GetSkyboxSide(UnitSphere), Bitmap);
}


v3 GetSphereCoordinateFromCube(v3 PointOnUnitCube)
{
  // (x*r)^2 + (y*r)^2 + (z*r)^2 = 1
  // x^2*r^2 + y^2*r^2 + z^2*r^2 = 1
  // (x^2 + y^2 + z^2)*r^2 = 1
  // r^2 = 1 / (x^2 + y^2 + z^2)
  // r = sqrt(1/(x^2 + y^2 + z^2))
  r32 r = sqrt(1.f/(PointOnUnitCube.X*PointOnUnitCube.X +
                    PointOnUnitCube.Y*PointOnUnitCube.Y +
                    PointOnUnitCube.Z*PointOnUnitCube.Z));
  return r * PointOnUnitCube;
}

// Read: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
// If Edgefunction returns:
//   > 0 : the point P is to the right side of the line a->b
//   = 0 : the point P is on the line a->b
//   < 0 : the point P is to the left side of the line a->b
// The edgefunction is equivalent to the magnitude of the cross product between the vector b-a and p-a
r32 EdgeFunction( v2& a, v2& b, v2& p )
{
  r32 Result = (a.X-p.X) * (b.Y-a.Y) - (b.X-a.X) * (a.Y-p.Y);
  return(Result);
}

void DrawPixels(v2* DstPixels, v2* SrcPixels, bitmap* DstBitmap, bitmap* SrcBitmap){

  // Bottom left triangle made up by right handed pointing vectors
  r32 Area2Inverse = 1/EdgeFunction(DstPixels[0], DstPixels[1], DstPixels[2]);
  
  v2 DstPixel0 = DstPixels[0];
  v2 DstPixel1 = DstPixels[1];
  v2 DstPixel2 = DstPixels[2];


  v2 SrcPixel0 = SrcPixels[0];
  v2 SrcPixel1 = SrcPixels[1];
  v2 SrcPixel2 = SrcPixels[2];

  if(Area2Inverse < 0)
  {
    DstPixel1 = DstPixels[2];
    DstPixel2 = DstPixels[1];
    SrcPixel1 = SrcPixels[2];
    SrcPixel2 = SrcPixels[1];
    Area2Inverse = 1/EdgeFunction(DstPixel0, DstPixel1, DstPixel2);
  }

  v4 BoundingBox = {};
  BoundingBox.X = Minimum(DstPixel0.X, Minimum(DstPixel1.X, DstPixel2.X)); // Min X
  BoundingBox.Y = Minimum(DstPixel0.Y, Minimum(DstPixel1.Y, DstPixel2.Y)); // Min Y
  BoundingBox.Z = Maximum(DstPixel0.X, Maximum(DstPixel1.X, DstPixel2.X)); // Max X
  BoundingBox.W = Maximum(DstPixel0.Y, Maximum(DstPixel1.Y, DstPixel2.Y)); // Max Y

  for(s32 Y = Round( BoundingBox.Y ); Y < BoundingBox.W; ++Y)
  {
    for(s32 X = Round( BoundingBox.X ); X < BoundingBox.Z; ++X)
    {
      v2 p = V2( (r32) X, (r32) Y);
      r32 PixelInRange0 = EdgeFunction( DstPixel0, DstPixel1, p);
      r32 PixelInRange1 = EdgeFunction( DstPixel1, DstPixel2, p);
      r32 PixelInRange2 = EdgeFunction( DstPixel2, DstPixel0, p);

      if( ( PixelInRange0 >= 0 ) && 
          ( PixelInRange1 >= 0 ) &&
          ( PixelInRange2 >= 0 ) )
      {
        r32 Lambda0 = PixelInRange1 * Area2Inverse;
        r32 Lambda1 = PixelInRange2 * Area2Inverse;
        r32 Lambda2 = PixelInRange0 * Area2Inverse;

        v2 InterpolatedTextureCoord = Lambda0 * SrcPixel0 + Lambda1 * SrcPixel1 + Lambda2 * SrcPixel2;
        u32 SrxPixelX = TruncateReal32ToInt32(Lerp(InterpolatedTextureCoord.X, 0, SrcBitmap->Width-1));
        u32 SrxPixelY = TruncateReal32ToInt32(Lerp(InterpolatedTextureCoord.Y, 0, SrcBitmap->Height-1));
        u32 SrcPixelIndex = SrcBitmap->Width *  SrxPixelY + SrxPixelX;
        Assert(SrcPixelIndex <  SrcBitmap->Width * SrcBitmap->Height);
        u32* SrcPixels = (u32*) SrcBitmap->Pixels;
        u32 TextureValue = SrcPixels[SrcPixelIndex];
        u32* DstPixels = (u32*) DstBitmap->Pixels;
        u32 TextureCoordIndex = Y * DstBitmap->Width + X;
        Assert(TextureCoordIndex < (DstBitmap->Width * DstBitmap->Height));
        DstPixels[TextureCoordIndex] = TextureValue;
      }
    }
  }
}


void BlitToSkybox(camera* Camera, r32 SkyAngle, bitmap SkyboxTexture, bitmap TgaBitmap)
{  
  local_persist v3 CamUp = {};
  local_persist v3 CamRight = {};
  local_persist v3 CamForward = {};
  local_persist m4 V_Inv = {};
  v3 CamPos = GetCameraPosition(Camera);
  if(CamPos == V3(0,0,0) || CamUp == V3(0,0,0))
  {
    GetCameraDirections(Camera, &CamUp, &CamRight, &CamForward);
    V_Inv = GetCamToWorld(Camera);
  }
  v3 TexForward = -CamForward;
  v3 TexRight = CamRight;
  v3 TexUp = CamUp;

  v3 P_xm_ym_zm = V3(-1,-1,-1);
  v3 P_xp_ym_zm = V3( 1,-1,-1);
  v3 P_xm_yp_zm = V3(-1, 1,-1);
  v3 P_xp_yp_zm = V3( 1, 1,-1);
  v3 P_xm_ym_zp = V3(-1,-1, 1);
  v3 P_xp_ym_zp = V3( 1,-1, 1);
  v3 P_xm_yp_zp = V3(-1, 1, 1);
  v3 P_xp_yp_zp = V3( 1, 1, 1);

  skybox_plane SkyboxPlanes[skybox_side::SIDE_COUNT] = {};
  SkyboxPlanes[skybox_side::X_MINUS] = SkyboxPlane(P_xm_ym_zm, P_xm_ym_zp, P_xm_yp_zp, P_xm_yp_zm, skybox_side::X_MINUS);
  SkyboxPlanes[skybox_side::X_PLUS]  = SkyboxPlane(P_xp_ym_zm, P_xp_yp_zm, P_xp_yp_zp, P_xp_ym_zp, skybox_side::X_PLUS);
  SkyboxPlanes[skybox_side::Y_MINUS] = SkyboxPlane(P_xm_ym_zm, P_xp_ym_zm, P_xp_ym_zp, P_xm_ym_zp, skybox_side::Y_MINUS);
  SkyboxPlanes[skybox_side::Y_PLUS]  = SkyboxPlane(P_xm_yp_zm, P_xm_yp_zp, P_xp_yp_zp, P_xp_yp_zm, skybox_side::Y_PLUS);
  SkyboxPlanes[skybox_side::Z_MINUS] = SkyboxPlane(P_xm_ym_zm, P_xm_yp_zm, P_xp_yp_zm, P_xp_ym_zm, skybox_side::Z_MINUS);
  SkyboxPlanes[skybox_side::Z_PLUS]  = SkyboxPlane(P_xm_ym_zp, P_xp_ym_zp, P_xp_yp_zp, P_xm_yp_zp, skybox_side::Z_PLUS);

  sky_vectors SkyVectors = GetSkyVectors(TexForward, TexRight, TexUp, SkyAngle);
  DrawDebugDot(SkyVectors.BotLeft,  V3(1,0,0), 0.022);
  DrawDebugDot(SkyVectors.TopLeft,  V3(0,1,0), 0.022);
  DrawDebugDot(SkyVectors.TopRight, V3(0,0,1), 0.022);
  DrawDebugDot(SkyVectors.BotRight, V3(1,1,1), 0.022);

  for (int SkyboxPlaneIndex = 0; SkyboxPlaneIndex < ArrayCount(SkyboxPlanes); ++SkyboxPlaneIndex)
  {
    skybox_plane* Plane = &SkyboxPlanes[SkyboxPlaneIndex];
    InsertPointsAlongBorder(Plane, SkyVectors);
    InsertCornerPoint(Plane, SkyVectors);
  }

  v3* Triangles = 0;
  v2* TextureCoordinates = 0;
  skybox_side* SkyboxSides = 0;
  u32 TriangleCount = TriangulatePlanes(ArrayCount(SkyboxPlanes), SkyboxPlanes, SkyVectors, -CamForward, V_Inv, &Triangles, &TextureCoordinates, &SkyboxSides);

  u32 PointIndex = 0;
  
  r32 XDist = Norm(SkyVectors.TopRight - SkyVectors.TopLeft);
  r32 YDist = Norm(SkyVectors.TopRight - SkyVectors.BotRight);

  for (int TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
  {
    v3 t0 = Triangles[PointIndex];
    skybox_side s0 = SkyboxSides[PointIndex];
    v3 t1 = Triangles[PointIndex+1];
    skybox_side s1 = SkyboxSides[PointIndex+1];
    v3 t2 = Triangles[PointIndex+2];
    skybox_side s2 = SkyboxSides[PointIndex+2];
    PointIndex+=3;
    v2 DstPixel0 = GetTextureCoordinateFromUnitSphereCoordinate(GetSphereCoordinateFromCube(t0), s0, &SkyboxTexture);
    v2 DstPixel1 = GetTextureCoordinateFromUnitSphereCoordinate(GetSphereCoordinateFromCube(t1), s1, &SkyboxTexture);
    v2 DstPixel2 = GetTextureCoordinateFromUnitSphereCoordinate(GetSphereCoordinateFromCube(t2), s2, &SkyboxTexture);
    

    v2 tu0 = TextureCoordinates[3*TriangleIndex];
    v2 tu1 = TextureCoordinates[3*TriangleIndex+1];
    v2 tu2 = TextureCoordinates[3*TriangleIndex+2];



    v2 SrcPixels[] =  {tu0, tu1, tu2};

    v2 DstPixels[] = {DstPixel0,DstPixel1,DstPixel2};
    DrawPixels(DstPixels, SrcPixels, &SkyboxTexture, &TgaBitmap);
  }
}