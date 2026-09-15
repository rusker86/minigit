#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <zconf.h>
#include <zlib.h>

#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd

#else
    #include <unistd.h>
#endif

void update_index(char *filename, char *hash) {
    FILE *index = fopen(".minigit/index", "a");
    if(index == NULL) {
        printf("Failed to open index\n");
        return;
    }

    fprintf(index, "%s %s\n", filename, hash);
    fclose(index);
}

void init() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    mkdir(strcat(cwd, "/.minigit"), 0755);
    mkdir(strcat(cwd, "/objects"), 0755);

    printf("Initialized repository\n");
}

void add(char *filename) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/");
    strcat(cwd, filename);
    char buffer_file;
    char header[64];
    size_t bytes_read;
    char *final_hash;
    unsigned char buffer[4096];


    FILE *file = fopen(cwd, "rb");

    if(file == NULL) {
        printf("File not found: %s\n", filename);
        return;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    int header_size = snprintf(
        header,
        sizeof(header),
        "blob %ld",
        file_size
    );

    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, header, header_size + 1);

    while((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA1_Update(&ctx, buffer, bytes_read);
    }
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &ctx);

    char hash_hex[SHA_DIGEST_LENGTH * 2 + 1];
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&hash_hex[i * 2], "%02x", hash[i]);
    }
    hash_hex[SHA_DIGEST_LENGTH * 2] = '\0';

    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/.minigit/objects/");
    strcat(cwd, hash_hex);

    FILE *out = fopen(cwd, "wb");
    if(out == NULL) {
        printf("Failed to open file: %s\n", cwd);
        return;
    }
    rewind(file);

    size_t blob_size = file_size + header_size + 1;

    unsigned char *blob_data = malloc(blob_size);
    if(blob_data == NULL) {
        printf("Failed to allocate memory\n");
        return;
    }

    memcpy(blob_data, header, header_size + 1);
    size_t offset = header_size + 1;

    while((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        memcpy(blob_data + offset, buffer, bytes_read);
        offset += bytes_read;
    }

    //Comprimir con zlib
    uLongf compressed_size = compressBound(blob_size);
    unsigned char *compressed_data = malloc(compressed_size);
    if(compressed_data == NULL) {
        printf("Failed to allocate memory\n");
        return;
    }

    int result = compress2(compressed_data, &compressed_size, blob_data, blob_size, Z_DEFAULT_COMPRESSION);
    if(result != Z_OK) {
        printf("Failed to compress data\n");
        return;
    }

    fwrite(compressed_data, 1, compressed_size, out);

    update_index(filename, hash_hex);

    fclose(file);
    fclose(out);

    free(compressed_data);
    free(blob_data);
}



void cat(char *hash) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    strcat(cwd, "/.minigit/objects/");
    strcat(cwd, hash);

    FILE *file = fopen(cwd, "rb");
    if(file == NULL) {
        printf("Failed to open file: %s\n", cwd);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    unsigned char *compressed_data = malloc(file_size);
    if(compressed_data == NULL) {
        printf("Failed to allocate memory\n");
        return;
    }

    size_t bytes_read = fread(compressed_data, 1, file_size, file);
    if(bytes_read != file_size) {
        printf("Failed to read file\n");
        free(compressed_data);
        return;
    }

    fclose(file);

    uLongf decompressed_capacity = 1;
    uLongf decompressed_size = decompressed_capacity;
    unsigned char *decompressed_data = malloc(decompressed_capacity);
    if(decompressed_data == NULL) {
        printf("Failed to allocate memory\n");
        free(compressed_data);
        return;
    }

    int result = uncompress(decompressed_data, &decompressed_size, compressed_data, file_size);
    while(result == Z_BUF_ERROR) {
        decompressed_capacity *= 2;
        decompressed_size = decompressed_capacity;

        unsigned char *new_buffer = realloc(decompressed_data, decompressed_capacity);
        if(new_buffer == NULL) {
            printf("Failed to allocate memory\n");
            free(compressed_data);
            return;
        }

        decompressed_data = new_buffer;
        result = uncompress(decompressed_data, &decompressed_size, compressed_data, file_size);
    }

    if(result != Z_OK) {
        printf("Failed to decompress data\n");
        free(compressed_data);
        free(decompressed_data);
        return;
    }

    size_t content_offset = 0;
    while(
        content_offset < decompressed_size != '\0' &&
        decompressed_data[content_offset]
    ) {
        content_offset++;
    }

    size_t content_size = decompressed_size - content_offset - 1;   // El -1 es para el carácter nulo al final
    fwrite(decompressed_data + content_offset + 1, 1, content_size, stdout);
    printf("\n");

    free(compressed_data);
    free(decompressed_data);
}

unsigned char hex_value(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    } else if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}

void hex_to_bytes(char *hex, unsigned char *bytes) {
    size_t len = strlen(hex);

    for(size_t i = 0, j = 0; i < len; i += 2, j++) {
        bytes[j] =
            hex_value(hex[i]) * 16 +
            hex_value(hex[i + 1]);
    }
}

unsigned char *create_tree_entry(char *filename, unsigned char *hash, size_t *entry_size) {
    char header[256];
    snprintf(header, sizeof(header), "100644 %s", filename);

    size_t header_size = strlen(header);
    size_t offset = 0;

    unsigned char *entry = malloc(header_size + 20 + 1);

    if(!entry) {
        printf("Failed to allocate memory for tree entry\n");
        return NULL;
    }

    memcpy(entry + offset, header, header_size + 1);

    offset += header_size + 1;

    memcpy(entry + offset, hash, 20);

    *entry_size = header_size + 20 + 1;
    return entry;
}

unsigned char *create_tree(char *filename, unsigned char *hash, size_t *ptr_tree_size) {
    unsigned char *entry;
    size_t entry_size;
    char header[256];
    size_t offset = 0;
    
    entry = create_tree_entry(filename, hash, &entry_size);

    if(!entry) {
        printf("Failed to create tree entry\n");
        return NULL;
    }

    snprintf(header, sizeof(header), "tree %zu", entry_size);

    // Size of header includes \0
    size_t header_size = strlen(header) + 1;

    size_t tree_size = header_size + entry_size;

    unsigned char *tree = malloc(tree_size);
    if(!tree) {
        printf("Failed to allocate memory for tree\n");
        free(entry);
        return NULL;
    }

    memcpy(tree, header, header_size);

    offset += header_size;

    memcpy(tree + offset, entry, entry_size);

    free(entry);

    *ptr_tree_size = tree_size;
    return tree;
}

void create_tree_file(unsigned char *tree, size_t tree_size) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, tree, tree_size);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &ctx);

    char hash_str[SHA_DIGEST_LENGTH * 2 + 1];
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        snprintf(&hash_str[i * 2], 3, "%02x", hash[i]);
    }
    hash_str[SHA_DIGEST_LENGTH * 2] = '\0';

    printf("Tree hash: %s\n", hash_str);

    char path[1024];
    
    getcwd(path, sizeof(path));
    strcat(path, "/.minigit/objects/");
    strcat(path, hash_str);
    
    FILE *fp = fopen(path, "wb");

    if(fp) {

        uLongf compressed_size = compressBound(tree_size);
        unsigned char *compressed_tree = malloc(compressed_size);

        if(compressed_tree == NULL) {
            printf("Failed to allocate memory for compressed tree\n");
            fclose(fp);
            return;
        }
        
        int result = compress2(compressed_tree, &compressed_size, tree, tree_size, Z_DEFAULT_COMPRESSION);
        if(result != Z_OK) {
            printf("Failed to compress tree\n");
            free(compressed_tree);
            fclose(fp);
            return;
        }
        
        fwrite(compressed_tree, 1, compressed_size, fp);
        free(compressed_tree);
        fclose(fp);
    }
}

void write_tree() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, ".minigit/index");
    FILE *index = fopen(cwd, "r");
    if(index == NULL) {
        printf("Failed to open index\n");
        return;
    }
    
    char line[256];
    
    while(fgets(line, sizeof(line), index)) {
        char *filename = strtok(line, " ");
        char *hash = strtok(NULL, " ");
        printf("%s %s\n", filename, hash);
    }
    fclose(index);
}

void commit() {  }

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <command>\n", argv[0]);
        return 1;
    }

    char *command = argv[1];

    if((strcmp(command, "init") == 0) && (argc == 2)) {
        init();
    } else if((strcmp(command, "add") == 0) && (argc == 3)) {
        add(argv[2]);
    } else if((strcmp(command, "commit") == 0) && (argc == 2)) {
        commit();
    } else if((strcmp(command, "cat") == 0) && (argc == 3)) {
        cat(argv[2]);
    } else {
        printf("Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
