#include <stdio.h>

#include <stdlib.h> 
#include <string.h>
#include <dirent.h>

#include "directory-parser.h"
#include "arrays.h"

#define CUR_DIR "."
#define UP_DIR ".."
#define HTML_BOILERPLATE_SIZE 266

typedef enum {
    TAG_RAW,
    TAG_H1,
    TAG_A
} TagType;

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
        /*printf("\n== scanning page '%s' ==\n\n", curPage->name);*/
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
                    /*printf("non-scannable directory '%s', skipped\n", dirName);*/
                   break; 
                }
                /*printf("scanned folder '%s' in page '%s'\n", dirEntry->d_name, currentPage->name);*/

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
                
                /*printf("added new page %s\n", newPage->name);*/

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
                /*printf("scanned file '%s' in page '%s'\n", dirEntry->d_name, currentPage->name);*/
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

static void initHtmlPage(HtmlPage* htmlPage) {
    htmlPage->capacity = 0;
    htmlPage->count = 0;
    htmlPage->html = NULL;
}

static int getTagLen(TagType type) {
    int tagLen;
    switch(type) {
        case TAG_RAW:
            tagLen = 0;
            break;
        case TAG_A:
            tagLen = 7;
            break;
        case TAG_H1:
            tagLen = 9;
            break;
        default:
            tagLen = 0;
            break;
    }
    return tagLen;
}

static int calcHtmlLen(char* text, TagType type, char* attributes) {
    int tagLen = getTagLen(type);

    // +1 for the space between tag and attributes
    int attributeLen = attributes == NULL ? 0 : (int) strlen(attributes) + 1;
    int textLen = text == NULL ? 0 : (int) strlen(text); 
    /*printf("calculated htmllen for text %s\n", text);*/
    return tagLen + attributeLen + textLen; 
}

static int getTagStartLen(TagType type) {
    switch(type) {
        case TAG_RAW:
            return 0;
        case TAG_H1:
            return 4;
        case TAG_A:
            return 3;
        default:
            return 0;
    }
}

static char* getTagStart(TagType type, char* attributes) {
    int attributeLen = attributes == NULL ? 0 : (int)strlen(attributes) + 1;
    char* tag = (char*)malloc((getTagStartLen(type) + attributeLen + 1) * sizeof(char));
    switch(type) {
        case TAG_H1:
            strcpy(tag, "<h1 ");
            break;
        case TAG_A:
            strcpy(tag, "<a ");
            break;
        default:
            strcpy(tag, "");
            break;
    }
    
    if(attributes != NULL) {
        strcat(tag, attributes);
    }
    strcat(tag, ">");
    return tag;
}

static char* getTagEnd(TagType type) {
    switch(type) {
        case TAG_RAW:
            return "";
        case TAG_A:
            return "</a>";
        case TAG_H1:
            return "</h1>";
        default:
            return "";
    }
}

static void appendHtml(HtmlPage* htmlPage, char* text, TagType type, char* attributes) {
    int addedLength = calcHtmlLen(text, type, attributes);
    printf("added %d length string\n", addedLength);
    while(htmlPage->count + addedLength + 1 > htmlPage->capacity) {
        htmlPage->capacity = GROW_CAPACITY(htmlPage->capacity);
        htmlPage->html = GROW_ARRAY(htmlPage->html, char, htmlPage->capacity);
        printf("Grew capacity to %d\n", htmlPage->capacity);
    }

    if((int)strlen(htmlPage->html) == 0) {
        printf("appending null byte\n");
        strcpy(htmlPage->html, "\0"); // appending null byte
    }
    if(type == TAG_RAW) {
        // just append text
        printf("tag_raw, just appending text\ntext: %s\n", text);
        strcat(htmlPage->html, text);
    } else {
        char* tagStart = getTagStart(type, attributes);
        strcat(htmlPage->html, tagStart);
        strcat(htmlPage->html, text);
        strcat(htmlPage->html, getTagEnd(type));
        printf("normal tag\ntext: %s%s%s\n", tagStart, text, getTagEnd(type));
        free(tagStart);
    }

    printf("appended html, new text\n%s\n\n", htmlPage->html);
    htmlPage->count += addedLength;
}

static void generateHtmlPage(Table* table, Page* page) {
    HtmlPage* htmlPage = (HtmlPage*)malloc(sizeof(HtmlPage));
    initHtmlPage(htmlPage);

    appendHtml(htmlPage, "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
<meta charset=\"UTF-8\">\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\
<title>HTML 5 Boilerplate</title>\
</head>\
<body>", TAG_RAW, NULL);    
   // generate some tags... 
    appendHtml(htmlPage, "NEW HTML PAGE", TAG_H1, NULL);
    appendHtml(htmlPage, page->name, TAG_H1, NULL);
    Page* curPage;
    for(curPage = page->nextPage; curPage != NULL; curPage = curPage->nextPage) {
        appendHtml(htmlPage, curPage->name, TAG_A, NULL); 
    }
    appendHtml(htmlPage, "</body>\
            </html>", TAG_RAW, NULL);


    if(!tableSet(table, page->name, htmlPage->html)) {
       printf("failed to set to table\n"); 
    }
}

Table* generateHtmlTable(Page* initPage) {
    Table* htmlTable = (Table*) malloc(sizeof(Table));
    Page* curPage;
    for(curPage = initPage; curPage != NULL; curPage = curPage->nextPage) {
        printf("\n\n== GENERATING HTML PAGE FOR %s ==\n\n", curPage->name);
        generateHtmlPage(htmlTable, curPage);
    }
    return htmlTable;
}
