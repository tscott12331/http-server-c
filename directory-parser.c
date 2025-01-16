#include <stdlib.h> 
#include <string.h>
#include <dirent.h>

#include "directory-parser.h"
#include "arrays.h"


Page* initPage(char* name, Folder* enclosingFolder, Folder* parentFolder) {
    Page* page = (Page*) malloc(sizeof(Page));
    page->parentFolder = parentFolder;
    page->enclosingFolder = enclosingFolder; 
    page->name = name;
    page->items = NULL;
    page->nextPage = NULL;
    return page;
}

void generatePages(Page* initPage) {
    Page* curPage;
    for(curPage = initPage; curPage != NULL; curPage = curPage->nextPage) {
        scanDirectory(curPage);
    }
}

static void addDirectoryItem(Page* page, DirectoryItem item) {
    if(page->itemCount + 1 > page->itemCapacity) {
        page->itemCapacity = GROW_CAPACITY(page->itemCapacity);
        page->items = GROW_ARRAY(page->items, DirectoryItem, page->itemCapacity);
    }
    page->items[page->itemCount] = item;
    page->itemCount++;
}

void scanDirectory(Page* currentPage) {
   DIR* curDirectory = opendir(currentPage->name);
   struct dirent* dirEntry;
   for(dirEntry = readdir(curDirectory); dirEntry != NULL; dirEntry = readdir(curDirectory)) {
       switch(dirEntry->d_type) {
            case DT_DIR: {
                Folder* newFolder = malloc(sizeof(Folder));
                newFolder->item.name = dirEntry->d_name;
                newFolder->item.size = dirEntry->d_reclen; 

                int pageNameLen = strlen(currentPage->name) + strlen(dirEntry->d_name) + 1;
                char* pageName = malloc(pageNameLen + 1);
                strcpy(pageName, currentPage->name);
                strcat(pageName, "/");
                strcat(pageName, dirEntry->d_name);
                pageName[pageNameLen] = '\0';
                Page* newPage = initPage(pageName, newFolder, currentPage->enclosingFolder);

                newFolder->page = newPage;
                currentPage->nextPage = newPage;
                addDirectoryItem(currentPage, *((DirectoryItem*)newFolder));
                break;
         }
            case DT_REG: {
                // regular file
                File* file = (File*) malloc(sizeof(File));
                file->item.size = dirEntry->d_reclen;
                file->item.name = dirEntry->d_name;
                addDirectoryItem(currentPage, *((DirectoryItem*) file));
                break;
         }
            default:
                break;
       }
   }
}
