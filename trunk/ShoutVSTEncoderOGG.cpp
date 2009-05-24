#include "ShoutVST.h"
#include "ShoutVSTEncoderOGG.h"

bool ShoutVSTEncoderOGG::Initialize(ShoutVST * p)
{
  bInitialized = false;
  pVST = p;

  int ret = 0; 

  pVST->Log("[ogg] vorbis_info_init()\r\n");
  vorbis_info_init(&vi);
  long sample = (long)pVST->updateSampleRate();
  pVST->Log("[ogg] Sampling rate is %d Hz\r\n",sample);

  ret = vorbis_encode_init_vbr(&vi,2,sample,pVST->GetQuality() / 10.0f); 
  if (ret) {
    pVST->Log("[ogg] Error vorbis_encode_init_vbr\r\n");
    return false;
  }

  pVST->Log("[ogg] vorbis_comment_init()\r\n");
  vorbis_comment_init(&vc);
  vorbis_comment_add_tag(&vc,"ENCODER","ShoutVST"); 

  pVST->Log("[ogg] vorbis_analysis_init()\r\n");
  vorbis_analysis_init(&vd,&vi);
  pVST->Log("[ogg] vorbis_block_init()\r\n");
  vorbis_block_init(&vd,&vb); 

  pVST->Log("[ogg] ogg_stream_init()\r\n");
  ogg_stream_init(&os,303); // totally random number

  bInitialized = true;

  {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
    ogg_stream_packetin(&os,&header); /* automatically placed in its own
                                      page */
    ogg_stream_packetin(&os,&header_comm);
    ogg_stream_packetin(&os,&header_code);

    while(true){
      int result=ogg_stream_flush(&os,&og);
      if(result==0)break;

      SendOGGPageToICE(&og);
    }

  } 

  pVST->Log("[ogg] ogg peachy!\r\n");


  return true;
}

bool ShoutVSTEncoderOGG::Close()
{
  bInitialized = false;

  pVST->Log("[ogg] StopOGGEncoding()\r\n");

  pVST->Log("[ogg] vorbis_analysis_wrote()\r\n");
  vorbis_analysis_wrote(&vd,0);

  pVST->Log("[ogg] ogg_stream_clear()\r\n");
  ogg_stream_clear(&os);
  pVST->Log("[ogg] vorbis_block_clear()\r\n");
  vorbis_block_clear(&vb);
  pVST->Log("[ogg] vorbis_dsp_clear()\r\n");
  vorbis_dsp_clear(&vd);
  pVST->Log("[ogg] vorbis_comment_clear()\r\n");
  vorbis_comment_clear(&vc);
  pVST->Log("[ogg] vorbis_info_clear()\r\n");
  vorbis_info_clear(&vi);

  pVST->Log("[ogg] done()\r\n");

  return true;
}

bool ShoutVSTEncoderOGG::Process( float **inputs, long sampleFrames )
{
  if (!bInitialized) return false;

  float **buffer=vorbis_analysis_buffer(&vd,sampleFrames);

  /* uninterleave samples */
  for(int i=0;i<sampleFrames;i++){
    buffer[0][i] = inputs[0][i];
    buffer[1][i] = inputs[1][i];
  }

  /* tell the library how much we actually submitted */
  vorbis_analysis_wrote(&vd,sampleFrames);

  /* vorbis does some data preanalysis, then divvies up blocks for
  more involved (potentially parallel) processing.  Get a single
  block for encoding now */
  int eos = 0;

  while(vorbis_analysis_blockout(&vd,&vb)==1){

    /* analysis, assume we want to use bitrate management */
    vorbis_analysis(&vb,NULL);
    vorbis_bitrate_addblock(&vb);

    while(vorbis_bitrate_flushpacket(&vd,&op)){

      /* weld the packet into the bitstream */
      ogg_stream_packetin(&os,&op);

      /* write out pages (if any) */
      while(!eos){
        int result=ogg_stream_pageout(&os,&og);
        if(result==0)break;
        if (!SendOGGPageToICE(&og)) return false;

        /* this could be set above, but for illustrative purposes, I do
        it here (to show that vorbis does know where the stream ends) */

        if(ogg_page_eos(&og))eos=1;
      }
    }
  } 

  return true;
}

bool ShoutVSTEncoderOGG::SendOGGPageToICE( ogg_page * og )
{
  if (!bInitialized) return false;
  //pVST->Log("[ogg] Send header: %d body: %d\r\n",og->header_len,og->body_len);
  if (!pVST->SendDataToICE(og->header,og->header_len)) return false;
  if (!pVST->SendDataToICE(og->body,og->body_len)) return false;
  return true;
}

