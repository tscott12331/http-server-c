#ifndef server_directory_parser
#define server_directory_parser

#include <time.h>

#include "table.h"

typedef enum {
    DI_FILE,
    DI_FOLDER,
} DirectoryItemType;

typedef struct {
    size_t size;
    char* name;
    DirectoryItemType type;
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

typedef struct {
    int count;
    int capacity;
    char* html;
} HtmlPage;

Table* generateHtmlTable(Page* initPage);
void generatePages(Page* initPage);
Page* initPage(char* name, Folder* enclosingFolder, Folder* parentFolder);
Page* scanDirectory(Page* currentPage, Page* lastPage);

#endif
