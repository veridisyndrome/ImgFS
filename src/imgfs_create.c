#include <string.h>
#include <stdlib.h>
#include "imgfs.h"
#include "error.h"

/**
 * @brief Create an empty database and allocate the appropriate memory
 *
 * @param imgfs_filename (const char*): Given name for the database
 * @param imgfs_file (struct imgfs_file*): Given database
 * @return (int): Error code
 */
int do_create(const char* imgfs_filename, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(imgfs_filename);
    M_REQUIRE_NON_NULL(imgfs_file);

    int ret = ERR_NONE; // Initializing the return value

    // Open the database file with the adequate mode (write and binary)
    FILE* pFile = fopen(imgfs_filename, "wb");
    if (pFile == NULL) {
        return ERR_IO;
    }

    strcpy(imgfs_file->header.name, CAT_TXT); // Modifying database name

    imgfs_file->header.version = 0; // No operation has been done on the database
    imgfs_file->header.nb_files = 0; // The database doesn't have any valid file

    uint32_t max_files = imgfs_file->header.max_files;

    // Allocating the memory of the database metadata for the maximum number of files that can be stored
    imgfs_file->metadata = calloc(max_files,
                                  sizeof(struct img_metadata)); // Initializing the database metadata

    if (imgfs_file->metadata == NULL) {
        do_close(imgfs_file);
        return ERR_OUT_OF_MEMORY;
    }

    imgfs_file->file = pFile;

    ret = (int) fwrite(&(imgfs_file->header)
                       , sizeof(struct imgfs_header), 1, pFile); // Writing the database header into the disk

    if(ret != 1) {
        do_close(imgfs_file);
        return ERR_IO;
    }

    ret = (int) fwrite(imgfs_file->metadata,
                       sizeof(struct img_metadata),
                       max_files, pFile); // Writing the database metadata into the disk

    if(ret != max_files) {
        do_close(imgfs_file);
        return ERR_IO;
    }

    printf("%d item(s) written\n", 1 + ret);

    return ERR_NONE;
}
