//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma warning(disable : 4996)
//#include "GlobalParam.h"
#include "ScrH264Encoder.h"
 
#include <ctime>

 
namespace ScrStreaming
{
   ScrH264Encoder::ScrH264Encoder(int ncol, int nrow)  
   {
 
   }
   char ScrH264Encoder::fetch_packet(uint8_t ** FrameBuffer, unsigned int * FrameSize)
   {
	   return 0;
   }
   char ScrH264Encoder::release_packet()
   {
	   return 0;
   }
   ;

    
}
