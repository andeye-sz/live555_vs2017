//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma  once

#include "misc.h"
#include "FramedSource.hh"
#include "UsageEnvironment.hh"
#include "Groupsock.hh"
#include "GroupsockHelper.hh"
#include "ScrH264Encoder.h"

namespace ScrStreaming
{
   class ScrFrameSource : public FramedSource {
   public:
      static ScrFrameSource* createNew(UsageEnvironment&, ScrH264Encoder*);
      ScrFrameSource(UsageEnvironment& env, ScrH264Encoder*);
      ~ScrFrameSource() = default;

   private:
      static void deliverFrameStub(void* clientData) {
         ((ScrFrameSource*)clientData)->deliverFrame();
      };
      void doGetNextFrame() override;
      void deliverFrame();
      void doStopGettingFrames() override;
      void onFrame();

   private:
      ScrH264Encoder   *fEncoder;
      EventTriggerId       fEventTriggerId;
   };
}

