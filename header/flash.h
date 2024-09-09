#ifndef HEADER_FLASH_H_
#define HEADER_FLASH_H_

#include  <msp430g2553.h>          // MSP430x2xx


extern void data_script(void);
extern void write_data(void);
typedef struct Files{
    short num_of_files; // Everytime indicate the specific file im working on
    char file_name[11];
    int* file_ptr[3];
    int file_size[3];
}Files;

extern Files file;
#endif /* HEADER_FLASH_H_ */
