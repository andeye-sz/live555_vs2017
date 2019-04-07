#pragma once
#ifndef HWDECODE_H
#define HWDECODE_H

#include <stdint.h>
#include "ringbuf.h"
#include "crossthread.h"

class HwDecode {
private:
 

    static ringbuf_t *rb ;
    static pthread_mutex_t reader_mutex;


public:

    static void init();
    static void uninit ();

 

 

    static void reset();

    static void write264Stream(uint8_t *data, int size);

	static unsigned int read264Stream(uint8_t * data, int size);

    static void stop();

	 
	 

	static void mirroringOpen();
};

#endif
