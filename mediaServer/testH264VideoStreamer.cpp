/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A test program that reads a H.264 Elementary Stream video file
// and streams it using RTP
// main program
//
// NOTE: For this application to work, the H.264 Elementary Stream video file *must* contain SPS and PPS NAL units,
// ideally at or near the start of the file.  These SPS and PPS NAL units are used to specify 'configuration' information
// that is set in the output stream's SDP description (by the RTSP server that is built in to this application).
// Note also that - unlike some other "*Streamer" demo applications - the resulting stream can be received only using a
// RTSP client (such as "openRTSP")


//-----------------------------datalist------------------------------
#pragma comment(lib,"pthreadVC2.lib")

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
//#include <windows.h>
 

#define STATIC_MEMORY

#define FRAME_MEM_SIZE	(1920*1080*2)
#define MAX_QUENE_LENGTH 10

static void usleep(unsigned int usec);

typedef struct NODE {
	void *msg;
	unsigned int msgsize;
	struct NODE *next;
}Node_t, *pNode_t;

typedef struct {
	//depend on Linux OS
	pthread_mutex_t qlock;
	sem_t sem;
	int q_num;
	pNode_t q_head;
}Quene_t, *pQuene_t;

static Quene_t ListData;
static unsigned char availibleNode[MAX_QUENE_LENGTH] = { 0,0,0,0,0,0,0,0,0,0 };
static Node_t nodeMemStore[MAX_QUENE_LENGTH];
static unsigned char availibleFrame[MAX_QUENE_LENGTH] = { 0,0,0,0,0,0,0,0,0,0 };
static char frameMemStore[MAX_QUENE_LENGTH][FRAME_MEM_SIZE];


static void *mallocNode(void)
{
	unsigned char i = 0;
	for (i = 0; i < MAX_QUENE_LENGTH; i++)
	{
		if (availibleNode[i] == 0)
		{
			availibleNode[i] = 1;
			return &nodeMemStore[i];
		}
	}
	return NULL;
}

static void freeNode(pNode_t node)
{
	unsigned char i = 0;
	for (i = 0; i < MAX_QUENE_LENGTH; i++)
	{
		if (&nodeMemStore[i] == node)
			availibleNode[i] = 0;
	}
}

static void *mallocFrame(void)
{
	unsigned char i = 0;
	for (i = 0; i < MAX_QUENE_LENGTH; i++)
	{
		if (availibleFrame[i] == 0)
		{
			availibleFrame[i] = 1;
			return &frameMemStore[i];
		}
	}
	return NULL;
}

static void freeFrame(void* frame)
{
	unsigned char i = 0;
	for (i = 0; i < MAX_QUENE_LENGTH; i++)
	{
		if (&frameMemStore[i] == frame)
			availibleFrame[i] = 0;
	}
}

//depend on Linux OS
static int InitDataList(void)
{
	if (pthread_mutex_init(&ListData.qlock, NULL) != 0)
		return -1;
	memset(availibleNode, 0, sizeof(availibleNode));
	memset(nodeMemStore, 0, sizeof(nodeMemStore));
	memset(availibleFrame, 0, sizeof(availibleFrame));
	memset(frameMemStore, 0, sizeof(frameMemStore));
	return 0;
}
//depend on Linux OS
static int dataLock(void)
{
	return pthread_mutex_lock(&ListData.qlock);
}
//depend on Linux OS
static int dataUnlock(void)
{
	return pthread_mutex_unlock(&ListData.qlock);
}

//depend on Linux OS
static int InitDataSignal(void)
{
	return sem_init(&ListData.sem, 0, 0);
}
//depend on Linux OS
static int waitDataSignal(void)
{
	return sem_wait(&ListData.sem);
}
//depend on Linux OS
static int emitDataSignal(void)
{
	return sem_post(&ListData.sem);
}

static int PutData(void * msg, unsigned int * size)
{
	int ret = -1;
	dataLock();
	if (ListData.q_num < MAX_QUENE_LENGTH)
	{
#ifndef STATIC_MEMORY
		pNode_t node = malloc(sizeof(Node_t));
#else
		pNode_t node = (pNode_t)mallocNode();
#endif
		if (node == NULL)
		{
			ret = -1;
		}
		else
		{
#ifndef STATIC_MEMORY
			node->msg = malloc(*size);
#else
			node->msg = mallocFrame();
#endif
			if (node->msg == NULL)
			{
#ifndef STATIC_MEMORY
				free(node);
#else
				freeNode(node);
#endif
				ret = -1;
			}
			else
			{
				int i = ListData.q_num;
				pNode_t n = ListData.q_head;
				memcpy(node->msg, msg, *size);
				node->msgsize = *size;
				node->next = NULL;
				if (i == 0)
				{
					ListData.q_head = node;
				}
				else
				{
					while (i - 1)
					{
						n = n->next;
						i--;
					}
					n->next = node;
				}
				ListData.q_num += 1;
				ret = 0;
			}
		}
	}
	dataUnlock();
	//emitDataSignal();
	return ret;
}

static int RemoveData(void)
{
	int ret = -1;
	dataLock();
	if (0 == ListData.q_num)
		ret = -1;
	else
	{
		pNode_t n = ListData.q_head;
		ListData.q_head = ListData.q_head->next;
		ListData.q_num -= 1;
#ifndef STATIC_MEMORY
		free(n->msg);
		free(n);
#else
		freeFrame(n->msg);
		n->msgsize = 0;
		freeNode(n);
#endif
		ret = 0;
	}
	dataUnlock();
	return ret;
}

static unsigned int GetNextFrameSize(void)
{
	unsigned int szie = 0;
	dataLock();
	if (ListData.q_num != 0)
		szie = ListData.q_head->msgsize;
	dataUnlock();
	return szie;
}

static unsigned int GetData(unsigned char * framebuf, unsigned int size)
{
	unsigned int ret = 0;
	dataLock();
	if (ListData.q_num == 0)
	{
		ret = 0;
		dataUnlock();
		return ret;
	}
	else
	{
		if (size < ListData.q_head->msgsize)
		{
			dataUnlock();
			printf("error:%s line:%d %d %d\n", __func__, __LINE__, size, ListData.q_head->msgsize);
			RemoveData();
			return ret;
		}
		memcpy(framebuf, ListData.q_head->msg, ListData.q_head->msgsize);
		ret = ListData.q_head->msgsize;
		dataUnlock();
		RemoveData();
		return ret;
	}
}

//---------------------------MyByteStreamFileSource--------------------------
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

class MyByteStreamFileSource : public FramedFileSource {
public:
	static MyByteStreamFileSource* createNew(UsageEnvironment& env,
		char const* fileName,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0)
	{
		MyByteStreamFileSource* newSource
			= new MyByteStreamFileSource(env, 0, preferredFrameSize, playTimePerFrame);
		newSource->fFileSize = 0;//GetFileSize(fileName, fid);
		printf(">fFileSize %d<:%llu\n", __LINE__, newSource->fFileSize);
		return newSource;
	}
	u_int64_t fileSize() const { return fFileSize; }
	// 0 means zero-length, unbounded, or unknown

	void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0)
	{
		printf("%s %d\n", __func__, __LINE__);
		fNumBytesToStream = numBytesToStream;
		fLimitNumBytesToStream = fNumBytesToStream > 0;
	}
	// if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
	void seekToByteRelative(int64_t offset, u_int64_t numBytesToStream = 0)
	{
		printf("%s %d\n", __func__, __LINE__);
		fNumBytesToStream = numBytesToStream;
		fLimitNumBytesToStream = fNumBytesToStream > 0;
	}
	void seekToEnd()// to force EOF handling on the next read
	{
		printf("%s %d\n", __func__, __LINE__);
	}

protected:
	MyByteStreamFileSource(UsageEnvironment& env, FILE* fid,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame)
		: FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
		fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
		fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0)
	{
		printf(">%d<\n", __LINE__);
		overFlowNum = 0;
		overFlowIndex = 0;
		// Test whether the file is seekable
		fFidIsSeekable = 0;
		printf(">fFidIsSeekable %d<:%d\n", __LINE__, fFidIsSeekable);
	}

	virtual ~MyByteStreamFileSource()
	{
		if (fFid == NULL)
			return;
		CloseInputFile(fFid);
	}

	static void fileReadableHandler(void* source)
	{
		MyByteStreamFileSource* pThis = (MyByteStreamFileSource*)source;
		if (!pThis->isCurrentlyAwaitingData())
		{
			pThis->doStopGettingFrames(); // we're not ready for the data yet
			return;
		}
		pThis->doReadFromFile();
	}

	void doReadFromFile()
	{
		//printf("%s %d\n",__func__,__LINE__);
		// Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
		if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize)
		{
			printf("fNumBytesToStream:%llu fMaxSize:%u fPreferredFrameSize:%u\n",
				fNumBytesToStream, fMaxSize, fPreferredFrameSize);
			fMaxSize = (unsigned)fNumBytesToStream;
		}
		if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize)
		{
			printf("fPreferredFrameSize:%llu fMaxSize:%u fPreferredFrameSize:%u\n",
				fNumBytesToStream, fMaxSize, fPreferredFrameSize);
			fMaxSize = fPreferredFrameSize;
		}
	f_again:
		if (overFlowNum)
		{
			if (overFlowNum - overFlowIndex > fMaxSize)
			{
				memcpy(fTo, &overFlow[overFlowIndex], fMaxSize);
				fFrameSize = fMaxSize;
				overFlowIndex += fFrameSize;
				printf("%s %d>\n", __func__, __LINE__);
			}
			else
			{
				memcpy(fTo, &overFlow[overFlowIndex], overFlowNum - overFlowIndex);
				fFrameSize = overFlowNum - overFlowIndex;
				overFlowIndex += fFrameSize;
				printf("%s %d<\n", __func__, __LINE__);
			}
			if (overFlowNum == overFlowIndex)
			{
				overFlowNum = 0;
				overFlowIndex = 0;
				printf("%s %d reset\n", __func__, __LINE__);
			}
		}
		else if (overFlowNum == 0 && GetNextFrameSize() > fMaxSize)
		{
			if ((overFlowNum = GetData(overFlow, FRAME_MEM_SIZE)) > 0)
			{
				printf("%s %d max\n", __func__, __LINE__);
				memcpy(fTo, overFlow, fMaxSize);
				fFrameSize = fMaxSize;
				overFlowIndex = fMaxSize;
				if (overFlowNum == overFlowIndex)
				{
					overFlowNum = 0;
					overFlowIndex = 0;
				}
			}
			else
			{
				printf("%s sleep %d\n", __func__, __LINE__);
				usleep(50000);
				goto f_again;
			}
		}
		else if ((fFrameSize = GetData(fTo, fMaxSize)) == 0)
		{
			printf("%s no src data %d\n", __func__, __LINE__);
			usleep(50000);
			goto f_again;
		}
		if (fFrameSize == 0)
		{
			handleClosure();
			return;
		}
		fNumBytesToStream -= fFrameSize;

		// Set the 'presentation time':
		if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0)
		{
			if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)
			{
				// This is the first frame, so use the current time:
				gettimeofday(&fPresentationTime, NULL);
			}
			else
			{
				// Increment by the play time of the previous data:
				unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
				fPresentationTime.tv_sec += uSeconds / 1000000;
				fPresentationTime.tv_usec = uSeconds % 1000000;
			}
			// Remember the play time of this data:
			fLastPlayTime = (fPlayTimePerFrame*fFrameSize) / fPreferredFrameSize;
			fDurationInMicroseconds = fLastPlayTime;
		}
		else
		{
			// We don't know a specific play time duration for this data,
			// so just record the current time as being the 'presentation time':
			gettimeofday(&fPresentationTime, NULL);
		}

		// Because the file read was done from the event loop, we can call the
		// 'after getting' function directly, without risk of infinite recursion:
		FramedSource::afterGetting(this);
	}

private:
	// redefined virtual functions:
	virtual void doGetNextFrame()
	{
		//printf("%s %d\n",__func__,__LINE__);
		envir().taskScheduler().scheduleDelayedTask(0, fileReadableHandler, this);
	}
	virtual void doStopGettingFrames()
	{
		//printf("%s %d\n",__func__,__LINE__);
		envir().taskScheduler().unscheduleDelayedTask(nextTask());
	}

protected:
	u_int64_t fFileSize;
	u_int64_t overFlowNum;
	u_int64_t overFlowIndex;
	unsigned char overFlow[FRAME_MEM_SIZE];

private:
	unsigned fPreferredFrameSize;
	unsigned fPlayTimePerFrame;
	Boolean fFidIsSeekable;
	unsigned fLastPlayTime;
	Boolean fHaveStartedReading;
	Boolean fLimitNumBytesToStream;
	u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

//------------------------main handle-------------------------

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

#include <sys/types.h>
#include <sys/stat.h>
//#include <unistd.h>
#include <io.h>  
#include <fcntl.h>
#include <errno.h>

typedef enum {
	server_idle = 0,
	server_inited,
	server_pause
}service_status_t;

typedef struct {
	service_status_t inited;
	UsageEnvironment* env;
	TaskScheduler* scheduler;

	unsigned short rtpPortNum;
	unsigned short rtcpPortNum;
	unsigned char ttl;

	Port *rtpPort;
	Port *rtcpPort;
	Groupsock *rtpGroupsock;
	Groupsock *rtcpGroupsock;

	H264VideoStreamFramer* videoSource;
	RTPSink* videoSink;
	RTCPInstance* rtcp;
	RTSPServer* rtspServer;
	ServerMediaSession* sms;
	PassiveServerMediaSubsession *passiveSubsession;
	MyByteStreamFileSource* fileSource;

	pthread_t putdata_pid;
	pthread_t loop_pid;
}LocalServer_t;

#define SIZES	2048
#define BANDWIDTH	(512)
#define MAX_CNAME_LEN	100

static const char *scrpath = "0.h264";
static FILE* fd = NULL;
static unsigned char running = 0;
static unsigned char CNAME[MAX_CNAME_LEN + 1];
static LocalServer_t SeverSetting = {
	SeverSetting.inited = server_idle,
	SeverSetting.env = NULL,
	SeverSetting.scheduler = NULL,

	SeverSetting.rtpPortNum = 0,
	SeverSetting.rtcpPortNum = 0,
	SeverSetting.ttl = 0,

	SeverSetting.rtpPort = NULL,
	SeverSetting.rtcpPort = NULL,
	SeverSetting.rtpGroupsock = NULL,
	SeverSetting.rtcpGroupsock = NULL,

	SeverSetting.videoSource = NULL,
	SeverSetting.videoSink = NULL,
	SeverSetting.rtcp = NULL,
	SeverSetting.rtspServer = NULL,
	SeverSetting.sms = NULL,
	SeverSetting.passiveSubsession = NULL,
	SeverSetting.fileSource = NULL,

	SeverSetting.putdata_pid = {NULL, 0},
	SeverSetting.loop_pid = {NULL, 0},
};

int InsertFrame(char * buf, int len)
{
	if (SeverSetting.inited == server_inited)
		return PutData((void *)buf, (unsigned int*)&len);
	else
		return -1;
}

void StopRtspServer(void)
{
	printf("%s relese\n", __func__);
	if (SeverSetting.inited == server_inited)
		SeverSetting.inited = server_pause;
	//SeverSetting.videoSink->stopPlaying();
	//Medium::close(SeverSetting.videoSource);	
	printf("%s relese all\n", __func__);
}

static void* receive_thread(void* param)
{
	unsigned int len = 0;
	unsigned char buf[FRAME_MEM_SIZE];
	printf("%s %d\r\n", __func__, __LINE__);
	fd = fopen(scrpath, "rb");
	if (fd)
		printf("open ok\n");
	else
	{
		printf("open \"%s\" fail\n", scrpath);
		_exit(1);
	}
	printf("%s %d\n", __func__, __LINE__);
	running = 1;
	while (running)
	{
		if ((len = fread(buf, 1, SIZES, fd)) > 0)
		{
		f_try:
			if (InsertFrame((char*)buf, (int)len) < 0)
			{
				usleep(100000);
				goto f_try;
			}
		}
		if (len < SIZES || len == 0)
			fseek(fd, 0, SEEK_SET);
	}
	fclose(fd);
	printf("%s %d\n", __func__, __LINE__);
	pthread_exit(NULL);
	return NULL;
}

static void* loop_thread(void* param)
{
	printf("task schedule start...[%d]\n", __LINE__);
	SeverSetting.env->taskScheduler().doEventLoop();
	printf("task schedule start...[%d]\n", __LINE__);
	pthread_exit(NULL);
	return NULL;
}

static void run_tasks(void)
{
	pthread_create(&SeverSetting.putdata_pid, NULL, receive_thread, NULL);
	pthread_detach(SeverSetting.putdata_pid);
	pthread_create(&SeverSetting.loop_pid, NULL, loop_thread, NULL);
	pthread_detach(SeverSetting.loop_pid);
}

static int start_play(void)
{
	// Open the input file as a 'byte-stream file source':
	SeverSetting.fileSource = MyByteStreamFileSource::createNew(*SeverSetting.env, NULL);
	if (!SeverSetting.fileSource)
	{
		//*SeverSetting.env << "Unable to open file \"" << inputFileName<< "\" as a byte-stream file source\n";
		return -1;
	}
	// Create a framer for the Video Elementary Stream:
	SeverSetting.videoSource = H264VideoStreamFramer::createNew(*SeverSetting.env, (FramedSource*)SeverSetting.fileSource);
	if (!SeverSetting.videoSource)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	// Finally, start playing:
	*SeverSetting.env << "start playing..." << __LINE__ << "\n";
	SeverSetting.videoSink->startPlaying(*SeverSetting.videoSource, NULL, NULL);
	return 0;
}

int StartRtspServer(char *url_suffix, int port)
{
	printf("%s %d[%d]\r\n", __func__, __LINE__, SeverSetting.inited);
	if (SeverSetting.inited == server_pause)
	{
		char* url = SeverSetting.rtspServer->rtspURL(SeverSetting.sms);
		*SeverSetting.env << "Play this stream using the URL \"" << url << "\"\n";
		delete[] url;
		*SeverSetting.env << "start playing..." << __LINE__ << "\n";
		//SeverSetting.videoSink->startPlaying(*SeverSetting.videoSource, NULL, NULL);
		SeverSetting.inited = server_inited;
		return 0;
	}
	else if (SeverSetting.inited == server_inited)
		return -1;
	if (InitDataList())
	{
		printf("InitDataLock fail\n");
		return -1;
	}
	SeverSetting.scheduler = BasicTaskScheduler::createNew();
	if (!SeverSetting.scheduler)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.env = BasicUsageEnvironment::createNew(*SeverSetting.scheduler);
	if (!SeverSetting.env)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.rtpPortNum = 18888;
	SeverSetting.rtcpPortNum = SeverSetting.rtpPortNum + 1;
	SeverSetting.ttl = 255;
	SeverSetting.rtpPort = new Port(SeverSetting.rtpPortNum);
	if (!SeverSetting.rtpPort)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.rtcpPort = new Port(SeverSetting.rtcpPortNum);
	if (!SeverSetting.rtcpPort)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	struct in_addr destinationAddress;
	destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*SeverSetting.env);
	SeverSetting.rtpGroupsock = new Groupsock(*SeverSetting.env, destinationAddress,
		*SeverSetting.rtpPort, SeverSetting.ttl);
	if (!SeverSetting.rtpGroupsock)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.rtpGroupsock->multicastSendOnly(); // we're a SSM source
	SeverSetting.rtcpGroupsock = new Groupsock(*SeverSetting.env, destinationAddress,
		*SeverSetting.rtcpPort, SeverSetting.ttl);
	if (!SeverSetting.rtcpGroupsock)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.rtcpGroupsock->multicastSendOnly(); // we're a SSM source
	// Create a 'H264 Video RTP' sink from the RTP 'groupsock':
	OutPacketBuffer::maxSize = 1024 * 1024;
	SeverSetting.videoSink = H264VideoRTPSink::createNew(*SeverSetting.env,
		SeverSetting.rtpGroupsock, 96);
	if (!SeverSetting.videoSink)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	// Create (and start) a 'RTCP instance' for this RTP sink:
	memset(CNAME, 0, MAX_CNAME_LEN);
	gethostname((char*)CNAME, MAX_CNAME_LEN);
	CNAME[MAX_CNAME_LEN] = '\0'; // just in case
	SeverSetting.rtcp = RTCPInstance::createNew(*SeverSetting.env, SeverSetting.rtcpGroupsock,
		BANDWIDTH,// in kbps; for RTCP b/w share
		CNAME,
		SeverSetting.videoSink, NULL /* we're a server */,
		True /* we're a SSM source */);
	if (!SeverSetting.rtcp)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.rtspServer = RTSPServer::createNew(*SeverSetting.env, port);
	if (SeverSetting.rtspServer == NULL)
	{
		*SeverSetting.env << "Failed to create RTSP server: " << SeverSetting.env->getResultMsg() << "\n";
		return -1;
	}
	SeverSetting.sms = ServerMediaSession::createNew(*SeverSetting.env, url_suffix, NULL,
		"Session streamed by \"testH264VideoStreamer\"", True);
	if (!SeverSetting.sms)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.passiveSubsession = PassiveServerMediaSubsession::createNew(*SeverSetting.videoSink, SeverSetting.rtcp);
	if (!SeverSetting.passiveSubsession)
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	SeverSetting.sms->addSubsession(SeverSetting.passiveSubsession);
	SeverSetting.rtspServer->addServerMediaSession(SeverSetting.sms);
	char* url = SeverSetting.rtspServer->rtspURL(SeverSetting.sms);
	*SeverSetting.env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;
	// Start the streaming:
	*SeverSetting.env << "Beginning streaming...\n";
	if (start_play())
	{
		printf("%s %d\r\n", __func__, __LINE__);
		return -1;
	}
	run_tasks();
	SeverSetting.inited = server_inited;
	return 0;
}

static void usleep(unsigned int usec)
{
	//std::this_thread::sleep_for(std::chrono::microseconds(usec));
	try {
		//Sleep(usec * 1000); // Note uppercase S
	}
	catch (...) {
		printf("sleep error\n");
	}

	//HANDLE timer;
	//LARGE_INTEGER ft;

	//ft.QuadPart = -(10 * (__int64)usec);

	//timer = CreateWaitableTimer(NULL, TRUE, NULL);
	//SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	//WaitForSingleObject(timer, INFINITE);
	//CloseHandle(timer);
}

int main(int argc, char *argv[])
{
	StartRtspServer((char*)"xxx", 8554);
	while (1)
	{
		Sleep(10000000 * 1000);
	}
	return 0;
}

#if 0
int main(int argc, char *argv[])
{
#if 0
	if (StartRtspServer((char*)"xxx", 8554))
		printf("StartRtspServer fail\r\n");
	else
		printf("StartRtspServer ok\r\n");
	sleep(3);
#else
	int i = 100;
	while (i--)
	{
		if (StartRtspServer((char*)"xxx", 8554))
		{
			printf("StartRtspServer fail\r\n");
			return -1;
		}
		else
		{
			printf("StartRtspServer ok\r\n");
			sleep(3);
			StopRtspServer();
			printf("\r\n\r\n\r\n\r\n");
			sleep(10);
		}
	}
#endif
	return 0;
}
#endif












