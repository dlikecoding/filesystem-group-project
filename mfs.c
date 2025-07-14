/**************************************************************
 * Class::  CSC-415-03 FALL 2024
 * Name:: Danish Nguyen, Atharva Walawalkar, Arvin Ghanizadeh, Cheryl Fong
 * Student IDs:: 923091933, 924254653, 922810925, 918157791
 * GitHub-Name:: dlikecoding, AtharvaWal2002, arvinghanizadeh, cherylfong
 * Group-Name:: 0xAACD
 * Project:: Basic File System
 *
 * File:: mfs.c
 *
 * Description::
 *	This is the file system interface.
 *	This is the interface needed by the driver to interact with
 *	your filesystem.
 *
 **************************************************************/

#include "mfs.h"
#include <string.h>
#include <stdlib.h>
#include "structs/ParsePath.h"
#include "structs/VCB.h"
#include "structs/DE.h"

// @author: Atharva Walawalkar

// Helper function to find specific DEs by name
directory_entry *FindHelper(directory_entry *current, const char *name)
{
    // printf("Finding directory: %s in directory size: %d\n", name, sizeOfDE(current));

    // First load the directory contents if needed
    if(!current || !name)
    {
        return NULL;
    }

    /*printf("Current directory contents:\n");
    for (int i = 0; i < sizeOfDE(current); i++) {
        if (current[i].is_used) {
            printf("  [%d] %s (dir: %d)\n", i, current[i].file_name, current[i].is_directory);
        }
    }*/

    for (int i = 0; i < sizeOfDE(current); i++)
    {
        // printf("Checking entry %d: %s (used: %d)\n", i, current[i].file_name,
        //     current[i].is_used);

        if (current[i].is_used && strcmp(current[i].file_name, name) == 0)
        {
            return &current[i];
        }
    }
    
    return NULL;
}

int parsePath(const char *path, parsepath_st *result)
{

    // printf("\nParsing path: %s\n", path);

    if (path == NULL || result == NULL || strlen(path) == 0)
    {
        printf("Invalid input parameters\n");
        return -1;
    }

    // Traverse pointers
    directory_entry *current;
    directory_entry *parent;

    if (path[0] == '/')
    {
        // printf("Using absolute path\n");
        // If path is absolute
        current = vcb->root_dir_ptr;
        if (strlen(path) == 1)
        { // Just "/"
            // printf("Root path requested\n");
            result->retParent = vcb->root_dir_ptr;
            result->index = 0;
            result->lastElement[0] = '.';
            result->lastElement[1] = '\0';
            return 0;
        }
        path++;
    }
    else
    {
        // If relative path
        // printf("Using relative path\n");
        current = (vcb->cwdLoadDE == NULL) ? vcb->root_dir_ptr : vcb->cwdLoadDE;
    }

    if (current == NULL)
    {
        // printf("Starting directory is NULL\n");
        return -1;
    }

    parent = current;
    // printf("Starting from directory with size: %d\n", sizeOfDE(current));

    // Checking and handling empty path
    if (strlen(path) == 0)
    {
        // printf("Empty path, returning root\n");
        result->retParent = parent;
        result->index = -1;
        result->lastElement[0] = '\0';
        return 0;
    }

    // Copy of path for tokenisation and as path is const
    char *path_copy = strdup(path);
    if (path_copy == NULL)
    {
        // printf("Failed to copy path\n");
        return -1;
    }

    // The result structure populated
    result->retParent = parent;
    result->index = -1;
    result->lastElement[0] = '\0';

    // initialising tokenisation pointers
    char *saveptr;
    char *token1 = strtok_r(path_copy, "/", &saveptr);

    // Checking and handling if root dir
    if (token1 == NULL)
    {
        free(path_copy);
        // printf("Path was just /");
        return 0;
    }

    while (token1 != NULL)
    {

        /*printf("\nCurrent directory contents:\n");
        for (int i = 0; i < sizeOfDE(current); i++) {
            if (current[i].is_used) {
                printf("  [%d] %s (dir: %d)\n", i, current[i].file_name, current[i].is_directory);
            }
        }*/

        char *token2 = strtok_r(NULL, "/", &saveptr);

        // printf("Processing token: %s (next: %s)\n", token1, token2 ? token2 : "NULL");

        // If next token is the last(token2) break look and save
        if (token2 == NULL)
        {
            strncpy(result->lastElement, token1, MAX_FILENAME - 1);
            result->lastElement[MAX_FILENAME - 1] = '\0';
            // printf("Last element: %s\n", result->lastElement);

            // Make sure current is loaded before searching
            if (current->ext_length > 0)
            {
                current = loadDir(current);
                if (!current)
                {
                    // printf("Failed to load parent directory for last element\n");
                    free(path_copy);
                    return -1;
                }
            }

            // Use FindDirHelper to find last element
            directory_entry *found = FindHelper(current, token1);
            if (found)
            {
                // Calculate index
                result->index = found - current;
                // printf("Found last element at index: %d\n", result->index);
            }
            else
            {
                // Entry not found
                result->index = -1;
                // printf("Last element not found (new entry)\n");
            }
            break;
        }

        // Handle if Dir name is . or ..
        if (strcmp(token1, ".") == 0)
        {
            token1 = token2;
            // printf("Skipping current directory (.)\n");
            continue;
        }

        if (strcmp(token1, "..") == 0)
        {
            // printf("Moving to parent directory (..)\n");
            if (parent != vcb->root_dir_ptr)
            { // Don't go up if already at root
                current = parent;
                if (current[1].ext_length > 0)
                { // ".." entry is always at index 1
                    parent = loadDir(&current[1]);
                    if (!parent)
                    {
                        free(path_copy);
                        return -1;
                    }
                }
            }
            token1 = token2;
            continue;
        }

        // Finder function to look for token2 and load dir

        directory_entry *next_dir = FindHelper(current, token1);
        if (next_dir == NULL || !next_dir->is_directory)
        {
            // printf("Failed to find/load directory: %s\n", token1);
            free(path_copy);
            return -1;
        }

        // Load the directory if it has extents
        if (next_dir->ext_length > 0)
        {
            directory_entry *loaded_dir = loadDir(next_dir);
            if (!loaded_dir)
            {
                // printf("Failed to load directory from LBA\n");
                free(path_copy);
                return -1;
            }
            parent = current;
            current = loaded_dir;
        }
        else 
        {
            parent = current;
            current = next_dir;
        } 

        token1 = token2;

    }

    result->retParent = current;

    free(path_copy);
    // printf("Path parsing complete. Index: %d\n\n", result->index);
    return 0;
}

/** Get current working directory
 * @returns the current working directory as a string in pathname
 * @author Danish Nguyen
 */
char *fs_getcwd(char *pathname, size_t size)
{
    strncpy(pathname, vcb->cwdStrPath, size); // copy CWD string with size limit
    return pathname;
}

/** Changes the current working directory to the new directory
 * Updates the current working directory, including the current load DE and cwd string path.
 * @return 0 on success, -1 on failure
 * @author Danish Nguyen
*/
int fs_setcwd(char* pathname) {
    /** Purpose of setcwd is to track user's current DE location
     * Before changing cwd, the following conditions must be met:
     * - The path must exist
     * - The index must not be -1
     * - The des must be a directory
     * 
     * Initially, cwdLoadedDE is set to NULL when user is located in root directory.
     * 
     * When user navigates to a different DE, cwdLoadedDE loads a new DE into memory
     * (root_dir_ptr should never freed)
     * When user navigates back to root DE, previously loaded DE is freed, 
     * and cwdLoadedDE is reset to NULL.
     */

    parsepath_st parser = { NULL, -1, "" };

    if (parsePath(pathname, &parser) != 0 || parser.index == -1) return -1;

    // If the last element exists but is not a directory, return failure
    if ( !parser.retParent[parser.index].is_directory ) return -1;
    
    int newDirLoc = parser.retParent[parser.index].extents->startLoc;

    // If navigating to another directory entry, ensure cwdLoadDE is not NULL and not pointing to root.
    // Free the old cwdLoadDE and load the new directory entry from disk.
    if (vcb->cwdLoadDE && ( vcb->cwdLoadDE != vcb->root_dir_ptr )) {
        freePtr((void**) &vcb->cwdLoadDE, "CWD Str Path");
        vcb->cwdLoadDE = NULL;
    }
    // If cwdLoadDE is root_dir_ptr or NULL, load new DE to cwdLoadDE
    vcb->cwdLoadDE = loadDir(&parser.retParent[parser.index]);
    
    // Create a pointer to point to old cwd string path
    char* oldStrPath = vcb->cwdStrPath;
    
    vcb->cwdStrPath = cleanPath(pathname);
    if (!vcb->cwdStrPath) return -1;

    freePtr((void**) &oldStrPath, "CWD Str Path");
    printf(">>> %s\n", vcb->cwdStrPath);
    
    return 0;
}

/** Sanitized a given path by processing each directory level, resolving any "." 
 * (current directory) or ".." (parent directory) references, and building a clean, 
 * absolute path. Result updates cwd path and stored in vcb->cwdStrPath
 * @anchor Danish Nguyen
 */
char* cleanPath(const char* srcPath) {
    char *tokens[MAX_PATH_LENGTH];  // Array to store tokens (DE's names)
    int curIdx = 0;  // Index of each DE's name in provied path
    
    // duplicate the path for tokenization
    char *pathCopy = (char*) calloc( sizeof(char), MAX_PATH_LENGTH);
    
    // Start with "/" for absolute paths or current working path
    if (srcPath[0] != '/') {
        strcpy(pathCopy, vcb->cwdStrPath);
        strncat(pathCopy, srcPath, (MAX_PATH_LENGTH - strlen(vcb->cwdStrPath) - 1) );
    } else {
        strncpy(pathCopy, srcPath, (MAX_PATH_LENGTH - 1) );
    }
    
    // Adjust memory for path string
    pathCopy = realloc(pathCopy, strlen(pathCopy));

    char *savePtr;
    char *token = strtok_r(pathCopy, "/", &savePtr); // Tokenize the path by '/'
    
    while (token != NULL) {
        if (strcmp(token, ".") == 0) { // ignore current directory

        } else if ((strcmp(token, "..") == 0)) {
            // parent - move up one level if possible
            if (curIdx > 0) curIdx--;
        
        } else {
            tokens[curIdx++] = token; // Add valid directory name to the stack
        }
        token = strtok_r(NULL, "/", &savePtr);
    }
    
    // Concatenate all tokens into a string
    char *newStrPath = malloc(MAX_PATH_LENGTH);
    if(newStrPath == NULL) {
        freePtr((void**) &pathCopy, "cleanPath - pathCopy");
        return NULL;
    }
   
    strcpy(newStrPath, "/");

    for (int i = 0; i < curIdx; i++) {
        strcat(newStrPath, tokens[i]);
        strcat(newStrPath, "/");  // add "/" between directories
    }

    freePtr((void**) &pathCopy, "cleanPath - pathCopy");
    return newStrPath;
}

/** Creates a new directory at the specified pathname with the given mode 
 * Validates the path, allocates a directory entry, updates directory metadata, and writes 
 * it back to disk.
 * @return 0 on success, -1 on failure
 * @author Danish Nguyen
 */
int fs_mkdir(const char *pathname, mode_t mode) {
    parsepath_st parser = { NULL, -1, "" };

    /** The path must be valid, and last index must be -1
     * indicating that there is no existing entry with the same name */
    if (parsePath(pathname, &parser) != 0) return -1;
    if ( parser.index != -1 ) {
        printf("Error - mkdir: \"%s\": File exists \n", parser.retParent[parser.index].file_name);
        return -1;
    }
    
    directory_entry *newDir = createDirectory(DIRECTORY_ENTRIES, parser.retParent);
    if (!newDir) return -1;

    int deIdx = makeDirOrFile(parser, 1, newDir);
    
    // Nomore entry availible in parent directory
    if (deIdx == -1) printf("Error - mkdir: Unable to create directory \n");

    /** Frees memory allocated for the new directory once the operation is complete.
        Writes the changes back to disk after updating. */
    freePtr((void**) &newDir, "DE msf.c");
    
    return (deIdx == -1) ? -1 : writeDirHelper(parser.retParent);
}

/** Deletes a file at a specified path
 * @return 0 on success, -1 on failure
 * @author Danish Nguyen
 */
int fs_delete(const char *filename)
{
    return deleteBlod(filename, 0);
}

/** Deletes a directory at a specified path,
 * @return 0 on success, -1 on failure
 * @author Danish Nguyen
 */
int fs_rmdir(const char *pathname)
{
    return deleteBlod(pathname, 1);
}

/** Checks if a given path corresponds to a directory
 * @returns 0 if is directory, -1 if not
 * @author Danish Nguyen
 */
int fs_isDir(char *path)
{
    parsepath_st parser = {NULL, -1, ""};
    int isValid = parsePath(path, &parser);

    if (isValid != 0 || parser.index < 0)
        return 0;
    return (parser.retParent[parser.index].is_directory);
}

/** Checks if a given path corresponds to a directory
 * @returns 0 if is file, -1 if not
 * @author Danish Nguyen
 */
int fs_isFile(char *path)
{
    return (!fs_isDir(path));
}

/**
 * The fs_stat Retrieves and populates file statistics for a given path.
 * The path of the file or directory to be queried. Also, pointer to a structure
 * where the file statistics will be stored.
 * @returns 0 if path is valid, -1 if path is not valid
 * @author Arvin Ghanizadeh
 */
int fs_stat(const char *path, struct fs_stat *buf)
{
    if (path == NULL || buf == NULL)
    {
        printf("Error: Path is invalid because it is NULL");
        return -1;
    }

    parsepath_st parser = {NULL, -1, ""};
    int isValid = parsePath(path, &parser);

    if (isValid != 0 || parser.retParent == NULL)
    {
        printf("Error: The Path is invalid %s\n", path);
        return -1;
    }

    buf->st_size = parser.retParent->file_size;

    buf->st_blksize = (blksize_t)4096; // this is 4 Kilobytes
    buf->st_blocks = (buf->st_size + MINBLOCKSIZE - 1) / MINBLOCKSIZE;

    buf->st_createtime = parser.retParent->creation_time;
    buf->st_modtime = parser.retParent->modification_time;
    buf->st_accesstime = parser.retParent->access_time;

    return 0;
}

/**
 * Opens a directory at pathname. Initializes tracked attributes of a directory
 * and returns a pointer to a new record in preparation for the file or directory's data.
 * @returns Pointer to fdDir or NULL if directory does not exist or error occurs.
 * @author Cheryl Fong
 */
fdDir *fs_opendir(const char *pathname)
{
    parsepath_st parser = {NULL, -1, ""};
    int isValid = parsePath(pathname, &parser);

    if (isValid != 0 || parser.retParent == NULL || !parser.retParent->is_directory)
    {
        printf("Error: no directory at %s\n", pathname);
        return NULL;
    }

    fdDir *dirp = (fdDir *)malloc(sizeof(fdDir));
    if (dirp == NULL)
    {
        printf("Error: fdDir malloc failed\n");
        return NULL;
    }

    dirp->d_reclen = sizeof(directory_entry);
    dirp->dirEntryPosition = 0;
    dirp->de = parser.retParent;
    dirp->di = NULL;

    return dirp;
}

/**
 * Retrieves the next directory entry in an "open" directory via dirp as input,
 * and returns the directory or file details.
 * @returns: Pointer to fs_diriteminfo or NULL when the end of the directory is
 * reached or an error occurs.
 * @author Cheryl Fong
 */
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{

    if (dirp == NULL || dirp->de == NULL || dirp->dirEntryPosition >= sizeOfDE(dirp->de))
    {
        return NULL;
    }
    directory_entry *currentEntry;

    // vcb->cwdLoadDE is only populated when CD command is invoked
    // otherwise use loaded Root  
    if( vcb->cwdLoadDE == NULL){
        currentEntry = &(dirp->de[dirp->dirEntryPosition]);
    }else{
        currentEntry = &(vcb->cwdLoadDE[dirp->dirEntryPosition]);
    }

    // ignore unused entries
    if (!currentEntry->is_used)
    {
        dirp->dirEntryPosition++;
        return fs_readdir(dirp);
    }

    // printf("CURR_DIR == %s\n",currentEntry->file_name);

    // malloc and populate the fs_diriteminfo structure
    if (dirp->di == NULL)
    {
        dirp->di = (struct fs_diriteminfo *)malloc(sizeof(struct fs_diriteminfo));
        if (dirp->di == NULL)
        {
            printf("Error: Memory allocation for fs_diriteminfo failed\n");
            return NULL;
        }
    }

    // copy data from file's or dir's directory_entry to fdDir structure used by displayFiles
    dirp->di->d_reclen = dirp->d_reclen;
    dirp->di->fileType = currentEntry->is_directory ? 'D' : 'F';
    strncpy(dirp->di->d_name, currentEntry->file_name, 255);
    dirp->di->d_name[255] = '\0';

    // move to next directory
    dirp->dirEntryPosition++;
    return dirp->di;
}

/**
 * Closes an "open" directory and frees associated resources by freeing the
 * pointer to the directory or file data record structure.
 * @returns: 0 if success otherwise -1 if an error occurs.
 * @author Cheryl Fong
 */
int fs_closedir(fdDir *dirp)
{
    if (dirp == NULL)
    {
        return -1;
    }

    if (dirp->di != NULL)
    {
        free(dirp->di);
    }

    free(dirp);
    return 0;
}


/** Create new file or directory
 * @returns index in parent DE on success, 0 on failure
 * @anchor Danish Nguyen
 */
int makeDirOrFile(parsepath_st parser, int isDir, directory_entry* newDir){
    /** Iterates through the parent directory to find an unused entry.
     * Updates that entry with the last element's name and new DE metadata.*/

    time_t curTime = time(NULL);
    
    for (int i = 0; i < sizeOfDE(parser.retParent); i++) {
        // Found unused directory entry
        if (!parser.retParent[i].is_used) {
            memset(parser.retParent[i].file_name, 0, MAX_FILENAME);
            strncpy(parser.retParent[i].file_name, parser.lastElement, MAX_FILENAME - 1);

            // Create new directory
            if (isDir && newDir) {
                memcpy(parser.retParent[i].extents, newDir->extents, newDir->ext_length * sizeof(extent_st));
                parser.retParent[i].ext_length = newDir->ext_length;
                parser.retParent[i].is_directory = 1;
                parser.retParent[i].file_size = newDir->file_size;
            
            // Create a new file
            } else {
                parser.retParent[i].is_directory = 0;
                parser.retParent[i].file_size = 0;
                parser.retParent[i].ext_length = 0;
            }
            parser.retParent[i].is_used = 1;
            parser.retParent[i].creation_time = curTime;
            parser.retParent[i].access_time = curTime;
            parser.retParent[i].modification_time = curTime;
            return i;
        }
    }
    return -1;
}

/** Iterate through each entry in the directory and count entries that are 
 * marked as "is_used"
 * @return true (non-zero) if the directory contains only "." and ".." entries.
 * @author Danish Nguyen
*/
int isDirEmpty(directory_entry *de) {
    int idx = 0;
    for (size_t i = 0; i < sizeOfDE(de); i++) {
        if (de[i].is_used) idx++;
    }
    return idx < 3;
}

/** The deleteBlod function deletes a file or directory at a specified path, 
 * ensuring file/directory exists, is the correct type, and is empty if it's a 
 * directory. It then releases any associated storage blocks and updates the 
 * parent directory's metadata.
 * @return 0 on success, -1 on failure
 * @author Danish Nguyen
*/
int deleteBlod(const char* pathname, int isDir) {
    parsepath_st parser = { NULL, -1, "" };

    if (parsePath(pathname, &parser) != 0) return -1;

    if (parser.index == -1) {
        printf("rm: %s: No such file or directory\n", parser.lastElement );
        return -1; // Can not remove not exist dir
    }

    if (parser.retParent[parser.index].is_directory != isDir) return -1;

    // If target is a directory, loaded to memory and check if it's empty
    if (isDir) { // isDir <=> 1
        directory_entry *removeDir = loadDir(&parser.retParent[parser.index]);
        
        // Ensure it can be loaded and is empty before deleting
        if ( !removeDir || !isDirEmpty(removeDir)) {
            printf("Cannot remove '%s': Is a directory and not empty\n", parser.retParent[parser.index].file_name);
            return -1;
        }
        freePtr((void**) &removeDir, "Free rm dir");
    }

    // Mark the target directory/file entry as unused in its parent metadata
    int status = removeDE(parser.retParent, parser.index, 0);

    // Update the parent directory on disk with the changes
    return (status == -1) ? -1 : writeDirHelper(parser.retParent);
}