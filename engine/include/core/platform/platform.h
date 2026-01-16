#ifndef PLATFORM_H
#define PLATFORM_H

#include "../../defines.h"

#define __PLATFORM_DIR_SEPERATOR__ '\\'  // for windows

namespace ic
{

/** FILESYSTEM */
char* __platformCalcBaseDir();
char* __platformCalcUserDir();
char* __platformCalcWriteDir();

bool __platformMkDir(const char* path);
bool __platformRmDir(const char* dirName);
bool __platformIsDirectory(const char* path);
bool __platformFileExists(const char* filename);

bool __platformDeleteFile(const char* filename);
uint64_t __platformGetLastModTime(const char* filename);
bool __platformCopyFile(const char* src, const char* dst);
bool __platformMoveFile(const char* src, const char* dst);

typedef struct PlatformDirIterator PlatformDirIterator;

PlatformDirIterator* __platformOpenDir(const char* path);
bool __platformReadDir(PlatformDirIterator* iter, char* out_name, size_t name_size, bool* out_is_dir);
void __platformCloseDir(PlatformDirIterator* iter);

const char* __platformGetBaseDir(void);
const char* __platformGetUserDir(void);
const char* __platformGetCurrentDir(void);

}  // namespace ic

#endif