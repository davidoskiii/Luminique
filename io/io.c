#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32

#include <direct.h>
#include <io.h>

#else

#include <unistd.h>
#define _chmod(path, mode) chmod(path, mode)
#define _getcwd(buffer, size) getcwd(buffer, size)
#define _mkdir(path) mkdir(path, 777)
#define _rmdir(path) rmdir(path)

#endif // _WIN32

#include "io.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../vm/vm.h"

static bool fileExists(ObjFile* file, struct stat* fileStat) {
  return (stat(file->name->chars, fileStat) == 0);
}

NATIVE_METHOD(File, __init__) {
  assertArgCount("File::__init__(pathname)", 1, argCount);
  assertArgIsString("File::__init__(pathname)", args, 0);
  ObjFile* self = newFile(AS_STRING(args[0]));
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) THROW_EXCEPTION(InstantiationError, "File or directory doesn't exist.");
  RETURN_OBJ(self);
}


NATIVE_METHOD(File, create) {
  assertArgCount("File::create()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (fileExists(self, &fileStat)) THROW_EXCEPTION(InstantiationError, "File or directory already exist.");
  FILE* file = fopen(self->name->chars, "w");
  if (file != NULL) {
    fclose(file);
    RETURN_TRUE;
  }
  RETURN_FALSE;
}

NATIVE_METHOD(File, delete) {
  assertArgCount("File::delete()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(remove(self->name->chars) == 0);
}

NATIVE_METHOD(File, exists) {
  assertArgCount("File::exists()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  RETURN_BOOL(fileExists(self, &fileStat));
}

NATIVE_METHOD(File, getAbsolutePath) {
  assertArgCount("File::getAbsolutePath()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) {
    assertError("Cannot get file absolute path because it does not exist.");
  }
  char buf[PATH_MAX];
  char *res = realpath(self->name->chars, buf);
  if (res == NULL) {
    assertError("Failed to retrieve the absolute path of the file.");
  }
  RETURN_OBJ(copyString(res, strlen(res)));
}

NATIVE_METHOD(File, isDirectory) {
  assertArgCount("File::isDirectory()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(fileStat.st_mode & S_IFDIR);
}

NATIVE_METHOD(File, isExecutable) {
  assertArgCount("File::isExecutable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(fileStat.st_mode & S_IEXEC);
}

NATIVE_METHOD(File, isFile) {
  assertArgCount("File::isFile()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(fileStat.st_mode & S_IFREG);
}

NATIVE_METHOD(File, isReadable) {
  assertArgCount("File::isReadable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(fileStat.st_mode & S_IREAD);
}

NATIVE_METHOD(File, isWritable) {
  assertArgCount("File::isWritable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(fileStat.st_mode & S_IWRITE);
}

NATIVE_METHOD(File, lastAccessed) {
  assertArgCount("File::lastAccessed()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) assertError("Cannot get file last accessed date because it does not exist.");
  RETURN_INT(fileStat.st_atime);
}

NATIVE_METHOD(File, lastModified) {
  assertArgCount("File::lastModified()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) assertError("Cannot get file last modified date because it does not exist.");
  RETURN_INT(fileStat.st_mtime);
}

NATIVE_METHOD(File, mkdir) {
  assertArgCount("File::mkdir()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(_mkdir(self->name->chars) == 0);
}

NATIVE_METHOD(File, name) {
  assertArgCount("File::name()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(File, rename) {
  assertArgCount("File::rename(name)", 1, argCount);
  assertArgIsString("File::rename(name)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(rename(self->name->chars, AS_STRING(args[0])->chars) == 0);
}

NATIVE_METHOD(File, rmdir) {
  assertArgCount("File::rmdir()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  RETURN_BOOL(_rmdir(self->name->chars) == 0);
}

NATIVE_METHOD(File, setExecutable) {
  assertArgCount("File::setExecutable(canExecute)", 1, argCount);
  assertArgIsBool("File::setExecutable(canExecute)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  if (AS_BOOL(args[0])) RETURN_BOOL(_chmod(self->name->chars, S_IEXEC));
  else RETURN_BOOL(_chmod(self->name->chars, ~S_IEXEC));
}

NATIVE_METHOD(File, setReadable) {
  assertArgCount("File::setReadable(canRead)", 1, argCount);
  assertArgIsBool("File::setReadable(canRead)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  if (AS_BOOL(args[0])) RETURN_BOOL(_chmod(self->name->chars, S_IREAD));
  else RETURN_BOOL(_chmod(self->name->chars, ~S_IREAD));
}

NATIVE_METHOD(File, setWritable) {
  assertArgCount("File::setWritable(canWrite)", 1, argCount);
  assertArgIsBool("File::setWritable(canWrite)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) RETURN_FALSE;
  if (AS_BOOL(args[0])) RETURN_BOOL(_chmod(self->name->chars, S_IWRITE));
  else RETURN_BOOL(_chmod(self->name->chars, ~S_IWRITE));
}


NATIVE_METHOD(File, size) {
  assertArgCount("File::size()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (!fileExists(self, &fileStat)) THROW_EXCEPTION(InstantiationError, "File or directory doesn't exist.");
  RETURN_NUMBER(fileStat.st_size);
}

NATIVE_METHOD(File, toString) {
  assertArgCount("File::toString()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

void registerIOPackage() {
  vm.fileClass = defineNativeClass("File");
  bindSuperclass(vm.fileClass, vm.objectClass);
  DEF_METHOD(vm.fileClass, File, __init__, 1);
  DEF_METHOD(vm.fileClass, File, create, 0);
  DEF_METHOD(vm.fileClass, File, delete, 0);
  DEF_METHOD(vm.fileClass, File, exists, 0);
  DEF_METHOD(vm.fileClass, File, getAbsolutePath, 0);
  DEF_METHOD(vm.fileClass, File, isDirectory, 0);
  DEF_METHOD(vm.fileClass, File, isExecutable, 0);
  DEF_METHOD(vm.fileClass, File, isFile, 0);
  DEF_METHOD(vm.fileClass, File, isReadable, 0);
  DEF_METHOD(vm.fileClass, File, isWritable, 0);
  DEF_METHOD(vm.fileClass, File, lastAccessed, 0);
  DEF_METHOD(vm.fileClass, File, lastModified, 0);
  DEF_METHOD(vm.fileClass, File, mkdir, 0);
  DEF_METHOD(vm.fileClass, File, name, 0);
  DEF_METHOD(vm.fileClass, File, rename, 1);
  DEF_METHOD(vm.fileClass, File, rmdir, 0);
  DEF_METHOD(vm.fileClass, File, setExecutable, 1);
  DEF_METHOD(vm.fileClass, File, setReadable, 1);
  DEF_METHOD(vm.fileClass, File, setWritable, 1);
  DEF_METHOD(vm.fileClass, File, size, 0);
  DEF_METHOD(vm.fileClass, File, toString, 0);
}
