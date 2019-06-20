#ifndef _H_GDCRC32
#define _H_GDCRC32

#ifdef __cplusplus
extern "C" {
#endif

unsigned long __stdcall crc32(const char* buffer, int bufSize, unsigned long seed);
#ifdef __cplusplus
}
#endif

#endif // #ifndef _H_GDCRC32
