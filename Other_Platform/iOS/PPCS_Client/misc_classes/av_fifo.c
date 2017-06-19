
#include "av_fifo.h"
#include <stdlib.h>
#include <string.h>


// copy buf to b(block_t)
int block_Alloc(block_t *b, void *buf, unsigned int size)
{
	if(b==NULL || size==0) return 0;

	b->p_next   = NULL;
	if(size > 0){
		b->p_buffer = (char *)malloc(size);
		memcpy(b->p_buffer, buf, size);
	}else b->p_buffer = NULL;

	b->i_buffer		= size;
	return 1;
}

void block_Release(block_t *p_block)
{
	if(p_block!=NULL){
		if(p_block->p_buffer!=NULL){
			free(p_block->p_buffer);
			p_block->p_buffer=NULL;
			p_block->i_buffer=0;			
		}
		free(p_block);
	}
}


//==========================================================
//create and init a new fifo
av_fifo_t *av_FifoNew( void)
{
	av_fifo_t *p_fifo = malloc( sizeof( av_fifo_t));
	if( !p_fifo ) return NULL;
	memset(p_fifo, 0, sizeof(av_fifo_t));
	
 	av_mutex_init( p_fifo->lock );
	p_fifo->p_first = NULL;
	p_fifo->pp_last = &p_fifo->p_first;
	p_fifo->i_depth = p_fifo->i_size = 0;

	return p_fifo;
}

//destroy a fifo and free all blocks in it.
void av_FifoRelease(av_fifo_t *p_fifo)
{
	if(p_fifo==NULL) return;

	av_FifoEmpty( p_fifo );
 	av_mutex_destroy( p_fifo->lock );
	free( p_fifo );
	p_fifo=NULL;
}

//free all blocks in a fifo
void av_FifoEmpty( av_fifo_t *p_fifo)
{
	block_t *block;
	if(p_fifo==NULL) return;

	av_mutex_lock(p_fifo->lock );
	block = p_fifo->p_first;
	if (block != NULL)
	{
		p_fifo->i_depth = p_fifo->i_size = 0;
		p_fifo->p_first = NULL;
		p_fifo->pp_last = &p_fifo->p_first;
	}
 	av_mutex_unlock(p_fifo->lock );

	while(block != NULL)
	{
		block_t *buf;

		buf = block->p_next;
		block_Release(block);
		block = buf;
	}	
}

unsigned int av_FifoPut( av_fifo_t *p_fifo, block_t *p_block)
{
	unsigned int i_size = 0, i_depth = 0;
	block_t *p_last;
	if(p_fifo==NULL || p_block == NULL) return 0;

	av_mutex_lock (p_fifo->lock);
	for(p_last = p_block; ; p_last = p_last->p_next)
	{
		i_size += p_last->i_buffer;
		i_depth++;
		if(!p_last->p_next) break;
	}

	*p_fifo->pp_last = p_block;
	p_fifo->pp_last  = &p_last->p_next;
	p_fifo->i_depth  += i_depth;
	p_fifo->i_size   += i_size;

	av_mutex_unlock( p_fifo->lock );

	return p_fifo->i_depth;
}

block_t *av_FifoGetAndRemove( av_fifo_t *p_fifo)
{
	block_t *b;
	if(p_fifo==NULL) return NULL;

 	av_mutex_lock( p_fifo->lock );
	b = p_fifo->p_first;

	if( b == NULL )
	{
		av_mutex_unlock( p_fifo->lock );
		return NULL;
	}

	p_fifo->p_first = b->p_next;
	p_fifo->i_depth--;
	p_fifo->i_size -= b->i_buffer;

	if( p_fifo->p_first == NULL )
	{
		p_fifo->pp_last = &p_fifo->p_first;
	}

 	av_mutex_unlock( p_fifo->lock );

	b->p_next = NULL;
	return b;
}

block_t *av_FifoGet( av_fifo_t *p_fifo)
{
	block_t *b;
	if(p_fifo==NULL) return NULL;
	b = p_fifo->p_first;
	return b;
}

//FIXME: not thread-safe
unsigned int av_FifoSize( const av_fifo_t *p_fifo )
{
	if(p_fifo==NULL) return 0;
	else return p_fifo->i_size;
}

//FIXME: not thread-safe
unsigned int av_FifoCount( const av_fifo_t *p_fifo)
{
	if(p_fifo==NULL) return 0;
	else return p_fifo->i_depth;
}


