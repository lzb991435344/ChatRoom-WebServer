/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TCAPDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// TCAPDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef TCAPDLL_EXPORTS
#define TCAPDLL_API __declspec(dllexport)
#else
#define TCAPDLL_API __declspec(dllimport)
#endif

// This class is exported from the tcapdll.dll
class TCAPDLL_API Ctcapdll {
public:
	Ctcapdll(void);
	// TODO: add your methods here.
};

extern TCAPDLL_API int ntcapdll;

TCAPDLL_API int fntcapdll(void);
