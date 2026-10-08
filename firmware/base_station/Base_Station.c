/*
 * Base_Station.c
 *
 *  Created on: Jul 1, 2017
 *      Author: ga83hin
 */

/*
 * Base_Station.c
 *
 *  Created on: Jul 1, 2017
 *      Author: ga83hil
 */

// Contiki-specific includes:
#include "contiki.h"
#include "dev/uart.h"
#include "dev/leds.h"          // Use LEDs.
#include "dev/serial-line.h"
#include "net/rime/rime.h"     // Establish connections.
#include "net/netstack.h"      // Wireless-stack definitions
#include "core/net/linkaddr.h"
#include "net/rime/unicast.h"
#include "net/rime/unicast.h"


// Standard C includes:
#include <stdio.h>			// For printf.
#include <math.h>
#include <stdint.h>

// declaration and definitions of variables
// for key related

static uint32_t k1,k2,k3,k4;
static uint32_t q,q1,q2,q3,ya1,ya2,ya3,ya4;
static uint32_t alpha;
static uint32_t xa = 23;

// for encryption function

static uint32_t si = 0;
static uint32_t delta = 0x9E3779B9;
static uint32_t fun = 0;
static uint32_t r1i = 0;
static uint32_t ri;
static uint32_t li;

static int flag_for_key;

typedef struct{
	uint32_t key1;
	uint32_t key2;
	uint32_t key3;
	uint32_t key4;

}send_key;

typedef struct{
	uint32_t left_data;
	uint32_t right_data;

}send_data;


static send_key trans_key;
static send_key recv_key;
static send_data decryp_data;


// can take input as a a flag which checks the value to generate key or shared key as input
// extract key;
void keyextract(send_key recv_key)
	  {
		  unsigned long keytemp;



		  // key deriving
	    //on certain condition set the flag
	     keytemp = powf(recv_key.key1,xa);
	     k1 = keytemp % q;

	     keytemp = powf(recv_key.key2,xa);
	     k2 = keytemp % q1;

	     keytemp = powf(recv_key.key3,xa);
	     k3 = keytemp % q2;

	     keytemp = powf(recv_key.key4,xa);
	     k4 = keytemp % q3;


	     printf("\n key : k1=%lu k2=%lu  k3=%lu  k4=%lu  \n",k1,k2,k3,k4);

	  }


static send_key keysend()
	  {
	   //  keys

	    static send_key return_key;

		unsigned long temp;
	    q=941083981;
	    alpha = 2;

		temp = (powf(alpha,xa));
		ya1 = temp  % q;
		return_key.key1=ya1;

		q1=961748941;
		alpha = 2;
		temp = (powf(alpha,xa));
		ya2  = temp  % q1;
		return_key.key2=ya2;

		q2=492876847;
		alpha = 3;
		temp = (powf(alpha,xa));
		ya3  = temp  % q2;
		return_key.key3=ya3;

		q3= 982451653;
		alpha = 3;
		temp = (powf(alpha,xa));
		ya4  = temp  % q3;
		return_key.key4=ya4;

	     printf("\nya1 = %lu, ya2 = %lu, ya3= %lu, ya4 = %lu\n ",ya1,ya2,ya3,ya4);
	     return return_key;
	  }


// decryption function

void decrypt()
{
uint32_t templ,tempr,temp3,templsh;
uint32_t temp2,tempd,temp0;
uint16_t i,j;
uint32_t di = 0;
printf("\n decryption function \n\n");
printf("Cipher text : %lx%lx \n ",li,ri);
for(i=32;i>0;i--)
	{
		di = ((i-1)/2)*delta;
		// key generation
		if(i%2==1)
			temp2 = di&3;
		else
			{ tempd = di;

			for(j=0; j<11; j++)
				{
				temp3 = tempd & 1;
				tempd = tempd >> 1;

				if(temp3)
					tempd = tempd ^ 0x80000000;
				else
					tempd = tempd ^ 0x00000000;
				}
				temp2 = tempd & 3;
			}
		switch(temp2)
			{
			case 0x00: si=k1;break;
			case 0x01: si=k2;break;
			case 0x02: si=k3;break;
			case 0x03: si=k4;break;
			default:break;
			}

		temp0 = li;
		for(j=0;j<5;j++)
			{
			temp3 = temp0 & 1;
			temp0 = temp0 >> 1;

			if(temp3)
				temp0 = temp0 ^ 0x80000000;
			else
				temp0 = temp0 ^ 0x00000000;
			}
		templsh= li;
		r1i = (templsh << 4) ^ temp0;
		fun = ((r1i + di) ^ li) + (di ^ si);

		templ = li;
		tempr = ri ^ fun;
		li = tempr;
		ri = templ;
	}
printf("Plain text: %lx / %lx \n ",li,ri);

}

static struct unicast_conn uc;


static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from){

	printf("Broadcast Routing message received from 0x%x%x: \n\r [RSSI %d]\n\r",
				from->u8[0], from->u8[1],
				packetbuf_attr(PACKETBUF_ATTR_RSSI));


			if(packetbuf_attr(PACKETBUF_ATTR_RSSI)==8)
		    {

				uint8_t length= packetbuf_copyto(&recv_key);
				printf("Received key");
				keyextract(recv_key);
				flag_for_key=1;

		    }


			if(packetbuf_attr(PACKETBUF_ATTR_RSSI)==10)
			{
				uint8_t length= packetbuf_copyto(&decryp_data);
				printf("Received data");
				li=decryp_data.left_data;
				ri=decryp_data.right_data;
			    decrypt();
			    printf("Received key");

		    }

}
static const struct unicast_callbacks unicast_callbacks = {unicast_recv};
//--------------------- PROCESS CONTROL BLOCK ---------------------
PROCESS(key_data, "Lesson 1: Debugging");
AUTOSTART_PROCESSES(&key_data);

//------------------------ PROCESS' THREAD ------------------------
PROCESS_THREAD(key_data, ev, data){
	    PROCESS_EXITHANDLER(unicast_close(&uc);)
		PROCESS_BEGIN();
		static uint8_t timer_interval = 5;	// In seconds
		static struct etimer et;
		NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,18);
		unicast_open(&uc, 146, &unicast_callbacks);
        static linkaddr_t to2;
        while(1)
        {
        etimer_set(&et, CLOCK_SECOND * timer_interval);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        etimer_reset(&et);
        if(flag_for_key==1)
        {

                to2.u8[0] = 0x56;
               	to2.u8[1] = 0x16;
               	trans_key = keysend();
        	    packetbuf_copyfrom(&trans_key,sizeof(trans_key));
        	    unicast_send(&uc,&to2);
        	    printf("Unicast message sent to receiver ");
        	    flag_for_key=0;

        }

        }


       	PROCESS_END();

}


