#include <stdio.h>

#include <stdlib.h> 
#include <string.h>
#include <dirent.h>

#include "directory-parser.h"
#include "arrays.h"

#define CUR_DIR "."
#define UP_DIR ".."

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
    Page* lastPage = initPage;
    for(curPage = initPage; curPage != NULL; curPage = curPage->nextPage) {
        printf("\n== scanning page '%s' ==\n\n", curPage->name);
        lastPage = scanDirectory(curPage, lastPage);
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

Page* scanDirectory(Page* currentPage, Page* lastPage) {
   DIR* curDirectory = opendir(currentPage->name);
   struct dirent* dirEntry;
   Page* addedPageList = NULL;
   Page* newLastPage = NULL;
   for(dirEntry = readdir(curDirectory); dirEntry != NULL; dirEntry = readdir(curDirectory)) {
       switch(dirEntry->d_type) {
            case DT_DIR: {
                char* dirName = dirEntry->d_name;
                int dirNameLen = strlen(dirName);
                if(dirNameLen <= 2 && 
                    (dirNameLen == 1 && memcmp(dirName, CUR_DIR, 1) == 0) ||
                    (dirNameLen == 2 && memcmp(dirName, UP_DIR, 2) == 0)) {
                    printf("non-scannable directory '%s', skipped\n", dirName);
                   break; 
                }
                printf("scanned folder '%s' in page '%s'\n", dirEntry->d_name, currentPage->name);

                Folder* newFolder = malloc(sizeof(Folder));
                newFolder->item.name = dirEntry->d_name;
                newFolder->item.size = dirEntry->d_reclen; 
                newFolder->item.type = DI_FOLDER;

                int pageNameLen = strlen(currentPage->name) + strlen(dirEntry->d_name) + 1;
                char* pageName = malloc(pageNameLen + 1);
                strcpy(pageName, currentPage->name);
                strcat(pageName, "/");
                strcat(pageName, dirEntry->d_name);
                pageName[pageNameLen] = '\0';
                Page* newPage = initPage(pageName, newFolder, currentPage->enclosingFolder);
                
                printf("added new page %s\n", newPage->name);

                newFolder->page = newPage;
                /*currentPage->nextPage = newPage;*/
                if(newLastPage == NULL) newLastPage = newPage;
                newPage->nextPage = addedPageList;
                /*addedPageList->nextPage = newPage;*/
                addedPageList = newPage; 

                addDirectoryItem(currentPage, *((DirectoryItem*)newFolder));
                break;
         }
            case DT_REG: {
                printf("scanned file '%s' in page '%s'\n", dirEntry->d_name, currentPage->name);
                // regular file
                File* file = (File*) malloc(sizeof(File));
                file->item.size = dirEntry->d_reclen;
                file->item.name = dirEntry->d_name;
                file->item.type = DI_FILE;
                addDirectoryItem(currentPage, *((DirectoryItem*) file));
                break;
         }
            default:
                break;
       }
   }

   lastPage->nextPage = addedPageList;
   /*Page* tmp;*/
   /*printf("\nAFTER SCANNING DIRECTORY %s\n\n", currentPage->name);*/
   /*for(tmp = addedPageList; tmp != NULL; tmp = tmp->nextPage) {*/
   /*    printf("^\n");*/
   /*    printf("|- page: %s\n", tmp->name);*/
   /*}*/
   return newLastPage == NULL ? lastPage : newLastPage;
}
