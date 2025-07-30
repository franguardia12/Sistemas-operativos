#define FUSE_USE_VERSION 30
#define MAX_CONTENT 1024
#define MAX_DIRECTORY_SIZE 1024
#define MAX_INODES 80
#define MAX_FILES 80
#define MAX_PATH 200
#define FREE 0
#define OCCUPIED 1
#define ROOT_PATH "/"
#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

#include <fuse.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>

enum inode_type {DIR = -1, REG = 1 }; // DIRECTORY, REGULAR

struct inode {
	enum inode_type type;                  
	mode_t mode; 
	size_t size;
	uid_t uid;
	gid_t gid;
	time_t i_atime;
	time_t i_mtime;
	time_t i_ctime;
	char path[MAX_PATH];
	char content[MAX_CONTENT];
	char directory_path[MAX_PATH];
};

struct super_block {
	struct inode inodes[MAX_INODES];
	int bitmap_inodes[MAX_INODES];  // (0 libre, 1 ocupado)
};

struct super_block _super_block = {};

char filedisk[MAX_PATH] = DEFAULT_FILE_DISK;


char *
parse_path(const char *path)
{
	size_t len = strlen(path);
	char *new_path = malloc(len);
	if (!new_path){
		return NULL;
	}

	memcpy(new_path, path + 1, len - 1);

	new_path[len - 1] = '\0';
	const char *last_slash = strrchr(path, '/');

	if (last_slash == NULL){
		return new_path;
	}

	size_t absolute_len = strlen(last_slash + 1);
	char *absolute_path = malloc(absolute_len + 1);
	if (!absolute_path) {
		free(new_path);
		return NULL;
	}

	memcpy(absolute_path, last_slash + 1, absolute_len);

	absolute_path[absolute_len] = '\0';
	free(new_path);

	return absolute_path;
}

// Devuelve el index del inodo o -1 si no existe
int
get_inode_index(const char *path)
{
	if (strcmp(path, ROOT_PATH) == 0){
		return 0;
	}

	char *_parse_path = parse_path(path);

	if (!_parse_path){
		return -1;
	}

	for (int i = 0; i < MAX_INODES; i++) {
		if (strcmp(_parse_path, _super_block.inodes[i].path) == 0) {
			return i;
		}
	}

	free(_parse_path);
	return -1;
}

// Devuelve el index del proximo inodo libre
// -ENOSPC si no hay mas espacio
// -EEXIST si ya existe un inodo con ese path
int
get_next_free_inode_index(const char *path)
{
	int index = -1;
	bool aux = false;

	for(int i = 0; i < MAX_INODES; i++){
		if(_super_block.bitmap_inodes[i] == FREE){
			index = i;
		}
		if(strcmp(_super_block.inodes[i].path, path) == 0){
			aux = true;
			break;
		}
	}

	if(aux){
		return -EEXIST;
	}else if(index != -1){
		return index;
	}else{
		return -ENOSPC;
	}
}

// Modifica el path del inodo pasado, cambiando el '/' por '\0'
void
get_parent_path(char *parent_path)
{
	char *last_slash = strrchr(parent_path, '/');

	if(last_slash != NULL){
		*last_slash = '\0';
	}else{
		parent_path[0] = '\0';
	}
}

// Crea un nuevo inodo. Lo guarda en _super_block y devuelve el index del inodo creado.
// En caso de error devuelve:
// -ENAMETOOLONG si el path es demasiado largo
// -ENOSPC si no hay mas espacio
// -EEXIST si ya existe un inodo con ese path
int
new_inode(const char *path, mode_t mode, int type)
{
	if (strlen(path) - 1 > MAX_CONTENT) {
		return -ENAMETOOLONG;
	}

	char *absolute_path = parse_path(path);
	if (!absolute_path){
		return -1;
	}

	int inode_index = get_next_free_inode_index(absolute_path);

	if (inode_index < 0){
		return inode_index;
	}

	struct inode new_inode;
	new_inode.type = type;
	new_inode.mode = mode;
	new_inode.size = 0;
	new_inode.uid = getuid();
	new_inode.gid = getgid();
	new_inode.i_atime = time(NULL);
	new_inode.i_mtime = time(NULL);
	strcpy(new_inode.path, absolute_path);

	if(type == REG){
		char parent_path[MAX_PATH];
		memcpy(parent_path, path + 1, strlen(path) - 1);
		parent_path[strlen(path) - 1] = '\0';

		get_parent_path(parent_path);

		if(strlen(parent_path) == 0){
			strcpy(parent_path, ROOT_PATH);
		}

		strcpy(new_inode.directory_path, parent_path);

	}else{
		strcpy(new_inode.directory_path, ROOT_PATH);
	}

	memset(new_inode.content, 0, sizeof(new_inode.content));

	_super_block.inodes[inode_index] = new_inode;
	_super_block.bitmap_inodes[inode_index] = OCCUPIED;
	free(absolute_path);

	return 0;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		return -ENOENT;  // No existe el archivo o directorio
	}
	
	struct inode inode = _super_block.inodes[inode_index];

	// Rellenar la estructura stat

	st->st_mode = inode.mode;
	st->st_uid = inode.uid; // Propietario (asumo que es el usuario actual)
	st->st_gid = inode.gid; // Grupo (asumo que es el grupo actual)
	st->st_size = inode.size; // Tamaño del archivo
	st->st_atime = inode.i_atime; // Último acceso
	st->st_mtime = inode.i_mtime; // Última modificación
	st->st_ctime = inode.i_ctime; // Creation time
	st->st_dev = 0;
	st->st_ino = inode_index;

	// Obtener el tipo de archivo
	if(inode.type == REG) {
		st->st_mode = __S_IFREG | 0644;
		st->st_nlink = 1;
	}else if (inode.type == DIR){
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	}else{
		return -EINVAL;
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	int inode_index = get_inode_index(path);

	if(inode_index == -1){
		return -ENOENT;
	}

	struct inode dir_inode = _super_block.inodes[inode_index];

	if(dir_inode.type != DIR){
		return -ENOTDIR;
	}

	// Listar el contenido del directorio
	for (int i = 1; i < MAX_INODES; i++) {
		if (_super_block.bitmap_inodes[i] == OCCUPIED) {
			if(strcmp(_super_block.inodes[i].directory_path, dir_inode.path) == 0){
				filler(buffer, _super_block.inodes[i].path, NULL, 0);
			}
		}
	}

	dir_inode.i_atime = time(NULL);

	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	if(offset < 0 || size < 0){
		return -EINVAL;
	}


	int inode_index = get_inode_index(path);
	if(inode_index == -1){
		return -ENOENT;
	}

	struct inode *inode = &_super_block.inodes[inode_index];

	if(inode->type == DIR){
		return -EISDIR;
	}

	char *content = inode->content;
	size_t file_size = inode->size;
	if(offset > file_size){
		return -EINVAL;
	}

	if(file_size - offset < size){
		size = file_size - offset;
	}

	strncpy(buffer, content + offset, size);
	inode->i_atime = time(NULL);

	return size;
}

void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisop_init\n");

	FILE *file = fopen(filedisk, "r");
	if(!file){
		initialize_filesystem();
	}else{
		int n = fread(&_super_block, sizeof(_super_block), 1, file);
		if(n != 1){
			return NULL;
		}
		fclose(file);
	}
	return 0;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_touch - path: %s\n", path);

	return new_inode(path, mode, REG);
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);

	return new_inode(path, mode, DIR);
}

//Libero inodo
void free_inode(int inode_index){
	struct inode *inode = &_super_block.inodes[inode_index];
	_super_block.bitmap_inodes[inode_index] = FREE;
	memset(inode, 0, sizeof(struct inode));
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index == -1){
		return -ENOENT;
		//return inode_index;
	}

	// Verificar que el inodo sea un directorio
	if (_super_block.inodes[inode_index].type != DIR) {
		return -ENOTDIR; //No es un directorio
	}

	// Verificar que el directorio esté vacío
	for (int i = 1; i < MAX_FILES; i++) {
		if (strcmp(_super_block.inodes[i].directory_path, path) == 0) {
			return -ENOTEMPTY;  // El directorio no está vacío
		}
	}

	// Liberar el inodo
	//struct inode *inode = &_super_block.inodes[inode_index];
	//_super_block.bitmap_inodes[inode_index] = FREE;
	//memset(inode, 0, sizeof(struct inode));
	free_inode(inode_index);
	
	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);

	int inode_index = get_inode_index(path);

	if (inode_index == -1) {
		return -ENOENT;  // No existe el archivo
	}

	if (_super_block.inodes[inode_index].type != REG) {
		return -EISDIR;  // No es un archivo
	}

	// Liberar el inodo
	//struct inode *inode = &_super_block.inodes[inode_index];
	//_super_block.bitmap_inodes[inode_index] = FREE;
	//memset(inode, 0, sizeof(struct inode));
	free_inode(inode_index);

	return 0;
}

/*
//COMENTAR
static int
fisopfs_open(const char path, struct fuse_file_infofi *fi)
{
	int inode_index = get_inode_index(path);

	if (inode_index == -1) {
		return -ENOENT;  // No existe el archivo
	}

	// Verificar que el inodo sea un archivo
	if (_super_block.inodes[inode_index].type != REG) {
		return -EISDIR;  // No es un archivo
	}

	return 0;
}
*/

int
fisopfs_write(const char *path,
              const char *data,
              size_t size_data,
              off_t offset,
              struct fuse_file_info *fuse_info)
{
	printf("[debug] fisops_write (%s) \n", path);

	// Verificar que el tamaño del archivo no exceda el límite
	if ((offset + size_data) > MAX_CONTENT) {
		return -EFBIG; // El archivo excede el tamaño máximo
	}

	int inode_index = get_inode_index(path);

	if (inode_index < 0) {// si no existe el archivo, lo creo
		int new_file = fisopfs_create(path, 33204, fuse_info);
		if (new_file < 0){
			return new_file;
		}
		inode_index = get_inode_index(path);
	}

	if (inode_index == -1) {
		return -ENOENT; // No existe el archivo
	}

	// Verificar que el inodo sea un archivo
	if (_super_block.inodes[inode_index].type != REG) {
		return -EISDIR;  // No es un archivo
	}

	if (_super_block.inodes[inode_index].size < offset) {
		return -EINVAL;
	}

	// Escribir en el archivo
	strncpy(_super_block.inodes[inode_index].content + offset, data, size_data);

	_super_block.inodes[inode_index].size = strlen(_super_block.inodes[inode_index].content);
	_super_block.inodes[inode_index].i_atime = time(NULL);
	_super_block.inodes[inode_index].i_mtime = time(NULL);
	_super_block.inodes[inode_index].content[_super_block.inodes[inode_index].size] = '\0';

	return (int) size_data;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s\n", path);

	int inode_index = get_inode_index(path);

	if (inode_index == -1) {
		return -ENOENT;
	}

	// Verificar que el tamaño del archivo no exceda el límite
	if (size > MAX_CONTENT) {
		return -EFBIG;  // El archivo excede el tamaño máximo
	}

	// Truncar el archivo
	_super_block.inodes[inode_index].size = size;
	_super_block.inodes[inode_index].i_mtime = time(NULL);
	_super_block.inodes[inode_index].i_atime = time(NULL);

	return 0;
}

/*
//COMENTAR
static int
fisopfs_opendir(const char path, struct fuse_file_infofi *fi)
{
	int inode_index = get_inode_index(path);

	if (inode_index == -1) {
		return -ENOENT;  // No existe el directorio
	}

	// Buscar el inodo correspondiente al path
	for (int i = 1; i < MAX_FILES; i++) {
		if (strcmp(_super_block.inodes[i].path, path) == 0) {
			inode_index = i;
			break;
		}
	}

	// Verificar que el inodo sea un directorio
	if (_super_block.inodes[inode_index].type != DIR) {
		return -ENOTDIR;  // No es un directorio
	}

	return 0;
}
*/

int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	int index_inodo = get_inode_index(path);

	if (index_inodo == -1) {
		return -ENOENT;
	}

	// Actualizar los tiempos de acceso y modificación
	_super_block.inodes[index_inodo].i_atime = tv[0].tv_sec;
	_super_block.inodes[index_inodo].i_mtime = tv[1].tv_sec;

	return 0;
}

void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisop_destroy\n");

	FILE *file = fopen(filedisk, "w");

	if (!file) {
		fprintf(stderr, "[debug] Error saving filesystem: %s\n", strerror(errno));
	}

	int n = fwrite(&_super_block, sizeof(_super_block), 1, file);
	if (n != 1) {
		fprintf(stderr, "[debug] Error destroy: %s\n", strerror(errno));
	}
	fflush(file);
	fclose(file);
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	fisopfs_destroy(NULL);
	return 0;
}

// Inicializa el sistema de archivos.
// Cuando no existe el archivo persistense_file.fisopfs, crea el archivo e
// inicializa el el directorio raiz y el _super_blockbloque
int
initialize_filesystem()
{
	memset(_super_block.inodes, 0, sizeof(_super_block.inodes));
	memset(_super_block.bitmap_inodes, 0, sizeof(_super_block.bitmap_inodes));

	struct inode *root_dir = &_super_block.inodes[0];
	root_dir->type = DIR;
	root_dir->mode = __S_IFDIR | 0755;
	root_dir->size = MAX_DIRECTORY_SIZE;
	root_dir->uid = 1717;
	root_dir->gid = getgid();
	root_dir->i_atime = time(NULL);
	root_dir->i_mtime = time(NULL);
	root_dir->i_ctime = time(NULL);
	strcpy(root_dir->path, ROOT_PATH);
	memset(root_dir->content, 0, sizeof(root_dir->content));
	strcpy(root_dir->directory_path, "");
	_super_block.bitmap_inodes[0] = OCCUPIED;
	return 0;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.init = fisopfs_init,
	.create = fisopfs_create,
	.mkdir = fisopfs_mkdir,
	.rmdir = fisopfs_rmdir,
	.unlink = fisopfs_unlink,
	.write = fisopfs_write,
	.truncate = fisopfs_truncate,
	.utimens = fisopfs_utimens,
	.destroy = fisopfs_destroy,
};

int
main(int argc, char *argv[])
{
	// El ultimo argumento es el path del archivo del fs, si es que se pasa
	if (strcmp(argv[1], "-f") == 0) {
		if (argc == 4) {
			strcpy(filedisk, argv[3]);
			argv[3] = NULL;
			argc--;
		}
	} else {
		if (argc == 3) {
			strcpy(filedisk, argv[2]);
			argv[2] = NULL;
			argc--;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}