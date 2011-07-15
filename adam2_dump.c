/* File: appmain.c
 *
 * Created: Sun Jan 15 18:24:53 2005
 *
 */
#include <stdio.h>
#include <errno.h>

#define ADAM2_MAGIC  0xfeedfa42

#define MAGIC_NUMBER 0xc453de23


unsigned long app_read_data (FILE *fp, long offset)
{
   unsigned char buf[4];

   fseek(fp, offset, SEEK_SET);

   fread(buf, 4, 1, fp);

   return *((unsigned long*)buf);  /* Indianity hazard ! */
};


unsigned long app_read_magic (FILE *fp)
{
   return app_read_data(fp, (long) 0);
};


unsigned long app_read_length (FILE *fp)
{
   return app_read_data(fp, (long) 4);
};

unsigned long app_read_address (FILE *fp)
{
   return app_read_data(fp, (long) 8);
};

int  cs_is_tagged(FILE *fp)
{
   unsigned char buf[8];

   fseek(fp, -8, SEEK_END);
   fread(buf, 8, 1, fp);
   if(*(unsigned long*)buf == MAGIC_NUMBER)
      return 1;
   return 0;
};


/* --------------------------------------------


  --------------------------------------------- */

int main(int argc, char **argv)
{
   FILE *fp;
   unsigned long magic = 0;
   unsigned long address = 0;
   unsigned long length = 0;
   unsigned long checksum1 = 0;
   unsigned long pad = 0;
   unsigned long entry = 0;
   unsigned long sum = 0;

   unsigned char buf[4] = "\x0\x0\x0\x0";

   long end_of_code_pos = 0;

   long entry_point_pos = 0;
   unsigned char code_buf[128];
   int code_pos;
   unsigned long kernel_address = 0;
   long kernel_pos = 0;
   long kernel_length = 0;

   if(argc != 2)
   {
      printf("Usage: adam2_dump filename\n\n");
      return 1;
   }

   fp = fopen(argv[1], "r");

/* */

   printf("\n\tADAM2 application Header:\n\n");

   magic=app_read_magic(fp);
   printf("ADAM2 Magic= 0x%08x",magic);

   if (magic == ADAM2_MAGIC) {
      printf (" - GOOD!\n");
   } else {
      printf (" - BAD!\n");
   };

   printf("\n\tADAM2 application Record #1 Header:\n\n");

   length=app_read_length(fp);
   printf("LENGTH   = 0x%08x (%d)\n",length,length);

   address=app_read_address(fp);
   printf("ADDRESS  = 0x%08x\n",address);

   end_of_code_pos=length+(3*4); /* Skip header+code */

   if (length>0) {
      printf (".......... DATA\n");
   };

   checksum1=app_read_data(fp, end_of_code_pos);
   printf("CHECKSUM = 0x%08x\n",checksum1);

   printf("\n\tADAM2 application Record #2 Header:");
#if defined DEBUG  
   printf(" at 0x%08x",end_of_code_pos);
#endif
   printf ("\n\n");

   pad=app_read_data(fp, end_of_code_pos+4);
   printf("LENGTH   = 0x%08x\n",pad);

   entry=app_read_data(fp, end_of_code_pos+8);
   printf("ADDRSESS = 0x%08x (entry point)\n",entry);

   sum=app_read_data(fp, end_of_code_pos+0xc);
   printf("CHECKSUM = 0x%08x\n",sum);

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   Some MIPS black magic there... */

   entry_point_pos = entry - address + (3*4); /* skip header */

#if defined DEBUG
   printf ("D:Code enrty point position = 0x%08x\n",entry_point_pos);
#endif

   fseek(fp, entry_point_pos, SEEK_SET);

   fread(code_buf, 128, 1, fp);

   code_pos=30;
/* try to find 0x9402 */

   while (code_pos<120) {
      if (code_buf[code_pos]==0x02 && code_buf[code_pos+1]==0x94)
         { break;
         };
      code_pos++;
   };

#if defined DEBUG
   printf("D:Found 0x9402 at %x\n",code_pos);
#endif

   if (code_buf[code_pos+8]==0x00 &&
       code_buf[code_pos+9]==0x00 && /*   0x0000 .half */
       code_buf[code_pos+6]==0x06 &&
       code_buf[code_pos+7]==0x0d )  /*   _jal xxxxxx_ */
   {
       buf[2] = code_buf[code_pos-8];
       buf[3] = code_buf[code_pos-7];
       buf[0] = code_buf[code_pos-4];
       buf[1] = code_buf[code_pos-3];
       if (buf[1]>0x7f) { buf[2]--;}; /*   correct for _addui_ */
   };

   kernel_address=*((unsigned long*)buf);  /* Indianity hazard ! */
#if DEBUG
   printf("D:Kernel image address = 0x%08x\n", kernel_address);
#endif

   kernel_pos = kernel_address - address + (3*4); /* skip header */

   if (kernel_address) {
      kernel_length=end_of_code_pos - kernel_pos;
      printf ("\nCompressed kernel image found!\n");
      printf ("Offset = 0x%x (%d), Length = 0x%x (%d)\n", kernel_pos, kernel_pos, kernel_length, kernel_length);

      fseek(fp, kernel_pos, SEEK_SET);

      fread(code_buf, 128, 1, fp);
      
      if (code_buf[0]=='7' && code_buf[1]=='z') {
         printf ("LZMA z7 compressed image.\n" );
      } else {
         if (code_buf[10]=='z' && code_buf[11]=='i' && code_buf[12]=='m'){
            printf ("Zlib compressed image.\n");
         } else {
            printf ("Unknown image format.\n");
         }
      }
   } else {
      printf("\nKernel image not found!\n");
   };

   if (cs_is_tagged(fp))
   {
      printf ("File is signed by TI_chksum.\n");
   } else {
      printf ("File isn't signed by TI_chksum.\n");
   };


   fclose(fp);

   return 0;
};

