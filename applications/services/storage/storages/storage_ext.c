#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <esp_littlefs.h>
#include <esp_partition.h>

#include "storage_ext.h"
#include "../filesystem_api_internal.h"
#include "../storage_internal_dirname_i.h"

#define TAG "StorageExt"
#define INTERNAL_FS_BASE "/ext"

typedef struct {
    bool mounted;
} StorageExtData;

typedef struct {
    FILE* file;
} ExtFile;

typedef struct {
    DIR* dir;
} ExtDir;

static FS_Error ext_errno_to_error(void) {
    switch(errno) {
    case ENOENT:
        return FSE_NOT_EXIST;
    case EEXIST:
        return FSE_EXIST;
    case EACCES:
    case EPERM:
        return FSE_DENIED;
    case ENOTDIR:
    case EISDIR:
        return FSE_INVALID_PARAMETER;
    default:
        return FSE_INTERNAL;
    }
}

static void ext_make_path(char* out, size_t out_size, const char* path) {
    if(path[0] == '/') {
        snprintf(out, out_size, INTERNAL_FS_BASE "%s", path);
    } else {
        snprintf(out, out_size, INTERNAL_FS_BASE "/%s", path);
    }
}

static bool ext_mount(void) {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = INTERNAL_FS_BASE,
        .partition_label = "littlefs",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if(err == ESP_ERR_INVALID_STATE) {
        return true;
    }
    if(err != ESP_OK) {
        FURI_LOG_E(TAG, "LittleFS mount failed: %s", esp_err_to_name(err));
        return false;
    }

    size_t total = 0;
    size_t used = 0;
    if(esp_littlefs_info("littlefs", &total, &used) == ESP_OK) {
        FURI_LOG_I(TAG, "Internal storage mounted: %u/%u bytes used", (unsigned)used, (unsigned)total);
    }

    return true;
}

static bool storage_ext_file_open(
    void* ctx,
    File* file,
    const char* path,
    FS_AccessMode access_mode,
    FS_OpenMode open_mode) {
    StorageData* storage = ctx;
    char full_path[256];
    char mode[4] = "rb";

    ext_make_path(full_path, sizeof(full_path), path);

    if(open_mode & FSOM_CREATE_NEW) {
        if(access_mode & FSAM_WRITE) {
            strcpy(mode, "wb");
        }
        if(access_mode == FSAM_READ_WRITE) {
            strcpy(mode, "w+b");
        }
    } else if(open_mode & FSOM_CREATE_ALWAYS) {
        strcpy(mode, access_mode == FSAM_READ_WRITE ? "w+b" : "wb");
    } else if(open_mode & FSOM_OPEN_APPEND) {
        strcpy(mode, access_mode == FSAM_READ_WRITE ? "a+b" : "ab");
    } else if(access_mode == FSAM_READ_WRITE) {
        strcpy(mode, "r+b");
    } else if(access_mode & FSAM_WRITE) {
        strcpy(mode, "r+b");
    } else {
        strcpy(mode, "rb");
    }

    if(open_mode & FSOM_CREATE_NEW) {
        struct stat st;
        if(stat(full_path, &st) == 0) {
            file->error_id = FSE_EXIST;
            return false;
        }
    }

    ExtFile* data = calloc(1, sizeof(ExtFile));
    data->file = fopen(full_path, mode);

    if(!data->file && (open_mode & FSOM_OPEN_ALWAYS) && (access_mode & FSAM_WRITE)) {
        strcpy(mode, access_mode == FSAM_READ_WRITE ? "w+b" : "wb");
        data->file = fopen(full_path, mode);
    }

    if(!data->file) {
        file->error_id = ext_errno_to_error();
        free(data);
        return false;
    }


    storage_set_storage_file_data(file, data, storage);
    file->error_id = FSE_OK;
    return true;
}

static bool storage_ext_file_close(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    int rc = data && data->file ? fclose(data->file) : -1;
    free(data);
    storage_set_storage_file_data(file, NULL, storage);
    file->error_id = rc == 0 ? FSE_OK : FSE_INTERNAL;
    return rc == 0;
}

static uint16_t storage_ext_file_read(
    void* ctx, File* file, void* buff, uint16_t bytes_to_read) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    size_t n = data && data->file ? fread(buff, 1, bytes_to_read, data->file) : 0;
    file->error_id = (n || !ferror(data->file)) ? FSE_OK : FSE_INTERNAL;
    return (uint16_t)n;
}

static uint16_t storage_ext_file_write(
    void* ctx, File* file, const void* buff, uint16_t bytes_to_write) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    size_t n = data && data->file ? fwrite(buff, 1, bytes_to_write, data->file) : 0;
    file->error_id = n == bytes_to_write ? FSE_OK : FSE_INTERNAL;
    return (uint16_t)n;
}

static bool storage_ext_file_seek(
    void* ctx, File* file, uint32_t offset, bool from_start) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    int rc = fseek(data->file, (long)offset, from_start ? SEEK_SET : SEEK_CUR);
    file->error_id = rc == 0 ? FSE_OK : FSE_INTERNAL;
    return rc == 0;
}

static uint64_t storage_ext_file_tell(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    long pos = ftell(data->file);
    file->error_id = pos >= 0 ? FSE_OK : FSE_INTERNAL;
    return pos >= 0 ? (uint64_t)pos : 0;
}

static bool storage_ext_file_truncate(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    long pos = ftell(data->file);
    if(pos < 0 || fflush(data->file) != 0) {
        file->error_id = FSE_INTERNAL;
        return false;
    }
    int fd = fileno(data->file);
    int rc = ftruncate(fd, (off_t)pos);
    file->error_id = rc == 0 ? FSE_OK : FSE_INTERNAL;
    return rc == 0;
}

static uint64_t storage_ext_file_size(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    long pos = ftell(data->file);
    if(pos < 0) {
        file->error_id = FSE_INTERNAL;
        return 0;
    }
    fseek(data->file, 0, SEEK_END);
    long size = ftell(data->file);
    fseek(data->file, pos, SEEK_SET);
    file->error_id = size >= 0 ? FSE_OK : FSE_INTERNAL;
    return size >= 0 ? (uint64_t)size : 0;
}

static bool storage_ext_file_sync(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    int rc = fflush(data->file);
    file->error_id = rc == 0 ? FSE_OK : FSE_INTERNAL;
    return rc == 0;
}

static bool storage_ext_file_eof(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtFile* data = storage_get_storage_file_data(file, storage);
    file->error_id = FSE_OK;
    return feof(data->file) != 0;
}

static bool storage_ext_dir_open(void* ctx, File* file, const char* path) {
    StorageData* storage = ctx;
    char full_path[256];
    ext_make_path(full_path, sizeof(full_path), path);

    ExtDir* data = calloc(1, sizeof(ExtDir));
    data->dir = opendir(full_path);
    if(!data->dir) {
        file->error_id = ext_errno_to_error();
        free(data);
        return false;
    }

    storage_set_storage_file_data(file, data, storage);
    file->error_id = FSE_OK;
    return true;
}

static bool storage_ext_dir_close(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtDir* data = storage_get_storage_file_data(file, storage);
    int rc = data && data->dir ? closedir(data->dir) : -1;
    free(data);
    storage_set_storage_file_data(file, NULL, storage);
    file->error_id = rc == 0 ? FSE_OK : FSE_INTERNAL;
    return rc == 0;
}

static bool storage_ext_dir_read(
    void* ctx,
    File* file,
    FileInfo* fileinfo,
    char* name,
    uint16_t name_length) {
    StorageData* storage = ctx;
    ExtDir* data = storage_get_storage_file_data(file, storage);
    struct dirent* entry;

    do {
        entry = readdir(data->dir);
        if(!entry) {
            file->error_id = FSE_NOT_EXIST;
            return false;
        }
    } while(entry->d_name[0] == '.' && entry->d_name[1] == '_');

    if(fileinfo) {
        fileinfo->flags = 0;
        fileinfo->size = 0;

        char full_path[256];
        snprintf(full_path, sizeof(full_path), INTERNAL_FS_BASE "/%s", entry->d_name);

        struct stat st;
        if(stat(full_path, &st) == 0) {
            fileinfo->size = (uint64_t)st.st_size;
            if(S_ISDIR(st.st_mode)) fileinfo->flags |= FSF_DIRECTORY;
        }
    }

    if(name && name_length) {
        snprintf(name, name_length, "%s", entry->d_name);
    }

    file->error_id = FSE_OK;
    return true;
}

static bool storage_ext_dir_rewind(void* ctx, File* file) {
    StorageData* storage = ctx;
    ExtDir* data = storage_get_storage_file_data(file, storage);
    rewinddir(data->dir);
    file->error_id = FSE_OK;
    return true;
}

static FS_Error storage_ext_common_stat(void* ctx, const char* path, FileInfo* fileinfo) {
    UNUSED(ctx);
    char full_path[256];
    struct stat st;
    ext_make_path(full_path, sizeof(full_path), path);

    if(stat(full_path, &st) != 0) return ext_errno_to_error();

    if(fileinfo) {
        fileinfo->size = (uint64_t)st.st_size;
        fileinfo->flags = S_ISDIR(st.st_mode) ? FSF_DIRECTORY : 0;
    }
    return FSE_OK;
}

static FS_Error storage_ext_common_remove(void* ctx, const char* path) {
    UNUSED(ctx);
    char full_path[256];
    ext_make_path(full_path, sizeof(full_path), path);
    return (remove(full_path) == 0) ? FSE_OK : ext_errno_to_error();
}

static FS_Error storage_ext_common_mkdir(void* ctx, const char* path) {
    UNUSED(ctx);
    char full_path[256];
    ext_make_path(full_path, sizeof(full_path), path);
    if(mkdir(full_path, 0777) == 0) return FSE_OK;
    return ext_errno_to_error();
}

static FS_Error storage_ext_common_fs_info(
    void* ctx,
    const char* fs_path,
    uint64_t* total_space,
    uint64_t* free_space) {
    UNUSED(ctx);
    UNUSED(fs_path);

    size_t total = 0;
    size_t used = 0;
    if(esp_littlefs_info("littlefs", &total, &used) != ESP_OK) {
        return FSE_INTERNAL;
    }

    if(total_space) *total_space = (uint64_t)total;
    if(free_space) *free_space = (uint64_t)(total - used);
    return FSE_OK;
}

static bool storage_ext_common_equivalent_path(const char* path1, const char* path2) {
    return strcasecmp(path1, path2) == 0;
}

static const FS_Api fs_api = {
    .file = {
        .open = storage_ext_file_open,
        .close = storage_ext_file_close,
        .read = storage_ext_file_read,
        .write = storage_ext_file_write,
        .seek = storage_ext_file_seek,
        .tell = storage_ext_file_tell,
        .truncate = storage_ext_file_truncate,
        .size = storage_ext_file_size,
        .sync = storage_ext_file_sync,
        .eof = storage_ext_file_eof,
    },
    .dir = {
        .open = storage_ext_dir_open,
        .close = storage_ext_dir_close,
        .read = storage_ext_dir_read,
        .rewind = storage_ext_dir_rewind,
    },
    .common = {
        .stat = storage_ext_common_stat,
        .mkdir = storage_ext_common_mkdir,
        .remove = storage_ext_common_remove,
        .fs_info = storage_ext_common_fs_info,
        .equivalent_path = storage_ext_common_equivalent_path,
    },
};

void storage_ext_init(StorageData* storage) {
    StorageExtData* data = calloc(1, sizeof(StorageExtData));
    data->mounted = ext_mount();

    storage->data = data;
    storage->api.tick = NULL;
    storage->fs_api = &fs_api;
    storage->status = data->mounted ? StorageStatusOK : StorageStatusErrorInternal;
    storage_data_timestamp(storage);
}

FS_Error sd_mount_card(StorageData* storage, bool notify) {
    UNUSED(notify);
    storage->status = StorageStatusOK;
    storage_data_timestamp(storage);
    return FSE_OK;
}

FS_Error sd_unmount_card(StorageData* storage) {
    UNUSED(storage);
    return FSE_OK;
}

FS_Error sd_format_card(StorageData* storage) {
    UNUSED(storage);
    return FSE_NOT_IMPLEMENTED;
}

FS_Error sd_card_info(StorageData* storage, SDInfo* sd_info) {
    UNUSED(storage);
    memset(sd_info, 0, sizeof(*sd_info));
    strncpy(sd_info->label, "Internal", sizeof(sd_info->label) - 1);
    return FSE_OK;
}
