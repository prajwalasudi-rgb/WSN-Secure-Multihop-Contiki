/*
 * Neighbor_Discovery.c
 *
 *  Created on: Jul 4, 2017
 *      Author: ga83hil
 */


#include "contiki.h"

#include "net/rime/rime.h"     // Establish connections.
#include "net/netstack.h"      // Wireless-stack definitions
#include "dev/leds.h"          // Use LEDs.
#include "core/net/linkaddr.h"
#include "net/rime/unicast.h"
#include "net/rime/broadcast.h"
#include "net/rime/runicast.h"
#include "net/rime/stunicast.h"
#include "core/lib/list.h"
#include "lib/memb.h"

// Standard C includes:
#include <stdio.h>      // For printf.
#include <math.h>
#define MEMBERS 10
#define MAX_NEIGHBORS 100
#define RIMEADDR_SIZE 2
#define MAX_RETRANSMISSIONS 4
 struct new_neighbor {
  struct new_neighbor *next;
  linkaddr_t addr;

};




 struct hop_info {
   uint8_t hop_count;
 };

 struct hop_element {
	 struct hop_element *next;
   linkaddr_t addr;
    uint8_t hop_count;
  };


//static process_event_t event_data_ready;
static int flag_to_send;

void print_list();
void print_list_temp();
static void send_ack_ping();
static int Compare_tables();
void rimeaddr_copy(linkaddr_t *dest, const linkaddr_t *src);
int rimeaddr_cmp(const linkaddr_t *addr1, const linkaddr_t *addr2);
void received_hop(const linkaddr_t *from);


static struct broadcast_conn bc;
static struct runicast_conn runicast;
LIST(neighbor_table);
MEMB(neighbor_mem, struct new_neighbor, MAX_NEIGHBORS);
LIST(Temp_neighbor_table);

LIST(hop_table);
MEMB(hop_mem, struct hop_element, MAX_NEIGHBORS);
/*
 * The function clears the temporary neighbor table
 */
static void deleteList_temp()
{
   /* deref head_ref to get the real head */
	struct new_neighbor* t = list_head(Temp_neighbor_table) ;
	struct new_neighbor ** head_ref = &t;
   struct new_neighbor* current = *head_ref;
   struct new_neighbor* next;

   while (current != NULL)
   {
       next = current->next;
       list_remove(Temp_neighbor_table, current);
       memb_free(&neighbor_mem, current);
       current = next;
   }

   /* deref head_ref to affect the real head back
      in the caller. */
   *head_ref = NULL;
}

static void delete_hoptable()
{
   /* deref head_ref to get the real head */
	struct hop_element* t = list_head(hop_table) ;
	struct hop_element ** head_ref = &t;
   struct hop_element* current = *head_ref;
   struct hop_element* next;

   while (current != NULL)
   {
       next = current->next;
       list_remove(Temp_neighbor_table, current);
       memb_free(&neighbor_mem, current);
       current = next;
   }

   /* deref head_ref to affect the real head back
      in the caller. */
   *head_ref = NULL;
}
/*
 * The function removes the neighbor from the table
 * because it has got no acknowledgment from the node.
 */

static void deleteList_perm()
{
   /* deref head_ref to get the real head */
	struct new_neighbor* p = list_head(neighbor_table) ;
	struct new_neighbor ** head_ref = &p;
   struct new_neighbor* current = *head_ref;
   struct new_neighbor* next;

   while (current != NULL)
   {
       next = current->next;
       list_remove(neighbor_table, current);
       memb_free(&neighbor_mem, current);
       current = next;
   }

   /* deref head_ref to affect the real head back
      in the caller. */
   *head_ref = NULL;
}

/*
 * The function updates neighbor table members
 * from temporary table , if it is different then it forwards
 * the table to neighbors.
 */

static void
update_neighbor_table_members()
{
  struct new_neighbor *e ;
  struct new_neighbor *e_new ;


  //printf("* Entry ---------------------------------------------------------------------------*\n\n\n");
  //print_list();
  //print_list_temp();

  if((list_length(neighbor_table)==0 )&& (list_length(Temp_neighbor_table)!=0))
  {
  for(e = list_head(Temp_neighbor_table); e != NULL; e = e->next)
  	{
	     e_new = memb_alloc(&neighbor_mem);
	     if(e != NULL) {
	      rimeaddr_copy(&e_new->addr,&e->addr);
	      list_add(neighbor_table, e_new);
	      //printf("add initially %x %x \n",e_new->addr.u8[0], e_new->addr.u8[1]);
	     }

  	}


  deleteList_temp();


  flag_to_send=1;
  //printf("Send my neighbor table to my neighbors %d \n",PACKETBUF_ATTR_RSSI);
  }
  else if ((list_length(neighbor_table)!=0 )&& (list_length(Temp_neighbor_table)!=0))
  {
	  if(Compare_tables()==0)
	  {

		  deleteList_perm();
		  //printf("* blahhhhhhhh ---------------------------------------------------------------------------*\n\n\n");
		  //print_list();
		  for(e = list_head(Temp_neighbor_table); e != NULL; e = e->next)
		   	{
		 	     e_new = memb_alloc(&neighbor_mem);
		 	     if(e != NULL) {
		 	      rimeaddr_copy(&e_new->addr,&e->addr);
		 	      list_add(neighbor_table, e_new);
		 	      //printf("add later %x %x \n",e_new->addr.u8[0], e_new->addr.u8[1]);
		 	     }

		   	}

		  deleteList_temp();


		  flag_to_send=1;
		  //printf("Send my neighbor table to my neighbors %d \n",PACKETBUF_ATTR_RSSI);
	  }
	  else
	  {
		  //printf("not sending my table \n");
		  deleteList_temp();
		  flag_to_send=0;

	  }
  }
  else if (list_length(Temp_neighbor_table)==0)
  {

	  deleteList_perm();
	  flag_to_send=0;
  }
  printf("* Exit ---------------------------------------------------------------------------*\n\n\n");
  print_list();
  print_list_temp();




}

/*
 * The function compares two tables and checks
 * if they are equal.
 */

 static int
Compare_tables()
{
  struct new_neighbor *e ;
  struct new_neighbor *e_new ;
  int flag =0;

  for(e = list_head(Temp_neighbor_table); e != NULL; e = e->next)
  {
	  for(e_new = list_head(neighbor_table); e_new != NULL; e_new = e_new->next)
  	    {
		  if(rimeaddr_cmp(&e_new->addr,&e->addr))
		  {

		   //printf("equal contents   %x %x %x %x  \n",e_new->addr.u8[0], e_new->addr.u8[1], e->addr.u8[0] , e->addr.u8[1]);
		   flag++;
	      }

		  else
		  {
		    //printf("unequal contents %x %x %x %x  \n",e_new->addr.u8[0], e_new->addr.u8[1], e->addr.u8[0] , e->addr.u8[1]);
		  }

  	    }

  }
  if ((list_length(neighbor_table)==flag )&& (list_length(Temp_neighbor_table)==flag))
  {
    	//printf("same table \n");
    	return 1;

  }
  else
   {

	      //printf("different table \n");
    	  return 0;
   }
}






/*---------------------------------------------------------------------------*/
/*
 * This function is called to copy one rime address to the other
 */
void rimeaddr_copy(linkaddr_t *dest, const linkaddr_t *src)
 {
   uint8_t i;
   for(i = 0; i < RIMEADDR_SIZE; i++) {
    dest->u8[i] = src->u8[i];
   }
 }
 /*---------------------------------------------------------------------------*/
/*
 * This function is called to compare two rime addresses
 */
 int rimeaddr_cmp(const linkaddr_t *addr1, const linkaddr_t *addr2)
 {
   uint8_t i;
   for(i = 0; i < RIMEADDR_SIZE; i++) {
    if(addr1->u8[i] != addr2->u8[i]) {
    return 0;

   }
   }
   return 1;
}
 /*---------------------------------------------------------------------------*/
/*
 * This function is called to print neighbor table list
 */

void print_list()
{
	struct new_neighbor *e;

	printf("length of list %d \n",list_length(neighbor_table));

	if(list_length(neighbor_table)>0)
	{
	for(e = list_head(neighbor_table); e != NULL; e = e->next)
	{
		printf("The list member %x %x \n",e->addr.u8[0],e->addr.u8[1]);
	}
	}
	else
	{
		printf("no new members \n");
	}
}



/*---------------------------------------------------------------------------*/
/*
* This function is called to print temporary table list
*/

void print_list_temp()
{
	struct new_neighbor *e;

	printf("length of list %d \n",list_length(Temp_neighbor_table));

	if(list_length(Temp_neighbor_table)>0)
	{
	for(e = list_head(Temp_neighbor_table); e != NULL; e = e->next)
	{
		printf("The temp list member %x %x \n",e->addr.u8[0],e->addr.u8[1]);
	}
	}
	else
	{
		printf("no new temp members \n");
	}
}
void print_hop_table()
{
	struct hop_element *e;

	printf("length of list %d \n",list_length(hop_table));

	if(list_length(hop_table)>0)
	{
	for(e = list_head(hop_table); e != NULL; e = e->next)
	{
		printf("The hop list member %x %x %d \n",e->addr.u8[0],e->addr.u8[1],e->hop_count);
	}
	}
	else
	{
		printf("no new hop members \n");
	}
}
/*---------------------------------------------------------------------------*/
/*
 * This function is called when a neighbor ping is received . The
 * function checks the neighbor table to see if the neighbor is
 * already present in the list. If the neighbor is not present in the
 * list, a new neighbor table entry is allocated and is added to the
 * neighbor table.
 */

static void received_ping(const linkaddr_t *from)

{
  struct new_neighbor *e;

      printf("Got announcement from %x ,%x \n",
      from->u8[0], from->u8[1]);

  /* We received a ping from a neighbor so we need to update
     the neighbor list, or add a new entry to the table. */
    for(e = list_head(Temp_neighbor_table); e != NULL; e = e->next) {
        if(rimeaddr_cmp(from, &e->addr)) {
        	//printf("compare from %x %x",from->u8[0], from->u8[1]);
        	//printf("compare e %x %x",e->addr.u8[0], e->addr.u8[1]);
      /* Our neighbor was found, so we update the timeout. */
        return;
    }
  }

  /* The neighbor was not found in the list, so we add a new entry by
     allocating memory from the neighbor_mem pool, fill in the
     necessary fields, and add it to the list. */
   e = memb_alloc(&neighbor_mem);
   if(e != NULL) {
    rimeaddr_copy(&e->addr, from);
    list_add(Temp_neighbor_table, e);
    //printf("add %x %x \n",e->addr.u8[0], e->addr.u8[1]);

  }
}



/*---------------------------------------------------------------------------*/
/*
 * This function is called when a neighbor ping is received and
 * an acknowledgment has to be sent
 */
/*---------------------------------------------------------------------------*/
static void send_ack_ping()
	{

		packetbuf_copyfrom("Hello1",7 );
		packetbuf_set_attr(PACKETBUF_ATTR_RSSI,15);
	    broadcast_send(&bc);
	    printf("sending ack %d \n",PACKETBUF_ATTR_RSSI);

	}




/*---------------------------------------------------------------------------*/
/*
 * This function is a broadcast receive callback function.
 * PACKETBUF_ATTR_RSSI=15 is an acknowledgment packet type from neighbors
 * PACKETBUF_ATTR_RSSI=10 is an ping packet type from neighbors
 */
/*---------------------------------------------------------------------------*/


static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {

	printf("Broadcast message received from 0x%x%x:  [RSSI %d]\n",from->u8[0], from->u8[1],

			  			packetbuf_attr(PACKETBUF_ATTR_RSSI));

	if(packetbuf_attr(PACKETBUF_ATTR_RSSI)==15)
			{

		     received_ping(from);

			}

	if(packetbuf_attr(PACKETBUF_ATTR_RSSI)==10)
		    {

		     send_ack_ping();

		    }

}

static void
recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{

  printf("runicast message received from %x.%x, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);
  if(packetbuf_attr(PACKETBUF_ATTR_RSSI)==25)
  			{
	  printf("runicast message received from %x.%x, seqno %d\n",
	  	 from->u8[0], from->u8[1], seqno);

  		     received_hop(from);

  			}




}
static void
sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %x.%x, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
}
static void
timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %x.%x, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
}



static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};
static struct runicast_conn runicast;
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};

/*---------------------------------------------------------------------------*/
/* This process broadcasts a ping and updates its neighbor table. */

/*---------------------------------------------------------------------------*/

PROCESS(ping_broadcast, "ping_broadcast");

PROCESS(periodic_check, "periodic_check");

PROCESS(hop_start, "hop_start");
AUTOSTART_PROCESSES(&periodic_check);
PROCESS_THREAD(ping_broadcast, ev, data) {


	PROCESS_BEGIN();

	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,18);
	broadcast_open(&bc, 146, &broadcast_callbacks);
	packetbuf_copyfrom("Hello", 6);
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI,10);
    printf("Looking for neighbors %d \n",PACKETBUF_ATTR_RSSI);
    broadcast_send(&bc);




	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* This process broadcasts its table to the neighbors and merges neighbor table
 * with its own.
 */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(periodic_check, ev, data) {

	static uint8_t timer_interval1 =  7;
	static struct etimer et1;
	static uint8_t timer_interval2 =  14;
		static struct etimer et2;
	broadcast_open(&bc, 146, &broadcast_callbacks);

	PROCESS_BEGIN();



    while(1)
    {
    	printf("Posting process \n");

    	    runicast_open(&runicast, 144, &runicast_callbacks);
    	    etimer_set(&et1, CLOCK_SECOND * timer_interval1);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
    		process_start(&ping_broadcast,NULL);
    		update_neighbor_table_members();
    		etimer_reset(&et1);
    		etimer_set(&et2, CLOCK_SECOND * timer_interval2);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et2));
    		process_start(&hop_start,NULL);
    		etimer_reset(&et2);
    		NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,8);






    }
	PROCESS_END();

}

/*---------------------------------------------------------------------------*/
/* This process periodically(once every 30 seconds) posts the ping_broadcast
 * process. */
/*---------------------------------------------------------------------------*/

static struct hop_element *MinInList()
{
	struct hop_element *r;
	struct hop_element *min;

	r=list_head(hop_table);
	min=list_head(hop_table);

	while(r!=NULL) {
	    if(r->hop_count<min->hop_count) {
	    	min->hop_count=r->hop_count;
	    	rimeaddr_copy(&min->addr,&r->addr);
	    	min->next=r->next;

	    }
	    r=r->next;
	}

	return min;
}


 void received_hop(const linkaddr_t *from)
{
	struct hop_info hop_recvd;
	struct hop_element *hop_new;
	struct hop_element *new;
	 int flag=0;

	uint16_t length  = packetbuf_copyto(&hop_recvd);
	printf("buffer contents  %d \n",hop_recvd.hop_count);
	struct hop_element *min =MinInList();
	printf("minimum hop count %x %x %d",min->addr.u8[0],min->addr.u8[1],min->hop_count);

	print_hop_table();

		if(list_length(hop_table)==0)
		{



			new = memb_alloc(&hop_mem);
			rimeaddr_copy(&new->addr,from);
			new->hop_count=hop_recvd.hop_count;
			list_add(hop_table, new);
			printf("add new %x %x %d \n",new->addr.u8[0], new->addr.u8[1], new->hop_count);

    		runicast_open(&runicast, 144, &runicast_callbacks);
			if(!runicast_is_transmitting(&runicast)) {
			linkaddr_t recv;
			struct new_neighbor *e;
			struct hop_info e_new;
			e_new.hop_count= hop_recvd.hop_count+1;
			printf("sent hop count %d",e_new.hop_count);
			for(e = list_head(neighbor_table); e != NULL; e = e->next)
		    {
               if(rimeaddr_cmp(&e->addr,from)==0)
				{
                     packetbuf_copyfrom(&e_new,sizeof(e_new));
			         recv.u8[0] = e->addr.u8[0];
					 recv.u8[1] = e->addr.u8[1];
                     printf("%x.%x: sending runicast to address %x.%x\n",
					 linkaddr_node_addr.u8[0],
					 linkaddr_node_addr.u8[1],
					 recv.u8[0],
					 recv.u8[1]);
					 packetbuf_set_attr(PACKETBUF_ATTR_RSSI,25);
					 runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
				}
		    }


		}
	}

else if (list_length(hop_table)!=0)

		{
			for(hop_new = list_head(hop_table); hop_new  != NULL; hop_new  = hop_new ->next)
				{
			       if(rimeaddr_cmp(&hop_new->addr,from)==1)
			       {   flag=1;
			    	   if(hop_new->hop_count!=hop_recvd.hop_count)
			    	   {

			    		   hop_new->hop_count=hop_recvd.hop_count;
			    		   printf("already existing member changed hop count  only %x %x %d \n",hop_new->addr.u8[0], hop_new->addr.u8[1], hop_new->hop_count);

			    	   }

			    	   else
			    	   {
			    		   printf("already existing member no change in hop count as well %x %x %d \n",hop_new->addr.u8[0], hop_new->addr.u8[1], hop_new->hop_count);
			    	   }

			    	   if(hop_recvd.hop_count<= min->hop_count)
			    	   	{
			    	   	runicast_open(&runicast, 144, &runicast_callbacks);
			    	   	if(!runicast_is_transmitting(&runicast)) {
			    	   	linkaddr_t recv;
			    	   	struct new_neighbor *e;
			    	   	struct hop_info e_new;
			    	   	e_new.hop_count= hop_recvd.hop_count+1;

			    	   	for(e = list_head(neighbor_table); e != NULL; e = e->next)
			    	   	{
			    	   	if(rimeaddr_cmp(&e->addr,from)==0)
			    	   		{
			    	   			  packetbuf_copyfrom(&e_new,sizeof(e_new));
			    	   			  recv.u8[0] = e->addr.u8[0];
			    	   			  recv.u8[1] = e->addr.u8[1];
			    	   			  printf("%x.%x: sending runicast to address %x.%x\n",
			    	   			  linkaddr_node_addr.u8[0],
			    	   			  linkaddr_node_addr.u8[1],
			    	   			  recv.u8[0],
			    	   			  recv.u8[1]);
			    	   			  packetbuf_set_attr(PACKETBUF_ATTR_RSSI,25);
			    	   			  runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
			    	   		}
			    	  }



			    	  }

			    	  }





                 }
               }

			if(flag==0)
			{
				            new = memb_alloc(&hop_mem);
							rimeaddr_copy(&new->addr,from);
							new->hop_count=hop_recvd.hop_count;
							list_add(hop_table, new);
							printf("add new %x %x %d \n",new->addr.u8[0], new->addr.u8[1], new->hop_count);

				    		runicast_open(&runicast, 144, &runicast_callbacks);
							if(!runicast_is_transmitting(&runicast)) {
							linkaddr_t recv;
							struct new_neighbor *e;
							struct hop_info e_new;
							e_new.hop_count= hop_recvd.hop_count+1;
							printf("sent hop count %d",e_new.hop_count);
							for(e = list_head(neighbor_table); e != NULL; e = e->next)
						    {
				               if(rimeaddr_cmp(&e->addr,from)==0)
								{
				                     packetbuf_copyfrom(&e_new,sizeof(e_new));
							         recv.u8[0] = e->addr.u8[0];
									 recv.u8[1] = e->addr.u8[1];
				                     printf("%x.%x: sending runicast to address %x.%x\n",
									 linkaddr_node_addr.u8[0],
									 linkaddr_node_addr.u8[1],
									 recv.u8[0],
									 recv.u8[1]);
									 packetbuf_set_attr(PACKETBUF_ATTR_RSSI,25);
									 runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
								}
						    }


						}
			}
		}
}









PROCESS_THREAD(hop_start, ev, data) {

	PROCESS_EXITHANDLER(runicast_close(&runicast);)

	  PROCESS_BEGIN();
	  NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,8);
	  runicast_open(&runicast, 144, &runicast_callbacks);
      PROCESS_END();
}
