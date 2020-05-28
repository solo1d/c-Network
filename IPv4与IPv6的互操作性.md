# 目录

- [双栈主机处理监听套接字的规则](#双栈主机处理监听套接字的规则)
- [数据报转换](#数据报转换)
- [IPv4客户与IPv6服务器](#IPv4客户与IPv6服务器)
  - [IPv4的TCP客户端与IPv6的TCP服务器进行通信的步骤](#IPv4的TCP客户端与IPv6的TCP服务器进行通信的步骤)
- [IPv6客户与IPv4服务器](#IPv6客户与IPv4服务器)
- [IPv6地址测试宏](#IPv6地址测试宏)
- [IPv4程序转换成IPv6程序](#IPv4程序转换成IPv6程序)
- [IPv6服务器例程.支持IPv4和IPv6客户的访问和服务](#IPv6服务器例程.支持IPv4和IPv6客户的访问和服务)
- [IPv4服务器例程.支持IPv4访问和服务](#IPv4服务器例程.支持IPv4访问和服务)
- [IPv6客户例程.支持IPv6服务器的访问](#IPv6客户例程.支持IPv6服务器的访问)
- [IPv4客户例程.支持IPv4和IPv6服务器的访问](#IPv4客户例程.支持IPv4和IPv6服务器的访问)





- ==**双栈 (dual stacks): 表示一个IPv4协议栈和一个IPv6协议栈 一起运行.**==

- ==未来的主要问题是 IPv4与IPv6相互访问的协调==

- **IPv4服务器 仅仅可以被  IPv4客户端访问**

- **IPv6服务器  可以被 IPv4和IPv6 访问**

- **IPv4客户端  可以访问 IPv4与IPv6服务器**

- **IPv6客户端  仅仅可以访问IPv6服务器**

  



## 双栈主机处理监听套接字的规则

- IPv4 监听套接字只能接受来自 IPv4客户的外来连接.
- **绑定通配地址的 IPv6 监听套接字,而且套接字 `未设置 IPV6_V6RONLY` 套接字选项, 那么既可以接收IPv6 还可以接收 IPv4, 服务器处理的本地地址是 和客户端通讯的本机IPv4映射而成的IPv6地址**

- **如果服务器绑定了某个不是由 IPv4映射成IPv6的地址, 并且又设置了 `IPV6_V6ONLY` 套接字选项,那么这个套接字 就只能接受IPv6客户的外来连接.(IPv4映射IPv6也不行)**

```c
客户端 (源IPv4,目的地IPv4) -> 发送给服务器 -> 服务器通过本机的 IPv4地址收到 客户的发送的数据报
TCP模块将客户端数据报中的 (源IPv4) 和 (目的地IPv4) 都转换成 IPv6. -> 传递给服务器进程处理.
处理完毕后准备发送回去  -> 数据报通过IP栈(源IPv6,目的IPv6) ->  IP栈将两者都修改为 IPv4 (源IPv4,目的IPv4)
客户端收到, 继续进行所设想的IPv4通信.
```

```c
双栈主机上的 IPv6服务器既能服务于 IPv6客户, 又能服务IPv4客户.
    IPv4客户发送给服务器的仍然是IPv4 数据报, 不过服务器的协议栈会把客户主机的地址转换成IPv4映射的IPv6地址,
        因为 IPv6服务器仅仅处理IPv6套接字地址结构.

双栈主机上的 IPv6客户 能够与IPv4服务器通信.
    客户的解析器会把服务器主机所有的 A记录 作为IPv4映射的IPv6地址返回给客户,
       而客户指定这些地址之一调用 connect() 将会使双栈发送一个 IPv4 SYN 分节.
   只有少量特殊的客户和服务器需要知道对端使用的具体协议, IP6_IS_ADDR_V4MAPPED宏可以判断对端是否在使用IPv4        
```



### 数据报转换

- ==**数据报每个首部的作用**==
  - **IPv4**
    - **SYN是在数据报中承载的**
    - IPv4的TCP分节在以太网线上表现为:  **一个以太网首部 后跟一个IPv4首部, 一个TCP首部 ,以及TCP数据.**
      - **以太网首部包含类型字段值为 0x0800**
        - 它把本以太网帧标识为一个IPv4帧.
      - **IPv4首部中包含的目的IP地址**
      - **TCP首部中包含的目的端口**
  - **IPv6**
    - **SYN是在数据报中承载的**
    - IPv6的TCP分节在以太网线上表现为: **一个以太网首部 后跟一个IPv6首部, 一个TCP首部以及TCP数据.**
      - **以太网首部包含类型字段为 0x86dd**
        - 它把本以太网帧标识为一个IPv6帧
      - **IPv6首部中包含的目的IP地址**
      - **TCP首部中包含的目的端口**

- **接收数据链路  通过查看 ==以太网类型字段==  把每个帧传递给相应的模块.**
  - **IPv4模块结合其上的TCP模块检测到 IPv4数据报的目的地端口对应一个IPv6套接字, 于是把该数据报IPv4首部中的原IPv4地址转换成一个等价的IPv4映射的IPv6地址**
    - ==是TCP模块在进行 IPv4地址到 IPv6地址的映射.==
    - ==`accept` 系统调用把这个已经接收的IPv4客户连接返回给服务器进程时, 这个映射后的地址将作为客户的IPv6地址返回到服务器的IPv6套接字. (该连接上其余的数据报同样都是IPv4数据报).==





## IPv4客户与IPv6服务器

- ==双栈主机 基本特性: 其上IPv6服务器既能处理IPv4客户,又能处理IPv6客户.==
  - **这是通过使用 IPv4映射成 IPv6地址实现的**
- ==是TCP模块在进行 IPv4地址到 IPv6地址的映射.==



### IPv4的TCP客户端与IPv6的TCP服务器进行通信的步骤

**服务器必须同时支持 IPv4与IPv6 ,也同时拥有这两种IP,  客户端只支持 IPv4, 下面的步骤才为可行方案.**

**==服务器接收IPv4数据报时,  IP栈会修改成为IPv6地址,  服务器向客户发送IPv6数据报时, IP栈会修改成为IPv4==**

==**无论调用`connect` 还是调用 `sendto`, IPv4客户都不能指定一个IPv6地址, 因为16字节的IPv6地址超出了 IPv4的 `sockaddr_in` 结构中的的 `in_addr` 成员结构的4字节长度.**==

**UDP的步骤也和下面类似,只是 每个数据报的 地址格式可能有所变动.**



1. **IPv6服务器启动后创建一个IPv6监听套接字, 并绑定地址(也可以是通配地址)**
2. **IPv4 客户端 使用 `服务器的主机名` 调用 `gethostbyname` 找到服务器主机的一个 A 记录([函数和A记录说明](名字与地址转换.md),`其实就是IPv4地址`),  服务器主机既有一个A记录 ,又有一个AAAA记录`(因为它同时支持IPv4和IPv6)`,但 IPv4客户需要的只是一个A记录.**
   1. ==客户端连接的是 服务器的IPv4地址. 当服务器收到数据报后, 将自己本机的IPv4 映射成IPv6,然后传递给程序.==
3. **客户调用 `connect`, 导致客户端主机发送一个IPv4 SYN 到服务器主机.**
4. **三次握手时: 服务器主机接收 由客户发送过来的这个目的地为 IPv6监听套接字的 `IPv4 SYN`,设置一个标志指示本连接应使用 IPv4映射的IPv6地址, 然后相应以一个`IPv4 SYN/ACK`. 该连接建立后,==也就是三次握手结束后==. 由`accept` 返回给服务器的地址就是这个IPv4映射的IPv6地址.**
5. **服务器向IPv4映射的IPv6地址发送 TCP分节时, 其IP栈产生目的地址为所映射IPv4地址的 IPv4载送数据报. 因此,==客户与服务器之间的所有通信都使用 IPv4的载送数据报.==**
   1. ==也就是说 虽然服务器程序在使用IPv6进行和IPv4客户端的通信,但是在发送时 IP栈会修改这个IPv6地址,使其变成原有的IPv4客户端地址,然后再发送数据报.从底层来看,还是使用的IPv4通信.==
6. **除非服务器显示检查这个 IPv6地址是不是一个 IPv4映射的IPv6地址 `(通过 IN6_IS_ADDR_V4MAPPED 宏)`, 否则服务器永远都不知道自己是在与一个IPv4客户通信, ==这个细节由 双协议栈 处理==, 同样IPv4也不知道自己在和 IPv6 服务器通信.**



- **内核会把IPv4映射的IPv6地址作为 `accept` 或 `recvfrom` 返回的对端IPv6地址.**
  - ==任何一个IPv4地址总能表示成一个IPv6地址, 相互交换的是IPv4数据报.==
  - ==IPv6地址无法表示成一个IPv4地址.==





## IPv6客户与IPv4服务器

- **首先假设 客户端是双栈的`(IPv4和IPv6地址都有)`, 服务器只支持 IPv4**
  - **通讯步骤:**
    - **IPv4服务器 在只支持的IPv4的主机上创建启动一个 IPv4监听套接字**
    - **IPv6客户的启动后 调用`getaddrinfo` 单纯查找IPv6地址 `(请求 AF_INET6地址族, 在hints结构中设置了 AI_V4MAPPED 标志)` , 但服务器只支持IPv4, 所以返回给客户的是一个 由IPv4映射的IPv6的服务器地址.**
    - **客户拿到由IPv4映射成的IPv6地址后,调用 `connect` 进行连接, 但是内核检测到这个映射过的地址后,就自动发送一个 `IPv4 SYN` 到服务器,并不是`IPv6 SYN`**
    - **服务器响应一个 `IPv4 SYN/ACK`, 连接是通过使用 IPv4 数据报建立的.**
- ==**如果IPv6的客户指定一个IPv4映射的IPv6地址以调用 `connect 或 sendto`  ,那么内核检测到这个映射地址后 改为发送一个IPv4数据报而不是一个IPv6数据报**==



## IPv6地址测试宏

```c
下面都是宏测试, 返回int 值, 参数是一个 in6_add 指针. 是宏,不是函数.
#include <netinet/in.h>
  int IN6_IS_ADDR_UNSPECIFIED (const struct in6_addr* aptr)
  int IN6_IS_ADDR_LOOPBACK (const struct in6_addr* aptr)
  int IN6_IS_ADDR_MULTICAST (const struct in6_addr* aptr)
  int IN6_IS_ADDR_LINKLOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_SITELOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_V4MAPPED (const struct in6_addr* aptr) //测试由解析器返回的IPv6地址,判定对端是否在使用IPv4
  int IN6_IS_ADDR_V4COMPAT (const struct in6_addr* aptr)
	/* 
	 * 前面7个宏 用于测试 IPv6地址的基本类型.
   * 下面5个用于测试 IPv6多播地址的范围. 
   */
	int IN6_IS_ADDR_MC_NODELOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_MC_LINKLOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_MC_SITELOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_MC_ORGLOCAL (const struct in6_addr* aptr)
  int IN6_IS_ADDR_MC_GLOBAL (const struct in6_addr* aptr)
  
```



## IPv4程序转换成IPv6程序

- 去除所有 `gethostbyname() 和 gethostbyaddr()` 函数调用.
  - **改用 `getaddrinfo() 和 getnameinfo()` 函数调用**
    - **这步使得 能够把套接字地址结构作为不透明对象来处理.和 `bind, connect, recvfrom` 等基本套接字函数所做的那样, 用一个指针及大小来指代他们.**
  - **使用自定义对 sock套接字的 封装函数能够帮助 独立与IPv4和IPv6 地址进行操纵.**





## IPv6服务器例程.支持IPv4和IPv6客户的访问和服务

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <netdb.h>

#define  S_IP6    AF_INET6
#define  S_IP4    AF_INET
#define  SOCK_TCP SOCK_STREAM
#define  SOCK_UDP SOCK_DGRAM

int
Socket (int domain, int type, int protocol);



/* IPv6  server : only   listen local addr IPv6 , porx 9999 */
int main(int argc, const char * argv[]) {
    
    int  fd = -1;
    if ( (fd = Socket( S_IP6, SOCK_TCP, 0 )) < 0){
        exit(errno);
    }
    
    struct sockaddr_in6 ser;
    struct sockaddr_in6 cli;
    
    socklen_t  ser_len =  sizeof(struct sockaddr_in6);
    socklen_t  cli_len = ser_len;
    
    memset(&ser, 0, ser_len);
    memset(&cli, 0, ser_len);
    
    ser.sin6_addr =  in6addr_any;      // 通配IPv6地址结构
    ser.sin6_port =  htons(9999);
    ser.sin6_family = S_IP6;
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//地址重用
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));//端口重用
    
    
    bind(fd, (struct sockaddr*)&ser, ser_len);
    
    {
        // 获得 本地IPv6的信息.
        char sIP[INET6_ADDRSTRLEN] = { [0 ... INET6_ADDRSTRLEN-1]='\0' };
        inet_ntop(AF_INET6, &(ser.sin6_addr),sIP, INET6_ADDRSTRLEN);
        printf("本地端IP : %s\n端口号: %d\n\n\n",  sIP , ntohs(ser.sin6_port));
    }
    
    listen(fd, 1024);
    while(1){
        int cfd = -1;
        if (  (cfd = accept(fd, (struct sockaddr*)&cli, &cli_len)) < 0 ){
            fprintf(stderr, "accept() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
        }
        
        if ( cfd > 0) {
            // 获得 客户端的信息.
            char cIP[INET6_ADDRSTRLEN] = { [0 ... INET6_ADDRSTRLEN-1]='\0' };
            inet_ntop(AF_INET6, &(cli.sin6_addr),cIP, INET6_ADDRSTRLEN);
            printf("客户端IP : %s\n端口号: %d\n",  cIP , ntohs(cli.sin6_port));
        }
        close(cfd);
    }
    close(fd);
    return 0;
}

/*
 * 创建一个 套接字
 */
int
Socket (int domain, int type, int protocol){
    int temp = socket(domain, type, protocol);
    if ( temp < 0 ){
        fprintf(stderr, "Socket() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
    }
    return temp;
}


/*
 Ipv6访问时,服务器的输出:
客户端IP : fe80::860:a71:bfd2:4fd5
端口号: 50332
  
 Ipv4访问时,服务器的输出:
客户端IP : ::ffff:10.0.0.94
端口号: 50341
*/
```

## IPv4服务器例程.支持IPv4访问和服务

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <netdb.h>

#define  S_IP6    AF_INET6
#define  S_IP4    AF_INET
#define  SOCK_TCP SOCK_STREAM
#define  SOCK_UDP SOCK_DGRAM

int
Socket (int domain, int type, int protocol);



/* IPv6  server : only   listen local addr IPv6 , porx 9999 */
int main(int argc, const char * argv[]) {
    
    int  fd = -1;
    if ( (fd = Socket( S_IP4, SOCK_TCP, 0 )) < 0){
        exit(errno);
    }

//    struct sockaddr_in6 ser;
//    struct sockaddr_in6 cli;
    struct sockaddr_in  ser;
    struct sockaddr_in  cli;
    socklen_t  ser_len =  sizeof(struct sockaddr_in);
    socklen_t  cli_len = ser_len;
    
    memset(&ser, 0, ser_len);
    memset(&cli, 0, ser_len);
    
    ser.sin_addr.s_addr =  htonl(INADDR_ANY);      // 通配IPv4地址结构
    ser.sin_port =  htons(9999);
    ser.sin_family = S_IP4;
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//地址重用
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));//端口重用
    
    
    bind(fd, (struct sockaddr*)&ser, ser_len);
    
    {
        // 获得 本地IPv4的信息.
        char sIP[INET_ADDRSTRLEN] = { [0 ... INET_ADDRSTRLEN-1]='\0' };
        inet_ntop(AF_INET, &(ser.sin_addr),sIP, INET_ADDRSTRLEN);
        printf("本地端IP : %s\n端口号: %d\n\n\n",  sIP , ntohs(ser.sin_port));
    }
    
    listen(fd, 1024);
    while(1){
        int cfd = -1;
        if (  (cfd = accept(fd, (struct sockaddr*)&cli, &cli_len)) < 0 ){
            fprintf(stderr, "accept() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
        }
        
        if ( cfd > 0) {
            // 获得 客户端的信息.
            char cIP[INET_ADDRSTRLEN] = { [0 ... INET_ADDRSTRLEN-1]='\0' };
            inet_ntop(AF_INET, &(cli.sin_addr),cIP, INET_ADDRSTRLEN);
            printf("客户端IP : %s\n端口号: %d\n",  cIP , ntohs(cli.sin_port));
        }
        close(cfd);
    }
    close(fd);
    return 0;
}


/*
 * 创建一个 套接字
 */
int
Socket (int domain, int type, int protocol){
    int temp = socket(domain, type, protocol);
    if ( temp < 0 ){
        fprintf(stderr, "Socket() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
    }
    return temp;
}


/* 该服务器是双栈, 既有IPv4地址,还有IPv6地址
  IPv4客户端访问时,服务器输出:
客户端IP : 10.0.0.94
端口号: 50426

  IPv6客户端访问时,服务器输出:  (失败了,无输出)
  客户端的输出:
  主机规范名称: macpro.local 
	主机的协议族: 30 ,AF_INET6
	服务器IP : 1c1e:270f::
	端口号: 9999
	connect 连接失败
*/
```





## IPv6客户例程.支持IPv6服务器的访问

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <netdb.h>

#define  S_IP6    AF_INET6
#define  S_IP4    AF_INET
#define  SOCK_TCP SOCK_STREAM
#define  SOCK_UDP SOCK_DGRAM

int
Socket (int domain, int type, int protocol);



/* IPv4  client : connect IPv6 (IPv4->IPv6) , porx 9999 */
int main(int argc, const char * argv[]) {
    
    int  fd = -1;
    if ( (fd = Socket( S_IP6, SOCK_TCP, 0 )) < 0){  //----------
        exit(errno);
    }
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//地址重用
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));//端口重用
    
    
    
    struct addrinfo hints, *result , *nnet;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;           // ------------
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME | AI_V4MAPPED ;
    if ( getaddrinfo("macpro.local", "9999", &hints, &result) < 0 ){
        fprintf(stderr, "error: %d\n", __LINE__ );
        exit(errno);
    }
    if ( result == NULL ){
        fprintf(stderr, "获取主机地址失败 \n");
        exit(errno);
    }
    
//    struct sockaddr_in cli;
//    memset( &cli, 0, sizeof(cli));
//    cli.sin_addr.s_addr = htonl(INADDR_ANY);
//    cli.sin_port = htons(9999);
//    bind(fd, (struct sockaddr*)&cli, sizeof(cli));
    
    nnet = result;
        
    do{
        {printf("主机规范名称: %s \n", nnet->ai_canonname);
                printf("主机的协议族: %d ,%s\n", nnet->ai_family, (nnet->ai_family == AF_INET) ? "AF_INET" : "AF_INET6" ); // ------------
                // 获得 客户端的信息.
                char cIP[INET6_ADDRSTRLEN] = { [0 ... INET6_ADDRSTRLEN-1]='\0' };
                inet_ntop(AF_INET6, nnet->ai_addr ,cIP, INET6_ADDRSTRLEN);
                // ------------

                printf("服务器IP : %s\n端口号: %d\n",  cIP ,
                       ntohs(  ((struct sockaddr_in6*)(nnet->ai_addr))->sin6_port ));
            }
        if ( connect(fd, nnet->ai_addr, nnet->ai_addrlen ) < 0 )
            fprintf(stderr, "connect 连接失败\n");
        else
            printf("连接成功 \n");
        close(fd);
        nnet = nnet->ai_next;
    } while( nnet != NULL );
    
    freeaddrinfo(result);
    close(fd);
    return 0;
}




/*
 * 创建一个 套接字
 */
int
Socket (int domain, int type, int protocol){
    int temp = socket(domain, type, protocol);
    if ( temp < 0 ){
        fprintf(stderr, "Socket() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
    }
    return temp;
}

/* 访问 IPv4服务器失败了. 客户端输出:
主机规范名称: pi.local 
主机的协议族: 30 ,AF_INET6
服务器IP : 1c1e:270f:0:0:fe80::
端口号: 9999

访问 IPv6服务器 成功了, 客户端输出:
主机规范名称: macpro.local 
主机的协议族: 30 ,AF_INET6
服务器IP : 1c1e:270f:0:0:fe80::
端口号: 9999
连接成功 
*/
```



## IPv4客户例程.支持IPv4和IPv6服务器的访问

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <netdb.h>

#define  S_IP6    AF_INET6
#define  S_IP4    AF_INET
#define  SOCK_TCP SOCK_STREAM
#define  SOCK_UDP SOCK_DGRAM

int
Socket (int domain, int type, int protocol);



/* IPv4  client : connect IPv6 (IPv4->IPv6) , porx 9999 */
int main(int argc, const char * argv[]) {
    
    int  fd = -1;
    if ( (fd = Socket( S_IP4, SOCK_TCP, 0 )) < 0){  //----------
        exit(errno);
    }
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//地址重用
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));//端口重用
    
    
    
    struct addrinfo hints, *result , *nnet;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;           // ------------
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if ( getaddrinfo("pi.local", "9999", &hints, &result) < 0 ){
        fprintf(stderr, "error: %d\n", __LINE__ );
        exit(errno);
    }
    if ( result == NULL ){
        fprintf(stderr, "获取主机地址失败 \n");
        exit(errno);
    }
    
//    struct sockaddr_in cli;
//    memset( &cli, 0, sizeof(cli));
//    cli.sin_addr.s_addr = htonl(INADDR_ANY);
//    cli.sin_port = htons(9999);
//    bind(fd, (struct sockaddr*)&cli, sizeof(cli));
    
    nnet = result;
   do{
        {printf("主机规范名称: %s \n", nnet->ai_canonname);
                printf("主机的协议族: %d ,%s\n", nnet->ai_family, (nnet->ai_family == AF_INET) ? "AF_INET" : "AF_INET6" ); // ------------
                // 获得 客户端的信息.
                char cIP[INET6_ADDRSTRLEN] = { [0 ... INET6_ADDRSTRLEN-1]='\0' };
                inet_ntop(AF_INET6, nnet->ai_addr ,cIP, INET6_ADDRSTRLEN);
                // ------------

                printf("服务器IP : %s\n端口号: %d\n",  cIP ,
                       ntohs(  ((struct sockaddr_in6*)(nnet->ai_addr))->sin6_port ));
            }
        if ( connect(fd, nnet->ai_addr, nnet->ai_addrlen ) < 0 )
            fprintf(stderr, "connect 连接失败\n");
        else
            printf("连接成功 \n");
        close(fd);
        nnet = nnet->ai_next;
    } while( nnet != NULL );
    
    freeaddrinfo(result);
    close(fd);
    return 0;
}




/*
 * 创建一个 套接字
 */
int
Socket (int domain, int type, int protocol){
    int temp = socket(domain, type, protocol);
    if ( temp < 0 ){
        fprintf(stderr, "Socket() error: %d, len:%d , %s \n", errno, __LINE__,         strerror(errno));
    }
    return temp;
}


/*
客户端输出: 连接IPv4服务器
主机规范名称: pi.local 
主机的协议族: 2 ,AF_INET
服务器IP : 16.2.39.15
端口号: 9999


客户端输出: 连接IPv6服务器
主机规范名称: macpro.local 
主机的协议族: 2 ,AF_INET
服务器IP : 1002:270f:7f00:1::
端口号: 9999
连接成功 

主机规范名称: macpro.local 
主机的协议族: 2 ,AF_INET
服务器IP : 1002:270f:a00:5e::
端口号: 9999
connect 连接失败


IPv6服务器端输出:

客户端IP : ::ffff:127.0.0.1
端口号: 50524

*/
```

