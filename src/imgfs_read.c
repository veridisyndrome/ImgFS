#include <string.h>
#include <stdlib.h>
#include "imgfs.h"
#include "error.h"
#include "image_content.h"

/**
 * @brief Reads the content of an image from a imgFS.
 *
 * @param img_id The ID of the image to be read.
 * @param resolution The desired resolution for the image read.
 * @param image_buffer Location of the location of the image content
 * @param image_size Location of the image size variable
 * @param imgfs_file The main in-memory data structure
 * @return Some error code. 0 if no error.
 */
int do_read(const char* img_id, int resolution, char** image_buffer,
            uint32_t* image_size, struct imgfs_file* imgfs_file)
{

    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);
    M_REQUIRE_NON_NULL(imgfs_file);

    if (resolution >= NB_RES) {
        return ERR_RESOLUTIONS;
    }

    int ret = ERR_IMAGE_NOT_FOUND;
    int index = 0;

    uint32_t max_files = imgfs_file->header.max_files;
    for(int i=0; i < max_files; i++) {
        if (imgfs_file->metadata[i].is_valid && !strcmp(img_id, imgfs_file->metadata[i].img_id)) {
            index = i;
            ret = ERR_NONE;
        }
    }

    if (ret == ERR_IMAGE_NOT_FOUND) {
        return ret;
    }

    if (resolution != ORIG_RES) {

        if(!imgfs_file->metadata[index].offset[resolution] ||
           !imgfs_file->metadata[index].size[resolution]) {

            ret = lazily_resize(resolution, imgfs_file, index);

            if (ret != ERR_NONE) {
                return ret;
            }

        }
    }


    fseek(imgfs_file->file, (long) imgfs_file->metadata[index].offset[resolution], SEEK_SET);

    *image_buffer = calloc(1, imgfs_file->metadata[index].size[resolution]);
    if (*image_buffer == NULL) {
        return ERR_OUT_OF_MEMORY;
    }


    ret = (int) fread(*image_buffer, imgfs_file->metadata[index].size[resolution], 1, imgfs_file->file);
    if (ret != 1) {
        free(*image_buffer);
        *image_buffer = NULL;
        return ERR_IO;
    }

    *image_size = imgfs_file->metadata[index].size[resolution];

    return ERR_NONE;
}



