#include "xparameters.h"
#include "xaxidma.h"
#include <stdio.h>
#include <xstatus.h>

#define BUFFER_DEPTH 28*28
#define MEASURE_SAMPLE_DEPTH 1000
#define TIMEOUT_CYCLES 1000000

XAxiDma AxiDma;
int init_dma(XAxiDma *AxiDma){
    XAxiDma_Config* CfgPtr;
    int status;

    // Look up the configuration in the hardware configuration table
    CfgPtr = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_BASEADDR);
    if (!CfgPtr) {
        printf("No configuration found for %d\n", XPAR_AXI_DMA_0_BASEADDR);
        return XST_FAILURE;
    }

    // Initialize the DMA handle with the configuration structure
    status = XAxiDma_CfgInitialize(AxiDma, CfgPtr);
    if (status != XST_SUCCESS) {
        printf("Initialization failed\n");
        return XST_FAILURE;
    }

    // Check for Scatter Gather mode, which is not supported in this example
    if (XAxiDma_HasSg(AxiDma)) {
        printf("Device configured as SG mode\n");
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}

int main(void){
    printf("hi mom");
    int status = init_dma(&AxiDma);
    if(status != XST_SUCCESS){
        printf("Error while initialize the DMA");
        return 1;
    }

    // Rx Buffer, the data we send
    char TxBuffer[BUFFER_DEPTH] __attribute__ ((aligned (32)));
    // Rx buffer, the respoinse we got
    char RxBuffer[MEASURE_SAMPLE_DEPTH] __attribute__ ((aligned (32)));

    // fill TX buffer with some placeholder data
    for(int i = 0; i < BUFFER_DEPTH; i++){
        TxBuffer[i] = (char)i*2;
    }

    // flush the cache
    Xil_DCacheFlushRange((UINTPTR)RxBuffer, BUFFER_DEPTH * sizeof(char));
    Xil_DCacheFlushRange((UINTPTR)TxBuffer, MEASURE_SAMPLE_DEPTH * sizeof(char));

    // START TRANFER
    status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)TxBuffer, BUFFER_DEPTH * sizeof(char), XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS) {
        printf("DMA transfer to FFT failed\n");
        return XST_FAILURE;
    }

    // test timeout
    for(int i = 0; i < TIMEOUT_CYCLES; i++){
        usleep(1);
    }


    status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBuffer, MEASURE_SAMPLE_DEPTH * sizeof(char), XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        printf("DMA transfer from failed\n");
        return XST_FAILURE;
    }

    while (1) {
        if(!XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE)
        && !XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)){
            break;
        }
    }

    // make sure to use printf() / not xil_printf() here
    for(int i = 0; i < MEASURE_SAMPLE_DEPTH; i++){
        printf("FPGA valeur RxBuffer[%d] = %d\n", i, RxBuffer[i]);
    }

    return 0;
}
