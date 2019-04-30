/*
 * Simple program to measure execution time of FRDM-K64F and the 
 * application board
 *
 * DK - 28-11-2018
 */

#include <mbed.h>
#include <aes.h>
#define FREESCALE_MMCAU 1
#ifdef __cplusplus
extern "C" {
#endif
#include "cau_api.h"
#ifdef __cplusplus
}
#endif

#include <stdbool.h>
#include <stdint.h>
#include <mbed.h>
#include "C12832.h"
#include "MMA7660.h"
#include "LM75B.h"
#include <stdio.h>
#include "counter.h"

enum controlData {N_ITERATIONS = 2500};

static DigitalOut red(LED_RED);
static DigitalIn sw3(PTA4);
C12832 lcd(D11, D13, D12, D7, D10);
Serial pc(USBTX, USBRX);
uint32_t timeElapsed;

void displayStats(uint32_t, uint32_t, uint32_t, uint64_t);

mbedtls_aes_context aes_ctx;

int encrypt_payload_software(const uint8_t *buffer, uint16_t size,
                                   const uint8_t *key, const uint32_t key_length,
                                   uint32_t address, uint8_t dir, uint32_t seq_counter,
                                   uint8_t *enc_buffer)
{
    uint16_t i;
    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;
    int ret = 0;
    uint8_t a_block[16] = {};
    uint8_t s_block[16] = {};

    mbedtls_aes_init(&aes_ctx);
    ret = mbedtls_aes_setkey_enc(&aes_ctx, key, key_length);
    if (0 != ret) {
        goto exit;
    }

    a_block[0] = 0x01;
    a_block[5] = dir;

    a_block[6] = (address) & 0xFF;
    a_block[7] = (address >> 8) & 0xFF;
    a_block[8] = (address >> 16) & 0xFF;
    a_block[9] = (address >> 24) & 0xFF;

    a_block[10] = (seq_counter) & 0xFF;
    a_block[11] = (seq_counter >> 8) & 0xFF;
    a_block[12] = (seq_counter >> 16) & 0xFF;
    a_block[13] = (seq_counter >> 24) & 0xFF;

    while (size >= 16) {
        a_block[15] = ((ctr) & 0xFF);
        ctr++;
        ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, a_block,
                                    s_block);
        if (0 != ret) {
            goto exit;
        }

        for (i = 0; i < 16; i++) {
            enc_buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ s_block[i];
        }
        size -= 16;
        bufferIndex += 16;
    }

    if (size > 0) {
        a_block[15] = ((ctr) & 0xFF);
        ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, a_block,
                                    s_block);
        if (0 != ret) {
            goto exit;
        }

        for (i = 0; i < size; i++) {
            enc_buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ s_block[i];
        }
    }

exit:
    mbedtls_aes_free(&aes_ctx);
    return ret;
}


int encrypt_payload_hardware(const uint8_t *buffer, uint16_t size,
                                   const uint8_t *key, const uint32_t key_length,
                                   uint32_t address, uint8_t dir, uint32_t seq_counter,
                                   uint8_t *enc_buffer)
{
    uint16_t i;
    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;
    int ret = 0;
    uint8_t a_block[16] = {};
    uint8_t s_block[16] = {};

    unsigned char key_schedule[44 * 4];//128bit aes is used so this is unchanged  
    ///
    cau_aes_set_key(key, key_length, key_schedule);//sets the key
    ///
    a_block[0] = 0x01;
    a_block[5] = dir;

    a_block[6] = (address) & 0xFF;
    a_block[7] = (address >> 8) & 0xFF;
    a_block[8] = (address >> 16) & 0xFF;
    a_block[9] = (address >> 24) & 0xFF;

    a_block[10] = (seq_counter) & 0xFF;
    a_block[11] = (seq_counter >> 8) & 0xFF;
    a_block[12] = (seq_counter >> 16) & 0xFF;
    a_block[13] = (seq_counter >> 24) & 0xFF;

    while (size >= 16) {
        a_block[15] = ((ctr) & 0xFF);
        ctr++;
        cau_aes_encrypt(a_block, key_schedule, 10, s_block);
        for (i = 0; i < 16; i++) {
            enc_buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ s_block[i];
        }
        size -= 16;
        bufferIndex += 16;
    }

    if (size > 0) {
        a_block[15] = ((ctr) & 0xFF);
        cau_aes_encrypt(a_block, key_schedule, 10, s_block);
        for (i = 0; i < size; i++) {
            enc_buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ s_block[i];
    pc.printf("\r\n hardware: ");
    for(i=0;i<sizeof(enc_buffer);i++){
    	pc.printf(" %d : %d ", i , enc_buffer[i]);
	}
        }

    }

exit:
    //mbedtls_aes_free(&aes_ctx);
    return ret;
}

uint8_t pos = 0;
unsigned char key[16]="FFFFFFFFFFFFFFF";
uint8_t keysize=128;
uint32_t frame;
uint32_t framecount=1;
uint8_t output[64];
uint16_t bufsize;
uint8_t buffer[128];

void buildTestData(){
    //3 keys-000000000000000-777777777777777-FFFFFFFFFFFFFFF //just choose keys from this list 
    //fill buffer size below with values 123456789ABCDEF
    //16,32,64,128,196
    if(pos>14){
        return;
    }
    uint16_t testVal[15] = {1,2,3,4,5,6,7,8,9,0xA,0xB,0xC,0xD,0xE,0xF};
    bufsize = sizeof(buffer);
    for(int i = 0; i < bufsize; i++){
        buffer[i] = testVal[pos];
    }   
    pc.printf("Buffer value : %d\n",buffer[1]);
    //pc.printf("counter  : %d\n", pos);
    //pc.printf("bufsize  : %d\n", bufsize);
    pos++;
}

int main(void) {
  uint32_t timeElapsed = 0;
  uint32_t minTime = 0xFFFFFFFF;
  uint32_t meanTime = 0;
  uint32_t maxTime = 0;
  uint64_t totalTime = 0;

  counterInit();
  while (true) {
    /* Turn off the red LED */
    red = 1;

    /* Measure the time taken to start and stop the counter
     * when there is no other code to execute
     */ 
    buildTestData();
    counterStart();
    timeElapsed = counterStop();
    //pc.printf("\nNOP  : %020lu\n", timeElapsed);

    /* Reset all statistics and display the values to
     * ensure that they have been reset
     */
    minTime = 0xFFFFFFFF;
    meanTime = 0;
    maxTime = 0;
    totalTime = 0;
    //displayStats(minTime, meanTime, maxTime, totalTime);

    /* Repeatedly measure the execution time of the code in
     * which we are interested - this is the 'Software under test'
     */
    for (int i = N_ITERATIONS; i > 0; i-=1) {
      counterStart();
      /********* Software under test *********************///Use either: encrypt_payload_hardware   OR   encrypt_payload_hardware
       encrypt_payload_hardware((uint8_t *) buffer,bufsize,key,keysize,frame,0,framecount,output);
      /********* End Software under test ****************/
      timeElapsed = counterStop();
      if (timeElapsed < minTime) {
        minTime = timeElapsed;
      }
      if (timeElapsed > maxTime) {
        maxTime = timeElapsed;
      }
      totalTime += timeElapsed;
    }

    /* Display the statistics that have been captured */
    meanTime = totalTime / N_ITERATIONS;
    displayStats(minTime, meanTime, maxTime, totalTime);

    //Toggle the red LED while waiting for user input 
    while(pos>14){
        wait(0.5);
    }
  }
}
//data buffer
//size of data buffer
//aes key to be used
//length of key in bits
//frame address
//frame direction 0:uplink,1:downlink
//frame seq counter
//output buffer


void displayStats(uint32_t minTime,
                  uint32_t meanTime,
                  uint32_t maxTime,
                  uint64_t totalTime) {
  pc.printf("Min  : %020lu\r\n", minTime);
  pc.printf("Mean : %020lu\r\n", meanTime);
  pc.printf("Max  : %020lu\r\n", maxTime);
  pc.printf("Tot  : %020llu\r\n", totalTime);
  //pc.printf("######\n");
}

