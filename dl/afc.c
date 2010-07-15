
#include "MobileDevice.h"
#include "_assert.h"
#include "output.h"
#include "afc.h"

void afc_iter_dir(struct afc_connection *conn, char *path, afc_iter_callback callback) {
	struct afc_directory *dir;
	char *dirent;
	
	if(AFCDirectoryOpen(conn, path, &dir)) return; 
	
	for (;;) { 
		_assertZero(AFCDirectoryRead(conn, dir, &dirent)); 
		if (!dirent) break; 
		if (strcmp(dirent, ".") == 0 || strcmp(dirent, "..") == 0) continue; 
		
		callback(conn, path, dirent);
	}
}

void afc_create_directory(struct afc_connection *conn, char *path) {
	_assertZero(AFCDirectoryCreate(conn, path));
}

void list_callback(struct afc_connection *conn, char *path, char *dirent) {
	Debug("%s", dirent);
}

void afc_list_files(struct afc_connection *conn, char *path) {
	afc_iter_dir(conn, path, (afc_iter_callback) list_callback);
}

void remove_callback(struct afc_connection *conn, char *path, char *dirent) {
	char subdir[255];
    snprintf(subdir, 255, "%s/%s", path, dirent);
    afc_remove_all(conn, subdir);

    Debug("Deleted %s/%s", path, dirent);
}

void afc_remove_all(struct afc_connection *conn, char *path) {
    int ret = AFCRemovePath(conn, path);
    if(ret == 0 || ret == 8) {
        // Successfully removed (it was empty) or does not exist
        return;
    }

	afc_iter_dir(conn, path, (afc_iter_callback) remove_callback);

    Debug("Deleting %s (ret=%d)", path, ret);
    _assertZero(AFCRemovePath(conn, path));
}
