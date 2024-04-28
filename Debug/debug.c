
#include "debug.h"
#include "hw_map.h"
#include "main.h"
#include "critical.h"
#include "uart3_dma_handler.h"
#include <string.h>

#define min(x, y) ((x) < (y)) ? (x) : (y)

void dma_uart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
	DMA_InitTypeDef DMA_InitStructure = {0};



    GPIO_InitStructure.GPIO_Pin = DEBUG_UART_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);
    DMA_DeInit(DMA1_Channel2);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART3->DATAR);
    DMA_InitStructure.DMA_MemoryBaseAddr = 0;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);

    USART_Cmd(USART3, ENABLE);
    DMA_Cmd(DMA1_Channel2, DISABLE);

    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

#define QUEUE_BUFFER_SIZE 128
struct dma_queue{
	uint32_t in_pos;
	uint32_t out_pos;
	uint32_t holding;
	uint32_t n_sending;
	uint32_t n_writing;
	char buffer[QUEUE_BUFFER_SIZE];
};

static volatile struct dma_queue dma_queue;


static uint32_t dma_queue_get_read_buffer(char **buffptr)
{
	uint32_t tx_size = 0;
	uint32_t flags;

	flags = critical_lock();

	if (dma_queue.n_sending > 0)
		goto out;

	if(dma_queue.holding == 0)
		goto out;

	tx_size = min(QUEUE_BUFFER_SIZE - dma_queue.out_pos, dma_queue.holding);

	*buffptr = (char *) &dma_queue.buffer[dma_queue.out_pos];
	dma_queue.out_pos = (dma_queue.out_pos + tx_size) % QUEUE_BUFFER_SIZE;
	dma_queue.n_sending = tx_size;

out:
	critical_unlock(flags);

	return tx_size;
}

static void dma_queue_release_read_buffer(void)
{
	uint32_t flags;

	flags = critical_lock();
	if (dma_queue.n_sending) {
		dma_queue.holding -= dma_queue.n_sending;
		dma_queue.n_sending = 0;
	}
	critical_unlock(flags);
}

static uint32_t dma_queue_get_write_buffer(char **buffptr, uint32_t desired_size)
{
	uint32_t write_size = 0;
	uint32_t flags;

	flags = critical_lock();

	if (dma_queue.n_writing > 0)
		goto out;

	if(dma_queue.holding == QUEUE_BUFFER_SIZE)
		goto out;

	write_size = min(QUEUE_BUFFER_SIZE - dma_queue.in_pos, QUEUE_BUFFER_SIZE - dma_queue.holding);
	write_size = min(write_size, desired_size);

	*buffptr = (char *) &dma_queue.buffer[dma_queue.in_pos];
	dma_queue.in_pos = (dma_queue.in_pos + write_size) % QUEUE_BUFFER_SIZE;
	dma_queue.n_writing = write_size;

out:
	critical_unlock(flags);

	return write_size;
}

static void dma_queue_release_write_buffer(void)
{
	uint32_t flags;

	flags = critical_lock();

	if (dma_queue.n_writing) {
		dma_queue.holding += dma_queue.n_writing;
		dma_queue.n_writing = 0;
	}

	critical_unlock(flags);
}

void dma_uart_start_if_needed(void)
{
	uint32_t tx_size;
	char *buffer = NULL;

	tx_size = dma_queue_get_read_buffer(&buffer);

	if (tx_size > 0) {
		DMA_Cmd(DMA1_Channel2, DISABLE);
		DMA1_Channel2->MADDR = (uint32_t) buffer;
		DMA1_Channel2->CNTR = tx_size;
		DMA1->INTFCR = 0xf << (4 * (2 -1));
		DMA_ITConfig(DMA1_Channel2,DMA_IT_TC,ENABLE);
		DMA_Cmd(DMA1_Channel2, ENABLE);
		USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	}
}

void USART_Printf_Init(uint32_t baudrate)
{
	dma_uart_init(baudrate);
}

void dma_uart_append(char *buffer, int size)
{
	while(size > 0) {
		char *wr_buffer;
		uint32_t n_to_write;

		n_to_write = dma_queue_get_write_buffer(&wr_buffer, size);

		if(n_to_write) {
			memcpy(wr_buffer, buffer, n_to_write);
			dma_queue_release_write_buffer();

			buffer += n_to_write;
			size -= n_to_write;
		}

		dma_uart_start_if_needed();
	}
}


void uart3_tx_dma_handler(void)
{
	DMA_ClearFlag(DMA1_FLAG_TC2);
	USART_DMACmd(USART3, USART_DMAReq_Tx, DISABLE);
	dma_queue_release_read_buffer();
	dma_uart_start_if_needed();
}

__attribute__((used))
int _write(int fd, char *buf, int size)
{
    (void)fd;

	dma_uart_append(buf, size);

    return size;
}



