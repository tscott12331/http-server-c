#include <stdio.h>

#include <stdlib.h> 
#include <string.h>
#include <dirent.h>

#include "arrays.h"
#include "directory-parser.h"

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
    page->itemCapacity = 0;
    page->itemCount = 0;
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
        /*printf("Old capacity %d\n", page->itemCapacity);*/
        int oldCapacity  = page->itemCapacity;
        page->itemCapacity = GROW_CAPACITY(page->itemCapacity);
        /*printf("New capacity %d\n", page->itemCapacity);*/
        page->items = GROW_ARRAY(page->items, DirectoryItem, oldCapacity, page->itemCapacity);
        /*page->items = realloc(page->items, page->itemCapacity * sizeof(DirectoryItem));*/
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
                char* pageName = malloc((pageNameLen + 1) * sizeof(char));
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
            tagLen = 8;
            break;
        case TAG_H1:
            tagLen = 10;
            break;
        default:
            tagLen = 0;
            break;
    }
    return tagLen;
}

static int calcHtmlLen(char* text, TagType type, char* attributes) {
    if(type == TAG_RAW) return strlen(text);
    int tagLen = getTagLen(type);

    // +1 for the space between tag and attributes
    int attributeLen = attributes == NULL ? 0 : (int) strlen(attributes);
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
    int attributeLen = attributes == NULL ? 0 : (int)strlen(attributes);
    /*if(attributes == NULL) printf("ATTRIBUTES IS NULL\n");*/
    /*printf("getTagStart: attributeLen %d\n", attributeLen);*/
    char* tag = (char*)malloc((getTagStartLen(type) + attributeLen + 2) * sizeof(char));
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
    /*printf("getTagStart: copied initial part of tag: %s\n", tag); */
    if(attributes != NULL) {
        strcat(tag, attributes);
        /*printf("getTagStart: catted attributes: %s\n", tag);*/
    }
    strcat(tag, ">");
    /*printf("getTagStart: catted end bracket: %s\n", tag);*/
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
    /*printf("adding %d length to string of count %d & capacity %d\n", addedLength, htmlPage->count, htmlPage->capacity);*/
    if(htmlPage->count + addedLength + 1 > htmlPage->capacity) {
        /*printf("need to grow capacity\n");*/
        int oldCapacity = htmlPage->capacity;
        do {
            htmlPage->capacity = GROW_CAPACITY(htmlPage->capacity);
            /*printf("new capacity %d\n", htmlPage->capacity);*/
        } while(htmlPage->count + addedLength + 1 > htmlPage->capacity); 
        
        htmlPage->html = GROW_ARRAY(htmlPage->html, char, oldCapacity, htmlPage->capacity);
        /*htmlPage->html = realloc(htmlPage->html, htmlPage->count * sizeof(char));*/
        /*printf("Grew capacity to %d\n", htmlPage->capacity);*/
        
        if(oldCapacity == 0) {
            /*printf("appending null byte\n");*/
            strcpy(htmlPage->html, "\0");
            htmlPage->count++;
        }

    }

    /*if((int)strlen(htmlPage->html) == 0) {*/
    /*    printf("appending null byte\n");*/
    /*    strcpy(htmlPage->html, "\0"); // appending null byte*/
    /*}*/
    /*printf("capacity is sufficient\n");*/
    if(type == TAG_RAW) {
        // just append text
        /*printf("tag_raw, just appending text (length: %d to page of length %d): %s\n", (int)strlen(text), (int)strlen(htmlPage->html), text);*/
        strcat(htmlPage->html, text);
    } else {
        /*printf("normal tag\n");*/
        char* tagStart = getTagStart(type, attributes);
        /*printf("got tag start\n");*/
        strcat(htmlPage->html, tagStart);
        strcat(htmlPage->html, text);
        strcat(htmlPage->html, getTagEnd(type));
        /*printf("text: %s%s%s\n", tagStart, text, getTagEnd(type));*/
        free(tagStart);
    }

    /*printf("appended html, new text\n%s\n\n", htmlPage->html);*/
    htmlPage->count += addedLength;
    /*printf("new htmlPage count %d\n", htmlPage->count);*/
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
    appendHtml(htmlPage, page->name, TAG_H1, NULL);
    
    if(page->parentFolder != NULL) {
        int parentFoldNameLen = strlen(page->parentFolder->page->name);
        if(parentFoldNameLen > 1) {
            char* parentFoldName = page->parentFolder->page->name;
            printf("parentFoldName: %s\n", parentFoldName);
            parentFoldNameLen = strlen(parentFoldName);
            char* parentPath;
            char* curPageName = &parentFoldName[1];
            parentPath = (char*) malloc((parentFoldNameLen + 7) * sizeof(char)); 
            strcpy(parentPath, "href=\"");
            strcat(parentPath, curPageName);
            strcat(parentPath, "\"");
        
            appendHtml(htmlPage, parentFoldName, TAG_A, parentPath);
        } else {
            appendHtml(htmlPage, ".", TAG_A, "href=\"/\"");
        }
    }
    
    for(int i = 0; i < page->itemCount; i++) {
        DirectoryItem item = page->items[i];
        char* href;
        int pageNameLen = strlen(page->name);
        int itemNameLen = strlen(item.name); 
        if(pageNameLen > 1) {
            char* curPageName = &page->name[2];
            href = (char*) malloc((pageNameLen + itemNameLen + 8) * sizeof(char)); 
            strcpy(href, "href=\"/");
            strcat(href, curPageName);
            strcat(href, "/");
            strcat(href, item.name);
            strcat(href, "\"");
        } else {
            href = (char*) malloc((itemNameLen + 9) * sizeof(char));
            strcpy(href, "href=\"/");
            strcat(href, item.name);
            strcat(href, "\"");
        }
        
        appendHtml(htmlPage, page->items[i].name, TAG_A, href); 
        free(href);
    }

    appendHtml(htmlPage, "</body>\
            </html>", TAG_RAW, NULL);

    if(!tableSet(table, page->name, htmlPage->html)) {
       printf("failed to set to table\n"); 
    }
}

Table* generateHtmlTable(Page* initPage) {
    Table* htmlTable = (Table*) malloc(sizeof(Table));
    initTable(htmlTable);
    Page* curPage;
    for(curPage = initPage; curPage != NULL; curPage = curPage->nextPage) {
        printf("\n\n== GENERATING HTML PAGE FOR %s ==\n\n", curPage->name);
        generateHtmlPage(htmlTable, curPage);
    }
    return htmlTable;
}
