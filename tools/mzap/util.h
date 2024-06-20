// Nancy's Utils Header

#ifndef NG_UTIL_H
#define NG_UTIL_H

#include <stdint.h>
#include <stddef.h>

//these should be redefinable, in case user wants to provide custom function
int file_exists(char *filename);
int path_is_folder(char *name);
int path_is_file(char *name);

int64_t file_size(char *filename);
uint64_t file_time(char *path);

uint8_t *read_whole_file(char *input_file_name, int64_t *file_size);
int write_whole_file(char *file_name, void *content, int size);

//also creates a path for thefile
int write_whole_file_path(char *file_name, void *content, int size);

typedef struct {
  char *dir;
  char *name;
  char *ext;
} url_t;

url_t *url_parts(char *path);


#define LF_SELF         0x01
#define LF_PARENT       0x02
#define LF_HIDDEN       0x04

char **list_folder(char *path, int flags);
char **list_tree(char *folder);

//specifies if the path has a filename part, otherwise treats it as a folder
#define NG_SKIPFN 1

void mkpath(char *path, int flags);

char *_hidden_cat_(char *a0, ...);



#define cat(...) _hidden_cat_(__VA_ARGS__, 0)
char *afmt(char *s, char *fmt, ...);
char *fmt(char *fmt, ...);

char *exec_command(char *cmd);

void *zmalloc(size_t size);

#define ALIGN(pgsz,ptr) (void*)(((uintptr_t)(ptr)+(pgsz)-1)/(pgsz)*(pgsz))

//aligns towards 0
#define ALIGN0(pgsz,ptr) (void*)(((uintptr_t)(ptr))/(pgsz)*(pgsz))

#define EMIT8(x) do {\
  uint32_t x1 = (uint32_t)(x); arrput(wb,(x1)&0xFF); } while(0)
#define EMIT16(x) do {\
  uint32_t x2 = (uint32_t)(x); EMIT8(x2);EMIT8((x2)>>8); } while(0)
#define EMIT24(x) do {\
  int32_t x4 = (uint32_t)(x); EMIT16(x4);EMIT8((x4)>>16); } while(0)
#define EMIT32(x) do {\
  uint32_t x4 = (uint32_t)(x); EMIT16(x4);EMIT16((x4)>>16); } while(0)
#define EMIT40(x) do {\
  uint64_t x8 = (uint64_t)(x); EMIT32(x8);EMIT8((x8)>>32); } while(0)
#define EMIT48(x) do {\
  uint64_t x8 = (uint64_t)(x); EMIT32(x8);EMIT16((x8)>>32); } while(0)
#define EMIT56(x) do {\
  uint64_t x8 = (uint64_t)(x); EMIT32(x8);EMIT24((x8)>>32); } while(0)
#define EMIT64(x) do {\
  uint64_t x8 = (uint64_t)(x); EMIT32(x8);EMIT32((x8)>>32); } while(0)



#define RD8 (*pin++)
#define RD16 (pin+=2, ((uint32_t)pin[-1]<<8) | (uint32_t)pin[-2])
#define RD24 (pin+=3,  ((uint32_t)pin[-1]<<16) \
                      |((uint32_t)pin[-2]<<8) \
                      | (uint32_t)pin[-3])
#define RD32 (pin+=4,  ((uint32_t)pin[-1]<<24) \
                      |((uint32_t)pin[-2]<<16) \
                      |((uint32_t)pin[-3]<<8) \
                      | (uint32_t)pin[-4])
#define RD40 (pin+=5,  ((uint64_t)pin[-1]<<32) \
                      |((uint64_t)pin[-2]<<24) \
                      |((uint64_t)pin[-3]<<16) \
                      |((uint64_t)pin[-4]<<8) \
                      | (uint64_t)pin[-5])

#define RD48 (pin+=6,  ((uint64_t)pin[-1]<<40) \
                      |((uint64_t)pin[-2]<<32) \
                      |((uint64_t)pin[-3]<<24) \
                      |((uint64_t)pin[-4]<<16) \
                      |((uint64_t)pin[-5]<<8) \
                      | (uint64_t)pin[-6])
#define RD56 (pin+=7,  ((uint64_t)pin[-1]<<48) \
                      |((uint64_t)pin[-2]<<40) \
                      |((uint64_t)pin[-3]<<32) \
                      |((uint64_t)pin[-4]<<24) \
                      |((uint64_t)pin[-5]<<16) \
                      |((uint64_t)pin[-6]<<8) \
                      | (uint64_t)pin[-7])
#define RD64 (pin+=8,  ((uint64_t)pin[-1]<<56) \
                      |((uint64_t)pin[-2]<<48) \
                      |((uint64_t)pin[-3]<<40) \
                      |((uint64_t)pin[-4]<<32) \
                      |((uint64_t)pin[-5]<<24) \
                      |((uint64_t)pin[-6]<<16) \
                      |((uint64_t)pin[-7]<<8) \
                      | (uint64_t)pin[-8])

#endif


#ifdef NG_UTIL_IMPLEMENTATION
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <dirent.h>

#include "stb_ds.h"

int file_exists(char *filename) {
  return access(filename, F_OK) != -1;
}

int64_t file_size(char *filename) {
  FILE *input_file = fopen(filename, "rb");
  if (!input_file) return 0;
  fseek(input_file, 0, SEEK_END);
  int64_t r = ftell(input_file);
  fclose(input_file);
  return r;
}

int path_is_folder(char *Name) {
  DIR *Dir;
  struct dirent *Ent;
  if(!(Dir = opendir(Name))) return 0;
  closedir(Dir);
  return 1;
}

int path_is_file(char *Name) {
  FILE *f;
  if (path_is_folder(Name)) return 0;
  f = fopen(Name, "r");
  if (!f) return 0;
  fclose(f);
  return 1;
}

uint64_t file_time(char *path) {
  struct stat attrib;
  if (!stat(path, &attrib)) return (uint64_t)attrib.st_mtime;
  return 0;
}

//it also adds 0 at the end of file, so a text file can be used immediatly
uint8_t *read_whole_file(char *path, int64_t *file_size) {
  uint8_t *file_contents;
  int64_t fsz;
  FILE *input_file = fopen(path, "rb");
  if (!input_file) return 0;
  fseek(input_file, 0, SEEK_END);
  fsz = ftell(input_file);
  rewind(input_file);
  file_contents = malloc(fsz + 1);
  file_contents[fsz] = 0;
  fread(file_contents, 1, fsz, input_file);
  fclose(input_file);
  if (file_size) *file_size = fsz;
  return file_contents;
}

int write_whole_file(char *file_name, void *content, int size) {
  FILE *file = fopen(file_name, "wb");
  if (!file) return 0;
  fwrite(content, 1, size, file);
  fclose(file);
  return 1;
}

url_t *url_parts(char *path) {
  int bufsz = strlen(path)+3;
  url_t *fn = malloc(sizeof(url_t)+bufsz);
  char *r = (char*)(fn+1);
  char *dir;
  char *name;
  char *ext;
  char *p, *q;
  int l;

  if ((p = strrchr(path, '/'))) {
    l = p-path + 1;
    dir = r;
    strncpy(dir, path, l);
    dir[l] = 0;
    p++;
    r += l+1;
  } else {
    p = path;
    dir = r;
    *r++ = 0;
  }
  if ((q = strrchr(p, '.'))) {
    l = q-p;
    name = r;
    strncpy(name, p, l);
    name[l] = 0;
    q++;
    r += l+1;

    ext = r;
    l = strlen(q);
    strcpy(ext, q);
  } else {
    ext = r;
    *r++ = 0;
    name = r;
    strcpy(name, p);
  }

  fn->dir = dir;
  fn->name = name;
  fn->ext = ext;

  return fn;
}


char **list_folder(char *path, int flags) {
  char tmp[1024];
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) == NULL) return 0;
  char **ns = 0;
  while ((ent = readdir(dir)) != NULL) {
    if (!(flags&LF_SELF) && !strcmp(ent->d_name, ".")) continue;
    if (!(flags&LF_PARENT) && !strcmp(ent->d_name, "..")) continue;
    if (!(flags&LF_HIDDEN)) {
      if (ent->d_name[0] == '.') {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
        } else {
          continue;
        }
      }
    }
    sprintf(tmp, "%s%s", path, ent->d_name);
    char *n;
    if (path_is_folder(tmp)) {
      sprintf(tmp, "%s/", ent->d_name);
      n = strdup(tmp);
    } else {
      n = strdup(ent->d_name);
    }
    arrput(ns,n);
  }
  closedir(dir);
  return ns;
}

char **list_tree(char *folder) {
  char **tree = 0;
  char **names = list_folder(folder,0);
  for (int i = 0; i < arrlen(names); i++) {
    char *n = names[i];
    int nl = strlen(n);
    char *p = cat(folder,n);
    arrput(tree, p);
    if (n[nl-1] == '/' || n[nl-1] == '\\') {
      char **stree = list_tree(p);
      for (int j = 0; j < arrlen(stree); j++) {
        arrput(tree, stree[j]);
      }
      arrfree(stree);
    }
    free(n);
  }
  arrfree(names);
  return tree;
}

char *_hidden_cat_(char *a0, ...) {
  char *a;
  va_list ap;
  int size = strlen(a0);
  va_start(ap, a0);
  for (;;) {
    a = va_arg(ap, char*);
    if (!a) break;
    size += strlen(a);
  }
  va_end(ap);
  size += 1;

  char *o = malloc(size);

  char *p = o;

  a = a0;
  while (*a) *p++ = *a++;

  va_start(ap, a0);
  for (;;) {
    a = va_arg(ap, char*);
    if (!a) break;
    while (*a) *p++ = *a++;
  }
  va_end(ap);

  *p++ = 0;

  return o;
}


char *afmt(char *s, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int l = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  int sl = arrlen(s);
  arrsetlen(s, sl+l+1);
  va_start(ap, fmt);
  vsprintf(s+sl, fmt, ap);
  va_end(ap);
  arrpop(s);
  return s;
}

char *fmt(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int l = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *s = NULL;
  arrsetlen(s, l+1);
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  return s;
}


char *exec_command(char *cmd) {
  char *r;
  int rdsz = 1024;
  int len = rdsz;
  int pos = 0;
  int s;
  FILE *in = popen(cmd, "r");

  if (!in) return 0;

  r = (char*)malloc(len);

  for(;;) {
    if (pos + rdsz > len) {
      char *t = r;
      len = len*2;
      r = (char*)malloc(len);
      memcpy(r, t, pos);
      free(t);
    }
    s = fread(r+pos,1,rdsz,in);
    pos += s;
    if (s != rdsz) break;
  }

  pclose(in);

  r[pos] = 0;
  if (pos && r[pos-1] == '\n') r[pos-1] = 0;

  return r;
}

void *zmalloc(size_t size) {
  void *r = malloc(size);
  memset(r, 0, size);
  return r;
}


#ifdef WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
static void mkfolder(const char *path, int mode) {
  mkdir(path);
}
#else
static void mkfolder(const char *path, int mode) {
  mkdir(path,mode);
}
#endif


#include <unistd.h>

void mkpath(char *path, int flags) {
  char t[1024], *p;
  strcpy(t, path);
  p = t;
  while((p = strchr(p+1, '/'))) {
    if (p[1] == '.' && p[2] == '/') {
      char *q = p;
      char *r = p+2;
      while ((*q++ = *r++));
    }
    *p = 0;
    // create a directory named with read/write/search permissions for
    // owner and group, and with read/search permissions for others.
    if (!file_exists(t)) mkfolder(t, 0775);
    *p = '/';
  }
  if (!(flags&NG_SKIPFN)) {
    if (!file_exists(t)) mkfolder(t, 0775);
    //free(shell("mkdir -p '%s'", Path));
  }
}

int write_whole_file_path(char *file_name, void *content, int size) {
  mkpath(file_name, NG_SKIPFN);
  return write_whole_file(file_name, content, size);
}


#if 0
#define FILE_SIZE_ERROR 0xFFFFFFFF
static uint32_t fileSize(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) return FILE_SIZE_ERROR;
  fseek(fp, 0L, SEEK_END);
  uint32_t sz = ftell(fp);
  fclose(fp);
  return sz;
}

static uint8_t *fileGet(uint32_t *rsize, char *filename) {
  uint32_t sz = fileSize(filename);
  if (sz == FILE_SIZE_ERROR) return 0;
  FILE *fp = fopen(filename, "rb");
  if (!fp) return 0;
  *rsize = sz;
  uint8_t *p = 0;
  arrsetlen(p, sz+1);
  p[sz] = 0;
  fread(p, 1, sz, fp);
  fclose(fp);
  return p;
}

static int fileSet(char *filename, uint8_t *data, uint32_t size) {
  FILE *fp = fopen(filename, "wb");
  if (!fp) return 0;
  fwrite(data, 1, size, fp);
  fclose(fp);
  return 1;
}

static int fileExist(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) return 0;
  fclose(fp);
  return 1;
}
#endif

#endif