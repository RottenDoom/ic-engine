#include "core/platform/platform.h"
#include "core/filesystem.h"
#include <sys/stat.h>
#include <windows.h>
#include <shlobj.h>

namespace ic
{

struct PlatformDirIterator
{
        HANDLE           handle;
        WIN32_FIND_DATAA find_data;
        bool             first;
};

char *__platformCalcBaseDir()
{
        char *buffer = (char *)ic_malloc(FS_MAX_PATH);
        if (!buffer)
                return NULL;

        DWORD length = GetModuleFileNameA(NULL, buffer, FS_MAX_PATH);
        if (length == 0 || length >= FS_MAX_PATH)
        {
                DWORD err = GetLastError();
                IC_CORE_ERROR("GetModuleFileNameA failed (Error: {})", err);
                ic_free(buffer);
                return NULL;
        }

        char *lastSlash = strrchr(buffer, '\\');
        if (lastSlash)
                *(lastSlash + 1) = '\0';
        else
                buffer[0] = '\0';

        return buffer;
}

char *__platformCalcUserDir()
{
        PWSTR   widePath = NULL;
        HRESULT hr       = SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &widePath);

        if (SUCCEEDED(hr))
        {
                /* Convert wide string to multibyte (ANSI) */
                int required = WideCharToMultiByte(CP_ACP, 0, widePath, -1, NULL, 0, NULL, NULL);
                if (required > 0)
                {
                        char *path = (char *)ic_malloc(required + 1); /* +1 for extra '\' */
                        if (path)
                        {
                                WideCharToMultiByte(CP_ACP, 0, widePath, -1, path, required, NULL, NULL);
                                /* Append trailing backslash if not already present */
                                if (path[strlen(path) - 1] != '\\')
                                {
                                        strcat(path, "\\");
                                }
                                CoTaskMemFree(widePath);
                                return path;
                        }
                }
                CoTaskMemFree(widePath);
        }

        /* Fallback: use USERPROFILE environment variable */
        const char *env = getenv("USERPROFILE");
        if (env)
        {
                size_t len  = strlen(env);
                char  *path = (char *)ic_malloc(len + 2);
                if (path)
                {
                        strcpy(path, env);
                        if (path[len - 1] != '\\')
                        {
                                strcat(path, "\\");
                        }
                        return path;
                }
        }

        return NULL;
}

char *__platformCalcWriteDir()
{
        char *writePath = (char *)ic_malloc(FS_MAX_PATH);
        // usually settings and bindings go into local dir and save files go into roaming
        // might also need to add some kind of fallback path resolution.
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, writePath)))
        {
                return writePath;
        }
        ic_free(writePath);
        return NULL;
}

bool __platformMkDir(const char *path)
{
        if (!path || !path[0])
                return false;

        char   temp[FS_MAX_PATH];
        size_t len = strlen(path);

        if (len >= sizeof(temp))
                return false;

        // Copy + normalize slashes
        strcpy(temp, path);
        for (char *c = temp; *c; ++c)
        {
                if (*c == '/')
                        *c = '\\';
        }

        // Must be absolute: C:\...
        if (!(isalpha((unsigned char)temp[0]) && temp[1] == ':' && temp[2] == '\\'))
                return false;

        char *p = temp + 3;  // skip "C:\"

        for (; *p; ++p)
        {
                if (*p == '\\')
                {
                        *p = '\0';

                        DWORD attrs = GetFileAttributesA(temp);
                        if (attrs == INVALID_FILE_ATTRIBUTES)
                        {
                                if (!CreateDirectoryA(temp, NULL))
                                {
                                        DWORD err = GetLastError();
                                        if (err != ERROR_ALREADY_EXISTS)
                                        {
                                                IC_CORE_ERROR("Failed to create directory: {} (Error: {})", temp, err);
                                                return false;
                                        }
                                }
                        }
                        else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
                        {
                                IC_CORE_ERROR("Path exists but is not a directory: {}", temp);
                                return false;
                        }

                        *p = '\\';
                }
        }

        // Final directory
        DWORD attrs = GetFileAttributesA(temp);
        if (attrs == INVALID_FILE_ATTRIBUTES)
        {
                if (!CreateDirectoryA(temp, NULL))
                {
                        DWORD err = GetLastError();
                        if (err != ERROR_ALREADY_EXISTS)
                        {
                                IC_CORE_ERROR("Failed to create directory: {} (Error: {})", temp, err);
                                return false;
                        }
                }
        }
        else if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
                IC_CORE_ERROR("Path exists but is not a directory: {}", temp);
                return false;
        }

        return true;
}

bool __platformRmDir(const char *dirName)
{
        return RemoveDirectoryA(dirName) != 0;
}

bool __platformFileExists(const char *fullpath)
{
        DWORD dwAttrib = GetFileAttributesA(fullpath);
        // check if a valid path and check if its not a directory
        return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool __platformIsDirectory(const char *path)
{
        DWORD attrs = GetFileAttributesA(path);
        return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool __platformDeleteFile(const char *filename)
{
        return DeleteFileA(filename) != 0;
}

uint64_t __platformGetLastModTime(const char *filename)
{
        WIN32_FILE_ATTRIBUTE_DATA attrib;
        if (GetFileAttributesExA(filename, GetFileExInfoStandard, &attrib))
        {
                ULARGE_INTEGER time;
                time.LowPart  = attrib.ftLastWriteTime.dwLowDateTime;
                time.HighPart = attrib.ftLastWriteTime.dwHighDateTime;
                return time.QuadPart;
        }
        return 0;
}

bool __platformCopyFile(const char *src, const char *dst)
{
        return CopyFile(src, dst, FALSE) != 0;
}

bool __platformMoveFile(const char *src, const char *dst)
{
        return MoveFile(src, dst) != 0;
}

PlatformDirIterator *__platformOpenDir(const char *path)
{
        PlatformDirIterator *iter = new PlatformDirIterator();

        char search_path[FS_MAX_PATH];
        snprintf(search_path, FS_MAX_PATH, "%s\\*", path);

        iter->handle = FindFirstFileA(path, &iter->find_data);
        iter->first  = true;

        if (iter->handle == INVALID_HANDLE_VALUE)
        {
                delete iter;
                return nullptr;
        }

        return iter;
}

bool __platformReadDir(PlatformDirIterator *iter, char *out_name, size_t name_size, bool *out_is_dir)
{
        if (!iter || iter->handle == INVALID_HANDLE_VALUE)
                return false;

        if (iter->first)
        {
                iter->first = false;

                while (strcmp(iter->find_data.cFileName, ".") == 0 || strcmp(iter->find_data.cFileName, "..") == 0)
                {
                        if (FindNextFileA(iter->handle, &iter->find_data) == 0)
                                return false;
                }

                strncpy(out_name, iter->find_data.cFileName, name_size - 1);
                out_name[name_size - 1] = '\0';

                if (out_is_dir)
                        *out_is_dir = (iter->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                return true;
        }

        if (FindNextFileA(iter->handle, &iter->find_data) == 0)
                return false;

        while (strcmp(iter->find_data.cFileName, ".") == 0 || strcmp(iter->find_data.cFileName, "..") == 0)
        {
                if (FindNextFileA(iter->handle, &iter->find_data) == 0)
                        return false;
        }

        strncpy(out_name, iter->find_data.cFileName, name_size - 1);
        out_name[name_size - 1] = '\0';

        if (out_is_dir)
                *out_is_dir = (iter->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        return true;
}

void __platformCloseDir(PlatformDirIterator *iter)
{
        if (iter)
        {
                if (iter->handle != INVALID_HANDLE_VALUE)
                        FindClose(iter->handle);
                delete iter;
        }
}

const char *__platformGetBaseDir(void)
{
        return __platformCalcBaseDir();
}
const char *__platformGetUserDir(void)
{
        return __platformCalcUserDir();
}
const char *__platformGetCurrentDir(void)
{
        DWORD len = GetCurrentDirectoryA(0, NULL);
        if (len == 0)
                return NULL;

        char *buffer = (char *)malloc(len + 1);
        if (!buffer)
                return NULL;

        if (GetCurrentDirectoryA(len + 1, buffer) == 0)
        {
                free(buffer);
                return NULL;
        }

        return buffer;  // caller owns memory
}

}  // namespace ic