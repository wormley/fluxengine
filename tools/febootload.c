
 
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cybtldr_parse.h"

 #define VERSION "0.1.0"
 #define VENDOR_ID 0x04b4
 #define PRODUCT_ID 0xb71d
 #define INTERFACE 0
 #define READ_BUF_SIZE 8192
#define MAX_DATA_LENGTH (64-7-9)
 const static int reqIntLen=64;
 const static int endpoint_Int_in=0x82; /* endpoint 0x81 address for IN */
 const static int endpoint_Int_out=0x01; /* endpoint 1 address for OUT */
 
 const static int timeout=5000; /* timeout in ms */
 
 void bad(const char *why) {
   fprintf(stderr,"Fatal error> %s\n",why);
   exit(17);
 }
 
 libusb_context *ctx = NULL;
 libusb_device_handle *find_lvr_hid();
 
 libusb_device_handle* setup_liblibusb_access() {
     libusb_device_handle *lvr_hid;
     int retval;
     int i;
     libusb_init(&ctx);
     

#if LIBUSB_API_VERSION >= 0x01000106
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 255);
#else
     libusb_set_debug(ctx, 255);
#endif
     struct libusb_config_descriptor *conf;

             
     if(!(lvr_hid = find_lvr_hid())) {
     printf("Couldn't find the USB device, Exiting\n");
     return NULL;
   }
 
 #ifdef LINUX
	if ( ! libusb_get_active_config_descriptor(libusb_get_device(lvr_hid), &conf) ) 
        	for(i = 0; i < conf->bNumInterfaces; i++) {
                printf("Detach %d\n",i);
		if ( (retval= libusb_detach_kernel_driver(lvr_hid, i)) ) {
                if (retval != LIBUSB_ERROR_NO_DEVICE && retval != LIBUSB_ERROR_NOT_FOUND) { 
                        printf("Detach failed with %d\n",retval);
			libusb_close(lvr_hid);
			libusb_free_config_descriptor(conf);
			return NULL;
                    };
		}
		printf(" - detached interface %u\n", i);
	}

 
 #endif
 
 
   retval=libusb_set_configuration(lvr_hid, 1);
   if ( retval < 0) {
     printf("Could not set configuration 1 : %d\n", retval);
     return NULL;
   }
   retval = libusb_claim_interface(lvr_hid, INTERFACE);
     if (retval == LIBUSB_ERROR_BUSY ) {
        libusb_detach_kernel_driver(lvr_hid, INTERFACE);
        retval = libusb_claim_interface(lvr_hid, INTERFACE); 
     }
     if (retval < 0 ) {
     printf("Could not claim interface: %d\n", retval);
     return NULL;
   }
 
   return lvr_hid;
 }
 
 libusb_device_handle *find_lvr_hid() 
 {
         libusb_device_handle *handle;
         if (!(handle=libusb_open_device_with_vid_pid(ctx,VENDOR_ID,PRODUCT_ID))) {
           printf("Could not open USB device\n");
           return NULL;
         };
 
         return handle;
 }

  unsigned int do_checksum(unsigned char *d, int l) {
    unsigned int checksum=0;
    int i;
    for (i=0 ; i<l; i++ ){
      checksum+=d[i];
    };
    return(~checksum + 1);

  }

  int send_interrupt_transfer(libusb_device_handle *dev,unsigned char cmd,unsigned char *data , int len, unsigned char *response)
  {
    int r;
    int count;
    uint8_t request[reqIntLen] ;
    uint16_t csum;
    int reqlen=0;
    request[0]=0x01;
    request[1]=cmd;
    request[2]=len & 0xff;
    request[3]= len >> 8 ;
    reqlen=4;
    if (len>0 ) {
	memcpy(&request[reqlen],data,len);
	reqlen+=len;
    };
    csum=do_checksum(request,reqlen);
    request[reqlen]=csum & 0xff;
    request[reqlen+1] =csum >> 8; 
    request[reqlen+2] = 0x17;
    reqlen += 3; 
    r = libusb_interrupt_transfer(dev, endpoint_Int_out, request, reqlen, &count,timeout);
    printf("Count: %d\n",count);
    if( r < 0 )
    {
     perror("USB interrupt write"); bad("USB write failed"); 
    }
    if (! response) {
    	return(-1);
    };
    r = libusb_interrupt_transfer(dev, endpoint_Int_in, response, reqIntLen, &count, timeout);
    if( r <0 )
    {
     perror("USB interrupt read"); bad("USB read failed"); 
    }
    printf("Read: %d %d\n",r,count);
    reqlen = 4 + response[2] + (response[3]<<8);
    csum = do_checksum(response,reqlen);
    if ( (csum & 0xff) != response[reqlen] || (csum >>8) != response[reqlen+1] ) {
      printf("Invalid checksum %d %02X %02X %02X %02X \n",reqlen,response[0],response[1],response[2],response[3]);
      return(-1);
    }


    return(count);
  }

int program_row(libusb_device_handle *dev,uint8_t array_id, uint16_t row_number,unsigned char *data , int len, unsigned char *response) 
  {
//program_row(lvr_hid,0x37,array_id,row_number,&row_buf[full_packets],packet_remainder,response)
//send_interrupt_transfer(libusb_device_handle *dev,unsigned char cmd,unsigned char *data , int len, unsigned char *response) 
  unsigned char reqbuf[reqIntLen];
  int reqlen;
  reqbuf[0]=array_id;
  reqbuf[1]=row_number & 0xff;
  reqbuf[2]=row_number >>8 ;
  reqlen=3;
  if (len >0 ) memcpy(&reqbuf[reqlen],data,len);

  reqlen += len;
  return(send_interrupt_transfer(dev,0x39,reqbuf,reqlen,response));
  }



 
 int main( int argc, char **argv)
 {
   int rv,i;
   uint32_t jtag_id;
   unsigned char response[reqIntLen];
   unsigned char read_buf[READ_BUF_SIZE];
   unsigned char row_buf[READ_BUF_SIZE];
   uint32_t siliconId;
   uint8_t siliconRev;
   uint8_t chksum;
   uint8_t arrayId;
   uint16_t rowNum;
   uint16_t rowSize;
   uint32_t readSize;
   int full_packets;
   int packet_remainder;
   libusb_device_handle *lvr_hid;
   if ((lvr_hid = setup_liblibusb_access()) == NULL) {
     exit(1);
   } 
  /* Enter the bootloader */
   rv=send_interrupt_transfer(lvr_hid,0x38,NULL,0,response);
   if (rv<0 || response[1] != 0x00) {
     bad("Failed to enter bootloader mode");
   }
  jtag_id = response[4];
  jtag_id |= response[5] << 8;
  jtag_id |= response[6] << 16;
  jtag_id |= response[7] << 24;
  printf("JTAG ID: %ud\n",jtag_id);

  if (argc > 1) {
    rv = CyBtldr_OpenDataFile(argv[1]);
    if (rv != CYRET_SUCCESS)  bad("Failed to open input file");
    rv = CyBtldr_ReadLine(&readSize,read_buf);
    if (rv != CYRET_SUCCESS) bad("Failed to read from input file");
    rv = CyBtldr_ParseHeader(readSize,read_buf,&siliconId,&siliconRev,&chksum);
    if (rv != CYRET_SUCCESS) bad("Failed to parse header from input file");
    if (siliconId != jtag_id) bad("Jtag ID does not match SiliconID from input file");
    /* Do the flash */
    while ((rv=CyBtldr_ReadLine(&readSize,read_buf)) == CYRET_SUCCESS) {
      rv=CyBtldr_ParseRowData(readSize,read_buf,&arrayId,&rowNum,row_buf,&rowSize,&chksum);
      if (rowSize> MAX_DATA_LENGTH) {
          full_packets = rowSize/MAX_DATA_LENGTH;
          packet_remainder = rowSize%MAX_DATA_LENGTH;
          for (i=0 ; i<full_packets;  i++) {
            rv=send_interrupt_transfer(lvr_hid,0x37,&row_buf[i*MAX_DATA_LENGTH],MAX_DATA_LENGTH,response);
            if (rv <0 || response[1] != 0x00 ) bad("Failed to send row data");
          };
          rv=program_row(lvr_hid,arrayId,rowNum,&row_buf[full_packets],packet_remainder,response);
            if (rv <0 || response[1] != 0x00 ) bad("Failed to program row");

      } else {
          rv=program_row(lvr_hid,arrayId,rowNum,row_buf,rowSize,response);
            if (rv <0 || response[1] != 0x00 ) bad("Failed to send program row(2)");

      }

    }


  }


   rv=send_interrupt_transfer(lvr_hid,0x3b,NULL,0,NULL);
   libusb_close(lvr_hid);
 
   return 0;
 }
 
 
