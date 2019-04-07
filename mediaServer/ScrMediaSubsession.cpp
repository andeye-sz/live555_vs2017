//***********************************************************
//  RTSP server for combined picture from multiple Scrs
//
//  Program written by Jiaxin Du, jiaxin.du@outlook.com
//  All right reserved.
//
//***********************************************************
#include <H264VideoStreamDiscreteFramer.hh>
#include "ScrMediaSubsession.h"

namespace ScrStreaming
{
   ScrMediaSubsession* ScrMediaSubsession::createNew(UsageEnvironment& env, StreamReplicator* replicator)
   {
      return new ScrMediaSubsession(env, replicator);
   }

   FramedSource* ScrMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
   {
      estBitrate = fBitRate;
      FramedSource* source = m_replicator->createStreamReplica();
	  return H264VideoStreamFramer::createNew(envir(), source);

//      return H264VideoStreamDiscreteFramer::createNew(envir(), source, False);
	 
   }

   RTPSink* ScrMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
   {
      return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, fSPSNAL, fSPSNALSize, fPPSNAL, fPPSNALSize);
   }
}
