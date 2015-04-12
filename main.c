#include <stdio.h>

#include <winsock2.h>
#include <Windows.h>

#define MIN_PAYLOAD_SIZE 12

int initSockets(SOCKET *sd);
int sendPacket(SOCKET *sd, SOCKADDR_IN *dest_addr, char *message, int size);
int sendLoop(SOCKET *sd, SOCKADDR_IN *dest_addr, int interval, int sendsize);

float seconds_after_midnight_utc();

int main(int argc, const char *argv[])
{
   SOCKET sd = 0;
   SOCKADDR_IN dest_addr;

   int payloadsize = MIN_PAYLOAD_SIZE;
   int interval = 100;
   int dest_port = 5116;
   char dest_ip[16];

   memset(&dest_addr,0,sizeof(SOCKADDR_IN));
   memset(dest_ip,0,16);

   snprintf(dest_ip, 15, "127.0.0.1");

   printf("Hello there!\n\n");
   printf("This is a tool for testing of UDP-receive-functions.\n");
   printf("It sends an identifier (0xABCDEF12), followed by an incrementing message counter, followed by the amount of bytes following\n");
   printf("You get the actual interval displayed as an average of the last 5 messages\n");
   printf("Have fun with it!\n\n");

   switch(argc)
   {

      case 1: // No arguments? Use defaults
         printf("\nDefault Config: \n\tdest_ip = %s\n\tdest_port = %i\n\tinterval = %i\n\tpayload-size = %i\n\n\n",dest_ip,dest_port,interval,payloadsize);
         break;
      case 5: //4 arguments? Take all
         snprintf(dest_ip, 15, argv[1]);
         dest_port   = atoi(argv[2]);
         interval    = atoi(argv[3]);
         payloadsize = atoi(argv[4]);
         if(payloadsize < MIN_PAYLOAD_SIZE) {
            payloadsize = MIN_PAYLOAD_SIZE;
            printf("payload-size = %i is minimum\n", MIN_PAYLOAD_SIZE);
         }
      break;
      default:
         printf("\nSyntax: %s [dest_ip dest_port interval packetsize]",argv[0]);
         printf("\nDefault Config: \n\tdest_ip = %s\n\tdest_port = %i\n\tinterval = %i\n\tpayload-size = %i (minimum)\n\n\n",dest_ip,dest_port,interval,payloadsize);
         return 0;
      break;
   }

   printf("\nUsed Config: \n\tdest_ip = %s\n\tdest_port = %i\n\tinterval = %i\n\tpayload-size = %i (minimum)\n\n\n",dest_ip,dest_port,interval,payloadsize);

   if(0 != initSockets(&sd)) {
      printf("Socket error\n");
      return 1;
   } else {
      printf("Initializing successful!\n");
   }

   //prepare address
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons(dest_port);
   dest_addr.sin_addr.s_addr = inet_addr(dest_ip);

   printf("\nGO!\n\n");

   //enter sending loop
   sendLoop(&sd, &dest_addr, interval, payloadsize);

   return 0;
}

/**
 * returns 0 on success
 *
 */
int initSockets(SOCKET *sd) {
   int retval;
   WSADATA wsa;

   // WSAStartup
   retval = WSAStartup( MAKEWORD(2,0), &wsa );
   if( retval!=0 ) {
      printf("startWinsock, error\n");
   } else {
//      printf("Winsock gestartet!\n");
   }

   // Socket
   //bind
   *sd = socket(AF_INET,SOCK_DGRAM,0);
   if(*sd == INVALID_SOCKET) {
      printf("error creating socket: %i\n", WSAGetLastError());
      return 1;
   } else {
//      printf("Socket erstellt\n");
   }

   return 0;
}

int sendLoop(SOCKET *sd, SOCKADDR_IN *dest_addr, int interval, int sendsize) {
   if(sendsize < MIN_PAYLOAD_SIZE) {
      sendsize = MIN_PAYLOAD_SIZE;
   }

   unsigned char message[sendsize];
   float actual_interval;

   unsigned int msg_cnt = 0;

   float tstart, tend, tduration;
   float last=0, second_last=0, third_last=0, fourth_last=0;

   //prepare message
   unsigned int buf;
   buf = 0xABCDEF12;
   memcpy(&message[0], &buf, sizeof(int));
   buf = msg_cnt;
   memcpy(&message[4], &buf, sizeof(int));
   buf = sendsize - MIN_PAYLOAD_SIZE; //how much byte do follow?
   memcpy(&message[8], &buf, sizeof(int));

   double sum_timestamps = 0;

   timeBeginPeriod(1); //set windows timer to best resolution

   while(1) {
      tstart = seconds_after_midnight_utc();

      static float lastTimestamp = -1;
      float        currentTimestamp = seconds_after_midnight_utc();
      if(0 < lastTimestamp) {
         // calculate average of last 5
         actual_interval = (currentTimestamp-lastTimestamp);

         sum_timestamps += actual_interval;

         float average = (actual_interval + last + second_last + third_last + fourth_last) / 5;

         fourth_last = third_last;
         third_last  = second_last;
         second_last = last;
         last        = actual_interval;

         printf("\raverage of last 5: %15f; msg_cnt: %i", (average*1000.0), msg_cnt);
      }
      lastTimestamp = currentTimestamp;

      //send the message
      sendPacket(sd, dest_addr, (char*)message, sendsize);

      //increment counter
      msg_cnt++;
      buf = msg_cnt;
      memcpy(&message[4], &buf, sizeof(int));

      //fill rest of message with pseudo-random
      int i;
      for(i = 12; i < (sendsize - MIN_PAYLOAD_SIZE); i++) {
         message[i] = rand() % 255;
      }

      tend = seconds_after_midnight_utc();
      tduration = tend-tstart;

      //sending takes time as well
      Sleep(interval - (tduration*1000) );
   }

   timeEndPeriod(1); //just in case you remove the while(true)

   return 0;
}

int sendPacket(SOCKET *sd, SOCKADDR_IN *dest_addr, char *message, int size) {
   int retval;

   retval = sendto(*sd,message,size,0,(SOCKADDR*)dest_addr,sizeof(SOCKADDR_IN));

   if(-1 == retval) {
      printf("error sending: %i\n",WSAGetLastError());
      return 1;
   }

   return 0;
}

float seconds_after_midnight_utc() {

   SYSTEMTIME st;
   GetSystemTime(&st); //gives utc-time

   float sam_utc = 0;
   sam_utc += st.wHour * 60 * 60;
   sam_utc += st.wMinute * 60;
   sam_utc += st.wSecond;
   sam_utc += (float)st.wMilliseconds / 1000;

   return sam_utc;
}
