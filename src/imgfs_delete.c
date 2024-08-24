#include <string.h>
#include "imgfs.h"
#include "error.h"

/**
 * @brief Delete the image corresponding to the given image identifier in the database
 *
 * @param img_id (const char*): Given image identifier
 * @param imgfs_file (struct imgfs_file*): Given database
 * @return (int): Error code
 */
int do_delete(const char* img_id, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(img_id);

    int ret = ERR_NONE; // Initializing the return value

    if (!imgfs_file->header.nb_files) {
        do_close(imgfs_file);
        return ERR_IMAGE_NOT_FOUND;
    }

    struct imgfs_header* header = &(imgfs_file->header);
    struct img_metadata* metadata = imgfs_file->metadata;

    uint32_t max_imgs = imgfs_file->header.max_files;
    for (size_t i = 0; i < max_imgs; ++i) {
        if(!strcmp(img_id, metadata[i].img_id) && metadata[i].is_valid) {
            metadata[i].is_valid = EMPTY; // Invalidating the corresponding image

            // Seeking the file to the corresponding i-index image metadata
            fseek(imgfs_file->file, sizeof(struct imgfs_header) + i * sizeof(struct img_metadata),
                  SEEK_SET);

            ret = (int) fwrite(&metadata[i], sizeof(struct img_metadata),
                               1, imgfs_file->file); // Writing the image new metadata

            if (ret != 1) {
                do_close(imgfs_file);
                return ERR_IO;
            }

            header->version++; header->nb_files--; // Updating the header
            // Seeking the file to the database header
            fseek(imgfs_file->file, 0, SEEK_SET);
            ret = (int) fwrite(header, sizeof(struct imgfs_header),
                               1, imgfs_file->file); // Writing the image new header

            if (ret != 1) {
                do_close(imgfs_file);
                return ERR_IO;
            }

            return ERR_NONE;
        }
    }

    do_close(imgfs_file);
    return ERR_IMAGE_NOT_FOUND;
}
