#include "xparameters.h"
#include "xaxidma.h"
#include <stdio.h>
#include <xstatus.h>

#define PIXELS 28*28
#define N_ELEMENTS 6
#define TIMEOUT_CYCLES 1000000

/*
This program is garbage when AXI DMA stalls (i mean, just like everythong when tis f---ing dma stalls)
*/

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
    volatile char TxBuffer[PIXELS*N_ELEMENTS] __attribute__ ((aligned (32)));
    // Rx buffer, the respoinse we got
    volatile char RxBuffer[N_ELEMENTS] __attribute__ ((aligned (32)));

    // fill TX buffer with some placeholder data
    for(int j = 0; j < N_ELEMENTS; j++){
        for(int i = 0; i < PIXELS; i++){
            switch(j%3){
                case 0:
                    TxBuffer[j * PIXELS + i] = (char)i*2; // SHOULD RECOGNIZE 2
                    break;
                case 1:
                    TxBuffer[j * PIXELS + i] = (char)255; // SHOULD RECOGNIZE 8
                    break;
                case 2:
                    TxBuffer[j * PIXELS + i] = (char)i*7; // SHOULD RECOGNIZE 3
                    break;
            }
        }
    }


    // START TRANSFERS
    for(int k = 0; k < N_ELEMENTS; k++){
        // flush the cache
        Xil_DCacheFlushRange((UINTPTR)TxBuffer, N_ELEMENTS * PIXELS * sizeof(char));
        Xil_DCacheFlushRange((UINTPTR)RxBuffer, N_ELEMENTS * sizeof(char));
        Xil_DCacheInvalidateRange((UINTPTR)&RxBuffer[k], sizeof(char));
        dmb();  // Data Memory Barrier

        status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)&TxBuffer[k*PIXELS], PIXELS * sizeof(char), XAXIDMA_DMA_TO_DEVICE);
        if (status != XST_SUCCESS) {
            printf("DMA transfer to device failed\n");
            return XST_FAILURE;
        }
        
        for(int j = 0; j < TIMEOUT_CYCLES; j++){
            usleep(1);        
        }

        Xil_DCacheInvalidateRange((UINTPTR)&RxBuffer[k], sizeof(char));
        dmb();  // Data Memory Barrier

        status = XAxiDma_SimpleTransfer(&AxiDma, (UINTPTR)&RxBuffer[k], sizeof(char), XAXIDMA_DEVICE_TO_DMA);
        if (status != XST_SUCCESS) {
            printf("DMA transfer from device failed\n");
            return XST_FAILURE;
        }

        for(int j = 0; j < TIMEOUT_CYCLES; j++){
            usleep(1);        
        }

        Xil_DCacheInvalidateRange((UINTPTR)&RxBuffer[k], sizeof(char));
        dmb();  // Data Memory Barrier
    }

    // display tthe results
    for(int i = 0; i < N_ELEMENTS; i++){
        printf("FPGA valeur RxBuffer[%d] = %d\n", i, RxBuffer[i]);
    }


    // Some Flags check for test purposes
    if(XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE)){
        printf("DMA_TO_DEVICE flagged busy\n");
    }

    if(XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)){
        printf("DEVICE_TO _DMA flagged busy\n");
    }

    return 0;
}
