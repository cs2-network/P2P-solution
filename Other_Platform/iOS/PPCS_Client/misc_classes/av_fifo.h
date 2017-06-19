
#ifndef _AV_FIFO_H_
#define _AV_FIFO_H_

#ifdef _WIN32
	#include <windows.h>

	typedef CRITICAL_SECTION av_mutex_t;
	#define av_mutex_init(theMutex)		InitializeCriticalSection(&theMutex)
	#define av_mutex_lock(theMutex)		EnterCriticalSection(&theMutex)
	#define av_mutex_unlock(theMutex)	LeaveCriticalSection(&theMutex)
	#define av_mutex_destroy(theMutex)	DeleteCriticalSection(&theMutex)

#else
    #include <pthread.h>

    typedef pthread_mutex_t	av_mutex_t;
    #define av_mutex_init(theMutex)		pthread_mutex_init(&theMutex, NULL)
    #define av_mutex_lock(theMutex)		pthread_mutex_lock(&theMutex)
    #define av_mutex_unlock(theMutex)	pthread_mutex_unlock(&theMutex)
    #define av_mutex_destroy(theMutex)
#endif


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct _block
{
	struct		  _block *p_next;

	unsigned int	i_buffer;	
	char			*p_buffer;
}block_t;

//@para  0: fail
//		>0: success
int  block_Alloc(block_t *b, void *buf, unsigned int size);
void block_Release(block_t *p_block);


struct _block_fifo
{
	av_mutex_t       lock;
	block_t          *p_first;
	block_t          **pp_last;

	unsigned int     i_depth;
	unsigned int     i_size;
};
typedef struct _block_fifo	av_fifo_t;

av_fifo_t	  *av_FifoNew( void);
void          av_FifoRelease(av_fifo_t *p_fifo);

void          av_FifoEmpty(av_fifo_t *p_fifo);
unsigned int  av_FifoPut(av_fifo_t *p_fifo, block_t *p_block);
block_t       *av_FifoGetAndRemove( av_fifo_t *p_fifo);
block_t       *av_FifoGet(av_fifo_t *p_fifo);
unsigned int  av_FifoSize(const av_fifo_t *p_fifo );
unsigned int  av_FifoCount(const av_fifo_t *p_fifo);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
