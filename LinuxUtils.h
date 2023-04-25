#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ftw.h>

static char* copy_to_path;
static char* copy_from_path;
static bool copy_busy = false;

bool copy_file(const char* from, char* to) {
    FILE* ff = fopen(from, "r");
    if(!ff) return false;

    FILE* ft = fopen(to, "w");
    if(!ft) return false;

    char buffer[4 * 1024];
    size_t r = 0;

    while(r = fread(buffer, 1, sizeof(buffer), ff)){
        fwrite(buffer, 1, r, ft);
    }

    fclose(ft);
    fclose(ff);

    return true; 
}

static int cp_callback(const char* fpath, const struct stat* sb, int type_flag, struct FTW* ftwbuf) {
    char to_location[1024];
    sprintf(to_location, "%s/%s", copy_to_path, fpath + strlen(copy_from_path) + 1);

    if(type_flag & FTW_D) {
        if(ftwbuf->level == 0) return 0;
        if(mkdir(to_location, 0775)){
            return -1;
        }
    } else {
       if(!copy_file(fpath, to_location)) return -1;
    }

    return 0;
}

bool copy_dir_contents(char* path, char* to) {
    if(copy_busy) return false;

    copy_busy = true;
    copy_to_path = to;
    copy_from_path = path;

    int ret = nftw(path, cp_callback, 64, FTW_PHYS);

    copy_busy = false;
    return ret == 0;
}
