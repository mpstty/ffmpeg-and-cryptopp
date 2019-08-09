
#ifndef __ALL_UTILITIES_H__
#define __ALL_UTILITIES_H__

#ifdef __cplusplus
extern "C"{
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <arpa/inet.h>
*/

#define M_COMMON_SIZE 32768 //4k

typedef int (*hdl_get_sha3_checksum)(const char*, char*);

extern hdl_get_sha3_checksum get_sha3_checksum[]; //0:file, 1:data

static inline char* get_next_filename(const char *path, const char *prefix, const char *suffix) {
  struct dirent *entry;
  DIR *dir = opendir(path);
  int idx = 0, last=0;
  static char filename[128]="";
  char *p = NULL;
  if (dir == NULL || prefix == NULL || suffix == NULL) {
    return NULL;
  }

  while ((entry = readdir(dir)) != NULL) {
    if(strncmp(entry->d_name, prefix, strlen(prefix))==0){
      p = entry->d_name + strlen(prefix);
      idx = atoi(p);
      if (idx >last) last = idx;
    }
  }

  closedir(dir);
  last++;
  sprintf(filename, "%s/%s%d.%s", path, prefix, last, suffix);
  printf("%s, %d, %s \n", __FUNCTION__, __LINE__, filename);
  return filename;
}

static inline int get_file_data(const char* filename, uint8_t *content, unsigned long* length){
  unsigned char *ptr = NULL;
  struct stat buffer;
  int ret = 0;
  int fd = 0, status = 0;
  if((fd=open(filename, O_RDONLY))<0){
    fprintf(stderr, "failed to open file '%s' (%s)\n", filename, strerror(errno));
    return -1;
  }

  if((status = fstat(fd, &buffer))<0){
    fprintf(stderr, "failed to stat file '%s' (%s)\n", filename, strerror(errno));
    close(fd);
    return -1;
  }

  if (buffer.st_size < 0 || buffer.st_size > (*length) ){
    fprintf(stderr, "insufficient size '%d' <> '%d' \n", buffer.st_size, (*length));
    close(fd);
    return -1;
  }

  //ptr = (unsigned char*)av_mallocz(buffer.st_size+1);
  ptr = (unsigned char*)malloc(buffer.st_size+1);
  memset(ptr, 0x00, buffer.st_size+1);
  if (read(fd, ptr, buffer.st_size) !=  buffer.st_size){
    fprintf(stderr, "failed to read file '%s' (%s)\n", filename, strerror(errno));
    //av_free(ptr);
    free(ptr);
    close(fd);
    return -1;
  }

  ptr[buffer.st_size] = '\0';
  memcpy(content, (void*)ptr, buffer.st_size);
  //av_free(ptr);
  free(ptr);
  close(fd);
  (*length) = strlen((const char*)content);
  printf("## File '%s' Length=%d \nContent:\n%s\n", filename, (*length),  content);
  return (*length);
}

static inline char* to_datetime(time_t ts) {
  static char output[32]="";
  memset(output, 0x00,sizeof(output));
  sprintf(output, "%lu", ts);

  time_t now = ts; 
  time_t rem = 0;

  if(strlen(output)>10){
    now = ts/1000000;
    rem = ts - now*1000000;
  }

  const char *format = "%Y-%m-%d %H:%M:%S";
  memset(output, 0x00, sizeof(output));
  if(strftime(output, sizeof(output), format, localtime(&now)) == 0) {
    fprintf(stderr,  "failed by strftime %s\n", strerror(errno));// sizeof(res), format);
    return NULL;
  }

  sprintf(output, "%s.%lu", output, rem);
  return output;
}

static inline time_t to_timestamp(const char* input){
  const char *format = "%Y-%m-%d %H:%M:%S";
  struct tm ctm;
  strptime("2019-07-08","%Y-%m-%d 10:20:30", &ctm);
  time_t now = mktime(&ctm);
  return now;
}


#ifdef __cplusplus
}
#endif

#endif //__ALL_UTILITIES_H__

