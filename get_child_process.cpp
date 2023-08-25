// https://software.intel.com/sites/landingpage/pintool/docs/98579/Pin/doc/html/index.html#APPDEBUG_WINDOWS
// Instrumenting Child Processes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <hash_map>
#include <hash_set>
#include <iostream>
#include <list>
#include <string>
#include <utility>

#include "pin.H"

BOOL FollowChild(CHILD_PROCESS cProcess, VOID* userData) {
  int argcc;
  char** argvv;
  int ProcessId = CHILD_PROCESS_GetId(cProcess);
  std::cout << std::hex << ProcessId << std::endl;
  CHILD_PROCESS_GetCommandLine(cProcess, &argcc, (const char* const**)&argvv);
  std::cout << argvv[0] << std::endl;
  return TRUE;
}
int main(int argc, char *argv[]) {
  PIN_Init(argc, argv);
  PIN_AddFollowChildProcessFunction(FollowChild, 0);
  PIN_StartProgram();
  return 0;
}
