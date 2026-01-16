#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../defines.h"
#include "allocators.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FS_MAX_MOUNTS 16
#define FS_MAX_PATH 512

namespace ic
{
/**
 * The filesystem for my project will be hierarchial filesystem. No '..', '\\' or ':' which will pollute the filesystem
 */

typedef struct File File;

/** Mount type only contains a path for now can be later used for archiving. */
typedef struct Mount Mount;

// File types
typedef enum
{
        FS_NONE = 0,
        FS_FILE,
        FS_DIRECTORY,
        FS_SYMLINK,
        FS_UNKNOWN
} FSFileType;

/** Filesystem struct for holding mounts and standard paths for a FS */
typedef struct FS_Info FS_Info;

/** File struct for file handles and lifetimes */
typedef struct File File;

bool fs_init(void);
void fs_deinit(void);

/** Mount a path onto a physical path on a drive */
bool fs_mount(const char* virtual_path, const char* physical_path);

/** Returns the directory seporator for a filesystem */
const char* fs_getDirSeperator(void);
const char* fs_getWriteDirectory(void);
void fs_setWriteDirectory(char* dir);

/** @brief adds a directory to a search path. The directory should be normalized and full. if append is true we add it
 * to the last of search paths else at the start
 * @note newDir should be a full path to the directory. Use fs base or user or root to access some directories. */
bool fs_addToSearchPath(char* newDir, bool appendToPath);
bool fs_removeFromSearchPath(const char* rmDir);
char** fs_getSearchPath(void);

/** By defualt mkdir makes the directory in the application base directory */
bool fs_mkdir(const char* dirName);
bool fs_rmdir(const char* dirName);

/** Returns joined path using a relative path and a full path, checks if that path exists. If yes returns true.
 * Equivalent to cd command */
bool fs_joinPath(const char* relPath, const char* fullpath, const char* out);
bool fs_delete(const char* filename);

/** Enumerate files in a directory. dir should be a full path */
char** fs_enumerateFiles(const char* dir);

/** Returns full path of a file from the search paths */
char* fs_getfullpath(const char* filename);

/** Check if a file exists in the search paths*/
bool fs_exists(const char* relative_path);

/** Check if a file exists. filepath should be a normalized full path to the file */
bool fs_fileExists(const char* filepath);
bool fs_isDirectory(const char* dir);
uint64_t fs_getLastModificationTime(const char* filename);

File* fs_openRead(const char* filename);
bool fs_close(File* handle);
size_t fs_read(File* handle, void* buffer, size_t objSize, size_t objCount);
size_t fs_write(File* handle, void* buffer, size_t objSize, size_t objCount);
bool fs_eof(File* handle);
size_t fs_tell(File* handle);
bool fs_seek(File* handle, size_t pos);
size_t fs_fileLength(File* handle);
size_t fs_setBuffer(File* handle, size_t bufsize);
bool fs_flush(File* handle);
bool fs_compress(File* handle);

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /**
         * @function IC_getfilename
         * @category filesystem
         * @brief Get the filename with extension from a path
         * @param path: virtual path to a file
         * @returns filename with extension, or NULL if path is invalid
         *
         * @example
         * const char* name = IC_getfilename("/assets/textures/player.png");
         * // Returns: "player.png"
         */
        IC_API const char* IC_getfilename(const char* path);

        /**
         * @function IC_fs_open
         * @category filesystem
         * @brief Open a file for reading (searches all mount points)
         * @param path: virtual path to the file
         * @returns FILE handle for reading, or NULL if not found
         *
         * @example
         * FILE* fp = IC_openFile("config.ini");
         * if (fp) {
         *     // Read from file
         *     fclose(fp);
         * }
         */
        IC_API FILE* IC_fs_open(const char* path);

        /**
         * @function IC_fs_read
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
         * char* content = IC_fs_read("shader.glsl", &size);
         * if (content) {
         *     printf("Loaded %zu bytes: %s\n", size, content);
         * }
         */
        IC_API char* IC_fs_read(const char* path, size_t* out_size);

        /**
         * @function IC_fs_write
         * @category filesystem
         * @brief Write data to a file
         * @param path: file path to write to
         * @param data: data to write
         * @param size: size of data in bytes
         * @returns true on success, false on failure
         *
         * @example
         * const char* data = "Hello, World!";
         * if (IC_fs_write("output.txt", data, strlen(data))) {
         *     printf("File written successfully\n");
         * }
         */
        IC_API bool IC_fs_write(const char* path, const void* data, size_t size);

        /**
         * @function IC_listfiles
         * @category filesystem
         * @brief List all files in a directory
         * @param dir: directory path to enumerate
         * @returns null-terminated array of filenames (allocated from bump allocator), or NULL on failure
         *
         * @example
         * char** files = IC_listfiles("./assets");
         * if (files) {
         *     for (int i = 0; files[i] != NULL; i++) {
         *         printf("File: %s\n", files[i]);
         *     }
         * }
         */
        IC_API char** IC_listfiles(const char* dir);

        /**
         * @function IC_fs_getbasedir
         * @category filesystem
         * @brief Get the base directory (where the executable is located)
         * @returns base directory path (never NULL after fs_init)
         *
         * @note The base directory cannot be modified after initialization.
         *       All relative paths are relative to this directory.
         *
         * @example
         * const char* base = IC_fs_getbasedir();
         * printf("Running from: %s\n", base);
         */
        IC_API const char* IC_fs_getbasedir(void);

        /**
         * @function IC_fs_getuserdir
         * @category filesystem
         * @brief Get the user's home directory
         * @returns user directory path (e.g., C:/Users/Username on Windows, /home/username on Linux)
         *
         * @example
         * const char* user_dir = IC_fs_getuserdir();
         * printf("User directory: %s\n", user_dir);
         */
        IC_API const char* IC_fs_getuserdir(void);

        /**
         * @function IC_fs_mount
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
         * IC_fs_mount("/assets", "./data/assets", true);
         * IC_fs_mount("/levels", "./data/levels", true);
         *
         * // Now files can be accessed via virtual paths:
         * FILE* fp = IC_openFile("/assets/texture.png");
         */
        IC_API bool IC_fs_mount(const char* physical_path, const char* virtual_path, bool append_path);

        /**
         * @function IC_fs_exists
         * @category filesystem
         * @brief Check if a file exists in any search path
         * @param filename: file to check for
         * @returns true if file exists, false otherwise
         *
         * @example
         * if (IC_fs_exists("save_game.dat")) {
         *     printf("Save file found\n");
         * }
         */
        IC_API bool IC_fs_exists(const char* filename);

        /**
         * @function IC_fs_mkdir
         * @category filesystem
         * @brief Create a directory
         * @param dirName: directory path to create
         * @returns true on success, false on failure
         *
         * @example
         * IC_fs_mkdir("./saves");
         * IC_fs_mkdir("./screenshots");
         */
        IC_API bool IC_fs_mkdir(const char* dirName);

        /**
         * @function IC_fs_delete
         * @category filesystem
         * @brief Delete a file
         * @param filename: file to delete
         * @returns true on success, false on failure
         *
         * @example
         * if (IC_fs_delete("temp.dat")) {
         *     printf("Temp file deleted\n");
         * }
         */
        IC_API bool IC_fs_delete(const char* filename);

        /**
         * @function IC_fs_isDirectory
         * @category filesystem
         * @brief Check if a path is a directory
         * @param path: path to check
         * @returns true if directory, false otherwise
         *
         * @example
         * if (IC_fs_isDirectory("./assets")) {
         *     printf("Assets directory exists\n");
         * }
         */
        IC_API bool IC_fs_isDirectory(const char* path);

        /**
         * @function IC_fs_initialize
         * @category filesystem
         * @brief Initialize the filesystem (must be called first)
         *
         * @example
         * IC_fs_initialize();
         * // ... use filesystem ...
         * IC_fs_shutdown();
         */
        IC_API void IC_fs_initialize(void);

        /**
         * @function IC_fs_shutdown
         * @category filesystem
         * @brief Shutdown the filesystem and free all resources
         *
         * @note All file handles should be closed before calling this.
         */
        IC_API void IC_fs_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif