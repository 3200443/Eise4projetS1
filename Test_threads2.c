 	 	#define _GNU_SOURCE
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <math.h>
//#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>

////////////////////SPI//////////////////
#define	TRUE	            (1==1)
#define	FALSE	            (!TRUE)
#define CHAN_CONFIG_SINGLE  8
////////////////////PROG/////////////////
#define TAILLE_INPUT 500
#define Fe  4000

//pthread_cond_t condition = PTHREAD_COND_INITIALIZER; /* Cr�ation de la condition */
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Cr�ation du mutex */

void* fctThreadMCP (void* arg);

static int myFd ;

char *usage = "Usage: mcp3008 all|analogChannel[1-8] [-l] [-ce1] [-d]";

///////////////////////////////////MCP_FONCTIONS/////////////////////////////////////////

void loadSpiDriver()
{
	if (system("gpio load spi") == -1)
	{
		fprintf (stderr, "Can't load the SPI driver: %s\n", strerror (errno)) ;
		exit (EXIT_FAILURE) ;
	}
}
void spiSetup (int spiChannel)
{
	if ((myFd = wiringPiSPISetup (spiChannel, 1000000)) < 0)
	{
		fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
		exit (EXIT_FAILURE) ;
	}
}

int myAnalogRead(int spiChannel,int channelConfig,int analogChannel)
{
	if(analogChannel<0 || analogChannel>7)
	{
		return -1;
	}
    unsigned char buffer[3] = {1}; // start bit
    buffer[1] = (channelConfig+
    analogChannel) << 4;
    wiringPiSPIDataRW(spiChannel, buffer, 3);
    return ( (buffer[1] & 3 ) << 8 ) + buffer[2]; // get last 10 bits
}
///////////////////////////////////LISTE_FONCTIONS///////////////////////////////////////
///
/////////////MCP/////////////////
///
	int loadSpi=FALSE;
	int analogChannel=1;
	int spiChannel=0;
	int channelConfig=CHAN_CONFIG_SINGLE;
///
/////////////BUFF///////////////
///
   double input[TAILLE_INPUT];
   gsl_fft_real_wavetable * tab;
   gsl_fft_real_workspace * tamp;
   double _F = 0; //la frequence a jouer
///
///////////////////////////////////MAIN_FONCTIONS////////////////////////////////////////
int main (int argc, char *argv [])
{
   //////////////Thread//////////////
   pthread_t ThreadMCP;
	//////////////RuntimeVariables////
		int i = 0;
	/////////INNITIALISATION//////////
	///SPI
		wiringPiSetup () ;
		spiSetup(spiChannel);
	///Buffer
      for(i = 0; i < TAILLE_INPUT; i++)
      {
         input[i] = 0;
      }
   ///FFT-Preparation
      tab   = gsl_fft_real_wavetable_alloc (TAILLE_INPUT/2);
      tamp  = gsl_fft_real_workspace_alloc (TAILLE_INPUT/2);
	///Thread d'aquisition des donnees
    pthread_create(&ThreadMCP, NULL, fctThreadMCP, (void*)NULL);
    pthread_join(ThreadMCP, NULL); /* Attente de la fin des threads */
	close (myFd) ;
	return 0;
}


void* fctThreadMCP (void* arg)
{
	int cont = 0,i=0,j;
	long clk_tck = CLOCKS_PER_SEC;
	clock_t t1;

	while(1) /* Boucle infinie */
	//for(cont =0 ; cont < 30; cont ++)
	{
      t1= clock();
      input[i] = (myAnalogRead(spiChannel,channelConfig,analogChannel-1)-512)*(5.0/1023.0);
      if(i==TAILLE_INPUT/2-1)
      { // TODO: A METTRE DANS UN THREAD!!!!!
         gsl_fft_real_transform (input, 1, TAILLE_INPUT/2,tab,tamp); //TODO: verifier le 2e argument (stride)
         int temp = 0;
         for(j = 0; j < TAILLE_INPUT/2-1; j++)
         {
            if(input[j] < input[temp] && input[j] != _F)
               temp = j;
         }
         _F = temp * Fe / (TAILLE_INPUT/2) ;
      }else if(i == TAILLE_INPUT -1)
      {
         gsl_fft_real_transform (input+TAILLE_INPUT/2, 1, TAILLE_INPUT/2,tab,tamp); //TODO: verifier le 2e argument (stride)
         int temp = 0;
         for(j = TAILLE_INPUT/2; j < TAILLE_INPUT-1; j++)
         {
            if(input[j] < input[temp] && input[j] != _F)
               temp = j;
         }
         _F = temp * Fe / (TAILLE_INPUT/2) ;
      }
      i++;
      printf("_F:%e\n",_F);
	   while((double)((clock()-t1)/(double)clk_tck)<0.000249){}
	   //printf("t:%f\n",(double)(clock()-t1)/clk_tck);
	   //usleep(500-((t2-t1)/clk_tck*1000000));
	}

	pthread_exit(NULL); /* Fin du thread */
}
