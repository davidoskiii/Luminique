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
#define fopen_s(fp,filename,mode) ((*(fp))=fopen((filename),(mode)))==NULL
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

static ObjFile* getFileArgument(Value arg) {
  ObjFile* file = NULL;
  if (IS_STRING(arg)) file = newFile(AS_STRING(arg));
  else if (IS_FILE(arg)) file = AS_FILE(arg);
  return file;
}

static ObjFile* getFileProperty(ObjInstance* object, char* field) {
  return AS_FILE(getObjProperty(object, field));
}


static void setFileProperty(ObjInstance* object, ObjFile* file, char* mode) {
  file->file = fopen(file->name->chars, mode);

  if (file->file == NULL) {
    assertError("Cannot create IOStream, file either does not exist or require additional permission to access.");
  }

  file->isOpen = true;
  file->mode = newString(mode);
  setObjProperty(object, "file", OBJ_VAL(file));
}


NATIVE_METHOD(BinaryReadStream, __init__) {
  assertArgCount("BinaryReadStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) assertError("Method BinaryReadStream::__init__(file) expects argument 1 to be a string or file.");
  setFileProperty(AS_INSTANCE(receiver), file, "rb");
  RETURN_OBJ(self);
}

NATIVE_METHOD(BinaryReadStream, next) {
  assertArgCount("BinaryReadStream::next()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot read the next byte because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    unsigned char byte;
    if (fread(&byte, sizeof(char), 1, file->file) > 0) {
      RETURN_INT((int)byte);
    }
    RETURN_NIL;
  }
  RETURN_NIL;
}

NATIVE_METHOD(BinaryReadStream, nextBytes) {
  assertArgCount("BinaryReadStream::nextBytes(length)", 1, argCount);
  assertArgIsInt("BinaryReadStream::nextBytes(length)", args, 0);
  assertNumberPositive("BinaryReadStream::nextBytes(length)", AS_NUMBER(args[0]), 0);
  int length = AS_INT(args[0]);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot read the next byte because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    ObjArray* bytes = newArray();
    push(OBJ_VAL(bytes));
    for (int i = 0; i < length; i++) {
      unsigned char byte;
      if (fread(&byte, sizeof(char), 1, file->file) == 0) break;
      writeValueArray(&bytes->elements, INT_VAL((int)byte));
    }
    pop(vm);
    RETURN_OBJ(bytes);
  }
}

NATIVE_METHOD(BinaryWriteStream, __init__) {
  assertArgCount("BinaryWriteStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) assertError("Method BinaryWriteStream::__init__(file) expects argument 1 to be a string or file.");
  setFileProperty(AS_INSTANCE(receiver), file, "wb");
  RETURN_OBJ(self);
}

NATIVE_METHOD(BinaryWriteStream, put) {
  assertArgCount("BinaryWriteStream::put(byte)", 1, argCount);
  assertArgIsInt("BinaryWriteStream::put(bytes)", args, 0);
  int byte = AS_INT(args[0]);
  assertIntWithinRange("BinaryWriteStream::put(byte)", byte, 0, 255, 0);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write byte to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    unsigned char bytes[1] = { (unsigned char)byte };
    fwrite(bytes, sizeof(char), 1, file->file);
    RETURN_NIL;
  }
}

NATIVE_METHOD(BinaryWriteStream, putBytes) {
  assertArgCount("BinaryWriteStream::putBytes(bytes)", 1, argCount);
  assertArgIsArray("BinaryWriteStream::putBytes(bytes)", args, 0);
  ObjArray* bytes = AS_ARRAY(args[0]);
  if (bytes->elements.count == 0) assertError("Cannot write empty byte array to stream.");

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write bytes to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    unsigned char* byteArray = (unsigned char*)malloc(bytes->elements.count);
    if (byteArray != NULL) {
      for (int i = 0; i < bytes->elements.count; i++) {
        if (!IS_INT(bytes->elements.values[i])) assertError("Cannot write bytes to stream because data is corrupted.");
        int byte = AS_INT(bytes->elements.values[i]);
        byteArray[i] = (unsigned char)byte;
      }
      fwrite(byteArray, sizeof(char), (size_t)bytes->elements.count, file->file);
      free(byteArray);
    }
    RETURN_NIL;
  }
}

NATIVE_METHOD(File, __init__) {
  assertArgCount("File::__init__(pathname)", 1, argCount);
  assertArgIsString("File::__init__(pathname)", args, 0);
  ObjFile* self = newFile(AS_STRING(args[0]));
  struct stat fileStat;
  RETURN_OBJ(self);
}


NATIVE_METHOD(File, create) {
  assertArgCount("File::create()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  struct stat fileStat;
  if (fileExists(self, &fileStat)) THROW_EXCEPTION(luminique::std::lang, InstantiationException, "File or directory already exist.");
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
  if (!fileExists(self, &fileStat)) THROW_EXCEPTION(luminique::std::lang, InstantiationException, "File or directory doesn't exist.");
  RETURN_NUMBER(fileStat.st_size);
}

NATIVE_METHOD(File, __str__) {
  assertArgCount("File::__str__()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(File, __format__) {
  assertArgCount("File::__format__()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(FileReadStream, __init__) {
  assertArgCount("FileReadStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method FileReadStream::__init__(file) expects argument 1 to be a string or file.");
  setFileProperty(AS_INSTANCE(receiver), file, "r");  RETURN_OBJ(self);
}

NATIVE_METHOD(FileReadStream, next) {
  assertArgCount("FileReadStream::next()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot read the next char because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    int c = fgetc(file->file);
    if (c == EOF) RETURN_NIL;
    char ch[2] = { c, '\0' };
    RETURN_STRING(ch, 1);
  }
}

NATIVE_METHOD(FileReadStream, nextLine) {
  assertArgCount("FileReadStream::nextLine()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot read the next line because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    char line[UINT8_MAX];
    if (fgets(line, sizeof line, file->file) == NULL) RETURN_NIL;
    RETURN_STRING(line, (int)strlen(line));
  }
}

NATIVE_METHOD(FileReadStream, peek) {
  assertArgCount("FileReadStream::peek()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot peek the next char because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  else {
    int c = fgetc(file->file);
    ungetc(c, file->file);
    if (c == EOF) RETURN_NIL;
    char ch[2] = { c, '\0' };
    RETURN_STRING(ch, 1);
  }
}

NATIVE_METHOD(FileWriteStream, __init__) {
  assertArgCount("FileWriteStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method FileWriteStream::__init__(file) expects argument 1 to be a string or file.");
  setFileProperty(AS_INSTANCE(receiver), file, "w");
  RETURN_OBJ(self);
}

NATIVE_METHOD(FileWriteStream, put) {
  assertArgCount("FileWriteStream::put(char)", 1, argCount);
  assertArgIsString("FileWriteStream::put(char)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write character to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;

  ObjString* character = AS_STRING(args[0]);
  if (character->length != 1) assertError("Method FileWriteStream::put(char) expects argument 1 to be a character(string of length 1)");
  fputc(character->chars[0], file->file);
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, putLine) {
  assertArgCount("FileWriteStream::putLine()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write new line to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  fputc('\n', file->file);
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, putSpace) {
  assertArgCount("FileWriteStream::putSpace()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write empty space to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;
  fputc(' ', file->file);
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, putString) {
  assertArgCount("FileWriteStream::putString(string)", 1, argCount);
  assertArgIsString("FileWriteStream::putString(string)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot write string to stream because file is already closed.");
  if (file->file == NULL) RETURN_NIL;

  ObjString* string = AS_STRING(args[0]);
  fputs(string->chars, file->file);
  RETURN_NIL;
}


NATIVE_METHOD(IOStream, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class IOStream.");
  RETURN_NIL;
}

NATIVE_METHOD(IOStream, close) {
  assertArgCount("IOStream::close()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  file->isOpen = false;
  RETURN_BOOL(fclose(file->file) == 0);
}


NATIVE_METHOD(IOStream, file) {
  assertArgCount("IOStream::file()", 0, argCount);
  RETURN_OBJ(getFileProperty(AS_INSTANCE(receiver), "file"));
}

NATIVE_METHOD(IOStream, getPosition) {
  assertArgCount("IOStream::getPosition()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot get stream position because file is already closed.");
  if (file->file == NULL) RETURN_INT(0);
  else RETURN_INT(ftell(file->file));
}

NATIVE_METHOD(IOStream, reset) {
  assertArgCount("IOStream::reset()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot reset stream because file is already closed.");
  if (file->file != NULL) rewind(file->file);
  RETURN_NIL;
}


NATIVE_METHOD(ReadStream, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class ReadStream.");
  RETURN_NIL;
}

NATIVE_METHOD(ReadStream, isAtEnd) {
  assertArgCount("ReadStream::isAtEnd()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen || file->file == NULL) RETURN_FALSE;
  else {
    int c = getc(file->file);
    ungetc(c, file->file);
    RETURN_BOOL(c == EOF);
  }
}

NATIVE_METHOD(ReadStream, next) {
  THROW_EXCEPTION(luminique::std::lang, CallException, "Cannot call method ReadStream::next(), it must be implemented by subclasses.");
  RETURN_NIL;
}

NATIVE_METHOD(ReadStream, skip) {
  assertArgCount("ReadStream::skip(offset)", 1, argCount);
  assertArgIsInt("ReadStream::skip(offset)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot skip stream by offset because file is already closed.");
  if (file->file == NULL) RETURN_FALSE;
  RETURN_BOOL(fseek(file->file, (long)AS_INT(args[0]), SEEK_CUR));
}

NATIVE_METHOD(WriteStream, flush) {
  assertArgCount("WriteStream::flush()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) assertError("Cannot flush stream because file is already closed.");
  if (file->file == NULL) RETURN_FALSE;
  RETURN_BOOL(fflush(file->file) == 0);
}

NATIVE_METHOD(WriteStream, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class WriteStream.");
  RETURN_NIL;
}

NATIVE_METHOD(WriteStream, put) {
  THROW_EXCEPTION(luminique::std::lang, CallException, "Cannot call method WriteStream::put(param), it must be implemented by subclasses.");
  RETURN_NIL;
}

void registerIOPackage() {
  ObjNamespace* ioNamespace = defineNativeNamespace("io", vm.stdNamespace);
  vm.currentNamespace = ioNamespace;

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
  DEF_METHOD(vm.fileClass, File, __str__, 0);
  DEF_METHOD(vm.fileClass, File, __format__, 0);


  ObjClass* ioStreamClass = defineNativeClass("IOStream");
  bindSuperclass(ioStreamClass, vm.objectClass);
  DEF_METHOD(ioStreamClass, IOStream, __init__, 1);
  DEF_METHOD(ioStreamClass, IOStream, close, 0);
  DEF_METHOD(ioStreamClass, IOStream, file, 0);
  DEF_METHOD(ioStreamClass, IOStream, getPosition, 0);
  DEF_METHOD(ioStreamClass, IOStream, reset, 0);


  ObjClass* readStreamClass = defineNativeClass("ReadStream");
  bindSuperclass(readStreamClass, ioStreamClass);
  DEF_METHOD(readStreamClass, ReadStream, __init__, 1);
  DEF_METHOD(readStreamClass, ReadStream, isAtEnd, 0);
  DEF_METHOD(readStreamClass, ReadStream, next, 0);
  DEF_METHOD(readStreamClass, ReadStream, skip, 1);

  ObjClass* writeStreamClass = defineNativeClass("WriteStream");
  bindSuperclass(writeStreamClass, ioStreamClass);
  DEF_METHOD(writeStreamClass, WriteStream, __init__, 1);
  DEF_METHOD(writeStreamClass, WriteStream, flush, 0);
  DEF_METHOD(writeStreamClass, WriteStream, put, 1);

  ObjClass* binaryReadStreamClass = defineNativeClass("BinaryReadStream");
  bindSuperclass(binaryReadStreamClass, readStreamClass);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, __init__, 1);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, next, 0);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, nextBytes, 1);

  ObjClass* binaryWriteStreamClass = defineNativeClass("BinaryWriteStream");
  bindSuperclass(binaryWriteStreamClass, writeStreamClass);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, __init__, 1);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, put, 1);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, putBytes, 1);

  ObjClass* fileReadStreamClass = defineNativeClass("FileReadStream");
  bindSuperclass(fileReadStreamClass, readStreamClass);
  DEF_METHOD(fileReadStreamClass, FileReadStream, __init__, 1);
  DEF_METHOD(fileReadStreamClass, FileReadStream, next, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, nextLine, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, peek, 0);


  ObjClass* fileWriteStreamClass = defineNativeClass("FileWriteStream");
  bindSuperclass(fileWriteStreamClass, writeStreamClass);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, __init__, 1);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, put, 1);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, putLine, 0);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, putSpace, 0);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, putString, 1);

  vm.currentNamespace = vm.rootNamespace;
}
