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

skybox_point_list* SortPointsAlongTriangleEdge(skybox_point_list* PointListToSort, v3 TrianglePointOrigin, v3 PlaneNormal, v3 PointOnPlane)
{
  skybox_point_list* SortedList = PushStruct(GlobalTransientArena, skybox_point_list);
  ListInitiate(SortedList);
  while(PointListToSort->Next != PointListToSort)
  {
    r32 lambda0 = RayPlaneIntersection( PlaneNormal, PointOnPlane, TrianglePointOrigin, V3(0,0,0));
    v3 ProjectedPoint0 = lambda0 * TrianglePointOrigin;
    skybox_point_list* ElementToAdd = 0;
    r32 Distance = R32Max;
    for(skybox_point_list* Element = PointListToSort->Next; Element != PointListToSort; Element = Element->Next)
    {
      r32 NewDistance = Norm(Element->Point - ProjectedPoint0);
      if(NewDistance < Distance)
      {
        ElementToAdd = Element;
        Distance = NewDistance;
      }
    }
    ListRemove(ElementToAdd);
    ListInsertBefore(SortedList, ElementToAdd);
  }
  return SortedList;
}