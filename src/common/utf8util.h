#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//https://stackoverflow.com/questions/36897781/how-to-uppercase-lowercase-utf-8-characters-in-c

char* StrToUprExt(char* pString);
char* StrToLwrExt(char* pString);
int StrnCiCmp(const char* s1, const char* s2, size_t ztCount);
int StrCiCmp(const char* s1, const char* s2);
char* StrCiStr(const char* s1, const char* s2);

#ifdef __cplusplus
}
#endif
