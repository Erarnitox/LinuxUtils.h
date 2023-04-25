#pragma once

#include <cstdarg>
#include <cstdlib>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ftw.h>
#include <fcntl.h>
#include <sys/wait.h>

static char* copy_to_path;
static char* copy_from_path;
static bool copy_busy = false;

int exec_command(char* exe, char* log_file, ...) {
    int pipe_fds[2];

    if(pipe(pipe_fds)) return -1;

    pid_t pid = fork();

    if(pid == -1) return -1;

    if(pid == 0) {
        char* args[32] = {0};

        dup2(pipe_fds[1], STDOUT_FILENO);
        dup2(pipe_fds[1], STDERR_FILENO);

        close(pipe_fds[1]);

        va_list valist;
        va_start(valist, log_file);

        char* p = 0;
        for(int i=1; p = va_arg(valist, char*); ++i) {
            if( i > 32) break;

            args[i] = p;
        }
        va_end(valist);
        execvp(exe, args);
        exit(-1);
    }
    
    char read_buffer[1024];
    size_t r = 0;
    int log_fd = 0;

    close(pipe_fds[1]);

    if(log_file) {
        log_fd = open(log_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    }

    while((r = read(pipe_fds[0], read_buffer, sizeof(read_buffer)))) {
        write(STDOUT_FILENO, read_buffer, r);

        if(log_fd)
            write(log_fd, read_buffer, r);
    }

    if(log_fd)
        close(log_fd);

    close(pipe_fds[0]);

    int status = -1;
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
}

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


