#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX_NAME_LENGTH 50
#define MAX_URL_LENGTH 100
#define DATABASE_NAME "projects.db"

struct Project
{
    char name[MAX_NAME_LENGTH];
    char link[MAX_URL_LENGTH];
    char imageUrl[MAX_URL_LENGTH];
    char githubLink[MAX_URL_LENGTH];
    char linkedinLink[MAX_URL_LENGTH];
};

void initializeProject(struct Project *project)
{
    strcpy(project->name, "");
    strcpy(project->link, "");
    strcpy(project->imageUrl, "");
    strcpy(project->githubLink, "");
    strcpy(project->linkedinLink, "");
}

int createTable(sqlite3 *db)
{
    char *sql = "CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, link TEXT, image_url TEXT, github_link TEXT, linkedin_link TEXT);";

    return sqlite3_exec(db, sql, 0, 0, 0);
}

int insertProject(sqlite3 *db, struct Project *project)
{
    char *sql;
    asprintf(&sql, "INSERT INTO projects (name, link, image_url, github_link, linkedin_link) VALUES ('%s', '%s', '%s', '%s', '%s');",
             project->name, project->link, project->imageUrl, project->githubLink, project->linkedinLink);

    int result = sqlite3_exec(db, sql, 0, 0, 0);
    free(sql);

    return result;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls)
{
    if (*con_cls == NULL)
    {
        *con_cls = malloc(sizeof(struct Project));
        initializeProject(*con_cls);
        return MHD_YES;
    }

    if (strcmp(method, "POST") == 0)
    {
        // Cia reikia kazkokios JSON library
        struct Project *newProject = *con_cls;
        sscanf(upload_data, "name=%[^&]&link=%[^&]&imageUrl=%[^&]&githubLink=%[^&]&linkedinLink=%s",
               newProject->name, newProject->link, newProject->imageUrl,
               newProject->githubLink, newProject->linkedinLink);

        // Sukurti arba atidaryti sqlite database
        sqlite3 *db;
        if (sqlite3_open(DATABASE_NAME, &db) != SQLITE_OK)
        {
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return MHD_NO;
        }

        if (createTable(db) != SQLITE_OK)
        {
            fprintf(stderr, "Cannot create table: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return MHD_NO;
        }

        // Ideti projekta i database
        if (insertProject(db, newProject) != SQLITE_OK)
        {
            fprintf(stderr, "Cannot insert project: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return MHD_NO;
        }

        sqlite3_close(db);
        return MHD_YES;
    }
    else if (strcmp(method, "GET") == 0)
    {
        // Get metodas gauti projektus, frontendas iskarto gaus visus projektus, taciau optimaliai cia turetu buti implementuota pagination
        struct MHD_Response *response;
        const char *json_response = "{ \"message\": \"Projektai\" }";
        response = MHD_create_response_from_buffer(strlen(json_response), (void *)json_response, MHD_RESPMEM_MUST_COPY);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    else
    {
        // Jeigu bus prasoma kazkokio kito methodo (pvz.: Post, bus grazinamas reject message)
        return MHD_NO;
    }
}

int main()
{
    struct MHD_Daemon *daemon;

    // Startint serveri
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8080, NULL, NULL,
                              &answer_to_connection, NULL, MHD_OPTION_END);

    if (daemon == NULL)
    {
        fprintf(stderr, "Serveris pafailino\n");
        return 1;
    }

    printf("Serveris startino\n");
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}