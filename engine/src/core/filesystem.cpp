#include "core/filesystem.h"
#include "core/allocators.h"
#include "core/platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Allocate 2MiB for filesystem operations */
constexpr size_t FS_ALLOCATION_SIZE = 2 * 1024 * 1024;
#define MAX_FILES_ENUMERATED 1000

static ic::FS_Info* g_filesystem = nullptr;

namespace ic
{

struct Mount
{
        char* path;  // relative path
};

struct File
{
        void* handle;
        FSFileType type;
        size_t size;
        uint64_t modified_time;
};

struct FS_Info
{
        bump_allocator_t* allocator;  // usually bump/linear allocators work well with FS
        Mount mounts[FS_MAX_MOUNTS];
        size_t mount_count;
        char* base_dir;
        char* user_dir;
        char* root_dir;
        char** search_paths;  // search paths for the relative path
        char** mount_points;
        size_t search_path_count;
};

static char* calculateBaseDir(void)
{
        // somehow free this
        g_filesystem->base_dir = __platformCalcBaseDir();
        if (g_filesystem->base_dir != nullptr)
        {
                return g_filesystem->base_dir;
        }

        IC_CORE_ERROR("Could not find of application");
        return nullptr;
}
static bool is_valid_path(const char* norm_path)
{
        if (strstr(norm_path, "..") || strstr(norm_path, "/.") || norm_path[0] == '.')
        {
                return false;
        }
        return true;
}

static char* normalize(bump_allocator_t* allocator, const char* path)
{
        size_t len = strlen(path);
        char* res  = (char*)bump_alloc_tagged(allocator, len, sizeof(char), IC_TAG_FILESYSTEM);  // Using generic
        if (!res)
                return NULL;

        char* dst          = res;
        const char* src    = path;
        int last_was_slash = 0;

        while (*src)
        {
                char c = *src++;
                if (c == '\\')
                        c = '/';

                if (c == '/')
                {
                        if (!last_was_slash)
                        {
                                *dst++ = c;
                        }
                        last_was_slash = 1;
                }
                else
                {
                        *dst++         = c;
                        last_was_slash = 0;
                }
        }
        *dst = '\0';

        // Remove trailing / if present (except for root "/")
        len = strlen(res);
        if (len > 1 && res[len - 1] == '/')
        {
                res[len - 1] = '\0';
        }

        return res;
}

static char* join_path(bump_allocator_t* allocator, const char* a, const char* b)
{
        size_t len_a = strlen(a);
        size_t len_b = strlen(b);
        size_t extra = (len_a > 0 && a[len_a - 1] != '/') ? 1 : 0;
        char* res    = (char*)malloc(len_a + extra + len_b + 1);  // TODO:
        if (!res)
                return NULL;

        strcpy(res, a);
        if (extra)
                strcat(res, "/");
        strcat(res, b);
        return res;
}

bool fs_init(void)
{
        // use global filesystem to control everything
        if (g_filesystem)
        {
                IC_CORE_WARN("Filesystem already exists");
                return false;
        }

        g_filesystem = (FS_Info*)malloc(sizeof(FS_Info));
        memset(g_filesystem, 0, sizeof(FS_Info));

        // Initialize allocation memory for the filesystem
        void* fs_memory         = malloc(FS_ALLOCATION_SIZE);
        g_filesystem->allocator = (bump_allocator_t*)malloc(sizeof(bump_allocator_t));
        bump_allocator_init(g_filesystem->allocator, fs_memory, FS_ALLOCATION_SIZE);

        // Initialize the home and user directories
        char* home             = normalize(g_filesystem->allocator, __platformCalcBaseDir());
        char* user             = normalize(g_filesystem->allocator, __platformCalcUserDir());
        size_t home_len        = strlen(home) + 1;
        size_t user_len        = strlen(user) + 1;

        g_filesystem->base_dir = (char*)bump_alloc_tagged(g_filesystem->allocator, home_len, 1, IC_TAG_FILESYSTEM);
        g_filesystem->user_dir = (char*)bump_alloc_tagged(g_filesystem->allocator, user_len, 1, IC_TAG_FILESYSTEM);

        memcpy(g_filesystem->base_dir, home, home_len);
        memcpy(g_filesystem->user_dir, user, user_len);

        // Initialize the search paths
        g_filesystem->search_path_count = 0;
        g_filesystem->search_paths      = (char**)bump_alloc_tagged(g_filesystem->allocator,
                                                               sizeof(char*) * FS_MAX_MOUNTS,
                                                               alignof(char*),
                                                               IC_TAG_FILESYSTEM);

        // Add user directory to search paths
        fs_addToSearchPath(g_filesystem->base_dir, false);

        IC_CORE_INFO("Initialized filesystem with {} MB allocator", FS_ALLOCATION_SIZE / (1024 * 1024));
        IC_CORE_INFO("Base directory: {}", g_filesystem->base_dir);
        IC_CORE_INFO("User directory: {}", g_filesystem->user_dir);

        return true;
}

void fs_deinit(void)
{
        if (!g_filesystem)
                return;
#if defined(_DEBUG)
        dump_allocations(g_filesystem->allocator);
#endif

        bump_allocator_clear(g_filesystem->allocator);

        free(g_filesystem->allocator->memory);
        free(g_filesystem->allocator);
        free(g_filesystem);

        g_filesystem = nullptr;
}

bool fs_mount(const char* path, const char* mountPoint)
{
        if (!path || !mountPoint)
                return false;
        if (g_filesystem->mount_count >= FS_MAX_MOUNTS)
        {
                IC_CORE_WARN("Cannot Mount more Directories!");
                return false;
        }

        Mount* mnt            = &g_filesystem->mounts[g_filesystem->mount_count++];

        bump_mark_t temp_mark = bump_mark_push(g_filesystem->allocator);

        /** Crate a new normalized string */
        char* norm_path = normalize(g_filesystem->allocator, path);
        if (!norm_path)
        {
                bump_mark_pop(g_filesystem->allocator, temp_mark);
                IC_CORE_ERROR("Failed to normalize path!");
                return false;
        }

        // Validate path format
        if (norm_path[0] != '/' || !is_valid_path(norm_path + 1))
        {
                bump_mark_pop(g_filesystem->allocator, temp_mark);
                IC_CORE_ERROR("Invalid mount path: {}", path);
                return false;
        }

        char* sub_path = norm_path + 1;

        // Normalize the physical mount point (temporary)
        char* norm_base = normalize(g_filesystem->allocator, mountPoint);
        if (!norm_base)
        {
                bump_mark_pop(g_filesystem->allocator, temp_mark);
                IC_CORE_ERROR("Failed to normalize mount point: {}", mountPoint);
                return false;
        }

        // Join paths (temporary)
        char* full_path = join_path(g_filesystem->allocator, norm_base, sub_path);
        if (!full_path)
        {
                bump_mark_pop(g_filesystem->allocator, temp_mark);
                IC_CORE_ERROR("Failed to join paths");
                return false;
        }

        // Now allocate permanent copies AFTER we know everything is valid
        // Pop the work mark first
        bump_mark_pop(g_filesystem->allocator, temp_mark);

        // Allocate permanent mount path
        size_t path_len = strlen(norm_path) + 1;
        mnt->path       = (char*)bump_alloc_tagged(g_filesystem->allocator, path_len, 1, IC_TAG_FILESYSTEM);

        if (!mnt->path)
        {
                IC_CORE_ERROR("Failed to allocate permanent mount path");
                return false;
        }

        strcpy(mnt->path, norm_path);

        /** Will this assert go here? */
        IC_CORE_ASSERT(fs_addToSearchPath(full_path, true), "Failed to add to the search paths");

        g_filesystem->mount_count++;
        IC_CORE_INFO("Mounted directory {} to search path {}.", mnt->path, mountPoint);

        return true;
}

char** fs_getSearchPath(void)
{
        g_filesystem->search_paths;
}

const char* fs_getDirSeperator(void)
{
        /** TODO: check for platform and return the dir seperator for the platfrom */
        char retval[2] = {__PLATFORM_DIR_SEPERATOR__, '\0'};
        return retval;
}

const char* fs_getWriteDirectory(void)
{
        return g_filesystem->root_dir; /** TODO set and get write dir */
}

void fs_setWriteDirectory(const char* dir) {}

bool fs_addToSearchPath(const char* newDir, bool appendToPath)
{
        if (!g_filesystem || !newDir)
                return false;

        if (g_filesystem->search_path_count >= FS_MAX_MOUNTS)
                return false;

        newDir          = normalize(g_filesystem->allocator, newDir);

        size_t dirlen   = strlen(newDir) + 1;
        char* searchDir = (char*)bump_alloc_tagged(g_filesystem->allocator, dirlen, 1, IC_TAG_FILESYSTEM);

        if (!searchDir)
                return false;

        strcpy(searchDir, newDir);

        if (appendToPath)
        {
                // append the path to the end
                g_filesystem->search_paths[g_filesystem->search_path_count] = searchDir;
        }
        else
        {
                // add to the start of the array
                for (size_t i = g_filesystem->search_path_count; i > 0; i--)
                {
                        g_filesystem->search_paths[i] = g_filesystem->search_paths[i - 1];
                }
                g_filesystem->search_paths[0] = searchDir;
        }

        g_filesystem->search_path_count++;

        IC_CORE_INFO("Added '{}' to the search path", newDir);
        return true;
}

bool fs_removeFromSearchPath(const char* rmDir)
{
        if (!g_filesystem || !rmDir)
                return false;

        // match the strings if matches then remove and rebuild the array
        for (size_t i = 0; i < g_filesystem->search_path_count; i++)
        {
                if (strcmp(g_filesystem->search_paths[i], rmDir) == 0)
                {
                        for (size_t j = 0; j < g_filesystem->search_path_count - 1; j++)
                        {
                                g_filesystem->search_paths[j] = g_filesystem->search_paths[j + 1];
                        }

                        g_filesystem->search_path_count--;
                        IC_CORE_INFO("Removed {} from the search paths", rmDir);
                        return true;
                }
        }
        return false;
}

char* fs_getfullpath(const char* filename)
{
        for (size_t i = 0; i < g_filesystem->mount_count; i++)
        {
                char* full = join_path(g_filesystem->allocator, g_filesystem->search_paths[i], filename);
                if (__platformFileExists(full))
                {
                        return full;
                }
                free(full);
        }
        return nullptr;
}

bool fs_mkdir(const char* dirName)
{
        return __platformMkDir(dirName);
}

bool fs_rmdir(const char* dirName)
{
        return __platformRmDir(dirName);
}

bool fs_delete(const char* filename)
{
        return __platformDeleteFile(filename);
}

char** fs_enumerateFiles(const char* dir)
{
        if (!dir)
                return nullptr;

        // allocator array for enumeration
        char** file_list = (char**)bump_alloc_tagged(g_filesystem->allocator,
                                                     sizeof(char*) * (MAX_FILES_ENUMERATED + 1),
                                                     alignof(char*),
                                                     IC_TAG_FILESYSTEM);

        if (!file_list)
                return nullptr;
        size_t file_cnt           = 0;

        PlatformDirIterator* iter = __platformOpenDir(dir);
        if (!iter)
        {
                file_list[0] = nullptr;
                return nullptr;
        }

        char entry_name[FS_MAX_PATH];
        bool is_dir;

        while (__platformReadDir(iter, entry_name, sizeof(entry_name), &is_dir) && file_cnt < MAX_FILES_ENUMERATED)
        {
                size_t name_len = strlen(entry_name) + 1;
                char* filename  = (char*)bump_alloc_tagged(g_filesystem->allocator, name_len, 1, IC_TAG_FILESYSTEM);

                if (filename)
                {
                        strcpy(filename, entry_name);
                        file_list[file_cnt++] = filename;
                }
        }
        __platformCloseDir(iter);
        file_list[file_cnt] = nullptr;

        IC_CORE_INFO("Directory '{}': ", dir);
        for (size_t i = 0; i < file_cnt; i++)
        {
                IC_CORE_INFO("- {}", file_list[i]);
        }

        return file_list;
}

/** test which one to chose from */
bool fs_exists(const char* fname, const char* relative_path)
{
        char* norm_rel = normalize(g_filesystem->allocator, relative_path);
        if (!norm_rel || norm_rel[0] == '/' || !is_valid_path(norm_rel))
        {
                free(norm_rel);  // TD: memory allocator
                return false;
        }

        for (int i = 0; i < g_filesystem->mount_count; i++)
        {
                char* full = join_path(g_filesystem->allocator, g_filesystem->search_paths[i], norm_rel);
                if (full)
                {
                        if (!__platformFileExists(full))
                        {
                                free(full);
                                free(norm_rel);
                                return true;
                        }
                        free(full);
                }
        }
        free(norm_rel);
        return false;
}

bool fs_isDirectory(const char* fname)
{
        return __platformIsDirectory(fname);
}

uint64_t fs_getLastModificationTime(const char* filename)
{
        return __platformGetLastModTime(filename);
}

File* fs_openRead(const char* filename)
{
        if (!filename || !g_filesystem)
                return nullptr;

        char* full_path = fs_getfullpath(filename);

        if (!full_path)
        {
                IC_CORE_ERROR("File not found: {}", filename);
                return nullptr;
        }

        FILE* r = fopen(full_path, "rb");
        if (!r)
        {
                IC_CORE_ERROR("Failed to open: {}", filename);
                return nullptr;
        }

        File* file = (File*)bump_alloc_tagged(g_filesystem->allocator, sizeof(File), alignof(File), IC_TAG_FILESYSTEM);

        if (!file)
        {
                fclose(r);
                return nullptr;
        }

        fseek(r, 0, SEEK_END);
        file->size = ftell(r);
        fseek(r, 0, SEEK_SET);

        file->handle = r;
        return file;
}

bool fs_close(File* handle)
{
        if (!handle || !handle->handle)
                return false;

        FILE* fp = (FILE*)handle->handle;
        fclose(fp);

        return true;
}

size_t fs_read(File* handle, void* buffer, size_t objSize, size_t objCount)
{
        if (!handle || !handle->handle || !buffer)
                return 0;

        FILE* fp = (FILE*)handle->handle;
        return fread(buffer, objSize, objCount, fp);
}

size_t fs_write(File* handle, void* buffer, size_t objSize, size_t objCount)
{
        if (!handle || !handle->handle || !buffer)
                return 0;

        FILE* fp = (FILE*)handle->handle;
        return fwrite(buffer, objSize, objCount, fp);
}

bool fs_eof(File* handle)
{
        if (!handle || !handle->handle)
                return true;

        FILE* fp = (FILE*)handle->handle;
        return feof(fp) != 0;
}

size_t fs_tell(File* handle)
{
        if (!handle || !handle->handle)
                return 0;

        FILE* fp = (FILE*)handle->handle;
        return ftell(fp);
}

bool fs_seek(File* handle, size_t pos)
{
        if (!handle || !handle->handle)
                return false;

        FILE* fp = (FILE*)handle->handle;
        return fseek(fp, pos, SEEK_SET) == 0;
}

size_t fs_fileLength(File* handle)
{
        return handle ? handle->size : 0;
}

bool fs_flush(File* handle)
{
        if (!handle || !handle->handle)
                return false;

        FILE* fp = (FILE*)handle->handle;
        return fflush(fp) == 0;
}

size_t fs_setBuffer(File* handle, size_t bufsize)
{
        if (!handle || !handle->handle)
                return 0;

        FILE* fp = (FILE*)handle->handle;

        // Allocate buffer from bump allocator
        void* buffer = bump_alloc_tagged(g_filesystem->allocator, bufsize, 16, IC_TAG_FILESYSTEM);

        if (!buffer)
                return 0;

        setvbuf(fp, (char*)buffer, _IOFBF, bufsize);
        return bufsize;
}

bool fs_compress(File* handle)
{
        IC_CORE_WARN("Compression is not implemented as its obsolute right now");
        return false;
}

}  // namespace ic

/** API Implementation */
const char* IC_getfilename(const char* path)
{
        if (!path)
                return nullptr;

        const char* last_slash     = strrchr(path, '/');
        const char* last_backslash = strrchr(path, '\\');

        const char* separator      = last_slash;

        if (last_backslash && (!last_slash || last_backslash > last_slash))
                separator = last_backslash;

        if (separator)
                return separator + 1;

        return path;  // no separator hence its a file in root
}

FILE* IC_openFile(const char* path)
{
        ic::File* f = ic::fs_openRead(path);
        return (FILE*)f->handle;
}

char* IC_readFile(const char* path, size_t* out_size)
{
        if (!path || !g_filesystem)
                return nullptr;

        ic::File* file = ic::fs_openRead(path);
        if (!file)
                return nullptr;

        char* buffer = (char*)ic::bump_alloc_tagged(g_filesystem->allocator, file->size + 1, 1, ic::IC_TAG_FILESYSTEM);

        if (!buffer)
        {
                ic::fs_close(file);
                return nullptr;
        }

        size_t bytes_read  = ic::fs_read(file, buffer, 1, file->size);
        buffer[bytes_read] = '\0';

        if (out_size)
                *out_size = bytes_read;

        ic::fs_close(file);

        return buffer;
}

bool IC_writeFile(const char* path, const void* data, size_t size)
{
        if (!path || !data || size == 0)
                return false;

        FILE* fp = fopen(path, "wb");
        if (!fp)
                return false;

        size_t written = fwrite(data, 1, size, fp);
        fclose(fp);

        return written == size;
}

char** IC_listfiles(const char* dir)
{
        ic::fs_enumerateFiles(dir);
}

bool IC_fs_mount(const char* path, const char* mountPoint, bool append_path)
{
        if (mountPoint == NULL)
        {
                mountPoint = "/";
        }

        /** Path is the physical normalized path and mountpoint is a point in the physical path */
        ic::fs_mount(mountPoint, path);
        return true;
}

const char* IC_fs_getbasedir(void)
{
        if (!g_filesystem)
        {
                IC_CORE_ERROR("Filesystem does not exist!");
                return nullptr;
        }
        return g_filesystem->base_dir;
}

const char* IC_fs_getuserdir(void)
{
        if (!g_filesystem)
        {
                IC_CORE_ERROR("Filesystem does not exist!");
                return nullptr;
        }
        return g_filesystem->user_dir;
}

bool IC_fs_exists(const char* filename)
{
        if (!g_filesystem)
                return false;

        return false;
}

bool IC_fs_mkdir(const char* dirName)
{
        return ic::fs_mkdir(dirName);
}

bool IC_fs_delete(const char* filename)
{
        return ic::fs_delete(filename);
}

bool IC_fs_isDirectory(const char* path)
{
        return ic::fs_isDirectory(path);
}
