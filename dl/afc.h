#pragma once

typedef void (*afc_iter_callback) (struct afc_connection *, char *, char *);
void afc_iter_dir(struct afc_connection *conn, char *path, afc_iter_callback callback);
void afc_remove_all(struct afc_connection *conn, char *path);
void afc_create_directory(struct afc_connection *conn, char *path);
void afc_list_files(struct afc_connection *conn, char *path);
