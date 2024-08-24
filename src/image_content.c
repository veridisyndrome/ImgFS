#include <stdlib.h>
#include <vips/vips.h>
#include "image_content.h"


/**
 * @brief Resize the corresponding image at the index in the requested resolution
 *
 * @param resolution (int): Given resolution
 * @param imgfs_file (struct imgfs_file*): Given database
 * @param index (size_t): Given image index in the corresponding database
 * @return (int): Error code
 */
int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index)
{
    M_REQUIRE_NON_NULL(imgfs_file);
    if (resolution < 0 || NB_RES <= resolution) {
        return ERR_RESOLUTIONS;
    }
    uint32_t max_files = imgfs_file->header.max_files;
    if (max_files <= index || !imgfs_file->metadata[index].is_valid) {
        return ERR_INVALID_IMGID;
    }

    int ret = ERR_NONE; // Initializing the return value

    struct imgfs_header* header = &(imgfs_file->header);
    struct img_metadata* metadata = imgfs_file->metadata;

#define RES_ALREADY_IMAGE metadata[index].offset[resolution]
    // If the image already exist in the requested resolution, we do nothing
    if (resolution == ORIG_RES || RES_ALREADY_IMAGE) {
        return ret;
    }
    if (header->nb_files > max_files) {
        return ERR_INVALID_IMGID;
    }

#define SIZE_IMAGE metadata[index].size[ORIG_RES]
    void* buf_orig = calloc(1, SIZE_IMAGE); // Initializing the original image buffer
    if (buf_orig == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

#define OFFSET_ORIG_IMAGE metadata[index].offset[ORIG_RES]
    // Seeking the file to the corresponding origin image
    fseek(imgfs_file->file,(long)OFFSET_ORIG_IMAGE,SEEK_SET);

    ret = (int) fread(buf_orig, SIZE_IMAGE, 1, imgfs_file->file); // Reading the original image
    if (ret != 1) {
        free(buf_orig);
        buf_orig = NULL;
        return ERR_IO;
    }

    // Initializing VIPS original and resized image
    VipsImage* orig_img = NULL;
    VipsImage* resized_img = NULL;

    ret = vips_jpegload_buffer(buf_orig, SIZE_IMAGE, &orig_img, NULL); // Loading the image from the corresponding buffer define above
    if (ret == -1) {
        g_object_unref(VIPS_OBJECT(orig_img));
        orig_img = NULL;
        g_object_unref(VIPS_OBJECT(resized_img));
        resized_img = NULL;
        free(buf_orig);
        buf_orig = NULL;
        return ERR_IMGLIB;
    }

#define WIDTH_RES_IMAGE header->resized_res[2 * resolution]
#define HEIGHT_RES_IMAGE header->resized_res[2 * resolution + 1]

    // Resizing the image with the desired parameters

    ret = vips_thumbnail_image(orig_img, &resized_img, WIDTH_RES_IMAGE,
                               "height", HEIGHT_RES_IMAGE,
                               NULL);

    if (ret == -1) {
        g_object_unref(VIPS_OBJECT(orig_img));
        orig_img = NULL;
        g_object_unref(VIPS_OBJECT(resized_img));
        resized_img = NULL;
        free(buf_orig);
        buf_orig = NULL;
        return ERR_IMGLIB;
    }

    void* buf_resized = NULL; // Initializing the resized image buffer

    size_t len = 0;

    ret = vips_jpegsave_buffer(resized_img, &buf_resized, &len, NULL); // Saving the image from the corresponding buffer define above
    if (ret == -1) {
        g_object_unref(VIPS_OBJECT(orig_img));
        orig_img = NULL;
        g_object_unref(VIPS_OBJECT(resized_img));
        resized_img = NULL;
        free(buf_orig);
        buf_orig = NULL;
        return ERR_IMGLIB;
    }

    // Seeking the file to the end of the file
    fseek(imgfs_file->file, 0, SEEK_END);
    long res_offset = ftell(imgfs_file->file); // Storing the resized image offset value

    ret = (int) fwrite(buf_resized, len, 1, imgfs_file->file); // Writing the resized image
    if (ret != 1) {
        g_object_unref(VIPS_OBJECT(orig_img));
        orig_img = NULL;
        g_object_unref(VIPS_OBJECT(resized_img));
        resized_img = NULL;
        free(buf_orig);
        buf_orig = NULL;
        g_free(buf_resized);
        buf_resized = NULL;
        return ERR_IO;
    }

    // Updating image metadata
    metadata[index].size[resolution] = len;
    metadata[index].offset[resolution] = res_offset;

    // Seeking the file to the corresponding image metadata
    fseek(imgfs_file->file, sizeof(struct imgfs_header) + index * sizeof(struct img_metadata), SEEK_SET);

    ret = (int) fwrite(&metadata[index], sizeof(struct img_metadata),
                       1, imgfs_file->file); // Writing the image new metadata

    // Freeing every pointer
    g_object_unref(VIPS_OBJECT(orig_img));
    orig_img = NULL;
    g_object_unref(VIPS_OBJECT(resized_img));
    resized_img = NULL;
    free(buf_orig);
    buf_orig = NULL;
    g_free(buf_resized);
    buf_resized = NULL;

    return ret != 1 ? ERR_IO : ERR_NONE;
}

/**
 * @brief Gets the resolution of an image.
 *
 * @param height Where to put the calculated image height.
 * @param width Where to put the calculated image width.
 * @param filename The image file name.
 * @return Some error code. 0 if no error.
 */
int get_resolution(uint32_t *height, uint32_t *width,
                   const char *image_buffer, size_t image_size)
{
    M_REQUIRE_NON_NULL(height);
    M_REQUIRE_NON_NULL(width);
    M_REQUIRE_NON_NULL(image_buffer);

    VipsImage* original = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const int err = vips_jpegload_buffer((void*) image_buffer, image_size,
                                         &original, NULL);
#pragma GCC diagnostic pop
    if (err != ERR_NONE) return ERR_IMGLIB;

    *height = (uint32_t) vips_image_get_height(original);
    *width  = (uint32_t) vips_image_get_width (original);

    g_object_unref(VIPS_OBJECT(original));
    return ERR_NONE;
}
