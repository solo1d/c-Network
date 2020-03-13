## 目录

- [windows下vs2019配置](#windows下vs2019配置)
  
  - [_DEBUG调试宏](#_DEBUG调试宏)
- [select模型 全平台](#select模型)
- [全平台通讯模版-做了移植操作](#全平台通讯模版)
  - [TCP-服务端](#TCP-服务端)
  - [TCP-客户端](#TCP-客户端)



## windows下vs2019配置

> **新建解决方案中的项目:**
>
> - **可以选择 项目的属性, 来确定生成位置**
>   - 属性页中的配置-> 所有配置,  平台中的->所有平台
>     - 输出目录:`$(SolutionDir)../bin/$(Platform)/$(Configuration)`
>     - 中间目录:`$(SolutionDir)../temp/$(Platform)/$(Configuration)/$(ProjectName)`

#### _DEBUG调试宏

```c++
/* _DEBUG 是win默认的调试宏, 在debug模式下有效, 在 release 下无效. 相当于开关 */
#ifdef _DEBUG
		if (i == 5)
			cout << "debug: i == 5, a= " << a << endl;
#endif // _DEBUG
```



- **可以通过设置断点中的 条件和操作来进行多线程和多进程的调试**

- **可以使用 `快速监视` 来观察指针所指向数组后面的多个元素的内容, 使用 , 逗号间隔 对象与数量.**
- 



## select模型

[关于select的详细说明](https://github.com/solo1d/TCP-IP/blob/master/套接字函数和信号处理以及多进程.md)

### 

## 全平台通讯模版

- windows
- linux
- macOS

### TCP-服务端

```c++
#ifdef _WIN32  //只有在Windows 下,才会定义这个宏. 出于移植考虑, 应该使用这种判别式, 以使用头文件和宏
    #define  WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
    #define  _WINSOCK_DEPRECATED_NO_WARNINGS
    #pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去
    #include <WinSock2.h>
    #include <Windows.h>     // 这两个头文件的文位置不同，会造成编译失败

#else    // 这个是 Unix和Linux 下才有的头文件, 也有用于方便移植的宏
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #define  SOCKET  int       //兼容 windows ,将 socket套接字的int, b变更为 SOCKET来表示, 两个平台的内容相同
    #define  INVALID_SOCKET  (SOCKET)(~0)     //取反的0, 也就是-1, 用于判别 socket套接字的创建错误
    #define  SOCKET_ERROR            (-1)     // -1, 用于判别 connect 的连接错误
#endif

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <vector>
#include <algorithm>

#define MAC_IP   "10.0.0.94"
#define WIN_IP   "10.0.0.33"
#define LIN_IP   "10.0.0.1"
#define CONNECT_IP  MAC_IP      //服务器的IP


class sort_SCOKET_function{
public:
    bool operator()(SOCKET sock1, SOCKET sock2){
        return  sock1 < sock2;
    }
};


// 命令
enum CMD
{
    CMD_LOGIN,     /* 登录 */
    CMD_LOGIN_RESULT,    /* LOGIN 返回值,返回消息*/
    CMD_LOGOUT,    /* 登出 */
    CMD_LOGOUT_RESULT,  /* LOGOUT 返回值,返回消息*/
    CMD_NEW_USER_JOIN,   /* 新用户加入 */
    CMD_ERROR      /* 错误*/
};

//数据包的 包头
struct DataHeader {
    short dataLength;   //数据长度
    short cmd;          //命令, 使用 enum CMD 枚举,来执行命令
};


// 数据包的  包体
 /* 登录数据结构 */
struct Login :public DataHeader
{
    Login():userName("") {
        DataHeader::dataLength = sizeof(Login);
        DataHeader::cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
};

/* 返回登陆是否成功的结构体 */
struct LoginResult : public DataHeader
{
    LoginResult():result(0){
        DataHeader::dataLength = sizeof(LoginResult);
        DataHeader::cmd = CMD_LOGIN_RESULT;
    }
    int result;    /* 登陆结果 */
};


/* 退出登录的用户名 */
struct LogOut : public DataHeader
{
    LogOut():userName(""){
        DataHeader::dataLength = sizeof(LogOut);
        DataHeader::cmd = CMD_LOGOUT;
    }
    char  userName[32];
};

/* 登出的结果 */
struct LogOutResult : public DataHeader
{
    LogOutResult(): result(0){
        DataHeader::dataLength = sizeof(LogOutResult);
        DataHeader::cmd = CMD_LOGOUT_RESULT;
    }
    int result;
};

/* 新用户加入 */
struct NewUserJoin : public DataHeader
{
    NewUserJoin() : sock(0) {
        DataHeader::dataLength = sizeof(NewUserJoin);
        DataHeader::cmd = CMD_NEW_USER_JOIN;
    }
    int sock;
};


std::vector<SOCKET> g_clients;



int
processr(SOCKET _cSock) {
    // 使用缓冲区接收数据.
    char  szRecv[4096] = {};
    
#ifdef _WIN32
    int ret;
#else
    ssize_t ret; // ssize_t 是 long , size_t 是 unsigned long
#endif
    
    ret = recv(_cSock, szRecv, sizeof(DataHeader), 0);  // 接受客户端发送过来的数据
    if (ret <= 0) {
        std::cout << "客户端< " << _cSock << " >发送了终止符,或已退出" << std::endl;
        return -1;
    }
    DataHeader* header = (DataHeader*)szRecv;
    // * 6 处理请求
    switch (header->cmd) {
        case CMD_LOGIN: {
            ret = recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Login* login = (Login*)szRecv;
            if (ret <= 0) {
                std::cerr << "对方 < " << _cSock << " >没有发送任何数据 或关闭了客户端, 这不符合流程,进行强制中断" << std::endl;
                return -1;
            }
            else {
                std::cout << "收到客户端 < " << _cSock << " >发送过来的命令:" << login->cmd
                << "  ,数据长度为: " << login->dataLength
                << "  ,Username: " << login->userName
                << "  ,PassWord: " << login->PassWord << std::endl;
            }
            LoginResult  lret;
            send(_cSock, (char*)&lret, sizeof(lret), 0);
            break;
        }
        case CMD_LOGOUT: {
            
            ret = recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LogOut* loginout = (LogOut*)szRecv;
            
            // 忽略判断用户名和密码
            if (ret <= 0)
                std::cerr << "对方 < " << _cSock << " > 没有发送任何数据 或关闭了客户端, 这不符合流程,进行强制中断" << std::endl;
            
            std::cout << "收到客户端< " << _cSock << " >发送过来的命令:" << loginout->cmd
            << "  ,数据长度为: " << loginout->dataLength
            << "  ,Username: " << loginout->userName << std::endl;
            
            LogOutResult  lret;
            send(_cSock, (char*)&lret, sizeof(lret), 0);
            break;
        }
        default: {
            DataHeader err;
            err.cmd = CMD_ERROR;
            err.dataLength = 0;
            send(_cSock, (char*)&err, sizeof(err), 0);
            break;
        }
    }
    return 0;
}



int
main()
{
#ifdef  _WIN32
    /* 建立和启动 Windows socket2.x 环境 */
    WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
    WSADATA dat;                 //数据指针
    (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
                            // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。
#endif
    
    // 1. 建立 scoket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // 需求一个 TCP的流式套接字
    if (INVALID_SOCKET == _sock) {
        std::cout << "建立套接字错误" << std::endl;
        return (errno);
    }
    else
        std::cout << "建立套接字成功" << std::endl;



   //  2. bind 绑定用于接收客户端连接的网络端口
    sockaddr_in  _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);

    
#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
#else
    _sin.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    
    
    if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR) // Win下特有的错误对比
        std::cout << "bind Not failure" << std::endl;
    else
        std::cout << "bind success" << std::endl;


   //  *3. listen  监听网络端口
    if (listen(_sock, 64) == SOCKET_ERROR )
        std::cout << "listen Not failure" << std::endl;
    else
        std::cout << "listen success" << std::endl;
    

   // *4. accept  等待接收客户端链接
    sockaddr_in clientAddr = {};    //接收客户端信息的结构体, 下面的长度信息.

#ifdef _WIN32
    int clientAddrLen = sizeof(clientAddr);         //win 下是 int
#else
    socklen_t clientAddrLen  = sizeof(clientAddr);  // macos和linux是 unsigned int
#endif
    

    SOCKET _cSock = INVALID_SOCKET;           // 该宏可以用来初始化和表示  无效的 SOCK套接字
    SOCKET _maxSock = _sock;
    timeval t = { 1,0 };
    g_clients.clear();
        
    while (true) {
        fd_set fdRead;
        fd_set fdWrite;
        fd_set fdExp;
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);

        FD_SET(_sock, &fdRead);
        FD_SET(_sock, &fdWrite);
        FD_SET(_sock, &fdExp);
        for (int n = static_cast<int>(g_clients.size()) -1 ; n >= 0; n--) {
            FD_SET(g_clients.at(n), &fdRead);
            FD_SET(g_clients.at(n), &fdWrite);
            FD_SET(g_clients.at(n), &fdExp);

            _maxSock < g_clients.at(n) ? _maxSock = g_clients.at(n) : _maxSock;
        }
// 伯克利 socket
// select 在windows下, 第一个参数不产生任何作用
// _In_ 代表传入参数,  _Inout 代表传入传出参数, _Inout_opt_ 传入传出操作参数
        int ret = select(  _maxSock+1 , &fdRead, nullptr, nullptr, &t);
             //等待描述符出现变化, 或者出错.
             //g_clients.back() +1 ;已经排序过的容器, 最后一个值是最大的描述符, +1 表示委托内核检测的描述符最大值是第几个.(0-1024)
        if (ret < 0) {
            std::cout << "select 任务结束" << std::endl;
            break;
        }

        //可读, 新客户端来了新链接
        if (FD_ISSET(_sock, &fdRead)) {
            FD_CLR(_sock, &fdRead);
            _cSock = accept(_sock, (struct sockaddr*) &clientAddr, &clientAddrLen);
            if (_cSock == INVALID_SOCKET) {
                std::cout << "接收得到无效的客户端" << std::endl;
                return errno;
            }
            
            for (int n = static_cast<int>(g_clients.size()) - 1; n >= 0; n--) {
                NewUserJoin userJoin;
                userJoin.sock = _cSock;
                send(g_clients.at(n), (const char*)&userJoin, sizeof(NewUserJoin), 0);
            }
            g_clients.push_back(_cSock);
            std::cout << "新客户端加入  < " << _cSock << " >\t ";
        
#ifdef _WIN32
            std::cout << "客户端的IP地址为: " << inet_ntoa(clientAddr.sin_addr); //将sin_addr
#else
            char client_addr_arr[16] = {};
            inet_pton(AF_INET, client_addr_arr , &clientAddr.sin_addr);
            std::cout <<  "客户端的IP地址为: " <<  client_addr_arr;
                              //将sin_addr结构体转换成字符串。
#endif
            std::cout << std::endl
                          << "客户端的端口为: " << ntohs(clientAddr.sin_port)
                          << std::endl;
        }
        

//已连接客户端向服务器发送了消息
#ifdef _WIN32
        for (size_t n = 0;  n <  fdRead.fd_count ; n++) {
            if (-1 == processr( fdRead.fd_array[n] )) {
                FD_CLR( fdRead.fd_array[n], &fdRead);
                auto iter = std::find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
                    closesocket(*iter);
                    std::cout << "通讯结束,关闭这个套接字" << *iter << std::endl;
                    g_clients.erase(iter);
             }

        }
#else
        for ( int n = static_cast<int>( g_clients.size()) -1 ; n >=  0; n-- ){
            if ( FD_ISSET(g_clients.at(n), &fdRead)){
                FD_CLR(g_clients.at(n), &fdRead);
                if ( -1 == processr(g_clients.at(n))){
                    std::cout << "通讯结束,关闭这个套接字" << g_clients.at(n) << std::endl;
                    shutdown(g_clients.at(n), SHUT_RDWR);
                    
                    auto it = g_clients.begin() + n;
                    g_clients.erase(it);
                }
            }
        }
#endif
    }

// *7. 关闭套接字 closesocket
// 优化了一次.
    while ( !g_clients.empty() ){
#ifdef _WIN32
        closesocket(g_clients.back());
#else
        shutdown(g_clients.back(), SHUT_WR);
#endif
        g_clients.pop_back();
    }
        
#ifdef _WIN32
    closesocket(_sock);
     /*  清除 Windows socket 环境  */
    WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
    Sleep(10000);
#else
    shutdown(_sock, SHUT_WR);
    sleep(10);

#endif
        
    std::cout << "结束通讯,关闭所有套接字" << std::endl;
    return 0;
}

```



### TCP-客户端

```c++
#ifdef _WIN32  //只有在Windows 下,才会定义这个宏. 出于移植考虑, 应该使用这种判别式, 以使用头文件和宏
    #define  WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
    #define  _WINSOCK_DEPRECATED_NO_WARNINGS
    #pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去
    #include <WinSock2.h>
    #include <Windows.h>     // 这两个头文件的文位置不同，会造成编译失败

#else    // 这个是 Unix和Linux 下才有的头文件, 也有用于方便移植的宏
    #include <unistd.h>

//    #ifdef _LINUX
        #include <sys/socket.h>
//    #else
//        #include <socket.h>
//    #endif
    #include <fcntl.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #define  SOCKET  int       //兼容 windows ,将 socket套接字的int, b变更为 SOCKET来表示, 两个平台的内容相同
    #define  INVALID_SOCKET  (SOCKET)(~0)     //取反的0, 也就是-1, 用于判别 socket套接字的创建错误
    #define  SOCKET_ERROR            (-1)     // -1, 用于判别 connect 的连接错误
#endif

#include <iostream>
#include <string>
#include <thread>
#include <cstring>

#define MAC_IP   "10.0.0.94"
#define WIN_IP   "10.0.0.33"
#define LIN_IP   "10.0.0.1"
#define CONNECT_IP  MAC_IP      //服务器的IP



// 命令
enum CMD
{
    CMD_LOGIN,     /* 登录 */
    CMD_LOGIN_RESULT,    /* LOGIN 返回值,返回消息*/
    CMD_LOGOUT,    /* 登出 */
    CMD_LOGOUT_RESULT,  /* LOGOUT 返回值,返回消息*/
    CMD_NEW_USER_JOIN,   /* 新用户加入 */
    CMD_ERROR      /* 错误*/
};

//数据包的 包头
struct DataHeader {
    short dataLength;   //数据长度
    short cmd;          //命令, 使用 enum CMD 枚举,来执行命令
};


// 数据包的  包体
 /* 登录数据结构 */
struct Login :public DataHeader
{
    Login() :userName("") {
        DataHeader::dataLength = sizeof(Login);
        DataHeader::cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
};

/* 返回登陆是否成功的结构体 */
struct LoginResult : public DataHeader
{
    LoginResult() :result(0) {
        DataHeader::dataLength = sizeof(LoginResult);
        DataHeader::cmd = CMD_LOGIN_RESULT;
    }
    int result;    /* 登陆结果 */
};


/* 退出登录的用户名 */
struct LogOut : public DataHeader
{
    LogOut() :userName("") {
        DataHeader::dataLength = sizeof(LogOut);
        DataHeader::cmd = CMD_LOGOUT;
    }
    char  userName[32];
};

/* 登出的结果 */
struct LogOutResult : public DataHeader
{
    LogOutResult() : result(0) {
        DataHeader::dataLength = sizeof(LogOutResult);
        DataHeader::cmd = CMD_LOGOUT_RESULT;
    }
    int result;
};

/* 新用户加入 */
struct NewUserJoin : public DataHeader
{
    NewUserJoin() : sock(0) {
        DataHeader::dataLength = sizeof(NewUserJoin);
        DataHeader::cmd = CMD_NEW_USER_JOIN;
    }
    int sock;
};



int
processr(SOCKET _cSock) {
    // 使用缓冲区接收数据.
    char  szRecv[4096] = {};
    
#ifdef _WIN32
    int ret;
#else
    ssize_t ret;   // ssize_t  是 long 类型,  size_t 是 unsigned long
#endif

    ret = recv(_cSock, szRecv, sizeof(DataHeader), 0);  // 接受客户端发送过来的数据
    if (ret <= 0) {
        std::cout << "与服务器断开连接, 任务结束" << std::endl;
        return -1;
    }

    DataHeader* header = (DataHeader*)szRecv;

    // * 6 处理请求
    switch (header->cmd) {
        case CMD_LOGIN_RESULT: {   // 期待服务器返回登录消息
            ret = recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult*)szRecv;
            if (ret <= 0) {
                std::cerr << "对方 < " << _cSock << " >没有发送任何数据 或关闭了服务端, 这不符合流程,进行强制中断" << std::endl;
                return -1;
            }
            else
                std::cout << "收到服务器消息: CMD_LOGIN_RESULT ,数据长度为" << login->dataLength << std::endl;
            
            break;
        }
        case CMD_LOGOUT_RESULT: {   // 期待服务器返回 退出登录的消息
            ret = recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LogOutResult* loginout = (LogOutResult*)szRecv;
            if (ret <= 0) {
                std::cerr << "对方 < " << _cSock << " >没有发送任何数据 或关闭了服务端, 这不符合流程,进行强制中断" << std::endl;
                return -1;
            }
            else
                std::cout << "收到服务器消息: CMD_LOGOUT_RESULT ,数据长度为" << loginout->dataLength << std::endl;
            
            break;
        }
        case CMD_NEW_USER_JOIN: {   // 有新的连接 加入到服务器的消息
            ret = recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            NewUserJoin* userJoin = (NewUserJoin*)szRecv;
            if (ret <= 0) {
                std::cerr << "对方 < " << _cSock << " >没有发送任何数据 或关闭了服务端, 这不符合流程,进行强制中断" << std::endl;
                return -1;
            }
            else
                std::cout << "收到服务器消息: CMD_NEW_USER_JOIN ,数据长度为" << userJoin->dataLength << std::endl;

            break;
        }
    }
    return 0;
}

bool g_bRun = true;


void
cmdThread(SOCKET _sock) {

    char cmdBuf[256] = {};

    while (true) {
        std::cin >> cmdBuf;

        if (0 == strcmp(cmdBuf, "exit")) {
            g_bRun = false;
            std::cout << "退出 cmdThread 线程" << std::endl;
            break;
        }
        else if (0 == strcmp(cmdBuf, "login")) {
            Login login;
#ifdef _WIN32
            strcpy_s(login.userName, sizeof(login.userName), "lyb");
            strcpy_s(login.PassWord, sizeof(login.userName), "lybpasswd");
#else
            strncpy(login.userName, "lyb", sizeof(login.userName));
            strncpy(login.PassWord, "lybpasswd", sizeof(login.PassWord));
#endif
            
            send(_sock, (char*)&login, sizeof(Login), 0);
        }
        else if (0 == strcmp(cmdBuf, "logout")) {
            LogOut logout;
#ifdef _WIN32
            strcpy_s(logout.userName, sizeof(logout.userName), "lyb");
#else
            strncpy(logout.userName, "lyb" ,sizeof(logout.userName));
#endif
            send(_sock, (char*)&logout, sizeof(LogOut), 0);
        }

        else {
            std::cout << "不支持的命令" << std::endl;
        }
    }
}





int
main()
{
#ifdef _WIN32
    /* 建立和启动 Windows socket2.x 环境 */
    WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
    WSADATA dat;                 //数据指针
    (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
                            // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。
#endif
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);  // 需求一个 TCP的流式套接字
    if (INVALID_SOCKET == _sock) {
        std::cout << "创建套接字错误" << std::endl;
        return (errno);
    }
    else
        std::cout << "创建套接字成功" << std::endl;

    sockaddr_in  _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);

    
// 服务器IP地址
#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = inet_addr(CONNECT_IP);
#else
    inet_pton(AF_INET, CONNECT_IP, &_sin.sin_addr);
#endif
    
    
    int ret = connect(_sock, (sockaddr*)&_sin, sizeof(_sin));
    if (SOCKET_ERROR == ret) {
        std::cerr << "与服务器连接错误" << std::endl;
        
        
#ifdef  _WIN32
        closesocket(_sock);
#else
        shutdown(_sock, SHUT_RDWR);
#endif

        
        return (errno);
    }
    else
        std::cout << "与服务器连接成功" << std::endl;

    timeval t = { 1,0 };

    std::thread t1(cmdThread, _sock);      //创建一个线程,并启动, 去执行 cmdThread 函数体的内容
    t1.detach();                             // 设置线程分离
    
    while (g_bRun) {
        fd_set fdReads;
        fd_set fdWrites;
        FD_ZERO(&fdReads);
        FD_ZERO(&fdWrites);
        FD_SET(_sock, &fdReads);
        FD_SET(_sock, &fdWrites);

//macos和linux 的 select 需要的是 套接字描述符的最大值+1
//win 的话, 无所谓, 第一个参数无用.
        int ret = select(_sock + 1, &fdReads, nullptr, nullptr, &t);

        if (ret < 0) {
            std::cout << "select任务结束" << std::endl;
            break;
        }
        if (FD_ISSET(_sock, &fdReads)) {
            FD_CLR(_sock, &fdReads);
            /*--*/
            int ret = processr(_sock);
            if (ret == -1) {
                std::cerr << "服务器出现某些错误 ,中断连接" << std::endl;
                break;
            }
        }
    }

    std::cout << "客户端已结束" << std::endl;

#ifdef _WIN32
    closesocket(_sock);
    /*  清除 Windows socket 环境  */
    WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
    Sleep(1000);
#else
    shutdown(_sock, SHUT_WR);
    sleep(1);
#endif
    return 0;
}
```


