#pragma once

struct skybox_edge;
struct skybox_plane;

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
skybox_point_list SkyboxPointList(v3 Point, skybox_plane* Plane)
{
  skybox_point_list Result = {};
  Result.Point = Point;
  Result.Plane = Plane;
  return Result;
}
skybox_point_list SkyboxPointList(v3 Point, skybox_edge* Edge)
{
  skybox_point_list Result = {};
  Result.Point = Point;
  Result.Edge = Edge;
  return Result;
}

struct skybox_plane
{
  v3 Point;
  v3 Normal;
  skybox_point_list* PointsOnPlane;
  v3 P[4];
};

skybox_plane SkyboxPlane(v3 Point, v3 Normal)
{
  skybox_plane Result = {};
  Result.Point = Point;
  Result.Normal = Normal;
  Result.PointsOnPlane = PushStruct(GlobalTransientArena, skybox_point_list);
  ListInitiate(Result.PointsOnPlane);
  return Result;
}

skybox_plane SkyboxPlane(v3 P0, v3 P1, v3 P2, v3 P3)
{
  skybox_plane Result = {};
  Result.P[0] = P0;
  Result.P[1] = P1;
  Result.P[2] = P2;
  Result.P[3] = P3;
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
    v3 ProjectedPoint = PorojectRayOntoPlane(PlaneNormal, PointOnPlane, TrianglePointOrigin, V3(0,0,0));
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

b32 GetIntersectionPoint( v3 TriangleLineOrigin, v3 TriangleLineEnd, v3 SkyboxLineOrigin, v3 SkyboxLineEnd, v3 ForwardDirection, v3* IntersectionPoint)
{
  skybox_point_list* Result;
  v3 ProjectionOrigin = V3(0,0,0);
  v3 SkyboxEdge = Normalize(SkyboxLineEnd - SkyboxLineOrigin);
  v3 EdgeMidPoint = (SkyboxLineOrigin+SkyboxLineEnd) * 0.5f;
  v3 PerpendicularLine = CrossProduct(SkyboxEdge, Normalize(ForwardDirection));
  // Note PlaneNormal is perpendicular to the SkyboxEdge and aligned with ForwardDirection
  v3 PlaneNormal = CrossProduct(PerpendicularLine, SkyboxEdge);
  v3 TriangleLineOriginProjection = PorojectRayOntoPlane( PlaneNormal, EdgeMidPoint, TriangleLineOrigin, ProjectionOrigin);
  v3 TriangleLineEndProjection = PorojectRayOntoPlane( PlaneNormal, EdgeMidPoint, TriangleLineEnd,    ProjectionOrigin);
  
  b32 TriangleEdgeIntersection = LineLineIntersection(
    TriangleLineOriginProjection,
    TriangleLineEndProjection,
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
  v3 TriangleCenter = (ProjectedPoint0 + ProjectedPoint1 + ProjectedPoint2) /3.f;
  r32 PointInFront = TriangleCenter * EdgePoint;
  r32 PointInRange = PointInFront > 0 && PointInRange0 >= 0 && PointInRange1 >= 0 && PointInRange2 >= 0;
  return PointInRange;
}

void AddCornerPoints(skybox_plane* Plane, v3* TrianglePoints, u32 SkyboxLineCount)
{
  skybox_point_list* ElementToAdd = 0;
  v3 CornerPoint = {};
  v3 ProjectionNormal = CrossProduct(TrianglePoints[1] - TrianglePoints[0], TrianglePoints[2] - TrianglePoints[0]); 
  for(int SkyboxLineIndex = 0; SkyboxLineIndex < SkyboxLineCount; SkyboxLineIndex++)
  {
    v3 LineOrigin = Plane->P[SkyboxLineIndex % SkyboxLineCount];

    if(PointInTrinagle(LineOrigin, Normalize(ProjectionNormal), TrianglePoints))
    {
      r32 Distance = R32Max;
      CornerPoint = LineOrigin;
      for(skybox_point_list* Element = Plane->PointsOnPlane->Next; Element != Plane->PointsOnPlane; Element = Element->Next)
      {
        r32 NewDistance = Norm(Element->Point - LineOrigin);
        if(NewDistance < Distance)
        {
          Distance = NewDistance;
          ElementToAdd = Element;
        }
      }
    }
  }

  if(ElementToAdd)
  {
    skybox_point_list* NextElement = ElementToAdd->Next != Plane->PointsOnPlane ? ElementToAdd->Next : ElementToAdd->Next->Next;
    skybox_point_list* PreviousElement = ElementToAdd->Previous != Plane->PointsOnPlane ? ElementToAdd->Previous : ElementToAdd->Previous->Previous;

    r32 DotAfter  = Normalize(ElementToAdd->Point - CornerPoint) * Normalize(NextElement->Point - CornerPoint);
    r32 DotBefore = Normalize(ElementToAdd->Point - CornerPoint) * Normalize(PreviousElement->Point - CornerPoint);

    skybox_point_list* Element = SkyboxPointList(CornerPoint);
    if(DotAfter < DotBefore)
    {
      ListInsertAfter(ElementToAdd, Element);
    }else{
      ListInsertBefore(ElementToAdd, Element);
    }
  }
}