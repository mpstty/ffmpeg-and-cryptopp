#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libavutil/avutil.h>  
#include <libswscale/swscale.h>  
#ifdef __cplusplus
};
#endif

AVFormatContext   *pFormatCtx = NULL;
AVOutputFormat    *pOutFmt = NULL;
AVStream          *pStream = NULL;
AVCodecContext    *pCodecCtx = NULL;
AVCodec           *pCodec = NULL;
AVFrame           *pFrame = NULL;
uint8_t           *rawBuffer = NULL;
FILE              *pInFile = NULL;
FILE              *pOutFile = NULL;
int               gVideoIndex = 0;
int               gMaxFrames = 128;
int               gIncrement = 0;
char              gOutputPath[64] = "";

int fmm_flush_encoder(){
  AVPacket cPacket;
  int vPicture = 0;
  if((pFormatCtx->streams[gVideoIndex]->codec->codec->capabilities & AV_CODEC_CAP_DELAY) == 0){
    return 0;
  }

  while (1) {
    cPacket.data = NULL;
    cPacket.size = 0;
    av_init_packet(&cPacket);
    if(avcodec_encode_video2(pFormatCtx->streams[gVideoIndex]->codec, &cPacket, NULL, &vPicture)<0){
      av_frame_free(NULL);
      return -1; 
    }
    if(vPicture == 0) {
      av_frame_free(NULL);
      return 0;
    }
    gIncrement++;
    printf("encode frame[%d] (remained)\n", gIncrement);

    cPacket.stream_index = gVideoIndex;
    av_packet_rescale_ts(&cPacket,
                          pFormatCtx->streams[gVideoIndex]->codec->time_base,
                          pFormatCtx->streams[gVideoIndex]->time_base);
    if(av_interleaved_write_frame(pFormatCtx, &cPacket)<0){
      av_frame_free(NULL);
      return -1;
    }
  }
  return 0;
}

int fmm_run_encoder(const char* input){
  int vPicture = 0;
  pInFile = fopen(input, "rb");
  if (pInFile == NULL) {
    printf("failed to open file %s\n", input);
    return -1;
  }

  sprintf(gOutputPath, "%s.mp4", input);
  avcodec_register_all();
  av_register_all();
  avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, gOutputPath);
  pOutFmt = pFormatCtx->oformat;

  if(avio_open(&pFormatCtx->pb, gOutputPath, AVIO_FLAG_READ_WRITE)) {
    printf("failed by avio_open\n");
    return -1;
  }

  if((pStream = avformat_new_stream(pFormatCtx, 0))==NULL){
    printf("failed by avformat_new_stream \n");
    return -1;
  }

  pStream->time_base.num = 1;
  pStream->time_base.den = 10; //30

  pCodecCtx = pStream->codec;
  pCodecCtx->codec_id = pOutFmt->video_codec;
  pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
  pCodecCtx->width = 640;
  pCodecCtx->height = 360;
  pCodecCtx->time_base.num = 1;
  pCodecCtx->time_base.den = 10; //fps
    //pCodecCtx->bit_rate = 6126000;
  pCodecCtx->bit_rate = 512000; //6126000
  pCodecCtx->gop_size = 12;

  if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
    pCodecCtx->refs = 1;
    pCodecCtx->qmin = 0;//10;
    pCodecCtx->qmax = 69;//51;
    pCodecCtx->qcompress = 0.6;//0.6;
  }

  if((pCodec = avcodec_find_encoder(pCodecCtx->codec_id))==NULL){
    printf("failed by avcodec_find_encoder\n");
    return -1;
  }

  if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    printf("open encoder fail!\n");
    return -1;
  }

//  av_dump_format(pFormatCtx, 0, gOutputPath, 1);

  pFrame = av_frame_alloc();
  pFrame->width = pCodecCtx->width;
  pFrame->height = pCodecCtx->height;
  pFrame->format = pCodecCtx->pix_fmt;
  int size_raw = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
  rawBuffer = (uint8_t*)av_malloc(size_raw);

  avpicture_fill((AVPicture*)pFrame, rawBuffer, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
  avformat_write_header(pFormatCtx, NULL);

  AVPacket cPacket; 
  int size_new = pCodecCtx->width*pCodecCtx->height;
  av_new_packet(&cPacket, size_raw * 3);

  int i = 0;
  for(i = 0; i < gMaxFrames; i++) {
    if(fread(rawBuffer, 1, size_new * 3 / 2, pInFile) < 0) {
      printf("failed by fread \n");
      return -1;
    }
    if(feof(pInFile)){
      return 0;
    }

    pFrame->data[0] = rawBuffer; 
    pFrame->data[1] = rawBuffer + size_new; 
    pFrame->data[2] = rawBuffer + size_new * 5 / 4; 
    pFrame->pts = i;

    if(avcodec_encode_video2(pCodecCtx, &cPacket, pFrame, &vPicture)<0){
      printf("failed by avcodec_encode_video2 \n");
      return -1;
    }

    if(vPicture == 1) {
      gIncrement++;
      printf("encode frame[%d] \n", gIncrement);

      cPacket.stream_index = pStream->index;
      av_packet_rescale_ts(&cPacket, pCodecCtx->time_base, pStream->time_base);
      cPacket.pos = -1;
      av_interleaved_write_frame(pFormatCtx, &cPacket);
      av_free_packet(&cPacket);
    }
  }
  if(fmm_flush_encoder()){
    printf("failed by fmm_flush_encoder\n");
    return -1;
  }

  av_write_trailer(pFormatCtx);

  return 0;
}

int fmm_stop_encoder(){
  if(pStream) {
    avcodec_close(pStream->codec);
    av_free(pFrame);
    av_free(rawBuffer);
  }
  if(pFormatCtx) {
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
  }

  fclose(pInFile);

  return 0;
}

int fmm_yuv_to_mp4(const char* input){
  fmm_run_encoder(input);
  fmm_stop_encoder();
  printf("%s OUTPUT: %s \n", __FUNCTION__, gOutputPath);
  return 0;
}

