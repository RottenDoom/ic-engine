#include "core/filesystem.h"
#include "core/allocators.h"
#include "core/platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Allocate 2MiB for filesystem operations */
constexpr size_t FS_ALLOCATION_SIZE = 2 * 1024 * 1024;
#define MAX_FILES_ENUMERATED 1000

static ic::FS_Info *g_filesystem = nullptr;

static bool is_valid_path(const char *norm_path);
static void normalize(char *path);
static char *join_path(const char *a, const char *b);
static bool is_absolute_path(const char *path);
static char *resolve_physical_path(const char *path);
static bool path_matches_mount(const char *virtual_path, const char *mount_point);
static bool translate_mount_path(const char *virtual_path, char *out_buffer, size_t buffer_size);

namespace ic
{

struct Mount
{
        char *virtual_path;   // virtual standard path
        char *physical_path;  // physical path mapped with the virtual path
        bool is_archive;      // check if its an archive. TODO: not in use right now.
};

struct File
{
        void *handle;
        FSFileType type;
        size_t size;
        uint64_t modified_time;
};

struct FS_Info
{
        bump_allocator_t *allocator;  // usually bump/linear allocators work well with FS
        Mount mounts[FS_MAX_MOUNTS];
        size_t mount_count;
        char *base_dir;
        char *user_dir;
        char *root_dir;
        char *write_dir;
        char **mount_points;
        char **search_paths;  // search paths for the relative path
        size_t search_path_count;
};

bool fs_init(void)
{
        // Use global filesystem to control everything
        if (g_filesystem)
        {
                IC_CORE_WARN("Filesystem already exists");
                return false;
        }

        g_filesystem = (FS_Info *)ic_malloc(sizeof(FS_Info));
        memset(g_filesystem, 0, sizeof(FS_Info));

        // Initialize allocation memory for the filesystem
        void *fs_memory         = ic_malloc(FS_ALLOCATION_SIZE);
        g_filesystem->allocator = (bump_allocator_t *)ic_malloc(sizeof(bump_allocator_t));
        bump_allocator_init(g_filesystem->allocator, fs_memory, FS_ALLOCATION_SIZE);

        // Get and normalize platform directories
        char *home  = __platformCalcBaseDir();
        char *user  = __platformCalcUserDir();
        char *write = __platformCalcWriteDir();

        normalize(home);
        normalize(user);
        normalize(write);

        // Copy to bump allocator
        size_t home_len         = strlen(home) + 1;
        size_t user_len         = strlen(user) + 1;
        size_t write_len        = strlen(write) + 1;

        g_filesystem->base_dir  = (char *)bump_allocate(g_filesystem->allocator, home_len, 1, IC_TAG_FILESYSTEM);
        g_filesystem->user_dir  = (char *)bump_allocate(g_filesystem->allocator, user_len, 1, IC_TAG_FILESYSTEM);
        g_filesystem->write_dir = (char *)bump_allocate(g_filesystem->allocator, write_len, 1, IC_TAG_FILESYSTEM);

        memcpy(g_filesystem->base_dir, home, home_len);
        memcpy(g_filesystem->user_dir, user, user_len);
        memcpy(g_filesystem->write_dir, write, write_len);
        // free the platform directory strings
        ic_free(home);
        ic_free(user);
        ic_free(write);

        // Initialize the search paths
        g_filesystem->search_path_count = 0;
        g_filesystem->search_paths      = (char **)
            bump_allocate(g_filesystem->allocator, sizeof(char *) * FS_MAX_MOUNTS, alignof(char *), IC_TAG_FILESYSTEM);

        // Add base directory to search paths
        fs_addToSearchPath(g_filesystem->base_dir, false);

        IC_CORE_INFO("Initialized filesystem with {} MiB allocator", FS_ALLOCATION_SIZE / (1024 * 1024));
        IC_CORE_INFO("Base directory:  {}", g_filesystem->base_dir);
        IC_CORE_INFO("User directory:  {}", g_filesystem->user_dir);
        IC_CORE_INFO("Write directory: {}", g_filesystem->write_dir);

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

        ic_free(g_filesystem->allocator->memory);
        ic_free(g_filesystem->allocator);
        ic_free(g_filesystem);

        g_filesystem = nullptr;
}

bool fs_mount(const char *physical_path, const char *virtual_path)
{
        if (!g_filesystem || !physical_path || !virtual_path)
        {
                IC_CORE_ERROR("Provide paths or make sure application is properly initialized");
                return false;
        }

        if (!is_valid_path(physical_path) || !is_valid_path(virtual_path))
        {
                IC_CORE_ERROR("Provide valid paths. Make sure they do not contain '..' or './' or '.' anywhere");
                return false;
        }

        if (g_filesystem->mount_count >= FS_MAX_MOUNTS)
        {
                IC_CORE_WARN("No more mount points left");
                return false;
        }

        // since physical_path can be a full or relative path we have to resolve it
        char *resolved_physical;

        if (is_absolute_path(physical_path))
        {
                resolved_physical = (char *)ic_malloc(strlen(physical_path) + 1);
                strcpy(resolved_physical, physical_path);
                normalize(resolved_physical);
                if (!resolved_physical)
                {
                        IC_CORE_ERROR("Could not resolve physical path");
                        ic_free(resolved_physical);
                        return false;
                }
        }
        else
        {
                size_t len        = strlen(g_filesystem->base_dir) + strlen(physical_path) + 2;
                resolved_physical = (char *)ic_malloc(len);
                snprintf(resolved_physical, len, "%s/%s", g_filesystem->base_dir, physical_path);
                normalize(resolved_physical);

                if (!resolved_physical)
                {
                        IC_CORE_ERROR("Could not resolve physical path");
                        ic_free(resolved_physical);
                        return false;
                }
        }

        if (!__platformIsDirectory(resolved_physical))
        {
                IC_CORE_WARN("The path provided {} does not exist", resolved_physical);
        }

        Mount *mnt        = &g_filesystem->mounts[g_filesystem->mount_count];

        mnt->virtual_path = (char *)
            bump_allocate(g_filesystem->allocator, strlen(virtual_path) + 1, alignof(char), IC_TAG_FILESYSTEM);
        mnt->physical_path = (char *)
            bump_allocate(g_filesystem->allocator, strlen(resolved_physical) + 1, alignof(char), IC_TAG_FILESYSTEM);

        strcpy(mnt->virtual_path, virtual_path);
        strcpy(mnt->physical_path, resolved_physical);

        if (!mnt->virtual_path || !mnt->physical_path)
        {
                IC_CORE_ERROR("Could not mount the virtual path");
                ic_free(resolved_physical);
                return false;
        }

        ic_free(resolved_physical);
        g_filesystem->mount_count++;
        IC_CORE_INFO("Mounted virtual path '{}' as physical path '{}'", mnt->virtual_path, mnt->physical_path);
        return true;
}

char **fs_getSearchPath(void)
{
        return g_filesystem->search_paths;
}

Mount *fs_getMounts(void)
{
        return g_filesystem->mounts;
}

const char *fs_getDirSeperator(void)
{
        /** TODO: check for platform and return the dir seperator for the platfrom */
        // const char retval[2] = {__PLATFORM_DIR_SEPERATOR__, '\0'};
        // return retval;
        return NULL;
}

const char *fs_getWriteDirectory(void)
{
        return g_filesystem->write_dir; /** TODO set and get write dir */
}

void fs_setWriteDirectory(char *dir)
{
        if (!g_filesystem)
        {
                IC_CORE_ERROR("Filesystem does not exist");
        }

        if (is_absolute_path(dir))
        {
                if (g_filesystem->write_dir)
                        ic_free(g_filesystem->write_dir);

                g_filesystem->write_dir = dir;
        }
        else
        {
                if (dir[0] == '.')
                {
                        char *resolved = join_path(g_filesystem->base_dir, dir);

                        if (g_filesystem->write_dir)
                                ic_free(g_filesystem->write_dir);

                        g_filesystem->write_dir = resolved;
                }
                else
                {
                        char *resolved = (char *)ic_malloc(FS_MAX_PATH);

                        if (translate_mount_path(dir, resolved, strlen(resolved) + 1))
                        {
                                if (!__platformIsDirectory(resolved))
                                {
                                        IC_CORE_ASSERT(!__platformMkDir(resolved), "Could not create directory");
                                }

                                if (g_filesystem->write_dir)
                                        ic_free(g_filesystem->write_dir);

                                g_filesystem->write_dir = resolved;
                        }

                        // ic_free(resolved);
                }
        }

        IC_CORE_INFO("Set write directory to {}", g_filesystem->write_dir);
}

bool fs_addToSearchPath(char *newDir, bool appendToPath)
{
        if (!g_filesystem || !newDir)
                return false;

        if (g_filesystem->search_path_count >= FS_MAX_MOUNTS)
                return false;

        normalize(newDir);

        size_t dirlen   = strlen(newDir) + 1;
        char *searchDir = (char *)bump_allocate(g_filesystem->allocator, dirlen, alignof(char), IC_TAG_FILESYSTEM);

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

bool fs_removeFromSearchPath(const char *rmDir)
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

char *fs_getParentPath(const char *path)
{
        if (!g_filesystem || !path)
                return NULL;

        size_t len = strlen(path);

        if (len == 0)
                return nullptr;

        // copy path so we can operate safely
        char *buffer = (char *)ic_malloc(len + 1);
        if (!buffer)
                return nullptr;

        strcpy(buffer, path);

        // remove trailing slashes
        while (len > 0 && (buffer[len - 1] == '/' || buffer[len - 1] == '\\'))
        {
                buffer[len - 1] = '\0';
                len--;
        }

        // find last separator
        char *lastSlash = strrchr(buffer, '/');
        char *lastBack  = strrchr(buffer, '\\');

        char *lastSep   = lastSlash > lastBack ? lastSlash : lastBack;

        if (!lastSep)
        {
                ic_free(buffer);
                return nullptr;
        }

        // if separator is first character → root directory
        if (lastSep == buffer)
        {
                buffer[1] = '\0';
                return buffer;
        }

        *lastSep = '\0';

        return buffer;
}

char *fs_getfullpath(const char *filename)
{
        for (size_t i = 0; i < g_filesystem->mount_count; i++)
        {
                char *full = join_path(g_filesystem->mounts[i].physical_path, filename);
                IC_CORE_ASSERT(full, "Path could not be joined");
                if (__platformFileExists(full))
                {
                        return full;
                }
                ic_free(full);
        }

        // check for search paths as well (this mostly works out if you put the desired search paths here)
        for (size_t i = 0; i < g_filesystem->search_path_count; i++)
        {
                char *full = join_path(g_filesystem->search_paths[i], filename);
                IC_CORE_ASSERT(full, "Path could not be joined");
                if (__platformFileExists(full))
                {
                        return full;
                }
                ic_free(full);
        }

        /** TODO: Better error handling. */
        return nullptr;
}

bool fs_mkdir(const char *dirName)
{

        if (is_absolute_path(dirName) || dirName[0] == '.')
                return ic::fs_mkdir(dirName);
        else
        {
                char resolved[FS_MAX_PATH];

                if (translate_mount_path(dirName, resolved, sizeof(resolved)))
                {
                        if (!ic::__platformIsDirectory(resolved))
                        {
                                IC_CORE_ASSERT(!ic::fs_mkdir(resolved), "Could not create directory");
                                return true;
                        }
                }
                char *resolvedPath = join_path(g_filesystem->write_dir, dirName);
                memcpy(resolved, resolvedPath, sizeof(resolvedPath));
                ic_free(resolvedPath);
                return __platformMkDir(resolved);
        }
}

bool fs_rmdir(const char *dirName)
{
        return __platformRmDir(dirName);
}

bool fs_delete(const char *filename)
{
        return __platformDeleteFile(filename);
}

char **fs_enumerateFiles(const char *dir)
{
        if (!dir)
                return nullptr;

        // allocator array for enumeration
        char **file_list = (char **)bump_allocate(g_filesystem->allocator,
                                                  sizeof(char *) * (MAX_FILES_ENUMERATED + 1),
                                                  alignof(char *),
                                                  IC_TAG_FILESYSTEM);

        if (!file_list)
                return nullptr;
        size_t file_cnt           = 0;

        PlatformDirIterator *iter = __platformOpenDir(dir);
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
                char *filename  = (char *)bump_allocate(g_filesystem->allocator, name_len, 1, IC_TAG_FILESYSTEM);

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
bool fs_exists(const char *path)
{
        if (!g_filesystem || !path)
                return false;

        if (is_absolute_path(path))
                return __platformFileExists(path);
        else
        {
                char full_path[FS_MAX_PATH];
                // try searching in the mounts
                if (translate_mount_path(path, full_path, FS_MAX_PATH))
                {
                        return __platformFileExists(full_path);
                }

                // Try search paths
                for (int i = g_filesystem->search_path_count - 1; i >= 0; i--)
                {
                        snprintf(full_path, sizeof(full_path), "%s/%s", g_filesystem->search_paths[i], path);
                        normalize(full_path);

                        return __platformFileExists(full_path);
                }

                return false;
        }
}

/** Only works with absolute paths */
bool fs_fileExists(const char *filepath)
{
        if (!__platformFileExists(filepath))
        {
                IC_CORE_WARN("File path does not exist");
                return false;
        }
        return true;
}

bool fs_isDirectory(const char *dir)
{
        return __platformIsDirectory(dir);
}

bool fs_joinPath(const char *relPath, const char *fullpath, const char **out)
{
        IC_CORE_ASSERT(relPath && fullpath && g_filesystem, "Filesystem error.");

        *out = join_path(fullpath, relPath);

        if (!fs_exists(*out))
        {
                IC_CORE_WARN("The directory {} does not exist cant join", *out);
                ic_free((void *)(*out));
                out = nullptr;
                return false;
        }

        return true;
}

uint64_t fs_getLastModificationTime(const char *filename)
{
        return __platformGetLastModTime(filename);
}

File *fs_openRead(const char *filename)
{
        if (!filename || !g_filesystem)
                return nullptr;

        char *full_path = (char *)ic_malloc(FS_MAX_PATH);
        bool found      = false;

        if (filename[0] == '/')
        {
                if (translate_mount_path(filename, full_path, FS_MAX_PATH))
                {
                        if (fs_exists(full_path))
                        {
                                found = true;
                        }
                }
        }

        if (!found)
        {
                for (uint32_t i = 0; i < g_filesystem->search_path_count; i++)
                {
                        snprintf(full_path, FS_MAX_PATH, "%s/%s", g_filesystem->search_paths[i], filename);
                        normalize(full_path);

                        if (!__platformFileExists(full_path))
                        {
                                found = false;
                                IC_CORE_ERROR("Could not find the file at path {}", full_path);
                                ic_free(full_path);
                                return nullptr;
                        }
                }
        }

        if (!full_path)
        {
                IC_CORE_ERROR("File not found: {}", filename);
                ic_free(full_path);
                return nullptr;
        }

        FILE *r = fopen(full_path, "rb");
        if (!r)
        {
                IC_CORE_ERROR("Failed to open: {} (resolved to {})", filename, full_path);
                IC_CORE_ERROR("Check file permissions and path");
                ic_free(full_path);
                return nullptr;
        }

        // Get file size
        fseek(r, 0, SEEK_END);
        long size = ftell(r);
        fseek(r, 0, SEEK_SET);

        if (size < 0)
        {
                IC_CORE_ERROR("Failed to get file size: {}", filename);
                fclose(r);
                ic_free(full_path);
                return NULL;
        }

        // Allocate File structure
        File *file = (File *)bump_allocate(g_filesystem->allocator, sizeof(File), alignof(File), IC_TAG_FILESYSTEM);
        if (!file)
        {
                IC_CORE_ERROR("Failed to allocate File structure");
                fclose(r);
                ic_free(full_path);
                return NULL;
        }

        file->size   = (size_t)size;
        file->handle = r;

        IC_CORE_INFO("Opened '{}' ({} bytes) from '{}'", filename, file->size, full_path);
        ic_free(full_path);
        return file;
}

bool fs_close(File *handle)
{
        if (!handle || !handle->handle)
                return false;

        FILE *fp = (FILE *)handle->handle;
        fclose(fp);

        return true;
}

size_t fs_read(File *handle, void *buffer, size_t objSize, size_t objCount)
{
        if (!handle || !handle->handle || !buffer)
                return 0;

        FILE *fp = (FILE *)handle->handle;
        return fread(buffer, objSize, objCount, fp);
}

size_t fs_write(File *handle, void *buffer, size_t objSize, size_t objCount)
{
        if (!handle || !handle->handle || !buffer)
                return 0;

        FILE *fp = (FILE *)handle->handle;
        return fwrite(buffer, objSize, objCount, fp);
}

bool fs_eof(File *handle)
{
        if (!handle || !handle->handle)
                return true;

        FILE *fp = (FILE *)handle->handle;
        return feof(fp) != 0;
}

size_t fs_tell(File *handle)
{
        if (!handle || !handle->handle)
                return 0;

        FILE *fp = (FILE *)handle->handle;
        return ftell(fp);
}

bool fs_seek(File *handle, size_t pos)
{
        if (!handle || !handle->handle)
                return false;

        FILE *fp = (FILE *)handle->handle;
        return fseek(fp, pos, SEEK_SET) == 0;
}

size_t fs_fileLength(File *handle)
{
        return handle ? handle->size : 0;
}

bool fs_flush(File *handle)
{
        if (!handle || !handle->handle)
                return false;

        FILE *fp = (FILE *)handle->handle;
        return fflush(fp) == 0;
}

size_t fs_setBuffer(File *handle, size_t bufsize)
{
        if (!handle || !handle->handle)
                return 0;

        FILE *fp = (FILE *)handle->handle;

        // Allocate buffer from bump allocator
        void *buffer = bump_allocate(g_filesystem->allocator, bufsize, 16, IC_TAG_FILESYSTEM);

        if (!buffer)
                return 0;

        setvbuf(fp, (char *)buffer, _IOFBF, bufsize);
        return bufsize;
}

bool fs_compress(File *handle)
{
        IC_CORE_WARN("Compression is not implemented as its obsolute right now");
        return false;
}

}  // namespace ic

/** API Implementation */
const char *IC_getfilename(const char *path)
{
        if (!path)
                return nullptr;

        const char *last_slash     = strrchr(path, '/');
        const char *last_backslash = strrchr(path, '\\');

        const char *separator      = last_slash;

        if (last_backslash && (!last_slash || last_backslash > last_slash))
                separator = last_backslash;

        if (separator)
                return separator + 1;

        return path;  // no separator hence its a file in root
}

FILE *IC_fs_open(const char *path)
{
        ic::File *f = ic::fs_openRead(path);
        return (FILE *)f->handle;
}

char *IC_fs_read(const char *path, size_t *out_size)
{
        if (!path || !g_filesystem)
                return nullptr;

        ic::File *file = ic::fs_openRead(path);
        if (!file)
                return nullptr;

        char *buffer = (char *)bump_allocate(g_filesystem->allocator, file->size + 1, 1, ic::IC_TAG_FILESYSTEM);

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

bool IC_fs_write(const char *path, const void *data, size_t size)
{
        if (!path || !data || size == 0)
                return false;

        FILE *fp = fopen(path, "wb");
        if (!fp)
                return false;

        size_t written = fwrite(data, 1, size, fp);
        fclose(fp);

        return written == size;
}

char **IC_listfiles(const char *dir)
{
        return ic::fs_enumerateFiles(dir);
}

bool IC_fs_mount(const char *physicalPoint, const char *virtualPoint, bool append_path)
{
        if (virtualPoint == NULL)
        {
                virtualPoint = "/";
        }

        /** Path is the physical normalized path and mountpoint is a point in the physical path */
        IC_CORE_ASSERT(ic::fs_mount(physicalPoint, virtualPoint), "Could not mount the directory");
        return true;
}

const char *IC_fs_getcwddir(void)
{
        return ic::__platformGetCurrentDir();
}

const char *IC_fs_getbasedir(void)
{
        if (!g_filesystem)
        {
                IC_CORE_ERROR("Filesystem does not exist!");
                return nullptr;
        }
        return g_filesystem->base_dir;
}

const char *IC_fs_getuserdir(void)
{
        if (!g_filesystem)
        {
                IC_CORE_ERROR("Filesystem does not exist!");
                return nullptr;
        }
        return g_filesystem->user_dir;
}

bool IC_fs_exists(const char *filename)
{
        if (!g_filesystem || !filename)
        {
                IC_CORE_ERROR("Filesystem or the filename does not exist");
                return false;
        }

        if (!ic::fs_exists(filename))
        {
                IC_CORE_WARN("File does not exist in the search paths.");
                return false;
        }

        return true;
}

bool IC_fs_mkdir(const char *dirName)
{
        return ic::fs_mkdir(dirName);
}

bool IC_fs_delete(const char *filename)
{
        return ic::fs_delete(filename);
}

bool IC_fs_isDirectory(const char *path)
{
        return ic::fs_isDirectory(path);
}

// Helper Implementation

static bool is_valid_path(const char *norm_path)
{
        if (strstr(norm_path, "..") || strstr(norm_path, "/.") || norm_path[0] == '.')
        {
                return false;
        }
        return true;
}

static void normalize(char *path)
{
        if (!path)
                return;

        char *dst          = path;
        char *src          = path;
        int last_was_slash = 0;

        while (*src)
        {
                char c = *src++;
                // Convert backslashes to forward slashes
                if (c == '\\')
                        c = '/';

                // Skip consecutive slashes
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

        // Remove trailing slash (except for root "/")
        size_t len = strlen(path);
        if (len > 1 && path[len - 1] == '/')
        {
                path[len - 1] = '\0';
        }
}

/** Returns a + b */
static char *join_path(const char *a, const char *b)
{
        size_t len_a = strlen(a);
        size_t len_b = strlen(b);
        size_t extra = (len_a > 0 && a[len_a - 1] != '/') ? 1 : 0;
        char *res    = (char *)ic_malloc(len_a + extra + len_b + 1);  // free this afterwards
        if (!res)
        {
                ic_free(res);
                return NULL;
        }

        strcpy(res, a);
        if (extra)
                strcat(res, "/");
        strcat(res, b);
        return res;
}

// written this function for windows gotta write for unix and linux
static bool is_absolute_path(const char *path)
{
        if (path[0] == '/')
                return false;

        // Windows: starts with drive letter like "C:\"
        if (isalpha(path[0]) && path[1] == ':')
                return true;

        return false;  // relative
}

// Helper: Resolve physical path (absolute or relative to base_dir)
static char *resolve_physical_path(const char *path)
{
        // Relative: resolve against base_dir
        size_t len     = strlen(g_filesystem->base_dir) + strlen(path) + 2;
        char *resolved = (char *)ic_malloc(len);
        snprintf(resolved, len, "%s/%s", g_filesystem->base_dir, path);
        normalize(resolved);
        return resolved;
}

// Helper: Check if virtual path matches mount point
static bool path_matches_mount(const char *virtual_path, const char *mount_point)
{
        size_t mount_len = strlen(mount_point);

        // Root mount "/" matches everything
        if (mount_len == 1 && mount_point[0] == '/')
                return true;

        // Check if virtual_path starts with mount_point
        if (strncmp(virtual_path, mount_point, mount_len) != 0)
                return false;

        // Must be exact match or followed by '/'
        return virtual_path[mount_len] == '\0' || virtual_path[mount_len] == '/';
}

// Helper: Translate virtual path to physical path using mounts
static bool translate_mount_path(const char *virtual_path, char *out_buffer, size_t buffer_size)
{
        // Iterate through mounts (most recent first = highest priority
        for (int i = 0; i < g_filesystem->mount_count; i++)
        {
                ic::Mount *mount = &g_filesystem->mounts[i];
                if (path_matches_mount(virtual_path, mount->virtual_path))
                {
                        size_t mount_len = strlen(mount->virtual_path);

                        if (strncmp(virtual_path, mount->virtual_path, mount_len) != 0)
                                continue;

                        const char *relative_part = virtual_path + mount_len;

                        if (*relative_part == '/')
                                relative_part++;

                        // Build physical path
                        snprintf(out_buffer, buffer_size, "%s/%s", mount->physical_path, relative_part);
                        normalize(out_buffer);
                        return true;
                }
        }

        IC_CORE_INFO("No mount found for the path: {}", virtual_path);
        return false;  // No matching mount found
}
