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