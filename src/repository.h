#ifndef MINIGIT_REPOSITORY_H
#define MINIGIT_REPOSITORY_H

void init_repository(void);
void add_file(const char *filename);
void cat_object(const char *hash);
void commit_repository(void);

#endif
