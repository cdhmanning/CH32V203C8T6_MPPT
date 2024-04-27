
#include "debug.h"
#include "hw_map.h"
#include "main.h"

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
}

static char buffer[100];
static uint32_t buffer_holding;


void dma_uart_start(void)
{
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA1_Channel2->MADDR = (uint32_t) buffer;
    DMA1_Channel2->CNTR = buffer_holding;
    DMA1->INTFCR = 0xf << (4 * (2 -1));
    DMA_Cmd(DMA1_Channel2, ENABLE);
    USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
}

void USART_Printf_Init(uint32_t baudrate)
{
	dma_uart_init(baudrate);
}

void dma_uart_append(char *buf, int size)
{

	int i;

	for(i = 0; i < size; i++){
		buffer[buffer_holding] = *buf;
		buf++;
		buffer_holding++;
	}

	if (buffer_holding < 10)
		return;

	dma_uart_start();

#if 1
	while(DMA_GetFlagStatus(DMA1_FLAG_TC2) == 0) {
    	/* spin */
    }
#endif
	buffer_holding = 0;
}

__attribute__((used))
int _write(int fd, char *buf, int size)
{
    (void)fd;

	dma_uart_append(buf, size);

    return size;
}

