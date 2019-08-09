#include "utilities.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <arpa/inet.h>
#include <extras/rtsp.h>

AVFormatContext *g_avFormatCtx = NULL;
//AVFormatContext *g_avFormatCtxRTP = NULL;
AVCodecContext  *g_avCodecCtxRef = NULL; //refer to received codec
AVCodecContext  *g_avCodecCtx = NULL;
AVCodec         *g_avCodec = NULL;
//AVFrame         *g_avFrame = NULL;
//AVFrame         *g_avFrameYUV = NULL;
//AVStream        *g_avStream = NULL;
//AVPacket        *g_avPacket = NULL;
AVIOContext     *g_avIoCtx = NULL;
//AVInputFormat   *g_avInputFmt = NULL;
//AVOutputFormat  *g_avOutputFmt = NULL;
//RTPDemuxContext *m_rtpDemuxCtx;
struct SwsContext *g_swsPictureCtx = NULL;
//uint8_t *m_rawBuffer = NULL;
int g_errCode = 0;
int g_vvIdx = -1;
int g_acIdx = -1;
int g_increment = 0;

//FILE *ff_tmp_file = NULL;

char *fmm_dump_packet(AVPacket* packet){

}

FILE* fmm_file(const char* prefix, const char* opt){
  static FILE* v_fout = NULL;
  if(strcmp(opt, "open") == 0 && v_fout == NULL){
    if((v_fout = fopen(get_next_filename("log", prefix, "yuv"), "wb+")) == NULL){
      fprintf(stderr, "failed to open '%s' %s \n", prefix, strerror(errno));
      return NULL;
    }
  }
  else if(strcmp(opt, "reopen") == 0){
    v_fout = fmm_file(prefix, "close");
    v_fout = fmm_file(prefix, "open");
#if 0
    if(v_fout){
      fclose(v_fout);
    }
    if((v_fout = fopen(get_next_filename(".", prefix, "yuv"), "wb+")) == NULL){
#endif
    if(v_fout == NULL){
      fprintf(stderr, "failed to reopen '%s' %s \n", prefix, strerror(errno));
      return NULL;
    }
  }
  else if(strcmp(opt, "close") == 0){
    if(v_fout)fclose(v_fout);
    v_fout = NULL;
  }
  return v_fout;
}

int fmm_get_protype(const char* input, char* output, int* outlen){
  char *ptr = NULL;

  ptr = strstr(input, ".sdp");
  if(ptr && get_file_data(input, output, outlen)>=0){
    return 0;
  }

  ptr = strstr(input, "udp://");
  if(ptr){
    strcpy(output, input);
    (*outlen) = strlen(output);
    return 1;
  }

  return -1;
}

int fmm_initialize(const char* input){
  //ff_tmp_file=fopen("t.log", "wb+");
  av_register_all();
  avformat_network_init();
  g_avFormatCtx = avformat_alloc_context();

  char *ptr = "nothing";
  unsigned long  v_size = M_COMMON_SIZE; 
  unsigned char  v_data[M_COMMON_SIZE]; // = (unsigned char *)av_mallocz(M_MAX_BUFFER_SIZE);

  switch(fmm_get_protype(input, v_data, &v_size)){
    case 0://sdp
      if((g_avIoCtx = avio_alloc_context(v_data, v_size, 0, (void*)NULL, NULL, NULL, NULL))==NULL){
        fprintf(stderr, "failed to alloc avio\n");
        return -1;
      }
      g_avFormatCtx->pb = g_avIoCtx;
      g_avFormatCtx->iformat = av_find_input_format("sdp");
      break;
    case 1:
      break;
    default:
      fprintf(stderr, "unsupported input\n");
      return -1;
  }

  //ptr = const_cast<char*>(input);
  ptr = input;
  if((g_errCode = avformat_open_input(&g_avFormatCtx, ptr, NULL, NULL))!=0){
    fprintf(stderr, "failed by avformat_open_input '%s': %s\n", ptr, av_err2str(g_errCode));
    return -1;
  }
  return 0;
}

int fmm_receive(const char* output){
  int i = 0;
  int ret = 0;
  FILE *v_foutput = NULL;
 
  if((g_errCode=avformat_find_stream_info(g_avFormatCtx, NULL)) <0) {
    fprintf(stderr, "failed by avformat_find_stream_info %s\n", av_err2str(g_errCode));
    return -1;
  }

  //av_dump_format(g_avFormatCtx, 0, "", 0);
  for(i=0; i<g_avFormatCtx->nb_streams; i++){
    if(g_avFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      g_vvIdx = i;
      //break;
    }
    if(g_avFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
      g_acIdx = i;
      //break;
    }
  }

  g_acIdx = 0;
  if(g_vvIdx == -1 || g_acIdx == -1) {
    fprintf(stderr, "failed to find video (%d) / audio (%d) stream \n", g_vvIdx, g_acIdx);
    return -1;
  }

  g_avCodecCtxRef = g_avFormatCtx->streams[g_vvIdx]->codec;

  if((g_avCodec=avcodec_find_decoder(g_avCodecCtxRef->codec_id))==NULL){
    fprintf(stderr, "failed by avcodec_find_decoder\n");
    return -1;
  }

  g_avCodecCtx= avcodec_alloc_context3(NULL);//g_avCodec);
  avcodec_get_context_defaults3(g_avCodecCtx, g_avCodec);
  if((g_errCode=avcodec_copy_context(g_avCodecCtx, g_avCodecCtxRef))!= 0){
    fprintf(stderr, "failed to copy codec context %s", av_err2str(g_errCode));
    return -1;
  }

  if((g_errCode=avcodec_open2(g_avCodecCtx, g_avCodec,NULL)) <0 ){
    fprintf(stderr, "failed by avcodec_open2 %s\n", av_err2str(g_errCode));
    return -1;
  }

  g_swsPictureCtx = sws_getContext( g_avCodecCtx->width,
                                    g_avCodecCtx->height, 
                                    g_avCodecCtx->pix_fmt,
                                    g_avCodecCtx->width, 
                                    g_avCodecCtx->height,
                                    AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                                    NULL, NULL, NULL);

  AVFrame *v_avframe_x = av_frame_alloc();

  unsigned char* v_raw_buffer=(unsigned char *)av_malloc(
                                av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 
                                                          g_avCodecCtx->width,
                                                          g_avCodecCtx->height, 1));
  if(v_raw_buffer == NULL){
    av_frame_free(&v_avframe_x);
    fprintf(stderr, "failed to alloc raw buffer \n", strerror(errno));
    return -1;
  }

  g_errCode = av_image_fill_arrays(v_avframe_x->data, v_avframe_x->linesize, v_raw_buffer,
                          AV_PIX_FMT_YUV420P, g_avCodecCtx->width, g_avCodecCtx->height, 1);

  if(g_errCode < 0){
    av_free(v_raw_buffer);
    av_frame_free(&v_avframe_x);
    fprintf(stderr, "failed by av_image_fill_arrays\n", av_err2str(g_errCode));
    return -1;
  }
  
  AVFrame *v_avframe = av_frame_alloc();
  AVPacket *v_avpacket = (AVPacket *)av_malloc(sizeof(AVPacket));;
  v_avpacket->data = NULL;
  v_avpacket->size = 0;
  av_init_packet(v_avpacket);

  FILE *v_fout = NULL;
  for(i=0, ret=0, v_fout=fmm_file(output, "open"); v_fout!=NULL && ret==0; i++){
    if((g_errCode = av_read_frame(g_avFormatCtx, v_avpacket)) < 0){
      fprintf(stderr, "failed by av_read_frame %s\n", av_err2str(g_errCode));
      ret = -1;
      break;
    }
    fmm_print_packet(g_avFormatCtx, v_avpacket);
/* 
    if(v_avpacket->stream_index!=g_vvIdx){
      fprintf(stderr, "stream[%d] is not a video packet, continue \n", g_avPacket->stream_index);
      continue
    }
*/
    ret = fmm_decode(v_avpacket, v_avframe, v_avframe_x, output);
    av_packet_unref(v_avpacket); //deprecated: av_free_packet(&v_av_packet);
  }

  for(i=0, ret=0, v_fout=fmm_file(output, "open"); v_fout!=NULL && ret==0; i++){
    ret = fmm_decode(v_avpacket, v_avframe, v_avframe_x, output);
    av_packet_unref(v_avpacket); //deprecated: av_free_packet(&v_av_packet);
  }
  v_fout=fmm_file(output, "close");

  av_free(v_raw_buffer);
  av_free(v_avpacket);
  av_frame_free(&v_avframe);
  av_frame_free(&v_avframe_x);
  return ret;
}

int fmm_decode(AVPacket* packet, AVFrame* frame, AVFrame* framex, const char* output){
  int v_sizex = 0;
  int v_has_pic = 0;
/*
  if((g_errCode = av_read_frame(g_avFormatCtx, packet)) < 0){
    fprintf(stderr, "failed by av_read_frame %s\n", av_err2str(g_errCode));
    return -1;
  }
*/
 
  if(packet->stream_index!=g_vvIdx){
    fprintf(stderr, "stream[%d] is not a video packet, continue \n", packet->stream_index);
    return 0;
  }
  //fmm_print_packet(g_avFormatCtx, packet);

  if((g_errCode=avcodec_decode_video2(g_avCodecCtx, frame, &v_has_pic, packet))<0){
    fprintf(stderr, "failed by avcodec_decode_video2 %s\n", av_err2str(g_errCode));
    return -1;
  }

  if(v_has_pic){
    g_increment++;
//    fprintf(stderr, "##Write Frame[%06d] ...\n", g_increment);
    sws_scale(g_swsPictureCtx, 
                    (const unsigned char* const*)frame->data, frame->linesize, 
                                                  0, g_avCodecCtx->height,
                                                  framex->data, framex->linesize);

    v_sizex = g_avCodecCtx->width * g_avCodecCtx->height;
    fwrite(framex->data[0], 1, v_sizex,   fmm_file(output, ""));  //Y 
    fwrite(framex->data[1], 1, v_sizex/4, fmm_file(output, ""));  //U
    fwrite(framex->data[2], 1, v_sizex/4, fmm_file(output, ""));  //V

    if(g_increment % 128 == 0){
      if(fmm_file(output, "reopen") == NULL){
        return -1;
      }
    }
  }
  return 0;
}

int fmm_finalize(){
  if(g_swsPictureCtx){
    sws_freeContext(g_swsPictureCtx);
  }
/*
  if(g_avPacket){
    av_free(g_avPacket);
  }
  if(g_avFrame){
    av_frame_free(&g_avFrame);
  }
  if(g_avFrameYUV){
    av_frame_free(&g_avFrameYUV);
  }
*/
  if(g_avCodecCtxRef){
    avcodec_close(g_avCodecCtxRef);
  }
  if(g_avCodecCtx){
    avcodec_close(g_avCodecCtx);
    avcodec_free_context(&g_avCodecCtx);
  }
  if(g_avFormatCtx){
    avformat_close_input(&g_avFormatCtx);
    //avformat_free_context(g_avFormatCtx);
    //av_read_pause(g_avFormatCtx);
  }
  return 0;
}

char* fmm_dump_avts(const int64_t tmstamp, AVRational *tmbase){
  static char buffer[AV_ERROR_MAX_STRING_SIZE + 1];
  memset(buffer, 0x00, sizeof(buffer));
  if(tmbase)
    av_ts_make_time_string(buffer, tmstamp, tmbase);
  else
    av_ts_make_string(buffer, tmstamp);

  buffer[AV_ERROR_MAX_STRING_SIZE] = '\0';
  return buffer;
}

void fmm_print_packet(const AVFormatContext *fmtctx, const AVPacket *packet) {
  char buffer[128]="";
  RTSPState* v_rtsp_state = (RTSPState*)g_avFormatCtx->priv_data;
  RTSPStream* v_rtsp_stream = v_rtsp_state->rtsp_streams[0];
  RTPDemuxContext* v_rtp_dmxctx = (RTPDemuxContext*)v_rtsp_stream->transport_priv;
#if 0
  uint64_t ntp_time = v_rtp_dmx_ctx->last_rtcp_reception_time;
  uint32_t seconds = (uint32_t) ((ntp_time >> 32) & 0xffffffff);
  uint32_t fraction  = (uint32_t) (ntp_time & 0xffffffff);
  double useconds = ((double) fraction / 0xffffffff);
  //printf("-----> %s  , %s\n", fmm_dump_avts(ntp_time, NULL), to_datetime(ntp_time));
#endif

  get_sha3_checksum[1]((const char*)packet, buffer); 
  fprintf(stderr, "Frame[%d]: SEQ(%u) TS(%s) STS(%lld) FTS(%lu) PTS(%s) DTS(%s) DUR(%s) \n  SHA3=(%s) %s", 
                        g_increment,
                        v_rtp_dmxctx->seq, 
                        //fmtctx->packet_size,
                        //fmtctx->start_time_realtime + packet->dts,
                        to_datetime(fmtctx->start_time_realtime + packet->dts),
                        fmtctx->start_time_realtime,
                        //to_datetime(fmtctx->start_time_realtime/1000),
                        v_rtp_dmxctx->timestamp, 
                       // to_datetime(v_rtp_dmxctx->timestamp),
                        fmm_dump_avts(packet->pts, NULL),
                        fmm_dump_avts(packet->dts, NULL),
                        fmm_dump_avts(packet->duration, NULL),
                        buffer,
                        "\n"); 


/*
  AVRational *v_timebase = &fmtctx->streams[packet->stream_index]->time_base;
  fprintf(stderr, "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d \n",
        fmm_dump_avts(packet->pts, NULL),
        fmm_dump_avts(packet->pts, v_timebase),
        fmm_dump_avts(packet->dts, NULL),
        fmm_dump_avts(packet->dts, v_timebase),
        fmm_dump_avts(packet->duration, NULL),
        fmm_dump_avts(packet->duration, v_timebase),
        packet->stream_index);
*/
}

/*
void MediaSink::Test_RTP(){
ff_rtp_parse_packet(rtp_demuxer, &packet, &rtp_packet->data, rtp_packet->size))G
}
*/


