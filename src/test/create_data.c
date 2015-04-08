/*
 *  This extra small demo sends a random samples to your speakers.
 */
#include <stdlib.h>
#include <stdio.h>
#define MAX_FRAMES 5*44100
#define MAX_SIZE 1024*16

int main(int argc,char *argv[])
{
        unsigned char *buffer;				/* some random data */
        FILE *stream;
        int ret;
        stream = fopen(argv[1],"w+");
        printf("stdout to pcm : filename = %s\n",argv[1]);
        if ( !stream ) {
            printf("open file %s error\n",argv[1]);
        }else{
            int i;
            int j;
            int nums;
            int count_frames;
            int frames;
            frames=0;
            count_frames=0;
            nums = 4410;
            int content = 0x0;
            int flag = 0;
            buffer = (unsigned char *)malloc(MAX_SIZE);
            while (1) {
                //data num_frames
                printf("j=%d,nums=%d,content=%d\n",j,nums,content);
                for (i = 0; i < sizeof(buffer); i++){
                     printf("i %% 32 * %d = %d\n",nums,i%(32*nums));
                     if(i%32==0){
                         frames++;
                         count_frames++;
                     }
                     if(count_frames>=nums){
                         if(flag==1){ 
                             nums = nums + 500;
                             flag = 0;
                         }else{
          		     //flag = 1;
                         }
                         printf("frames=%d,nums=%d,content=%d\n",frames,nums,content);
                         content=(content==0xff?0x0:0xff);
                         count_frames=0;
                     }
                     buffer[i] = content;
                }
                printf("======frames=%d,nums=%d,content=%d\n",frames,nums,content);
                if(!fwrite(buffer,sizeof(buffer),1,stream)){
                    printf("muisc play end\n");
                    break;
                }   
                if(frames>MAX_FRAMES){
                      break;
                } 
            }
            free(buffer);
        }
        fclose(stream);
        return 0;
}

