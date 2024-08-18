#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "io.h"
#include "../os/os.h"
#include "../vm/vm.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../date/date.h"
#include "../string/string.h"
#include "../file/file.h"

NATIVE_METHOD(BinaryReadStream, __init__) {
  assertArgCount("BinaryReadStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method BinaryReadStream::__init__(file) expects argument 1 to be a string or file.");
  if (!setFileProperty(AS_INSTANCE(receiver), file, "rb")) {
    THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create BinaryReadStream, file either does not exist or require additional permission to access.");
  }
  if (!loadFileRead(file)) THROW_EXCEPTION(luminique::std::io, IOException, "Unable to read from binary stream.");
  RETURN_OBJ(self);
}

NATIVE_METHOD(BinaryReadStream, read) {
  assertArgCount("BinaryReadStream::read()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next byte because file is already closed.");
  if (file->fsOpen != NULL && file->fsRead != NULL) RETURN_INT(fileReadByte(file));
  RETURN_NIL;
}

NATIVE_METHOD(BinaryReadStream, readAsync) {
  assertArgCount("BinaryReadStream::readAsync()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next byte because file is already closed.");
  loadFileRead(file);

  ObjPromise* promise = fileReadAsync(file, fileOnReadByte);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read byte from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(BinaryReadStream, readBytes) {
  assertArgCount("BinaryReadStream::readBytes(length)", 1, argCount);
  assertArgIsInt("BinaryReadStream::readBytes(length)", args, 0);
  int length = AS_INT(args[0]);
  if (length < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method BinaryReadStream::readBytes(length) expects argument 1 to be a positive integer but got %g.", length);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next bytes because file is already closed.");
  if (file->fsOpen != NULL && file->fsRead != NULL) {
    ObjArray* bytes = fileReadBytes(file, length);
    if (bytes == NULL) THROW_EXCEPTION_FMT(luminique::std::lang, OutOfMemoryException, "Not enough memory to read the next %d bytes.", length);
    RETURN_OBJ(bytes);
  }
  RETURN_NIL;
}

NATIVE_METHOD(BinaryReadStream, readBytesAsync) {
  assertArgCount("BinaryReadStream::readBytesAsync(length)", 1, argCount);
  assertArgIsInt("BinaryReadStream::readBytesAsync(length)", args, 0);
  int length = AS_INT(args[0]);
  if (length < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method BinaryReadStream::readBytesAsync(length) expects argument 1 to be a positive integer but got %g.", length);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next bytes because file is already closed.");
  loadFileRead(file);

  ObjPromise* promise = fileReadStringAsync(file, (size_t)length, fileOnReadBytes);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read byte from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(BinaryWriteStream, __init__) {
  assertArgCount("BinaryWriteStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method BinaryWriteStream::__init__(file) expects argument 1 to be a string or file.");
  if (!setFileProperty(AS_INSTANCE(receiver), file, "wb")) {
    THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create BinaryWriteStream, file either does not exist or require additional permission to access.");
  }
  if (!loadFileWrite(file)) THROW_EXCEPTION(luminique::std::io, IOException, "Unable to write to binary stream.");
  RETURN_OBJ(self);
}

NATIVE_METHOD(BinaryWriteStream, write) {
  assertArgCount("BinaryWriteStream::write(byte)", 1, argCount);
  assertArgIsInt("BinaryWriteStream::write(byte)", args, 0);
  int arg = AS_INT(args[0]);
  assertIntWithinRange("BinaryWriteStream::write(byte)", arg, 0, 255, 0);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write byte to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) fileWriteByte(file, (uint8_t)arg);
  RETURN_NIL;
}

NATIVE_METHOD(BinaryWriteStream, writeAsync) {
  assertArgCount("BinaryWriteStream::writeAsync(byte)", 1, argCount);
  assertArgIsInt("BinaryWriteStream::writeAsync(byte)", args, 0);
  int arg = AS_INT(args[0]);
  assertIntWithinRange("BinaryWriteStream::writeAsync(byte)", arg, 0, 255, 0);

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write byte to stream because file is already closed.");
  loadFileWrite(file);

  uint8_t byte = (uint8_t)arg;
  ObjPromise* promise = fileWriteByteAsync(file, byte, fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write byte to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(BinaryWriteStream, writeBytes) {
  assertArgCount("BinaryWriteStream::writeBytes(bytes)", 1, argCount);
  assertArgIsArray("BinaryWriteStream::writeBytes(bytes)", args, 0);
  ObjArray* bytes = AS_ARRAY(args[0]);
  if (bytes->elements.count == 0) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write empty byte array to stream.");

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write bytes to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) fileWriteBytes(file, bytes);
  RETURN_NIL;
}

NATIVE_METHOD(BinaryWriteStream, writeBytesAsync) {
  assertArgCount("BinaryWriteStream::writeBytesAsync(bytes)", 1, argCount);
  assertArgIsArray("BinaryWriteStream::writeBytesAsync(bytes)", args, 0);
  ObjArray* bytes = AS_ARRAY(args[0]);
  if (bytes->elements.count == 0) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write empty byte array to stream.");

  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write bytes to stream because file is already closed.");
  loadFileWrite(file);

  ObjPromise* promise = fileWriteBytesAsync(file, bytes, fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, __init__) {
  assertArgCount("File::__init__(pathname)", 1, argCount);
  assertArgIsString("File::__init__(pathname)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  self->name = AS_STRING(args[0]);
  self->mode = emptyString();
  self->isOpen = false;
  RETURN_OBJ(self);
}

NATIVE_METHOD(File, create) {
  assertArgCount("File::create()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (fileExists(self)) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create new file because it already exists");
  RETURN_BOOL(fileCreate(self));
}

NATIVE_METHOD(File, createAsync) {
  assertArgCount("File::createAsync()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (fileExists(self)) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create new file because it already exists");
  ObjPromise* promise = fileCreateAsync(self, fileOnCreate);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to create file because of system runs out of memory.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, delete) {
  assertArgCount("File::delete()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot delete file because it does not exist.");
  RETURN_BOOL(fileDelete(self));
}

NATIVE_METHOD(File, deleteAsync) {
  assertArgCount("File::deleteAsync()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot delete file because it does not exist.");
  ObjPromise* promise = fileDeleteAsync(self, fileOnHandle);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to delete file because of system runs out of memory.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, exists) {
  assertArgCount("File::exists()", 0, argCount);
  RETURN_BOOL(fileExists(AS_FILE(receiver)));
}

NATIVE_METHOD(File, getAbsolutePath) {
  assertArgCount("File::getAbsolutePath()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  ObjString* realPath = fileGetAbsolutePath(self);
  if (realPath == NULL) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot get file absolute path because it does not exist.");
  RETURN_OBJ(realPath);
}

NATIVE_METHOD(File, isDirectory) {
  assertArgCount("File::isDirectory()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) RETURN_FALSE;
  RETURN_BOOL(self->fsStat->statbuf.st_mode & S_IFDIR);
}

NATIVE_METHOD(File, isExecutable) {
  assertArgCount("File::isExecutable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) RETURN_FALSE;
  RETURN_BOOL(self->fsStat->statbuf.st_mode & S_IEXEC);
}

NATIVE_METHOD(File, isFile) {
  assertArgCount("File::isFile()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) RETURN_FALSE;
  RETURN_BOOL(self->fsStat->statbuf.st_mode & S_IFREG);
}

NATIVE_METHOD(File, isReadable) {
  assertArgCount("File::isReadable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) RETURN_FALSE;
  RETURN_BOOL(self->fsStat->statbuf.st_mode & S_IREAD);
}

NATIVE_METHOD(File, isWritable) {
  assertArgCount("File::isWritable()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) RETURN_FALSE;
  RETURN_BOOL(self->fsStat->statbuf.st_mode & S_IWRITE);
}

NATIVE_METHOD(File, lastAccessed) {
  assertArgCount("File::lastAccessed()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot get file last accessed date because it does not exist.");
  ObjInstance* lastAccessed = dateTimeObjFromTimestamp(getNativeClass("luminique::std::chrono", "DateTime"), (double)self->fsStat->statbuf.st_atim.tv_sec);
  RETURN_OBJ(lastAccessed);
}

NATIVE_METHOD(File, lastModified) {
  assertArgCount("File::lastModified()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot get file last modified date because it does not exist.");
  ObjInstance* lastModified = dateTimeObjFromTimestamp(getNativeClass("luminique::std::chrono", "DateTime"), (double)self->fsStat->statbuf.st_mtim.tv_sec);
  RETURN_OBJ(lastModified);
}

NATIVE_METHOD(File, mkdir) {
  assertArgCount("File::mkdir()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (fileExists(self)) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot create directory as it already exists in the file system.");
  RETURN_BOOL(fileMkdir(self));
}

NATIVE_METHOD(File, mkdirAsync) {
  assertArgCount("File::mkdirAsync()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (fileExists(self)) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot create directory as it already exists in the file system.");
  ObjPromise* promise = FileMkdirAsync(self, fileOnHandle);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to create directory because of system runs out of memory.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, name) {
  assertArgCount("File::name()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(File, rename) {
  assertArgCount("File::rename(name)", 1, argCount);
  assertArgIsString("File::rename(name)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot rename file as it does not exist.");
  RETURN_BOOL(fileRename(self, AS_STRING(args[0])));
}

NATIVE_METHOD(File, renameAsync) {
  assertArgCount("File::renameAsync(name)", 1, argCount);
  assertArgIsString("File::renameAsync(name)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot rename file as it does not exist.");

  ObjPromise* promise = fileRenameAsync(self, AS_STRING(args[0]), fileOnHandle);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to rename file because of system runs out of memory.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, rmdir) {
  assertArgCount("File::rmdir()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot remove directory as it does not exist.");
  RETURN_BOOL(fileRmdir(self));
}

NATIVE_METHOD(File, rmdirAsync) {
  assertArgCount("File::rmdir()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot remove directory as it does not exist.");
  ObjPromise* promise = fileRmdirAsync(self, fileOnHandle);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to delete directory because of system runs out of memory.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(File, setExecutable) {
  assertArgCount("File::setExecutable(canExecute)", 1, argCount);
  assertArgIsBool("File::setExecutable(canExecute)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot change file permission as it does not exist.");

  uv_fs_t fsChmod;
  int changed = uv_fs_chmod(vm.eventLoop, &fsChmod, self->name->chars, AS_BOOL(args[0]) ? S_IEXEC : ~S_IEXEC, NULL);
  uv_fs_req_cleanup(&fsChmod);
  RETURN_BOOL(changed == 0);
}

NATIVE_METHOD(File, setReadable) {
  assertArgCount("File::setReadable(canRead)", 1, argCount);
  assertArgIsBool("File::setReadable(canRead)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot change file permission as it does not exist.");

  uv_fs_t fsChmod;
  int changed = uv_fs_chmod(vm.eventLoop, &fsChmod, self->name->chars, AS_BOOL(args[0]) ? S_IREAD : ~S_IREAD, NULL);
  uv_fs_req_cleanup(&fsChmod);
  RETURN_BOOL(changed == 0);
}

NATIVE_METHOD(File, setWritable) {
  assertArgCount("File::setWritable(canWrite)", 1, argCount);
  assertArgIsBool("File::setWritable(canWrite)", args, 0);
  ObjFile* self = AS_FILE(receiver);
  if (!fileExists(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot change file permission as it does not exist.");

  uv_fs_t fsChmod;
  int changed = uv_fs_chmod(vm.eventLoop, &fsChmod, self->name->chars, AS_BOOL(args[0]) ? S_IWRITE : ~S_IWRITE, NULL);
  uv_fs_req_cleanup(&fsChmod);
  RETURN_BOOL(changed == 0);
}

NATIVE_METHOD(File, size) {
  assertArgCount("File::size()", 0, argCount);
  ObjFile* self = AS_FILE(receiver);
  if (!loadFileStat(self)) THROW_EXCEPTION(luminique::std::io, FileNotFoundException, "Cannot get file size because it does not exist.");
  RETURN_NUMBER((double)self->fsStat->statbuf.st_size);
}

NATIVE_METHOD(File, __str__) {
  assertArgCount("File::__str__()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(File, __format__) {
  assertArgCount("File::__format__()", 0, argCount);
  RETURN_OBJ(AS_FILE(receiver)->name);
}

NATIVE_METHOD(FileClass, open) {
  assertArgCount("File class::open(pathname, mode)", 2, argCount);
  assertArgIsString("File class::open(pathname, mode)", args, 0);
  assertArgIsString("File class::open(pathname, mode)", args, 1);
  char* mode = AS_CSTRING(args[1]);
  ObjFile* file = newFile(AS_STRING(args[0]));

  push(OBJ_VAL(file));
  char* streamClass = streamClassName(mode);
  if (streamClass == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Invalid file open mode specified.");
  ObjInstance* stream = newInstance(getNativeClass("luminique::std::io", streamClass));
  
  if (!setFileProperty(stream, file, mode)) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot open IO stream, file either does not exist or require additional permission to access."); 
  loadFileOperation(file, streamClass);
  pop();
  RETURN_OBJ(stream);
}

NATIVE_METHOD(FileClass, openAsync) {
  assertArgCount("File class::openAsync(pathname, mode)", 2, argCount);
  assertArgIsString("File class::openAsync(pathname, mode)", args, 0);
  assertArgIsString("File class::openAsync(pathname, mode)", args, 1);
  char* mode = AS_CSTRING(args[1]);
  ObjFile* file = newFile(AS_STRING(args[0]));
  ObjPromise* promise = fileOpenAsync(file, mode, fileOnOpen);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to open IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileReadStream, __init__) {
  assertArgCount("FileReadStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) assertError("Method FileReadStream::__init__(file) expects argument 1 to be a string or file.");
  if (!setFileProperty(AS_INSTANCE(receiver), file, "r")) {
      THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create FileReadStream, file either does not exist or require additional permission to access.");
  }
  if (!loadFileRead(file)) THROW_EXCEPTION(luminique::std::io, IOException, "Unable to read from file stream.");
  RETURN_OBJ(self);
}

NATIVE_METHOD(FileReadStream, peek) {
  assertArgCount("FileReadStream::peek()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot peek the next char because file is already closed.");
  if (file->fsOpen == NULL) RETURN_NIL;
  else {
    ObjString* ch = fileRead(file, true);
    if (ch == NULL) RETURN_NIL;
    RETURN_OBJ(ch);
  }
}

NATIVE_METHOD(FileReadStream, read) {
  assertArgCount("FileReadStream::read()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next char because file is already closed.");
  if (file->fsOpen == NULL) RETURN_NIL;
  else {
    ObjString* ch = fileRead(file, false);
    if (ch == NULL) RETURN_NIL;
    RETURN_OBJ(ch);
  }
}

NATIVE_METHOD(FileReadStream, readAsync) {
  assertArgCount("FileReadStream::readAsync()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next char because file is already closed.");
  loadFileRead(file);

  ObjPromise* promise = fileReadAsync(file, fileOnRead);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read char from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileReadStream, readLine) {
  assertArgCount("FileReadStream::readLine()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next line because file is already closed.");
  if (file->fsOpen == NULL) RETURN_NIL;
  else {
    ObjString* line = fileReadLine(file);
    if (line == NULL) THROW_EXCEPTION(luminique::std::lang, OutOfMemoryException, "Not enough memory to allocate for next line read.");
    RETURN_OBJ(line);
  }
}

NATIVE_METHOD(FileReadStream, readLineAsync) {
  assertArgCount("FileReadStream::readLineAsync()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next line because file is already closed.");
  loadFileRead(file);

  ObjPromise* promise = fileReadStringAsync(file, UINT8_MAX, fileOnReadLine);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read line from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileReadStream, readString) {
  assertArgCount("FileReadStream::readString(length)", 1, argCount);
  assertArgIsInt("FileReadStream::readString(length)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  int length = AS_INT(args[0]);
  if (length < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method FileReadStream::readString(length) expects argument 1 to be a positive integer but got %g.", length);

  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read the next line because file is already closed.");
  if (file->fsOpen == NULL) RETURN_NIL;
  else {
    ObjString* string = fileReadString(file, length);
    if (string == NULL) THROW_EXCEPTION(luminique::std::lang, OutOfMemoryException, "Not enough memory to allocate for next line read.");
    RETURN_OBJ(string);
  }
}

NATIVE_METHOD(FileReadStream, readStringAsync) {
  assertArgCount("FileReadStream::readStringAsync(length)", 1, argCount);
  assertArgIsInt("FileReadStream::readStringAsync(length)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  int length = AS_INT(args[0]);
  if (length < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method FileReadStream::readStringAsync(length) expects argument 1 to be a positive integer but got %g.", length);
  loadFileRead(file);

  ObjPromise* promise = fileReadStringAsync(file, (size_t)length, fileOnReadString);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read line from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileReadStream, readToEnd) {
  assertArgCount("FileReadStream::readToEnd()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read to the end of file because file is already closed.");
  loadFileStat(file);
  
  if (file->fsOpen == NULL) RETURN_NIL;
  else {
    ObjString* string = fileReadString(file, (int)file->fsStat->statbuf.st_size);
    if (string == NULL) THROW_EXCEPTION(luminique::std::lang, OutOfMemoryException, "Not enough memory to allocate to read to end end of file.");
    RETURN_OBJ(string);
  }
}

NATIVE_METHOD(FileReadStream, readToEndAsync) {
  assertArgCount("FileReadStream::readToEndAsync()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot read to the end of file because file is already closed.");
  loadFileStat(file);
  loadFileRead(file);

  ObjPromise* promise = fileReadStringAsync(file, file->fsStat->statbuf.st_size, fileOnReadString);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to read to the end of file from IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileWriteStream, __init__) {
  assertArgCount("FileWriteStream::__init__(file)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjFile* file = getFileArgument(args[0]);
  if (file == NULL) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method FileWriteStream::__init__(file) expects argument 1 to be a string or file.");
  if (!setFileProperty(AS_INSTANCE(receiver), file, "w")) {
    THROW_EXCEPTION(luminique::std::io, IOException, "Cannot create FileWriteStream, file either does not exist or require additional permission to access.");
  }
  if (!loadFileWrite(file)) THROW_EXCEPTION(luminique::std::io, IOException, "Unable to write to file stream.");
  RETURN_OBJ(self);
}

NATIVE_METHOD(FileWriteStream, write) {
  assertArgCount("FileWriteStream::write(char)", 1, argCount);
  assertArgIsString("FileWriteStream::write(char)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write character to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) {
      ObjString* character = AS_STRING(args[0]);
      if (character->length != 1) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method FileWriteStream::put(char) expects argument 1 to be a character(string of length 1)");
      fileWrite(file, character->chars[0]);
  }
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, writeAsync) {
  assertArgCount("FileWriteStream::writeAsync(char)", 1, argCount);
  assertArgIsString("FileWriteStream::writeAsync(char)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write character to stream because file is already closed.");
  loadFileWrite(file);

  ObjString* character = AS_STRING(args[0]);
  if (character->length != 1) THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method FileWriteStream::putAsync(char) expects argument 1 to be a character(string of length 1)");
  ObjPromise* promise = fileWriteAsync(file, character, fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileWriteStream, writeLine) {
  assertArgCount("FileWriteStream::writeLine()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write new line to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) fileWrite(file, '\n');
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, writeLineAsync) {
  assertArgCount("FileWriteStream::writeLineAsync(char)", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write line to stream because file is already closed.");
  loadFileWrite(file);
  ObjPromise* promise = fileWriteAsync(file, copyString("\n", 1), fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileWriteStream, writeSpace) {
  assertArgCount("FileWriteStream::writeSpace()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write empty space to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) fileWrite(file, ' ');
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, writeSpaceAsync) {
  assertArgCount("FileWriteStream::writeSpaceAsync(char)", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write space to stream because file is already closed.");
  loadFileWrite(file);

  ObjPromise* promise = fileWriteAsync(file, copyString(" ", 1), fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(FileWriteStream, writeString) {
  assertArgCount("FileWriteStream::writeString(string)", 1, argCount);
  assertArgIsString("FileWriteStream::writeString(string)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write string to stream because file is already closed.");
  if (file->fsOpen != NULL && file->fsWrite != NULL) {
    ObjString* string = AS_STRING(args[0]);
    uv_buf_t uvBuf = uv_buf_init(string->chars, string->length);
    uv_fs_write(vm.eventLoop, file->fsWrite, (uv_file)file->fsOpen->result, &uvBuf, 1, file->offset, NULL);
    int numWrite = uv_fs_write(vm.eventLoop, file->fsWrite, (uv_file)file->fsOpen->result, &uvBuf, 1, file->offset, NULL);
    if (numWrite > 0) file->offset += string->length;
  }
  RETURN_NIL;
}

NATIVE_METHOD(FileWriteStream, writeStringAsync) {
  assertArgCount("FileWriteStream::writeStringAsync(char)", 1, argCount);
  assertArgIsString("FileWriteStream::writeStringAsync(string)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot write string to stream because file is already closed.");
  loadFileWrite(file);

  ObjString* string = AS_STRING(args[0]);
  ObjPromise* promise = fileWriteAsync(file, string, fileOnWrite);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to write to IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(IOStream, __init__) {
  assertError("Cannot instantiate from class IOStream.");
  RETURN_NIL;
}

NATIVE_METHOD(IOStream, close) {
  assertArgCount("IOStream::close()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  file->isOpen = false;
  RETURN_BOOL(fileClose(file));
}

NATIVE_METHOD(IOStream, closeAsync) {
  assertArgCount("IOStream::close()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  ObjPromise* promise = fileCloseAsync(file, fileOnClose);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to close IO stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(IOStream, file) {
  assertArgCount("IOStream::file()", 0, argCount);
  RETURN_OBJ(getFileProperty(AS_INSTANCE(receiver), "file"));
}

NATIVE_METHOD(IOStream, getPosition) {
  assertArgCount("IOStream::getPosition()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot get stream position because file is already closed.");
  if (file->fsOpen == NULL) RETURN_INT(0);
  RETURN_INT(file->offset);
}

NATIVE_METHOD(IOStream, reset) {
  assertArgCount("IOStream::reset()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot reset stream because file is already closed.");
  file->offset = 0;
  RETURN_NIL;
}

NATIVE_METHOD(ReadStream, __init__) {
  THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot instantiate from class ReadStream.");
}

NATIVE_METHOD(ReadStream, isAtEnd) {
  assertArgCount("ReadStream::isAtEnd()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) RETURN_FALSE;
  else {
    unsigned char c;
    uv_buf_t uvBuf = uv_buf_init(&c, 1);
    int numRead = uv_fs_read(vm.eventLoop, file->fsRead, (uv_file)file->fsOpen->result, &uvBuf, 1, file->offset, NULL);       
    RETURN_BOOL(numRead == 0);
  }
}

NATIVE_METHOD(ReadStream, read) {
  THROW_EXCEPTION(luminique::std::lang, NotImplementedException, "Not implemented, subclass responsibility.");
}

NATIVE_METHOD(ReadStream, skip) {
  assertArgCount("ReadStream::skip(offset)", 1, argCount);
  assertArgIsInt("ReadStream::skip(offset)", args, 0);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot skip stream by offset because file is already closed.");
  if (file->fsOpen == NULL) RETURN_FALSE;
  file->offset += AS_INT(args[0]);
  RETURN_TRUE;
}

NATIVE_METHOD(WriteStream, __init__) {
  THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot instantiate from class WriteStream.");
}

NATIVE_METHOD(WriteStream, flush) {
  assertArgCount("WriteStream::flush()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot flush write stream because file is already closed.");
  if (file->fsOpen == NULL) RETURN_FALSE;
  RETURN_BOOL(fileFlush(file));
}

NATIVE_METHOD(WriteStream, flushAsync) {
  assertArgCount("WriteStream::flushAsync()", 0, argCount);
  ObjFile* file = getFileProperty(AS_INSTANCE(receiver), "file");
  if (!file->isOpen) THROW_EXCEPTION(luminique::std::io, IOException, "Cannot flush write stream because file is already closed.");
  loadFileWrite(file);

  ObjPromise* promise = fileFlushAsync(file, fileOnFlush);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::io, IOException, "Failed to flush write stream.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(WriteStream, write) {
  THROW_EXCEPTION(luminique::std::lang, NotImplementedException, "Not implemented, subclass responsibility.");
}

void registerIOPackage() {
  ObjNamespace* ioNamespace = defineNativeNamespace("io", vm.stdNamespace);
  vm.currentNamespace = ioNamespace;

  vm.fileClass = defineNativeClass("File", false);
  bindSuperclass(vm.fileClass, vm.objectClass);
  vm.fileClass->classType = OBJ_FILE;
  DEF_METHOD(vm.fileClass, File, __init__, 1);
  DEF_METHOD(vm.fileClass, File, create, 0);
  DEF_METHOD_ASYNC(vm.fileClass, File, createAsync, 0);
  DEF_METHOD(vm.fileClass, File, delete, 0);
  DEF_METHOD_ASYNC(vm.fileClass, File, deleteAsync, 0);
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
  DEF_METHOD_ASYNC(vm.fileClass, File, mkdirAsync, 0);
  DEF_METHOD(vm.fileClass, File, name, 0);
  DEF_METHOD(vm.fileClass, File, rename, 1);
  DEF_METHOD_ASYNC(vm.fileClass, File, renameAsync, 1);
  DEF_METHOD(vm.fileClass, File, rmdir, 0);
  DEF_METHOD_ASYNC(vm.fileClass, File, rmdirAsync, 0);
  DEF_METHOD(vm.fileClass, File, setExecutable, 1);
  DEF_METHOD(vm.fileClass, File, setReadable, 1);
  DEF_METHOD(vm.fileClass, File, setWritable, 1);
  DEF_METHOD(vm.fileClass, File, size, 0);
  DEF_METHOD(vm.fileClass, File, __str__, 0);
  DEF_METHOD(vm.fileClass, File, __format__, 0);

  ObjClass* fileMetaclass = vm.fileClass->obj.klass;
  DEF_METHOD(fileMetaclass, FileClass, open, 2);
  DEF_METHOD_ASYNC(fileMetaclass, FileClass, openAsync, 2);

  ObjClass* ioStreamClass = defineNativeClass("IOStream", true);
  bindSuperclass(ioStreamClass, vm.objectClass);
  DEF_METHOD(ioStreamClass, IOStream, __init__, 1);
  DEF_METHOD(ioStreamClass, IOStream, close, 0);
  DEF_METHOD_ASYNC(ioStreamClass, IOStream, closeAsync, 0);
  DEF_METHOD(ioStreamClass, IOStream, file, 0);
  DEF_METHOD(ioStreamClass, IOStream, getPosition, 0);
  DEF_METHOD(ioStreamClass, IOStream, reset, 0);

  ObjClass* readStreamClass = defineNativeClass("ReadStream", true);
  bindSuperclass(readStreamClass, ioStreamClass);
  DEF_METHOD(readStreamClass, ReadStream, __init__, 1);
  DEF_METHOD(readStreamClass, ReadStream, isAtEnd, 0);
  DEF_METHOD(readStreamClass, ReadStream, read, 0);
  DEF_METHOD(readStreamClass, ReadStream, skip, 1);

  ObjClass* writeStreamClass = defineNativeClass("WriteStream", true);
  bindSuperclass(writeStreamClass, ioStreamClass);
  DEF_METHOD(writeStreamClass, WriteStream, __init__, 1);
  DEF_METHOD(writeStreamClass, WriteStream, flush, 0);
  DEF_METHOD_ASYNC(writeStreamClass, WriteStream, flushAsync, 0);
  DEF_METHOD(writeStreamClass, WriteStream, write, 1);

  ObjClass* binaryReadStreamClass = defineNativeClass("BinaryReadStream", false);
  bindSuperclass(binaryReadStreamClass, readStreamClass);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, __init__, 1);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, read, 0);
  DEF_METHOD_ASYNC(binaryReadStreamClass, BinaryReadStream, readAsync, 0);
  DEF_METHOD(binaryReadStreamClass, BinaryReadStream, readBytes, 1);
  DEF_METHOD_ASYNC(binaryReadStreamClass, BinaryReadStream, readBytesAsync, 1);

  ObjClass* binaryWriteStreamClass = defineNativeClass("BinaryWriteStream", false);
  bindSuperclass(binaryWriteStreamClass, writeStreamClass);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, __init__, 1);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, write, 1);
  DEF_METHOD_ASYNC(binaryWriteStreamClass, BinaryWriteStream, writeAsync, 1);
  DEF_METHOD(binaryWriteStreamClass, BinaryWriteStream, writeBytes, 1);
  DEF_METHOD_ASYNC(binaryWriteStreamClass, BinaryWriteStream, writeBytesAsync, 1);

  ObjClass* fileReadStreamClass = defineNativeClass("FileReadStream", false);
  bindSuperclass(fileReadStreamClass, readStreamClass);
  DEF_METHOD(fileReadStreamClass, FileReadStream, __init__, 1);
  DEF_METHOD(fileReadStreamClass, FileReadStream, peek, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, read, 0);
  DEF_METHOD_ASYNC(fileReadStreamClass, FileReadStream, readAsync, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, readLine, 0);
  DEF_METHOD_ASYNC(fileReadStreamClass, FileReadStream, readLineAsync, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, readString, 0);
  DEF_METHOD_ASYNC(fileReadStreamClass, FileReadStream, readStringAsync, 0);
  DEF_METHOD(fileReadStreamClass, FileReadStream, readToEnd, 0);
  DEF_METHOD_ASYNC(fileReadStreamClass, FileReadStream, readToEndAsync, 0);

  ObjClass* fileWriteStreamClass = defineNativeClass("FileWriteStream", false);
  bindSuperclass(fileWriteStreamClass, writeStreamClass);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, __init__, 1);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, write, 1);
  DEF_METHOD_ASYNC(fileWriteStreamClass, FileWriteStream, writeAsync, 1);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, writeLine, 0);
  DEF_METHOD_ASYNC(fileWriteStreamClass, FileWriteStream, writeLineAsync, 0);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, writeSpace, 0);
  DEF_METHOD_ASYNC(fileWriteStreamClass, FileWriteStream, writeSpaceAsync, 0);
  DEF_METHOD(fileWriteStreamClass, FileWriteStream, writeString, 1);
  DEF_METHOD_ASYNC(fileWriteStreamClass, FileWriteStream, writeStringAsync, 1);

  ObjClass* ioExceptionClass = defineNativeException("IOException", vm.exceptionClass);
  defineNativeException("EOFException", ioExceptionClass);
  defineNativeException("FileNotFoundException", ioExceptionClass);

  vm.currentNamespace = vm.rootNamespace;
}
