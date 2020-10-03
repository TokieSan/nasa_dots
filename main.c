#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define WORD_BUFFER 1024
#define FAILED_READING -1
#define FILE_ADDED -2
#define FILE_EXISTS -3
#define FILE_FAILED_ADDING -4
#define SUCCESS 0

typedef struct {
    char *word;
    int appear;
} keyword;

sqlite3 *DB_INDEXES = NULL;
sqlite3 *DB_FILES = NULL;

int check_documents(const char *Documents, char *target)
{
    char buf[WORD_BUFFER];
    int appears;
    char *ptr = (char *)Documents;
    char *start;
    while (*ptr != '\0') {
        if (*ptr == '(') {
            start = ptr+1;
        }
        else if (*ptr == ':') {
            if (strncmp(target, start, ptr-start) == 0)
                return 1;
        }
        ptr++;
    }
    return 0;
}

int indexing(FILE *file, char *name)
{
    if (DB_INDEXES == NULL) {
        int exit = 0; 
        exit = sqlite3_open("indexing.db", &DB_INDEXES); 
      
        if (exit) { 
            printf("FAILED OPENING indexing.db\n");
            return FAILED_READING; 
        } 
        else {
            printf("Opened indexing.db Successfully!\n");
        }
    }

    keyword *document_keywords = NULL;
    int keywords_index = 0;

    char read_word[WORD_BUFFER];
    char word[WORD_BUFFER];
    //WORD_BUFFER don't work in the next line but it ain't a much of a problem rn
    while (fscanf(file, " %1023s", read_word) == 1) {
        int index=0;
        for (int i = 0; i < strlen(read_word); ++i) {
            if (isalnum(read_word[i])) {
                word[index++] = tolower(read_word[i]);
            }
        }
        word[index] = '\0';
        int flag = 0;
        for (int i = 0; i < keywords_index; ++i) {
            if (strcmp(document_keywords[i].word, word) == 0) {
                document_keywords[i].appear++;
                flag = 1;
                break;
            }
        }
        if (!flag) { //not found add it 
            keywords_index++;
            document_keywords = realloc(document_keywords, keywords_index * sizeof(keyword));
            document_keywords[keywords_index-1].word = malloc(WORD_BUFFER);
            strcpy(document_keywords[keywords_index-1].word, word);
            document_keywords[keywords_index-1].appear = 1;
        }
    }
    char buf[WORD_BUFFER];
    char *err_msg = 0;
    int rc = 0;
    char value_buf[WORD_BUFFER*10];
    const char *DOCUMENTS;


    struct sqlite3_stmt *selectstmt;
    for (int i = 0; i < keywords_index; ++i) {
        sprintf(buf, "SELECT DOCUMENTS FROM INDEXING WHERE KEYWORD='%s'", document_keywords[i].word);
        int result = sqlite3_prepare_v2(DB_INDEXES, buf, -1, &selectstmt, NULL);
        if(result == SQLITE_OK && sqlite3_step(selectstmt) == SQLITE_ROW) {
            DOCUMENTS = sqlite3_column_text(selectstmt, 0);
            //printf("%s %s\n", DOCUMENTS, name);
            //printf("%s\n", strstr(DOCUMENTS, name));
            //printf("%s %s\n", DOCUMENTS, name);
            //char *ptr = strstr(DOCUMENTS, name);
            //if (ptr == NULL) {      //Add document to known documents
            //    sprintf(value_buf, "UPDATE INDEXING SET DOCUMENTS='%s(%s:%d)' WHERE KEYWORD='%s'", DOCUMENTS, name, document_keywords[i].appear,document_keywords[i].word);
            //    sqlite3_prepare_v2(DB_INDEXES,value_buf, -1, &selectstmt, NULL);
            //    sqlite3_step(selectstmt);
            //}
            //printf("FOUND\n");
            //if (ptr == NULL) {
            //    sprintf(value_buf, "UPDATE INDEXING SET DOCUMENTS || (%s:%d)  WHERE KEYWORD='%s'", name, document_keywords[i].appear,document_keywords[i].word);
            //    sqlite3_prepare_v2(DB_INDEXES,value_buf, -1, &selectstmt, NULL);
            //    sqlite3_step(selectstmt);
            //}
            if (!check_documents(DOCUMENTS, name)) {
                sprintf(value_buf, "UPDATE INDEXING SET DOCUMENTS='%s(%s:%d)' WHERE KEYWORD='%s'", DOCUMENTS, name, document_keywords[i].appear,document_keywords[i].word);
                sqlite3_prepare_v2(DB_INDEXES,value_buf, -1, &selectstmt, NULL);
                sqlite3_step(selectstmt);
            }
        }
        else {      //keyword doesn't exist
            sprintf(buf, "INSERT INTO INDEXING VALUES('%s','(%s:%d)')", document_keywords[i].word, name, document_keywords[i].appear);
            //result = sqlite3_prepare_v2(DB_INDEXES, buf, -1, &selectstmt, NULL);
            char *err_msg;
            result = sqlite3_exec(DB_INDEXES, buf ,0,0, &err_msg);
            if (result !=SQLITE_OK) {
                printf("\n%s\n", err_msg);
                fprintf(stderr, "%s", "Error writing into indexing.db\n");
            }
        }
    }
    free(document_keywords);
}

int add_file(char *name)
{
    if (DB_FILES == NULL) {
        int exit = 0; 
        exit = sqlite3_open("files.db", &DB_FILES); 
      
        if (exit) { 
            printf("FAILED OPENING files.db\n");
            return FAILED_READING; 
        } 
        else {
            printf("Opened files.db Successfully!\n");
        }
    }

    char buf[WORD_BUFFER];
    char *err_msg = 0;
    int rc = 0;

    sprintf(buf, "SELECT FILE FROM FILES WHERE FILE='%s'", name);

    struct sqlite3_stmt *selectstmt;
    int result = sqlite3_prepare_v2(DB_FILES, buf, -1, &selectstmt, NULL);
    if(result == SQLITE_OK) {
        if (sqlite3_step(selectstmt) == SQLITE_ROW) {
            printf("%s already exists in files.db.\n", name);
            return FILE_EXISTS;
        }
        else {
            //printf("Opened %s for reading\n", buf);
            sprintf(buf, "INSERT INTO FILES VALUES('%s')", name);
            //rc = sqlite3_exec(DB_FILES, buf ,0,0, &err_msg);
            result = sqlite3_prepare_v2(DB_FILES, buf, -1, &selectstmt, NULL);
            sqlite3_step(selectstmt);
            sprintf(buf, "documents/%s", name);
            FILE *file = fopen(buf, "r");
            indexing(file, name);
            fclose(file);
            return FILE_ADDED;
        }
    }
    return (0); 
}

int retrive_files()
{
    FILE *document;
    DIR *dir;
    struct dirent *in_file;

    dir = opendir("./documents");
    if (dir==NULL) {
        fprintf(stderr, "%s", "FAILED opening directory\n");
        return FAILED_READING;
    }
    while ((in_file=readdir(dir)) != NULL) {
        if (strcmp(in_file->d_name, "..") != 0 && strcmp(in_file->d_name, ".") != 0) {
            printf("READING %s\n", in_file->d_name);
            int result = add_file(in_file->d_name);
            if (result == FILE_ADDED)
            {
                printf("done reading %s\n", in_file->d_name);
            }
        }

    }
}

int get_result(char **keys, int n)
{
    if (DB_INDEXES == NULL) {
        int exit = 0; 
        exit = sqlite3_open("indexing.db", &DB_INDEXES); 

        if (exit) { 
            printf("FAILED OPENING indexes.db\n");
            return FAILED_READING; 
        } 
        else {
            //printf("Opened indexes.db Successfully!\n");
        }
    }

    char buf[WORD_BUFFER];
    const char *DOCUMENTS;
    struct sqlite3_stmt *selectstmt;

    char *err_msg;
    for (int i = 0; i < n; ++i) {
        sprintf(buf, "SELECT DOCUMENTS FROM INDEXING WHERE KEYWORD='%s'", keys[i]);
        int result = sqlite3_prepare_v2(DB_INDEXES, buf, -1, &selectstmt, NULL);
        //sqlite3_step(selectstmt);
        //int result = sqlite3_exec(DB_INDEXES, buf ,0,0, &err_msg);
        //printf("%s\n", err_msg);
        if (result == SQLITE_OK && sqlite3_step(selectstmt) == SQLITE_ROW) {
            DOCUMENTS = sqlite3_column_text(selectstmt, 0);
            printf("%s --> %s\n", keys[i], DOCUMENTS);
        }
        else { 
            fprintf(stderr, "Sorry %s isn't found in any document present\n", keys[i]);
        }
    }
    return (0); 

}

int main(int argc, char *argv[])
{

    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        printf("%s -r\t\tretrive files\n", argv[0]);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "-r") == 0) {
        retrive_files();
    }
    else if (argc == 1){
        printf("Enter keywords: ");
        char buf[WORD_BUFFER];
        char **keys = malloc(80 * sizeof (char *));
        int n = 0;
        while (fscanf(stdin, " %1023s", buf) == 1) {
            keys[n++] = strdup(buf);
        }
        get_result(keys, n);
    }
    else {
        get_result(argv+1, argc-1);
    }
    sqlite3_close(DB_INDEXES);
    sqlite3_close(DB_FILES);

}
