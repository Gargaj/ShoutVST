#pragma once
#include "shoutvstencoder.h"
#include "vorbis/vorbisenc.h"

class ShoutVSTEncoderOGG : public ShoutVSTEncoder
{
public:
  bool Initialize(ShoutVST * p);
  bool Close();
  bool Process( float **inputs, long sampleFrames );
  
protected:
  bool SendOGGPageToICE( ogg_page * og );

  ogg_stream_state os; /* take physical pages, weld into a logical
                       stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                       settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */ 

  bool bInitialized;
  ShoutVST * pVST;
};
