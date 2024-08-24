#include "imgfs.h"
#include "util.h"
#include "error.h"
#include "http_prot.h"
#include <json-c/json.h>
#include <string.h>

/**
 * @brief
 *
 * @param imgfs_file (const struct imgfs_file*): Given database
 * @param output_mode (enum do_list_mode): Given output mode
 * @param json (char**):
 * @return (int): Error code
 */
int do_list(const struct imgfs_file* imgfs_file,
            enum do_list_mode output_mode, char** json)
{
    // Checking that the arguments are valid
    M_REQUIRE_NON_NULL(imgfs_file);

    uint32_t max_files = imgfs_file->header.max_files;

    if (output_mode >= NB_DO_LIST_MODES) {
        return ERR_INVALID_ARGUMENT;
    }

    if (output_mode == STDOUT) {
        const struct imgfs_header* header = &(imgfs_file->header);
        print_header(header);

        if (!header->nb_files) {
#define EMPTY_PRINT "<< empty imgFS >>\n"
            printf(EMPTY_PRINT);
        } else {
            const struct img_metadata* metadata = imgfs_file->metadata;

            for (uint32_t i = 0; i < max_files; ++i) {
                if (metadata[i].is_valid) {
                    print_metadata(&metadata[i]);
                }
            }
        }

    } else {
        struct json_object* img_id_array = json_object_new_array();
        for (uint32_t i = 0; i < max_files; ++i) {
            if (imgfs_file->metadata[i].is_valid) {
                json_object* metadata_item = json_object_new_string(imgfs_file->metadata[i].img_id);
                json_object_get(metadata_item);
                json_object_array_add(img_id_array, metadata_item);
                json_object_put(metadata_item);
            }
        }

        struct json_object* json_obj = json_object_new_object();
        if(json_object_object_add(json_obj, "Images", img_id_array) < 0) {
            return ERR_RUNTIME;
        }

        *json = strdup(json_object_to_json_string(json_obj));
        json_object_put(json_obj);
    }

    return ERR_NONE;
}



