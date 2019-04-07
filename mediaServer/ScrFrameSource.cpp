//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#pragma warning(disable : 4996)
#include "ScrFrameSource.h"

namespace ScrStreaming
{
   ScrFrameSource* ScrFrameSource::createNew(UsageEnvironment& env, ScrH264Encoder *encoder) 
   {
      return new ScrFrameSource(env, encoder);
   };

   ScrFrameSource::ScrFrameSource(UsageEnvironment& env, ScrH264Encoder* encoder) :
      FramedSource(env), fEncoder(encoder)
   {
      fEventTriggerId = envir().taskScheduler().createEventTrigger(ScrFrameSource::deliverFrameStub);
      std::function<void()> callbackfunc = std::bind(&ScrFrameSource::onFrame, this);
      //fEncoder->set_callback_func(callbackfunc);
   };

  
   void ScrFrameSource::doStopGettingFrames()
   {
      FramedSource::doStopGettingFrames();
   };

   void ScrFrameSource::onFrame()
   {
      envir().taskScheduler().triggerEvent(fEventTriggerId, this);
   };

   void ScrFrameSource::doGetNextFrame()
   {
      deliverFrame();
   };

   void ScrFrameSource::deliverFrame()
   {
      if (!isCurrentlyAwaitingData()) return; // we're not ready for the buff yet

      static uint8_t* newFrameDataStart;
      static unsigned newFrameSize = 0;

      /* get the buff frame from the Encoding thread.. */
      if (fEncoder->fetch_packet(&newFrameDataStart, &newFrameSize)) {
         if (newFrameDataStart != nullptr) {
            /* This should never happen, but check anyway.. */
            if (newFrameSize > fMaxSize) {
               fFrameSize = fMaxSize;
               fNumTruncatedBytes = newFrameSize - fMaxSize;
            }
            else {
               fFrameSize = newFrameSize;
            }

            gettimeofday(&fPresentationTime, nullptr);
            memcpy(fTo, newFrameDataStart, fFrameSize);

            //delete newFrameDataStart;
            //newFrameSize = 0;

            fEncoder->release_packet();
         }
         else {
            fFrameSize = 0;
            fTo = nullptr;
            handleClosure(this);
         }
      }
      else {
         fFrameSize = 0;
      }

      if (fFrameSize > 0)
         FramedSource::afterGetting(this);
   };
}
