/**
 * @file imgfscmd_functions.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// default values
static const uint32_t default_max_files = 128;
static const uint16_t default_thumb_res =  64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;


static void create_name(const char* img_id, int resolution, char** new_name);
static int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size);
static int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size);

/**********************************************************************
 * Displays some explanations.
 ********************************************************************** */
int help()
{
    printf("imgfscmd [COMMAND] [ARGUMENTS]\n"
           "  help: displays this help.\n"
           "  list <imgFS_filename>: list imgFS content.\n"
           "  create <imgFS_filename> [options]: create a new imgFS.\n"
           "      options are:\n"
           "          -max_files <MAX_FILES>: maximum number of files.\n"
           "                                  default value is %d\n"
           "                                  maximum value is 4294967295\n"
           "          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "          -small_res <X_RES> <Y_RES>: resolution for small images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "  read   <imgFS_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n"
           "      read an image from the imgFS and save it to a file.\n"
           "      default resolution is \"original\".\n"
           "  insert <imgFS_filename> <imgID> <filename>: insert a new image in the imgFS.\n"
           "  delete <imgFS_filename> <imgID>: delete image imgID from imgFS.\n",
           default_max_files, default_thumb_res, default_thumb_res, MAX_THUMB_RES, MAX_THUMB_RES,
           default_small_res, default_small_res, MAX_SMALL_RES, MAX_SMALL_RES);
    return ERR_NONE;
}

/**********************************************************************
 * Opens imgFS file and calls do_list().
 ********************************************************************** */
int do_list_cmd(int argc, char** argv)
{
    M_REQUIRE_NON_NULL(argv);

    if (argc != 1) {
        return ERR_INVALID_COMMAND;
    }

    int ret = ERR_NONE;

    struct imgfs_file db = {0};
    ret = do_open(argv[0], "rb", &db);
    if (ret == ERR_NONE) {
        ret = do_list(&db, STDOUT, NULL);
        do_close(&db);
    }
    return ret;
}

/**********************************************************************
 * Prepares and calls do_create command.
********************************************************************** */
int do_create_cmd(int argc, char** argv)
{
    M_REQUIRE_NON_NULL(argv);

    int ret = ERR_NONE;

    if (argc < 1) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    struct imgfs_file db = {0};

    db.header.max_files = default_max_files;

    for (int i=0; i < (NB_RES - 1); i++) {
        db.header.resized_res[2 * THUMB_RES + i] = default_thumb_res;
        db.header.resized_res[2 * SMALL_RES + i] = default_small_res;
    }

    // retrieve the mandatory argument
    const char* filename = argv[0];
    argc--; argv++;


    while(argc > 0) {
        if (!strcmp(argv[0], "-max_files")) {

            // skip -max_files flag
            argc--; argv++;

            if(argc < 1) {
                filename = NULL;
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            uint32_t valid_max_files = atouint32(argv[0]);
            if(!valid_max_files) {
                filename = NULL;
                return ERR_MAX_FILES;
            }
            db.header.max_files = valid_max_files;

            // skip flag parameter(s)
            argc--; argv++;

        } else if (!strcmp(argv[0], "-thumb_res")) {

            // skip -thumb_res flag
            argc--; argv++;

            if (argc < 2) {
                filename = NULL;
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            uint16_t valid_thumb_res = 0;

            for (int i=0; i < 2; i++) {

                valid_thumb_res = atouint16(argv[i]);
                if (!(valid_thumb_res) || (valid_thumb_res > MAX_THUMB_RES))  {
                    filename = NULL;
                    return ERR_RESOLUTIONS;
                }
                db.header.resized_res[2 * THUMB_RES + i] = valid_thumb_res;
            }
            // skip flag parameter(s)
            argc -= 2; argv += 2;

        } else if (!strcmp(argv[0], "-small_res")) {

            // skip -small_res flag
            argc--; argv++;

            if (argc < 2) {
                filename = NULL;
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }

            uint16_t valid_small_res = 0;

            for (int i=0; i < 2; i++) {

                valid_small_res = atouint16(argv[i]);
                if (!(valid_small_res) || (valid_small_res > MAX_SMALL_RES) ) {
                    filename = NULL;
                    return ERR_RESOLUTIONS;
                }
                db.header.resized_res[2 * SMALL_RES + i] = valid_small_res;
            }

            // skip flag parameter(s)
            argc -= 2; argv += 2;

        } else {
            filename = NULL;
            return ERR_INVALID_ARGUMENT;
        }
    }

    ret = do_create(filename, &db);
    filename = NULL;

    if (ret == ERR_NONE) {
        do_close(&db);
    }

    return ret;
}

/**********************************************************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char** argv)
{
    M_REQUIRE_NON_NULL(argv);

    int ret = ERR_NONE;

    if (argc < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    // check if the image id is valid
    if ((strlen(argv[1]) == 0) || (strlen(argv[1]) > MAX_IMG_ID)) {
        return ERR_INVALID_IMGID;
    }

    struct imgfs_file db;
    ret = do_open(argv[0], "rb+", &db);
    if(ret == ERR_NONE) {
        ret = do_delete(argv[1], &db);
        if (ret == ERR_NONE) {
            do_close(&db);
        }
    }
    return ret;
}

/********************************************************************
 * Reads an image from the imgFS.
 *******************************************************************/
int do_read_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 2 && argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    const char * const img_id = argv[1];

    const int resolution = (argc == 3) ? resolution_atoi(argv[2]) : ORIG_RES;
    if (resolution == -1) return ERR_RESOLUTIONS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size = 0;
    error = do_read(img_id, resolution, &image_buffer, &image_size, &myfile);
    do_close(&myfile);
    if (error != ERR_NONE) {
        return error;
    }

    // Extracting to a separate image file.
    char* tmp_name = NULL;
    create_name(img_id, resolution, &tmp_name);
    if (tmp_name == NULL) return ERR_OUT_OF_MEMORY;
    error = write_disk_image(tmp_name, image_buffer, image_size);
    free(tmp_name);
    free(image_buffer);

    return error;
}


/********************************************************************
 * Inserts an image into the imgFS.
 *******************************************************************/
int do_insert_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size;

    // Reads image from the disk.
    error = read_disk_image(argv[2], &image_buffer, &image_size);
    if (error != ERR_NONE) {
        do_close(&myfile);
        return error;
    }
    error = do_insert(image_buffer, image_size, argv[1], &myfile);
    free(image_buffer);
    do_close(&myfile);
    return error;
}


/********************************************************************
 * Writes name of the file used for image reading in a buffer
 *******************************************************************/
void create_name(const char *img_id, int resolution, char **new_name)
{

    if (!img_id) {
        return;
    }

#define EXT_SUFFIX ".jpg"
    const char* suffixes[] = {"_orig", "_small", "_thumb"};

    char* suffix = (char*) suffixes[resolution];
    *new_name = calloc(1, strlen(img_id) + strlen(suffix) + strlen(EXT_SUFFIX) + 1);
    if (*new_name == NULL) {
        return;
    }
    strcpy(*new_name, img_id);
    strcat(*new_name, suffix);
    strcat(*new_name, EXT_SUFFIX);
}

/********************************************************************
 * Writes content of image buffer to disk
 *******************************************************************/
int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(image_buffer);

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return ERR_IO;
    }

    return (fwrite(image_buffer, image_size, 1, file) != 1) ? ERR_IO : ERR_NONE;
}

/********************************************************************
 * Reads an image from disk
 *******************************************************************/
int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return ERR_IO;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    *image_buffer = calloc(file_size, 1);

    if (*image_buffer == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    fseek(file, 0, SEEK_SET);
    int ret = (int) fread(*image_buffer, file_size, 1, file);
    if (ret != 1) {
        free(*image_buffer);
        *image_buffer = NULL;
        return ERR_IO;
    }

    *image_size = file_size;

    return ERR_NONE;
}
