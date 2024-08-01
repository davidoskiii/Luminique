#ifndef cluminique_file_h
#define cluminique_file_h

#include "../common.h"
#include "../loop/loop.h"
#include "../value/value.h"

typedef struct {
  VM* vm;
  ObjFile* file;
  ObjPromise* promise;
  uv_buf_t buffer;
} FileData;

bool fileClose(ObjFile* file);
ObjPromise* fileCloseAsync(ObjFile* file, uv_fs_cb callback);
bool fileCreate(ObjFile* file);
ObjPromise* fileCreateAsync(ObjFile* file, uv_fs_cb callback);
bool fileDelete(ObjFile* file);
ObjPromise* fileDeleteAsync(ObjFile* file, uv_fs_cb callback);
bool fileExists(ObjFile* file);
bool fileFlush(ObjFile* file);
ObjPromise* fileFlushAsync(ObjFile* file, uv_fs_cb callback);
ObjString* fileGetAbsolutePath(ObjFile* file);
bool fileMkdir(ObjFile* file);
ObjPromise* FileMkdirAsync(ObjFile* file, uv_fs_cb callback);
int fileMode(const char* mode);
void fileOnClose(uv_fs_t* fsClose);
void fileOnCreate(uv_fs_t* fsOpen);
void fileOnFlush(uv_fs_t* fsSync);
void fileOnHandle(uv_fs_t* fsHandle);
void fileOnOpen(uv_fs_t* fsOpen);
void fileOnRead(uv_fs_t* fsRead);
void fileOnReadByte(uv_fs_t* fsRead);
void fileOnReadBytes(uv_fs_t* fsRead);
void fileOnReadLine(uv_fs_t* fsRead);
void fileOnReadString(uv_fs_t* fsRead);
void fileOnWrite(uv_fs_t* fsWrite);
bool fileOpen(ObjFile* file, const char* mode);
ObjPromise* fileOpenAsync(ObjFile* file, const char* mode, uv_fs_cb callback);
ObjString* fileRead(ObjFile* file, bool isPeek);
ObjPromise* fileReadAsync(ObjFile* file, uv_fs_cb callback);
uint8_t fileReadByte(ObjFile* file);
ObjArray* fileReadBytes(ObjFile* file, int length);
ObjString* fileReadLine(ObjFile* file);
ObjString* fileReadString(ObjFile* file, int length);
ObjPromise* fileReadStringAsync(ObjFile* file, size_t length, uv_fs_cb callback);
bool fileRename(ObjFile* file, ObjString* name);
ObjPromise* fileRenameAsync(ObjFile* file, ObjString* name, uv_fs_cb callback);
bool fileRmdir(ObjFile* file);
ObjPromise* fileRmdirAsync(ObjFile* file, uv_fs_cb callback);
void fileWrite(ObjFile* file, char c);
ObjPromise* fileWriteAsync(ObjFile* file, ObjString* string, uv_fs_cb callback);
void fileWriteByte(ObjFile* file, uint8_t byte);
ObjPromise* fileWriteByteAsync(ObjFile* file, uint8_t byte, uv_fs_cb callback);
void fileWriteBytes(ObjFile* file, ObjArray* bytes);
ObjPromise* fileWriteBytesAsync(ObjFile* file, ObjArray* bytes, uv_fs_cb callback);
ObjFile* getFileArgument(Value arg);
ObjFile* getFileProperty(ObjInstance* object, char* field);
bool loadFileOperation(ObjFile* file, const char* streamClass);
bool loadFileRead(ObjFile* file);
bool loadFileStat(ObjFile* file);
bool loadFileWrite(ObjFile* file);
bool setFileProperty(ObjInstance* object, ObjFile* file, const char* mode);
char* streamClassName(const char* mode);

#endif
