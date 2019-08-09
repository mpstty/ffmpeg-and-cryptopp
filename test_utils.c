#include "utilities.h"

int main(int argc, char** argv){
  char buffer[128]="";
  int ret = 0;

  if(argc<3){
    printf("\n invalid argument number \n");
    printf("\n%s convt | sha3 | sdp FILENAME \n\n", argv[0]);
    return 0;
  }

  if(strcmp(argv[1], "convt")==0){
    fmm_yuv_to_mp4(argv[2]);
    return 0;
  }

  if(strcmp(argv[1], "sha3")==0){
    ret = get_sha3_checksum[0]((const char*)argv[2], buffer);
    printf("CHECKSUM: %s \n", buffer);
    ret = get_sha3_checksum[1]((const char*)argv[2], buffer);
    printf("CHECKSUM: %s \n", buffer);
    return 0;
  }

  if(strcmp(argv[1], "sdp")==0){
    fmm_initialize(argv[2]);
    fmm_receive("output");
    fmm_finalize();
    return 0;
  }

  printf("\n%s convt | sha3 | sdp FILENAME \n\n", argv[0]);
  return 0;
}
