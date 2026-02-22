#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "defines.h"
#include "allocators.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FS_MAX_MOUNTS 16
#define FS_MAX_PATH 512

namespace ic
{

typedef struct
{
        /* basic file handle info*/
        void  *handle;
        int    flags; /* flags for open, read and write */
        size_t size;

        /* mmap file info */
        void  *mmap_addr;
        size_t mmap_size;

        /* modified time info */
        uint64_t modified_time;
} File;

/* mounttypes to correctly resolve file directories */
typedef enum
{
        MOUNT_TYPE_DIRECTORY, /* only support this for now. */
        MOUNT_TYPE_ARCHIVE,
        MOUNT_TYPE_SYMLINK,
} MountType;

/** Mount type only contains a path for now can be later used for archiving. */
typedef struct
{
        char     *virtual_path;
        char     *physical_path;
        MountType type;
        void     *driver;    // future: archive handle
        uint16_t  priority;  // higher = checked first
} Mount;

typedef enum
{
        FS_OPEN_READ   = 1 << 0,
        FS_OPEN_WRITE  = 1 << 1,
        FS_OPEN_APPEND = 1 << 2,
        FS_OPEN_CREATE = 1 << 3,
} FSOpenFlags;

/** Filesystem struct for holding mounts and standard paths for a FS */
typedef struct
{
        bump_allocator_t *allocator;  // usually bump/linear allocators work well with FS

        /* mount info */
        Mount  mounts[FS_MAX_MOUNTS];
        size_t mount_count;
        char **mount_points;

        /* base directories for OS */
        char *base_dir;
        char *user_dir;
        char *root_dir;
        char *write_dir;

        /* search paths for falling back from mount paths */
        char **search_paths;
        size_t search_path_count;
} FS_Info;

bool fs_init(void);
void fs_deinit(void);

/** Mount a path onto a physical path on a drive */
bool fs_mount(const char *virtual_path, const char *physical_path, MountType type, uint16_t priority);

/** Returns the directory seporator for a filesystem */
const char *fs_getDirSeperator(void);
const char *fs_getWriteDirectory(void);
void        fs_setWriteDirectory(char *dir);

/** @brief adds a directory to a search path. The directory should be normalized and full. if append is true we add it
 * to the last of search paths else at the start
 * @note newDir should be a full path to the directory. Use fs base or user or root to access some directories. */
bool   fs_addToSearchPath(char *newDir, bool appendToPath);
bool   fs_removeFromSearchPath(const char *rmDir);
char **fs_getSearchPath(void);

/** By defualt mkdir makes the directory in the application base directory */
bool fs_mkdir(const char *dirName);
bool fs_rmdir(const char *dirName);

/** Returns joined path using a relative path and a full path, checks if that path exists. If yes returns true.
 * Equivalent to cd command */
bool fs_joinPath(const char *relPath, const char *fullpath, const char **out);
bool fs_delete(const char *filename);

/** Enumerate files in a directory. dir should be a full path */
char **fs_enumerateFiles(const char *dir);

/** Returns full path of a file from the search paths */
char *fs_getfullpath(const char *filename);

/** Get parent path from a file path or directory */
char *fs_getParentPath(const char *path);

/** File and directory checks */
bool     fs_exists(const char *relative_path);
bool     fs_isDirectory(const char *dir);
uint64_t fs_getLastModificationTime(const char *filename);

File  *fs_openRead(const char *filename);
bool   fs_close(File *handle);
size_t fs_read(File *handle, void *buffer, size_t objSize, size_t objCount);
size_t fs_write(File *handle, void *buffer, size_t objSize, size_t objCount);
bool   fs_eof(File *handle);
size_t fs_tell(File *handle);
bool   fs_seek(File *handle, size_t pos);
size_t fs_fileLength(File *handle);
size_t fs_setBuffer(File *handle, size_t bufsize);
bool   fs_flush(File *handle);
bool   fs_compress(File *handle);

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /**
         * @function ic_getfilename
         * @category filesystem
         * @brief Get the filename with extension from a path
         * @param path: virtual path to a file
         * @returns filename with extension, or NULL if path is invalid
         *
         * @example
         * const char* name = ic_getfilename("/assets/textures/player.png");
         * // Returns: "player.png"
         */
        IC_API const char *ic_getfilename(const char *path);

        /**
         * @function ic_open
         * @category filesystem
         * @brief Open a file for reading (searches all mount points)
         * @param path: virtual path to the file
         * @returns FILE handle for reading, or NULL if not found
         *
         * @example
         * FILE* fp = ic_open("config.ini");
         * if (fp) {
         *     // Read from file
         *     fclose(fp);
         * }
         */
        IC_API FILE *ic_open(const char *path);

        /**
         * @function ic_read
         * @category filesystem
         * @brief Read entire file contents into a buffer (allocated from bump allocator)
         * @param path: virtual path of the file
         * @param out_size: pointer to store file size (can be NULL)
         * @returns buffer containing file contents (null-terminated), or NULL on failure
         *
         * @note The returned buffer is managed by the filesystem allocator.
         *       It will be freed when fs_deinit() is called or allocator is cleared.
         *
         * @example
         * size_t size;
         * char* content = ic_read("shader.glsl", &size);
         * if (content) {
         *     printf("Loaded %zu bytes: %s\n", size, content);
         * }
         */
        IC_API char *ic_read(const char *path, size_t *out_size);

        /**
         * @function ic_write
         * @category filesystem
         * @brief Write data to a file
         * @param path: file path to write to
         * @param data: data to write
         * @param size: size of data in bytes
         * @returns true on success, false on failure
         *
         * @example
         * const char* data = "Hello, World!";
         * if (ic_write("output.txt", data, strlen(data))) {
         *     printf("File written successfully\n");
         * }
         */
        IC_API bool ic_write(const char *path, const void *data, size_t size);

        /**
         * @function ic_listfiles
         * @category filesystem
         * @brief List all files in a directory
         * @param dir: directory path to enumerate
         * @returns null-terminated array of filenames (allocated from bump allocator), or NULL on failure
         *
         * @example
         * char** files = ic_listfiles("./assets");
         * if (files) {
         *     for (int i = 0; files[i] != NULL; i++) {
         *         printf("File: %s\n", files[i]);
         *     }
         * }
         */
        IC_API char **ic_listfiles(const char *dir);

        /**
         * @function ic_getbasedir
         * @category filesystem
         * @brief Get the base directory (where the executable is located)
         * @returns base directory path (never NULL after fs_init)
         *
         * @note The base directory cannot be modified after initialization.
         *       All relative paths are relative to this directory.
         *
         * @example
         * const char* base = ic_getbasedir();
         * printf("Running from: %s\n", base);
         */
        IC_API const char *ic_getbasedir(void);

        /**
         * @function ic_getcwddir
         * @category filesystem
         * @brief Get the cwd directory (get the current main file directory)
         * @returns cwd directory path (never NULL after fs_init)
         *
         * @note The cwd directory cannot be modified after initialization.
         *
         * @example
         * const char* cwd = ic_getcwddir();
         * printf("Running from: %s\n", cwd);
         */
        IC_API const char *ic_getcwddir(void);

        /**
         * @function ic_getuserdir
         * @category filesystem
         * @brief Get the user's home directory
         * @returns user directory path (e.g., C:/Users/Username on Windows, /home/username on Linux)
         *
         * @example
         * const char* user_dir = ic_getuserdir();
         * printf("User directory: %s\n", user_dir);
         */
        IC_API const char *ic_getuserdir(void);

        /**
         * @function ic_mount
         * @category filesystem
         * @brief Mount a physical directory to a virtual path
         * @param physical_path: physical directory path (e.g., "./game_data/assets")
         * @param virtual_path: virtual path (must start with '/', e.g., "/assets")
         * @param append_path: if true, appends to search paths; if false, prepends
         * @returns true on success, false on failure
         *
         * @note Virtual paths must not contain "..", "\\", or ":"
         *
         * @example
         * // Mount game assets
         * ic_mount("/assets", "./data/assets", true);
         * ic_mount("/levels", "./data/levels", true);
         *
         * // Now files can be accessed via virtual paths:
         * FILE* fp = IC_openFile("/assets/texture.png");
         */
        IC_API bool ic_mount(const char *virtual_path, const char *physical_path, uint16_t priority);

        /**
         * @function ic_exists
         * @category filesystem
         * @brief Check if a file exists in any search path
         * @param filename: file to check for
         * @returns true if file exists, false otherwise
         *
         * @example
         * if (ic_exists("save_game.dat")) {
         *     printf("Save file found\n");
         * }
         */
        IC_API bool ic_exists(const char *filename);

        /**
         * @function ic_mkdir
         * @category filesystem
         * @brief Create a directory
         * @param dirName: directory path to create
         * @returns true on success, false on failure
         *
         * @example
         * ic_mkdir("./saves");
         * ic_mkdir("./screenshots");
         */
        IC_API bool ic_mkdir(const char *dirName);

        /**
         * @function ic_delete
         * @category filesystem
         * @brief Delete a file
         * @param filename: file to delete
         * @returns true on success, false on failure
         *
         * @example
         * if (ic_delete("temp.dat")) {
         *     printf("Temp file deleted\n");
         * }
         */
        IC_API bool ic_delete(const char *filename);

        /**
         * @function ic_isDirectory
         * @category filesystem
         * @brief Check if a path is a directory
         * @param path: path to check
         * @returns true if directory, false otherwise
         *
         * @example
         * if (ic_isDirectory("./assets")) {
         *     printf("Assets directory exists\n");
         * }
         */
        IC_API bool ic_isDirectory(const char *path);

#ifdef __cplusplus
}
#endif

#endif