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

void FindIntersectionPoints(skybox_plane* Plane, sky_vectors SkyVectors)
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