#include "core/platform/platform.h"
#include "core/filesystem.h"
#include <sys/stat.h>
#include <windows.h>
#include <shlobj.h>

namespace ic
{

struct PlatformDirIterator
{
        HANDLE handle;
        WIN32_FIND_DATAA find_data;
        bool first;
};

char* __platformCalcBaseDir()
{
        char* buffer  = NULL;
        DWORD bufSize = FS_MAX_PATH;
        DWORD length;

        /* First attempt with MAX_PATH */
        buffer = (char*)ic_malloc(bufSize);  // TODO: need allocator here
        if (!buffer)
                return NULL;
        /** TODO: the while loop bs */
        length = GetModuleFileNameA(NULL, buffer, FS_MAX_PATH);
        if (length == 0)
        {
                ic_free(buffer);
                DWORD err = GetLastError();
                if (err != 0L)
                {
                        IC_CORE_ERROR("WinAPI Error code {}", err);
                        return nullptr;
                }
        }

        /* If buffer was too small, length will be >= bufSize */
        while (length >= bufSize - 1)
        {
                // TODO: check how to copy memory and check how to fix this.
                bufSize         *= 2;                                /* double the buffer size */
                char* newBuffer  = (char*)realloc(buffer, bufSize);  // need the allocator here
                if (!newBuffer)
                {
                        ic_free(buffer);
                        return NULL;
                }
                buffer = newBuffer;
                length = GetModuleFileNameA(NULL, buffer, bufSize);
        }

        char* lastSlash = strrchr(buffer, '\\');
        if (lastSlash)
        {
                *(lastSlash + 1) = '\0'; /* truncate after the slash */
        }
        else
        {
                /* Very unlikely - no directory separator */
                buffer[0] = '\0';
        }

        return buffer;  // caller must free this
}

char* __platformCalcUserDir()
{
        PWSTR widePath = NULL;
        HRESULT hr     = SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &widePath);

        if (SUCCEEDED(hr))
        {
                /* Convert wide string to multibyte (ANSI) */
                int required = WideCharToMultiByte(CP_ACP, 0, widePath, -1, NULL, 0, NULL, NULL);
                if (required > 0)
                {
                        char* path = (char*)ic_malloc(required + 1); /* +1 for extra '\' */
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
        const char* env = getenv("USERPROFILE");
        if (env)
        {
                size_t len = strlen(env);
                char* path = (char*)ic_malloc(len + 2);
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

bool __platformMkDir(const char* path)
{
        // check for thread safe functions
        return CreateDirectoryA(path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool __platformRmDir(const char* dirName)
{
        return RemoveDirectoryA(dirName) != 0;
}

bool __platformFileExists(const char* fullpath)
{
        DWORD dwAttrib = GetFileAttributes((LPCSTR)fullpath);
        // check if a valid path and check if its not a directory
        return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool __platformIsDirectory(const char* path)
{
        DWORD attrs = GetFileAttributesA(path);
        return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool __platformDeleteFile(const char* filename)
{
        return DeleteFileA(filename) != 0;
}

uint64_t __platformGetLastModTime(const char* filename)
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

bool __platformCopyFile(const char* src, const char* dst)
{
        return CopyFile(src, dst, FALSE) != 0;
}

bool __platformMoveFile(const char* src, const char* dst)
{
        return MoveFile(src, dst) != 0;
}

PlatformDirIterator* __platformOpenDir(const char* path)
{
        PlatformDirIterator* iter = new PlatformDirIterator();

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

bool __platformReadDir(PlatformDirIterator* iter, char* out_name, size_t name_size, bool* out_is_dir)
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

void __platformCloseDir(PlatformDirIterator* iter)
{
        if (iter)
        {
                if (iter->handle != INVALID_HANDLE_VALUE)
                        FindClose(iter->handle);
                delete iter;
        }
}

}  // namespace ic