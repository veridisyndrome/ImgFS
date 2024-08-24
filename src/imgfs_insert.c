#include <string.h>
#include "imgfs.h"
#include "error.h"
#include "image_content.h"
#include "image_dedup.h"

/**
 * @brief Insert image in the imgFS file
 *
 * @param buffer Pointer to the raw image content
 * @param size Image size
 * @param img_id Image ID
 * @return Some error code. 0 if no error.
 */
int do_insert(const char* image_buffer, size_t image_size,
              const char* img_id, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);

    if (imgfs_file->header.nb_files >= imgfs_file->header.max_files) {
        return ERR_IMGFS_FULL;
    }

    int ret = ERR_NONE;

    struct imgfs_header* header = &(imgfs_file->header);
    struct img_metadata* metadata = imgfs_file->metadata;


    for (int i = 0; i < header->max_files; ++i) {
        if (!metadata[i].is_valid) {
            memset(&metadata[i], 0, sizeof(struct img_metadata));
            SHA256((const unsigned char*) image_buffer, image_size, metadata[i].SHA);
            strcpy(metadata[i].img_id, img_id);
            metadata[i].size[ORIG_RES] = (uint32_t) image_size;
            ret = get_resolution(&metadata[i].orig_res[1], &metadata[i].orig_res[0], image_buffer, image_size);
            if (ret != ERR_NONE) {
                return ret;
            }

            ret = do_name_and_content_dedup(imgfs_file, i);
            if (ret != ERR_NONE) {
                return ret;
            }

            fseek(imgfs_file->file, 0, SEEK_END);
            long res_offset = ftell(imgfs_file->file); // Storing the image offset value
            if (!metadata[i].offset[ORIG_RES]) {

                ret = (int) fwrite(image_buffer, image_size, 1, imgfs_file->file); // Writing the resized image
                if (ret != 1) {
                    return ERR_IO;
                }
                // Updating image offset value
                metadata[i].offset[ORIG_RES] = res_offset;
            }

            // Updating image metadata
            metadata[i].size[ORIG_RES] = image_size;
            metadata[i].is_valid = NON_EMPTY;

            header->version++; header->nb_files++;
            fseek(imgfs_file->file, 0, SEEK_SET);
            ret = (int) fwrite(header, sizeof(struct imgfs_header), 1, imgfs_file->file); // Writing the image new header
            if (ret != 1) {
                return ERR_IO;
            }

#define OFFSET_METADATA_IMAGE sizeof(struct imgfs_header) + i * sizeof(struct img_metadata)
            // Seeking the file to the corresponding image metadata
            fseek(imgfs_file->file, OFFSET_METADATA_IMAGE, SEEK_SET);
            ret = (int) fwrite(&metadata[i], sizeof(struct img_metadata), 1, imgfs_file->file); // Writing the image new metadata

            return ret != 1 ? ERR_IO : ERR_NONE;
        }
    }

    return ERR_IMGFS_FULL;
}
