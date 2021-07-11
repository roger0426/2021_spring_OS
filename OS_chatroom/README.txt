多人聊天室:
使用c語言來進行socket的連線並讓client傳送訊息,server當作接收端並傳送訊息和使用者資訊給所有的client

1. Environment: 
- Ubuntu 20.04

2. File hierarchy:
   -src
       client.c
       server.c
       server.h
   Makefile
   server.out
   server.o
   client.out
   client.o
   README.txt


3. Compile and Run:
     make
     server
         ./server.out
     client
         ./client.out


4. To clean the .o and .out file:
     make clean


5. server的使用:
   server要先執行(./server.out)，client後執行，才能成功連線.


6. client的使用:
   client執行(./client.out)連線到server後，輸入自己的使用者名稱，就能傳送訊息給其他client，輸入”exit”或按”ctrl-c”後便能離開聊天室
 

