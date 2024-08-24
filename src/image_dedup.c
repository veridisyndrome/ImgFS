#include "image_dedup.h"

#include <stdint.h> // for uint32_t
#include <string.h> // for strncmp

/**
 * @brief Check if the image at the corresponding index in the database is a duplicate, name wise or content wise
 *
 * @param imgfs_file (struct imgfs_file*): Given database
 * @param index (uint32_t): Given image index
 * @return (int): Error code
 */
int do_name_and_content_dedup(struct imgfs_file* imgfs_file, uint32_t index)
{
    M_REQUIRE_NON_NULL(imgfs_file);

    struct imgfs_header* header = &(imgfs_file->header);
    struct img_metadata* metadata = imgfs_file->metadata;

#define NB_IMGS header->max_files
    if (NB_IMGS <= index) {
        return ERR_IMAGE_NOT_FOUND;
    }

#define IMG_ID metadata[index].img_id
#define IMG_SHA metadata[index].SHA
    metadata[index].offset[ORIG_RES] = 0; // We assume that the index image has no duplicate
    for (size_t i = 0; i < NB_IMGS; ++i) {
        if (metadata[i].is_valid && (i != index)) {
            if (!strcmp(metadata[i].img_id, IMG_ID)) {
                return ERR_DUPLICATE_ID;
            }
            if (!memcmp(metadata[i].SHA, IMG_SHA, SHA256_DIGEST_LENGTH)) {
                // Another image have the same content but not the same image identifier
                for (size_t j = 0; j < NB_RES; ++j) {
                    // Correcting the duplication by making the index image the same has the i-index image
                    metadata[index].offset[j] = metadata[i].offset[j];
                    metadata[index].size[j] = metadata[i].size[j];
                }
            }
        }

    }

    return ERR_NONE;
}

