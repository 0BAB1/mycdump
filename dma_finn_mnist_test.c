#include "xparameters.h"
#include "xaxidma.h"
#include <stdio.h>
#include <xstatus.h>

#define PIXELS 1000
#define N_ELEMENTS 1
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
    int status = init_dma(&AxiDma);
    if(status != XST_SUCCESS){
        printf("Error while initialize the DMA");
        return 1;
    }

    // Rx Buffer, the data we send
    char TxBuffer[PIXELS*N_ELEMENTS] __attribute__ ((aligned (32)));
    // Rx buffer, the respoinse we got
    char RxBuffer[N_ELEMENTS] __attribute__ ((aligned (32)));

    // fill TX buffer with some placeholder data
    for(int i = 0; i < PIXELS; i++){
        TxBuffer[i] = (char)255;
    }

    // flush the cache
    Xil_DCacheFlushRange((UINTPTR)RxBuffer, PIXELS * sizeof(char));
    Xil_DCacheFlushRange((UINTPTR)TxBuffer, PIXELS * sizeof(char));



    // START TRANFER
    status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)TxBuffer, PIXELS * sizeof(char), XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS) {
        printf("DMA transfer to FFT failed\n");
        return XST_FAILURE;
    }
    
    for(int j = 0; j < TIMEOUT_CYCLES; j++){
        usleep(1);        
    }

    status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)RxBuffer, N_ELEMENTS * sizeof(char), XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        printf("DMA transfer from failed\n");
        return XST_FAILURE;
    }

    for(int j = 0; j < TIMEOUT_CYCLES; j++){
        usleep(1);        
    }

    // make sure to use printf() / not xil_printf() here
    for(int i = 0; i < N_ELEMENTS; i++){
        printf("FPGA valeur RxBuffer[%d] = %d\n", i, RxBuffer[i]);
    }

    if(XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE)){
        printf("DMA_TO_DEVICE flagged busy");
    }

    if(XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)){
        printf("DEVICE_TO _DMA flagged busy");
    }

    return 0;
}
