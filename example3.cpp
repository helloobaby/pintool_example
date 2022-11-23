/*
 * 利用pin hook NtCreateFile
 * 
 */

#include "pin.H"
#include <iostream>
using std::cerr;
using std::endl;
using std::string;

typedef struct _UNICODE_STRING {
    unsigned short Length;
    unsigned short MaximumLength;
    unsigned short * Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef struct _OBJECT_ATTRIBUTES {
    uint32_t Length;
    void* RootDirectory;
    PUNICODE_STRING ObjectName;
    uint32_t Attributes;
    void* SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    void* SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

// 这个调用约定要注意(PROTO_Allocate有一个参数也是关于调用约定的)
typedef int32_t (__stdcall* ProtoType)(void* FileHandle, uint32_t DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes,
                             void* IoStatusBlock, void* AllocationSize,
                             uint32_t FileAttributes, uint32_t ShareAccess,
                             uint32_t CreateDisposition, uint32_t CreateOptions,
                             void* EaBuffer, uint32_t EaLength);

char* wctoc(unsigned short* str) {
  char* t = (char*)malloc(256);
  size_t s = wcstombs(t,(const wchar_t*)str, 256);
  return t;
}

int32_t NtCreateFileDetour(ProtoType OriPtr, void* FileHandle,
                           uint32_t DesiredAccess,
                           POBJECT_ATTRIBUTES ObjectAttributes,
                           void* IoStatusBlock, void* AllocationSize,
                           uint32_t FileAttributes, uint32_t ShareAccess,
                           uint32_t CreateDisposition, uint32_t CreateOptions,
                           void* EaBuffer, uint32_t EaLength) {
  cerr << FileHandle << endl;
  cerr << DesiredAccess << endl;
  cerr << wctoc(ObjectAttributes->ObjectName->Buffer) << endl;

  return OriPtr(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
                AllocationSize, FileAttributes, ShareAccess, CreateDisposition,
                CreateOptions, EaBuffer, EaLength);
}
//

/* ===================================================================== */
VOID ImageLoad(IMG img, VOID* v) {
  if (!IMG_IsMainExecutable(img)) {
    // 类似GetProcAddress
    RTN rtn = RTN_FindByName(img, "NtCreateFile");

    if (RTN_Valid(rtn) && RTN_IsSafeForProbedReplacement(rtn)) {
      cerr << "Find NtCreateFile "
           << " " << std::hex << RTN_Address(rtn) << " ..." << endl;

      PROTO ProtoAlloc = PROTO_Allocate(
          PIN_PARG(int32_t), CALLINGSTD_STDCALL, "NtCreateFile",
          PIN_PARG(void*), PIN_PARG(uint32_t), PIN_PARG(void*), PIN_PARG(void*),
          PIN_PARG(void*), PIN_PARG(uint32_t), PIN_PARG(uint32_t),
          PIN_PARG(uint32_t), PIN_PARG(uint32_t), PIN_PARG(void*),
          PIN_PARG(uint32_t), PIN_PARG_END());

      RTN_ReplaceSignatureProbed(
          rtn, AFUNPTR(NtCreateFileDetour), 
          IARG_PROTOTYPE, ProtoAlloc,
          IARG_ORIG_FUNCPTR,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 5,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 6,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 7,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 8,
          IARG_FUNCARG_ENTRYPOINT_VALUE, 9,
          IARG_END);

      PROTO_Free(ProtoAlloc);
    }
  }
}


/* ===================================================================== */

int main(int argc, CHAR* argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    IMG_AddInstrumentFunction(ImageLoad, NULL);

    cerr << "PIN Start ..." << endl;

    

    //PIN_StartProgram();
    PIN_StartProgramProbed();

    return 1;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
