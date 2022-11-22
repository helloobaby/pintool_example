/* ===================================================================== */
/* Imported Headers														 */
/* ===================================================================== */

//#include "Zydis.h"
#include "pin.H"

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <utility>
#include <hash_map>
#include <hash_set>

std::ofstream TraceFile;


struct Module {
  std::string name_;
  ADDRINT start_;
  ADDRINT end_;
};
// 存储模块信息
std::vector<Module> kModuleInfo;
std::vector<std::hash_map<ADDRINT,std::string>> kSymbolInfo;


/* ===================================================================== */
/* Commandline Switches													 */
/* ===================================================================== */

// If you don't specify the the -o in command line, then the default output will be saved in pinitor.txt
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "pinitor.out", "specify trace file name");

// https://software.intel.com/sites/landingpage/pintool/docs/98484/Pin/html/group__IMG.html#ga4a067152140ead3e23279ab2bd6cd723
// 收集映像加载的一些信息
VOID Image(IMG img, VOID *v) {
  kModuleInfo.push_back({IMG_Name(img), IMG_StartAddress(img),
                         IMG_SizeMapped(img) + IMG_StartAddress(img)});

  // 每个pe模块存储符号名和地址
  std::hash_map<ADDRINT,std::string> t;
  
  for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym)) {
    t.insert({SYM_Address(sym),SYM_Name(sym)});

    //TraceFile << "[" << IMG_Name(img) << "] " << SYM_Name(sym) << " " << SYM_Address(sym) << endl;
  }
  kSymbolInfo.push_back(t);
}
/* ===================================================================== */
/* File Operation				                                         */
/* ===================================================================== */

VOID Fini(INT32 code, VOID *v) {
  TraceFile << "Process exit code " << std::hex << code << std::endl;

  TraceFile.close();
}


VOID PrintInsAddress(VOID *ip) {
  ADDRINT addr = (ADDRINT)ip;

  static std::vector<Module>::iterator NtdllModule = nullptr;
  static int NtdllModuleIndex = -1;

  if (!NtdllModule) {
    NtdllModule =
        std::find_if(kModuleInfo.begin(), kModuleInfo.end(), [&](Module &i) {
          NtdllModuleIndex++;
          if (i.name_.find("ntdll.dll") != std::string::npos)
            return true;
          else
            return false;
        });
    // TraceFile << "NtdllModuleIndex " << NtdllModuleIndex << endl;
  };

  if (!NtdllModule) {
    TraceFile << "can find ntdll.dll ..." << std::endl;
    TraceFile.close();
    exit(0);
  }

  // 当前指令在ntdll.dll模块里
  if (addr < kModuleInfo[NtdllModuleIndex].end_ &&
      addr > kModuleInfo[NtdllModuleIndex].start_) {
    auto it = kSymbolInfo[NtdllModuleIndex].find(addr);
    if (it != kSymbolInfo[NtdllModuleIndex].end()) {
      TraceFile << it->second << std::endl; // 将调用API存储在文件中
    }
  }
}
VOID Instruction(INS ins, VOID *v) {
  static bool PrintModule = false;
  if (!PrintModule) {
    for (auto m : kModuleInfo) {
      TraceFile << m.name_ << std::endl;
    }
    PrintModule = true;
  }

  // https://people.cs.vt.edu/~gback/cs6304.spring07/pin2/Doc/Pin/html/group__INS__INST__API.html#g74a956a0acde197043d04f4adcde4626
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintInsAddress, IARG_INST_PTR,
                 IARG_END);
}

// https://stackoverflow.com/questions/23465779/intel-pin-tool-cannot-catch-thrown-exceptions
// 有些时候程序会抛异常
EXCEPT_HANDLING_RESULT ExceptionHandler(THREADID tid, EXCEPTION_INFO *pExceptInfo, PHYSICAL_CONTEXT *pPhysCtxt, VOID *v)
{
    return EHR_UNHANDLED;
}
void EntryPoint(void *) {
}
int main(int argc, char *argv[]) {
  // Initialize pin & symbol manager
  PIN_InitSymbols();

  // Write to a file since cout and cerr maybe closed by the application
  TraceFile.open(KnobOutputFile.Value().c_str());
  TraceFile << std::hex;
  TraceFile.setf(std::ios::showbase);

  TraceFile << "-- init symbol done" << std::endl;
  if (PIN_Init(argc, argv)) {
    TraceFile << "-- pin init failed" << std::endl;
    TraceFile.close();
    exit(0);
  }
  TraceFile << "-- init pin done" << std::endl;

  // Register Image to be called to instrument functions.
  IMG_AddInstrumentFunction(Image, 0);
  PIN_AddFiniFunction(Fini, 0);

  // https://people.cs.vt.edu/~gback/cs6304.spring07/pin2/Doc/Pin/html/group__INS__INST__API.html#g56957614e1271afdd912c485c87fd406
  // 大幅度减慢程序执行速度
  //
  INS_AddInstrumentFunction(Instruction, 0);

  PIN_AddApplicationStartFunction(EntryPoint, 0);

  PIN_AddInternalExceptionHandler(ExceptionHandler, 0);

  // Never returns
  // pin会利用CreateProcess启动那个进程,等那个进程结束之后就会返回
  PIN_StartProgram();

  TraceFile.close();
  return 0;
}

/* ===================================================================== */
/* eof 																     */
/* ===================================================================== */
