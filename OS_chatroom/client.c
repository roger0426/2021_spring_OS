#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
// 定義字串長度
#define LEN_NAME 20
#define LEN_MSG 50
#define LEN_SEND 100

void str_out(){
    printf("\r%s", ">> ");  // '\r': 換行並跳到行首
    fflush(stdout); // 清空輸出緩衝區
}

void str_trim (char* arr, int length){
    int i;
    for (i = 0; i < length; i++) { // 把'\n'變成'\0'
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

// Global variables
volatile sig_atomic_t flag = 0;
int skt = 0;  // socket
char username[LEN_NAME]; // 使用者名稱

void ctrl_c_exit(){ // 抓取"ctrl_c"和exit
    flag = 1;
}

void recv_handler(){
    char rcvMsg[LEN_SEND]; // 接收的訊息
    for(;;){
        int receive = recv(skt, rcvMsg, LEN_SEND, 0);
        if(receive > 0){
            printf("\r%s\n", rcvMsg);
            str_out();
        } 
        else if(receive == 0){
            break;
        }
    }
}

void send_handler(){
    char msg[LEN_MSG]; // 要傳送的訊息
    for(;;){
        str_out();
        while(fgets(msg, LEN_MSG, stdin) != NULL){ // 讀取輸入的訊息
            str_trim(msg, LEN_MSG); // 把換行字元('\n')拿掉，改成'\0'
            if (strlen(msg) == 0)
                str_out(); // 代表沒輸入就直接按enter，系統就會再要求輸入一次
            else
                break; // 收到正確訊息就跳出迴圈
        }
        send(skt, msg, LEN_MSG, 0);
        if(strcmp(msg, "exit") == 0){ // 當接收到訊息為"exit"就跳出迴圈
            break;
        }
    }
    ctrl_c_exit();
}

int main()
{
    signal(SIGINT, ctrl_c_exit);// SIGINT: interrupt from keyboard e.g. ctrl-c (當使用者輸入ctrl-c，就執行後面的函式)

    printf("Please enter your name: ");
    if(fgets(username, LEN_NAME, stdin) != NULL){ // 讀取LENGTH_NAME長度的字串，並設定從標準輸入串流來讀取資料
        str_trim(username, LEN_NAME); // 因為fgets會將'\n'也讀進去，所以透過str_trim_lf來去除
    }
    // 使用者輸入的名字需要大於2個字元，
	// 若輸入僅有一個字元或是超過我們所設定的最大字串長度，系統會跳出警告，並且結束此程式	
    if(strlen(username) == 0 || strlen(username) >= LEN_NAME-1){
        printf("\nERROR FORMAT!\n");
        exit(EXIT_FAILURE); // 結束此client程式
    }

    // 建立socket
    // 通訊端的協定集(domain): 
    // IPv4: AF_INET,  IPv6: AF_INET6
    // type:
    // SOCK_STREAM 提供雙向, 可靠的, 連線導向的串流連線 (TCP)
    // protocol:
    // 0: 根據選定的domain和type選擇使用預設協定
    skt = socket(AF_INET , SOCK_STREAM , 0); // domain, type, protocol
    if(skt == -1){ // 如果建立失敗的話，就跳出警告訊息並且退出系統
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }
    // Socket information
    /*
    #include <sys/socket.h>
        struct sockaddr {  
            unsigned short    sa_family;       // 2 bytes address family, AF_xxx  
            char              sa_data[14];     // 14 bytes of protocol address  
        }; 
        struct sockaddr_in { 
            short      sin_family;    // 2 bytes e.g. AF_INET, AF_INET6 
            unsigned short  sin_port;  // 2 bytes e.g. htons(3490) 
            struct in_addr  sin_addr;   // 4 bytes see struct in_addr, below 
            char       sin_zero[8];   // 8 bytes zero this if you want to 
        }; 
        struct in_addr {  
            unsigned long s_addr;              // 4 bytes load with inet_pton()  
        }; 
    */
    struct sockaddr_in ser_info, cli_info;
    int s_addrlen = sizeof(ser_info);
    int c_addrlen = sizeof(cli_info);
    memset(&ser_info, 0, s_addrlen); // 把地址歸0
    memset(&cli_info, 0, c_addrlen); // 把地址歸0
    ser_info.sin_family = PF_INET;  // PF是protocol family，AF是address family，但可以混用
    ser_info.sin_addr.s_addr = inet_addr("127.0.0.1"); // inet_addr()將IP地址從點數格式轉換成無符號長整型
    ser_info.sin_port = htons(8000); //htons()表示將16位的主機位元組順序轉化為16位的網路位元組順序

    // 連線到Server
    int err = connect(skt, (struct sockaddr *)&ser_info, s_addrlen);
    if (err == -1) {
        printf("Connection to Server error!\n");
        exit(EXIT_FAILURE);
    }
    
    // Names
    getsockname(skt, (struct sockaddr*) &cli_info, (socklen_t*) &c_addrlen); // 獲取本地IP和port
    getpeername(skt, (struct sockaddr*) &ser_info, (socklen_t*) &s_addrlen); // 獲取對方IP和port
    printf("Connect to Server: %s:%d\n", inet_ntoa(ser_info.sin_addr), ntohs(ser_info.sin_port));
    printf("You are: %s:%d\n", inet_ntoa(cli_info.sin_addr), ntohs(cli_info.sin_port));

    send(skt, username, LEN_NAME, 0); // 傳送username

    pthread_t send_msg_thread;
    pthread_t recv_msg_thread;
    pthread_create(&send_msg_thread, NULL, (void *) send_handler, NULL); // 建立一個thread用於發送訊息
    pthread_create(&recv_msg_thread, NULL, (void *) recv_handler, NULL); // 建立一個thread用於接收訊息

    for(;;){
        if(flag){ // 表示此使用者要退出聊天室
            printf("\nBye~~\n");
            break;
        }
    }
    close(skt); //  關閉socket連線
    return 0;
}