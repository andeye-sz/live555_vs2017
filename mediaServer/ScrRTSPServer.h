//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************

#include "misc.h"
#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "ScrMediaSubsession.h"
#include "ScrFrameSource.h"
#include "ScrH264Encoder.h"

namespace ScrStreaming
{
   class ScrRTSPServer
   {
   public:

      ScrRTSPServer(ScrH264Encoder  * enc, int port, int httpPort);
      ~ScrRTSPServer();
     
      void run();

      void signal_exit() {
         fQuit = 1;
      };

      void set_bit_rate(uint64_t br) {
         if (br < 102400) {
            fBitRate = 100;
         }
         else {
            fBitRate = static_cast<unsigned int>(br / 1024); //in kbs
         }
      };

   private:
      int                 fPortNum;
      int                 fHttpTunnelingPort;
      unsigned int        fBitRate;
      ScrH264Encoder  *fH264Encoder;
      char                fQuit;
   };
}
