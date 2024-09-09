#include  "../header/flash.h"    // private library - flash layer
#include  "../header/halGPIO.h"    // private library - halGPIO layer
#include  "string.h"

//-----------------------------------------------------------------------------
//           FLASH driver
//-----------------------------------------------------------------------------
Files file;


void data_script(void)
{
    file.file_size[file.num_of_files - 1] = strlen(file_content) - 1;
}

void write_data(void)
{
    char *Flash_ptr_write ;                          // Flash pointer
    unsigned int k;
    Flash_ptr_write = file.file_ptr[file.num_of_files - 1];      // pointer to the file_ptr to the file index
    FCTL1 = FWKEY + ERASE;                    // Set Erase bit
    FCTL3 = FWKEY;                            // Clear Lock bit
   *Flash_ptr_write = 0;                   // Dummy write to erase Flash segment
    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
    // this for loop save the data in the flash, the the content in the file_content char by char and save it in the flash with the using of pointer
    for (k = 0; k < file.file_size[file.num_of_files - 1]; k++)
    {
        if (file_content[k] == 0x0A || file_content[k] == 0x0D ){
            continue;
        }
        *Flash_ptr_write++ = file_content[k];            // Write value to flash
    }
    memset(stringFromPC,0,strlen(stringFromPC)); //clear file_content
    FCTL1 = FWKEY;                            // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}


