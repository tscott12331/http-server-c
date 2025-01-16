#ifndef server_directory_parser
#define server_directory_parser

#include <time.h>

typedef struct {
    size_t size;
    char* name;
} DirectoryItem;

typedef struct {
    DirectoryItem item;
} File;

typedef struct folder_t Folder;
typedef struct page_t Page;

struct folder_t {
    DirectoryItem item;
    struct page_t* page; 
};

struct page_t {
    DirectoryItem* items;
    int itemCount;
    int itemCapacity;
    char* name;
    struct folder_t* parentFolder; 
    struct folder_t* enclosingFolder;
    struct page_t* nextPage;
};


void generatePages(Page* initPage);
Page* initPage(char* name, Folder* enclosingFolder, Folder* parentFolder);
void scanDirectory(Page* currentPage);

#endif
