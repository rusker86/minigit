#include "repository.h"

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#define make_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_directory(path) mkdir(path, 0755)
#endif

#define REPOSITORY_DIR ".minigit"
#define INDEX_FILE ".minigit/index"
#define PATH_SIZE 1024
#define BUFFER_SIZE 4096

static int repository_path(char *path, size_t path_size, const char *suffix)
{
    char cwd[PATH_SIZE];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("Failed to get current directory\n");
        return 0;
    }

    if (suffix == NULL || suffix[0] == '\0') {
        return snprintf(path, path_size, "%s/%s", cwd, REPOSITORY_DIR) < (int)path_size;
    }

    return snprintf(path, path_size, "%s/%s/%s", cwd, REPOSITORY_DIR, suffix) < (int)path_size;
}

static int object_path(char *path, size_t path_size, const char *hash)
{
    char objects_path[PATH_SIZE];

    if (!repository_path(objects_path, sizeof(objects_path), "objects")) {
        return 0;
    }

    return snprintf(path, path_size, "%s/%s", objects_path, hash) < (int)path_size;
}

static void hash_to_hex(const unsigned char *hash, char *hex)
{
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        snprintf(&hex[i * 2], 3, "%02x", hash[i]);
    }

    hex[SHA_DIGEST_LENGTH * 2] = '\0';
}

static void update_index(const char *filename, const char *hash)
{
    FILE *index = fopen(INDEX_FILE, "a");
    if (index == NULL) {
        printf("Failed to open index\n");
        return;
    }

    fprintf(index, "%s %s\n", filename, hash);
    fclose(index);
}

void init_repository(void)
{
    char repository[PATH_SIZE];
    char objects[PATH_SIZE];

    if (!repository_path(repository, sizeof(repository), NULL) ||
        !repository_path(objects, sizeof(objects), "objects")) {
        return;
    }

    make_directory(repository);
    make_directory(objects);
    printf("Initialized repository\n");
}

void add_file(const char *filename)
{
    char file_path[PATH_SIZE];
    char object_file[PATH_SIZE];
    char header[64];
    unsigned char buffer[BUFFER_SIZE];
    unsigned char hash[SHA_DIGEST_LENGTH];
    char hash_hex[SHA_DIGEST_LENGTH * 2 + 1];
    size_t bytes_read;

    if (snprintf(file_path, sizeof(file_path), "%s/%s", ".", filename) >= (int)sizeof(file_path)) {
        printf("File path is too long: %s\n", filename);
        return;
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        printf("File not found: %s\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    int header_size = snprintf(header, sizeof(header), "blob %ld", file_size);
    SHA_CTX context;
    SHA1_Init(&context);
    SHA1_Update(&context, header, header_size + 1);

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA1_Update(&context, buffer, bytes_read);
    }

    SHA1_Final(hash, &context);
    hash_to_hex(hash, hash_hex);

    if (!object_path(object_file, sizeof(object_file), hash_hex)) {
        fclose(file);
        return;
    }

    FILE *output = fopen(object_file, "wb");
    if (output == NULL) {
        printf("Failed to open file: %s\n", object_file);
        fclose(file);
        return;
    }

    rewind(file);
    size_t blob_size = (size_t)file_size + header_size + 1;
    unsigned char *blob_data = malloc(blob_size);
    if (blob_data == NULL) {
        printf("Failed to allocate memory\n");
        fclose(file);
        fclose(output);
        return;
    }

    memcpy(blob_data, header, header_size + 1);
    size_t offset = header_size + 1;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        memcpy(blob_data + offset, buffer, bytes_read);
        offset += bytes_read;
    }

    uLongf compressed_size = compressBound(blob_size);
    unsigned char *compressed_data = malloc(compressed_size);
    if (compressed_data == NULL) {
        printf("Failed to allocate memory\n");
        free(blob_data);
        fclose(file);
        fclose(output);
        return;
    }

    int result = compress2(
        compressed_data,
        &compressed_size,
        blob_data,
        blob_size,
        Z_DEFAULT_COMPRESSION
    );
    if (result != Z_OK) {
        printf("Failed to compress data\n");
        free(compressed_data);
        free(blob_data);
        fclose(file);
        fclose(output);
        return;
    }

    fwrite(compressed_data, 1, compressed_size, output);
    update_index(filename, hash_hex);

    free(compressed_data);
    free(blob_data);
    fclose(file);
    fclose(output);
}

void cat_object(const char *hash)
{
    char path[PATH_SIZE];
    if (!object_path(path, sizeof(path), hash)) {
        return;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Failed to open file: %s\n", path);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    unsigned char *compressed_data = malloc(file_size);
    if (compressed_data == NULL) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return;
    }

    size_t bytes_read = fread(compressed_data, 1, file_size, file);
    fclose(file);
    if (bytes_read != (size_t)file_size) {
        printf("Failed to read file\n");
        free(compressed_data);
        return;
    }

    uLongf decompressed_capacity = 1;
    uLongf decompressed_size = decompressed_capacity;
    unsigned char *decompressed_data = malloc(decompressed_capacity);
    if (decompressed_data == NULL) {
        printf("Failed to allocate memory\n");
        free(compressed_data);
        return;
    }

    int result = uncompress(
        decompressed_data,
        &decompressed_size,
        compressed_data,
        file_size
    );
    while (result == Z_BUF_ERROR) {
        decompressed_capacity *= 2;
        decompressed_size = decompressed_capacity;

        unsigned char *new_buffer = realloc(decompressed_data, decompressed_capacity);
        if (new_buffer == NULL) {
            printf("Failed to allocate memory\n");
            free(compressed_data);
            free(decompressed_data);
            return;
        }

        decompressed_data = new_buffer;
        result = uncompress(
            decompressed_data,
            &decompressed_size,
            compressed_data,
            file_size
        );
    }

    if (result != Z_OK) {
        printf("Failed to decompress data\n");
        free(compressed_data);
        free(decompressed_data);
        return;
    }

    unsigned char *content = memchr(decompressed_data, '\0', decompressed_size);
    if (content != NULL) {
        content++;
        fwrite(content, 1, decompressed_size - (content - decompressed_data), stdout);
    }
    printf("\n");

    free(compressed_data);
    free(decompressed_data);
}

void commit_repository(void)
{
}
