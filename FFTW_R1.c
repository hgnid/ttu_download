/*
 ============================================================================
 Name        : FFTW_R1.c
 Author      : SHANG HELONG
 Version     : V1.0
 Copyright   : 
 Description : Harmony calculation with FFTW3 in C, ANSI-style
 ============================================================================
 */

#include <fftw3.h>
#include <complex.h>
#include <math.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include "shm.h"
#include "sem.h"

#define FS 4000				//采样频率
#define N 80				//FFT点数
#define pi 3.1416

int main(int argc, char *argv[]) {
	int i;
	double *in;				//定义输入
	fftw_complex *out;		//定义输出
	fftw_plan p;			//定义plan

	key_t key;
	memory *sms = NULL;
	FFT_status harmony;
	int shm_id;
	int create_flag = 0;
	int sem_id;

	if ((key = ftok("/", 1)) < 0)
	{
	    perror("failed to get key");
	    exit(-1);
	}

	if ((sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0)
	{
	    if (errno == EEXIST)
	    {
	        if ((sem_id = semget(key, 1, 0666)) < 0)
	        {
	            perror("failed to get semaphore");
	            exit(-1);
	        }
	    }
	}

	init_sem(sem_id, 1);

	if ((shm_id = shmget(key, sizeof(memory), 0666 | IPC_CREAT | IPC_EXCL)) < 0)
	{
	    if (errno == EEXIST)
	    {
	        if ((shm_id = shmget(key, sizeof(memory), 0666)) < 0)
	        {
	            perror("failed to get shared memory segment");
	            exit(-1);
	        }
	    }
	    else
	    {
	        perror("failed to get shared memory segment");
	        exit(-1);
	    }
	}
	else
	    create_flag = 1;

	if ((sms = shmat(shm_id, NULL, 0)) == (void *)(-1))
	{
	    perror("failed to attach shared memory segment");
	    exit(-1);
	}

	in = (double*) fftw_malloc(sizeof(double) * 160);					//为输入分配内存空间
	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2+1));	//为输出分配内存空间

	p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_MEASURE);					//为FFT计算设定plan，FFTW_MEASURE代表在初始化时预先进行算法优化
	fftw_execute(p);													//通过执行空plan进行算法优化
	/*初始化完毕*/

	while(1)
	{
		//输入数组赋值
		if((in==NULL)||(out==NULL))
		{
			printf("Error: Insufficient available memory!\n");
		}
		else
		{
			for(i=0; i<N; i++)
			{
				in[i] = cos(2*pi*50*i/FS)+0.1*cos(2*pi*100*i/FS)+0.05*cos(2*pi*150*i/FS)+0.02*cos(2*pi*200*i/FS)+0.01*cos(2*pi*250*i/FS);
				//in[i] = buffer[i];
			}
		}

		//执行FFT计算，可循环调用
		fftw_execute(p);

		for(int i=0;i<22;i++)
		{
			harmony.Harmony_Amplitude[i] = 2*sqrt(out[i][0]*out[i][0]+out[i][1]*out[i][1])/N;
			harmony.Harmony_Phase[i] = atan(out[i][1]/out[i][0]);
			printf("%d次谐波幅值为%.6f，相位为%.6f；\n",i,harmony.Harmony_Amplitude[i],harmony.Harmony_Phase[i]);
		}

		sem_p(sem_id);
		memcpy(&(sms->fft_sta), &harmony, sizeof(harmony));

		sem_v(sem_id);

		sleep(5);
	}
/*
	printf("Input array:\n");
	for(i=0; i<N; i++)
	{
		printf("in[%d] = %f\n", i, in[i]);
	}

	printf("\nOutput array:\n");
	for(i=0; i<N/2+1; i++)
	{
		printf("out[%d] = %f, %f\n", i, out[i][0], out[i][1]);
	}

	printf("\nAmplitude array:\n");
	for(i=0; i<21; i++)
	{
		amp[i] = 2*sqrt(out[i][0]*out[i][0]+out[i][1]*out[i][1])/N;
		printf("amplitude[%d] = %f\n", i, amp[i]);
	}

	printf("\nPhase array:\n");
	for(i=0; i<21; i++)
	{
		pha[i] = atan(out[i][1]/out[i][0]);
		printf("phase[%d] = %f\n", i, pha[i]);
	}
*/

      //把共享内存从当前进程中分离
    	if (shmdt(p) == -1) {
    		fprintf(stderr, "shmdt failed\n");
    		exit(EXIT_FAILURE);
    	}
    	sleep(2);


	fftw_destroy_plan(p);			//在主函数退出前摧毁plan
	fftw_free(in);					//释放输入数组内存空间
	fftw_free(out);					//释放输出数组内存空间

	return 0;
}
