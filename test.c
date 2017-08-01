#include <stdio.h>

unsigned char test[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
unsigned char len = 16;  

//逐位计算：内存占用少，但比较耗时
unsigned int bit_crc()
{
   unsigned long temp = 0;  
   unsigned int crc;  
   unsigned char i;  
   unsigned char *ptr = test;  
   while( len-- )
   {  
       for(i = 0x80; i != 0; i = i >> 1) {  
           temp = temp << 1;  
           if((temp & 0x10000) != 0)  
               temp = temp ^ 0x11021;  

           if((*ptr & i) != 0)   
               temp = temp ^ (0x10000 ^ 0x11021);  
       }  
       ptr++;  
   }  
   crc = temp;  
   printf("0x%x\n",crc);  
}

unsigned int easy_crc()
{
    unsigned int crc = 0;  
    unsigned char i;  
    unsigned char *ptr = test;  
    while( len--  ) {  
        for(i = 0x80; i != 0; i = i >> 1) {  
            if((crc & 0x8000) != 0) {  
                crc = crc << 1;  
                crc = crc ^ 0x1021;  
            }  
            else {  
                crc = crc << 1;  
            }  
            if((*ptr & i) != 0) {  
                crc = crc ^ 0x1021;   
            }  
        }  
        ptr++;  
    }  
    printf("0x%x\n",crc);  
}

unsigned short get_crc(unsigned char *buff, int len)
{
    unsigned short crc = 0;
    unsigned char R;
    int k, m;

    if( len <= 0 )
        return 0;

    for( int i = 0; i < len; i++ )
    {
        R = buff[i];
        for( int j = 0; j < 8; j++ )
        {
            if( R > 0x7f )
                k = 1;
            else
                k = 0;

            R <<= 1;
            if( crc > 0x7fff )
                m = 1;
            else
                m = 0;

            if( k + m == 1 )
                k = 1;
            else
                k = 0;
            crc <<= 1;
            if(k == 1)
                crc ^= 0x1021;
        }
    }
    return crc;
}

unsigned int byte_crc()
{
    unsigned int crc = 0;  
    unsigned char i;  
    unsigned int j;  
    for(j = 0; j < 256; j++) 
    {  
        crc = 0;  
        for(i = 0x80; i != 0; i = i >> 1) {  
            if((crc & 0x8000) != 0) {  
                crc = crc << 1;  
                crc = crc ^ 0x1021;  
            }  
            else {  
                crc = crc << 1;  
            }  
            if((j & i) != 0) {  
                crc = crc ^ 0x1021;  
            }  
        }  
        printf("0x");  
        if(crc < 0x10) {  
            printf("000");  
        }  
        else if(crc < 0x100) {  
            printf("00");  
        }  
        else if(crc < 0x1000) {  
            printf("0");  
        }  
        printf("%x, ",crc);  
    }
}

int main()
{
    bit_crc();
    easy_crc();
    byte_crc();

    printf("\n%x\n", get_crc(test, 16));

    return 0;
}
