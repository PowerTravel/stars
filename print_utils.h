#pragma once 

// Debug Print Utils
namespace dpu {
void Print(m4 M){
  Platform.DEBUGPrint("| % 1.2f % 1.2f % 1.2f % 1.2f |\n", M.E[ 0],M.E[ 1],M.E[ 2],M.E[ 3]);
  Platform.DEBUGPrint("| % 1.2f % 1.2f % 1.2f % 1.2f |\n", M.E[ 4],M.E[ 5],M.E[ 6],M.E[ 7]);
  Platform.DEBUGPrint("| % 1.2f % 1.2f % 1.2f % 1.2f |\n", M.E[ 8],M.E[ 9],M.E[10],M.E[11]);
  Platform.DEBUGPrint("| % 1.2f % 1.2f % 1.2f % 1.2f |\n", M.E[12],M.E[13],M.E[14],M.E[15]);
}
void PrintAsArray(m4 M){
  Platform.DEBUGPrint("[% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f,\n% 1.2f]\n", 
    M.E[ 0],M.E[ 1],M.E[ 2],M.E[ 3],M.E[ 4],M.E[ 5],M.E[ 6],M.E[ 7],M.E[ 8],M.E[ 9],M.E[10],M.E[11],M.E[12],M.E[13],M.E[14],M.E[15]);
}

}// namespace dpu