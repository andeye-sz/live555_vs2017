//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#pragma once

#include "misc.h"
 
 
namespace ScrStreaming
{
    

   class ScrH264Encoder
   {
   public:
      static const char* codec;
   public:
      ScrH264Encoder(int ncol = 2, int nrow = 2);
      ~ScrH264Encoder() {
     
      };
 
	  char fetch_packet(uint8_t** FrameBuffer, unsigned int* FrameSize);

	  char release_packet();
   };
}
