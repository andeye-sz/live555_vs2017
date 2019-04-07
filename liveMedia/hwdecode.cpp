#include <string>
#include <stdint.h>
#include <vector>
 
//#include <pthread.h>
//#include <sys/param.h>

#include "hwdecode.h"
#include "ringbuf.h"

#define LOGE printf

#define RINGBUF_SIZE  (10 * 1024 * 1024 -1)

 
ringbuf_t *HwDecode::rb = NULL;
pthread_mutex_t HwDecode::reader_mutex;

void HwDecode::init() {
    LOGE("HwDecode::init");
	pthread_mutex_init(&reader_mutex, nullptr);

    if (rb == NULL) {
        rb = ringbuf_new(RINGBUF_SIZE - 1);
    }
}

void HwDecode::uninit() {
    LOGE("HwDecode::uninit");
    if (rb != NULL) {
        ringbuf_free(&rb);
        rb = NULL;
    }
}

void HwDecode::reset() {
    ringbuf_reset(rb);
}

 
 

void HwDecode::stop() {
    LOGE("HwDecode::stop");
    reset();

 
}

void HwDecode::write264Stream(uint8_t *data, int size) {
	//LOGE("%p wr size  %d, used %d   \n ", data, size, ringbuf_bytes_used(rb) );
    pthread_mutex_lock(&reader_mutex);
    ringbuf_memcpy_into(rb, data, size);
    pthread_mutex_unlock(&reader_mutex);
}


unsigned int HwDecode::read264Stream(uint8_t *data, int size) {
	 int len = 0;

	if (size > 0 && ringbuf_bytes_used(rb) > 0) {
		len = MIN(size, ringbuf_bytes_used(rb));
		LOGE("%p read size  %d, used %d  retlen %d \n ", data, size, ringbuf_bytes_used(rb), len);
		pthread_mutex_lock(&reader_mutex);
		ringbuf_memcpy_from(data, rb, len);
		pthread_mutex_unlock(&reader_mutex);
	}
	return len;
}


/*
static void *reader_thread(void *arg) {
    const int READER_BUF_SIZE = 128 * 4096;
    uint8_t buf[READER_BUF_SIZE];
    FILE *handle = fopen("/data/data/com.blueberrytek/test2.h264", "wb");

    while (true) {
        int cnt = MIN(READER_BUF_SIZE, ringbuf_bytes_used(rb));

        if (cnt > 0) {
            LOGE("write cnd %d, used %d ", cnt, ringbuf_bytes_used(rb));
            pthread_mutex_lock(&reader_mutex);
            ringbuf_memcpy_from(buf, rb, cnt);
            pthread_mutex_unlock(&reader_mutex);


            fwrite(buf, 1, cnt, handle);
            fflush(handle);
        } else
            usleep(50000);
    }
}*/

/*
static int8_t *pBuffer = NULL;
 
static bool eof = false;

JNIEXPORT jint JNICALL
HwDecode::read264Stream(JNIEnv *env, jobject clazz, jlong pos, jbyteArray buf, jint offset,
                        jint size) {
 
    size_t len = 0;
    if (size > 0 && ringbuf_bytes_used(rb) > 0) {
 
        if (pBuffer == NULL) {
            eof = false;
            buffer = buf;
            pBuffer = env->GetByteArrayElements(buf, NULL);  // fixme:  memory leak
        }

        //if (pBuffer != NULL)
        {
            len = MIN(size, ringbuf_bytes_used(rb));
            //LOGE("%p read size  %d, used %d  retlen %d  ", pBuffer, size, ringbuf_bytes_used(rb), len);
            pthread_mutex_lock(&reader_mutex);
            ringbuf_memcpy_from(pBuffer, rb, len);
            pthread_mutex_unlock(&reader_mutex);

            
        }
    }

    return len;
}
*/

 

 
void HwDecode::mirroringOpen() {
   

    reset();
   
}

 
 

