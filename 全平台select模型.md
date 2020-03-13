## 目录

- [全平台通讯模版-做了移植操作](#全平台通讯模版-做了移植操作)
  - [TCP-服务端](#TCP-服务端)
  - [TCP-客户端](#TCP-客户端)







## 全平台通讯模版-做了移植操作



### TCP-服务端  文件内容

> **EasyTcpServer.hpp **文件内容

```c++
#ifndef   __EASYTCPSERVER_HPP__
#define   __EASYTCPSERVER_HPP__


#ifdef _WIN32  //只有在Windows 下,才会定义这个宏. 出于移植考虑, 应该使用这种判别式, 以使用头文件和宏
#define  WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去


#define   FD_SETSIZE  1024
#include <WinSock2.h>
#include <Windows.h>     // 这两个头文件的文位置不同，会造成编译失败

#else    // 这个是 Unix和Linux 下才有的头文件, 也有用于方便移植的宏
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define  SOCKET  int       //兼容 windows ,将 socket套接字的int,变更为 SOCKET来表示,     两个平台的内容相同
#define  INVALID_SOCKET  (SOCKET)(~0)     //取反的0, 也就是-1, 用于判别 socket套接字的创建错误
#define  SOCKET_ERROR            (-1)     // -1, 用于判别 connect 的连接错误
#endif

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <vector>
#include <algorithm>
#include "MessageHeader.hpp"


#define MAC_IP    "10.0.0.94"
#define WIN_IP    "10.0.0.95"
#define LIN_IP    "10.0.0.1"
#define LOCAL_IP  "127.0.0.1"
#define ALY_IP    "47.240.114.70"
#define CONNECT_IP  LOCAL_IP      //服务器的IP
#define  SERVER_PORT  4567


#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE  10240   // 消息缓冲区长度
#endif

class ClientSocket {
public:
    ClientSocket(const SOCKET _sockfd = INVALID_SOCKET) :_sockfd(_sockfd), _szMsggBuf(""), _lastPos(0) {}

    ~ClientSocket() {}


    SOCKET& sockfd(void) {
        return _sockfd;
    }


    char* msgBuf(void) {
        return _szMsggBuf;
    }


    int& getLastPos(void) {
        return _lastPos;
    }


    void setLastPos(const int& pos) {
        _lastPos = pos;
    }

private:
    SOCKET _sockfd; // 套接字

     // 使用缓冲区接收数据, 接收缓冲区
 //   char  _szRecv[RECV_BUFF_SIZE];

    //第二缓冲区, 消息缓冲区
    char  _szMsggBuf[RECV_BUFF_SIZE * 2];
    int    _lastPos;    // 第二缓冲区 目前存在的数据长度

};


int NewScok = 0;


class EasyTcpServer
{
private:
    SOCKET _sock; // 监听套接字
    std::vector<ClientSocket*> _clients; // 存储已连接的客户端 SOCK 套接字
    char  _szRecv[RECV_BUFF_SIZE];
    int count;
public:
    EasyTcpServer() :_sock(INVALID_SOCKET), count(0) {
        _clients.clear();
    }

    ~EasyTcpServer() {
        Close();
    }

    // 初始化 Socket
    SOCKET InitSocket(void) {

        // 判断一下, 是否已经建立了一个套接字, 如果建立了,应该关闭它,并断开连接.
        if (_sock != INVALID_SOCKET) {
            std::cout << "< socket = " << _sock << " >"
                << "关闭之前的套接字(或已连接的套接字),并重新建立一个新的套接字" << std::endl;
            Close();
        }

#ifdef _WIN32
        /* 建立和启动 Windows socket2.x 环境 */
        WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
        WSADATA dat;                 //数据指针
        (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
        // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。
#endif
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // 需求一个 TCP的流式套接字
        if (INVALID_SOCKET == _sock) {
            std::cerr << "创建套接字错误" << std::endl;
        }
        else
            std::cout << "创建套接字 < " << _sock << " > 成功" << std::endl;
        return _sock;
    }

    //绑定IP和端口号
    int Bind(const char* ip, const uint16_t port) {
        if (_sock == INVALID_SOCKET) {
            InitSocket();
        }
        //  2. bind 绑定用于接收客户端连接的网络端口
        sockaddr_in  _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);

        // 判断IP是否传入的是空值.
        if (ip == nullptr) {
#ifdef _WIN32
            _sin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
            _sin.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
        }
        else {
#ifdef _WIN32
            _sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
            inet_pton(AF_INET, ip, &_sin.sin_addr);  // INADDR_ANY
#endif
        }
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
        if (-1 == ret) {      // Win下特有的错误对比
            std::cout << "端口号 " << ntohs(port) << " 绑定错误" << std::endl;
            Close();
            return -1;
        }
        else
            std::cout << "端口绑定成功" << std::endl;
        return ret;
    }

    //监听端口号
    int Listen(const int number) {
        //  *3. listen  监听网络端口
        int ret = listen(_sock, number);
        if (SOCKET_ERROR == ret)
            std::cout << "listen Not failure" << std::endl;
        else
            std::cout << "listen success" << std::endl;

        return ret;
    }

    //接收客户端连接
    SOCKET Accept(void) {
        // *4. accept  等待接收客户端链接
        SOCKET cSock = INVALID_SOCKET;           // 该宏可以用来初始化和表示  无效的 SOCK套接字
        sockaddr_in clientAddr = {};    //接收客户端信息的结构体, 下面是长度信息.
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);         //win 下是 int
#else
        socklen_t clientAddrLen = sizeof(clientAddr);  // macos和linux是 unsigned int
#endif
        cSock = accept(_sock, (struct sockaddr*) & clientAddr, &clientAddrLen);
        if (cSock == INVALID_SOCKET) {
            std::cout << "<socket=" << _sock << ">接收得到无效的客户端" << std::endl;
        }
        else {
            //            NewUserJoin userjoin;
            //            SendDataToALL(&userjoin);
            _clients.push_back(new ClientSocket(cSock));
            std::cout << "新客户端 " <<  ++NewScok <<  " 加入  < " << cSock << " >\t ";
/*========================================================================================*/
#ifdef _WIN32
            std::cout << "客户端的IP地址为: " << inet_ntoa(clientAddr.sin_addr); //将sin_addr
#else
            char client_addr_arr[INET6_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &clientAddr.sin_addr, client_addr_arr, INET6_ADDRSTRLEN);
            std::cout << "客户端的IP地址为: " << client_addr_arr;
            //将sin_addr结构体转换成字符串。
#endif
            std::cout << std::endl << "客户端的端口为: " << ntohs(clientAddr.sin_port) << std::endl;
        }
        return cSock;
    }

    //关闭 Socket
    void Close(void) {
        if (INVALID_SOCKET != _sock) {
            std::cout << "清理sock< " << _sock << ">  环境和结束通讯,客户端已结束" << std::endl;
            for (int n = static_cast<int>(_clients.size()) - 1; n >= 0; n--) {
#ifdef _WIN32
                closesocket(_clients.at(n)->sockfd());
#else
                shutdown(_clients.at(n)->sockfd(), SHUT_WR);
#endif
                delete _clients.at(n);
            }

#ifdef _WIN32
            closesocket(_sock);
            /*  清除 Windows socket 环境  */
            WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
#else
            shutdown(_sock, SHUT_WR);
#endif
            _clients.clear();
            std::cout << "结束通讯,关闭所有套接字" << std::endl;
            _sock = INVALID_SOCKET;
        }
        else
            std::cout << "Close(sock) 已经清理过了." << std::endl;
    }


    //处理网络消息
    bool OnRun() {
        if (isRun()) {
            SOCKET _maxSock = _sock;
            fd_set fdRead;
            fd_set fdWrite;
            fd_set fdExp;
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExp);
            FD_SET(_sock, &fdRead);
            FD_SET(_sock, &fdWrite);
            FD_SET(_sock, &fdExp);

            for (int n = static_cast<int>(_clients.size()) - 1; n >= 0; n--) {
                FD_SET(_clients.at(n)->sockfd(), &fdRead);
                FD_SET(_clients.at(n)->sockfd(), &fdWrite);
                FD_SET(_clients.at(n)->sockfd(), &fdExp);

#ifndef _WIN32 /* win 下 ,该代码无用, Mac 和 Linux 则至关重要*/
                _maxSock < _clients.at(n)->sockfd() ? _maxSock = _clients.at(n)->sockfd() : _maxSock;
#endif
            }
            timeval t = { 1,0 };
            // _In_ 代表传入参数,  _Inout 代表传入传出参数, _Inout_opt_ 传入传出操作参数
            int ret = select(_maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
            //等待描述符出现变化, 或者出错.

            if (ret < 0) {
                std::cout << "select 任务结束" << std::endl;
                Close();
                return false;
            }

            //可读, 新客户端来了新链接
            if (FD_ISSET(_sock, &fdRead)) {
                FD_CLR(_sock, &fdRead);
                Accept();
            }


            //已连接客户端向服务器发送了消息
            for (int n = static_cast<int>(_clients.size()) - 1; n >= 0; n--) {
                if (FD_ISSET(_clients.at(n)->sockfd(), &fdRead)) {
                    FD_CLR(_clients.at(n)->sockfd(), &fdRead);
                    if (-1 == RecvData(_clients.at(n))) {
                        std::cout << "通讯结束,关闭这个套接字" << _clients.at(n)->sockfd() << std::endl;
#ifdef _WIN32
                        closesocket(_clients.at(n)->sockfd());
#else
                        shutdown(_clients.at(n)->sockfd(), SHUT_RDWR);
#endif
                        delete  _clients.at(n);
                        auto iter = _clients.begin() + n;
                        _clients.erase(iter);
                    }
                }
            }
            return true;
        }
        return false;
    }

    // 判读套接字是否在运行中, 运行中返回true , 没有运行则返回 false
    bool isRun() {
        return _sock != INVALID_SOCKET && SOCKET_ERROR != _sock;
    }

    //接收数据, 处理粘包, 拆分包.
    int RecvData(ClientSocket* pClient) {
        // 使用缓冲区接收数据.
#ifdef _WIN32
        int ret;
#else
        ssize_t ret; // ssize_t 是 long , size_t 是 unsigned long
#endif
        ret = recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);  // 接受客户端发送过来的数据
        if (ret <= 0) {
            std::cerr << "客户端<socket=" << pClient->sockfd() << "> 已退出, 任务结束\n" << std::endl;
            return -1;
        }

        // 从接收缓冲区 拷贝到 消息缓冲区. (10倍差距),有多少拷贝多少.
        memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, ret);
        pClient->setLastPos(pClient->getLastPos() + static_cast<int>(ret));

        //判断 消息缓冲区内的数据长度大于消息头, 这时就可以知道当前消息体的长度.
        while (pClient->getLastPos() >= sizeof(DataHeader)) {
            DataHeader* header = reinterpret_cast<DataHeader*>(pClient->msgBuf());
            //判断 消息缓冲区的数据长度大于消息长度
            if (pClient->getLastPos() >= header->dataLength) {
                //剩余未处理消息缓存区数据的长度, 先行计算
                auto nSize = pClient->getLastPos() - header->dataLength;
                OnNetMsg(pClient->sockfd(), header);    //处理消息
                memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize); // 将第二缓存数据前移
                pClient->setLastPos(nSize);     // 需要交还给 第二缓冲区目前存在的数据长度 的数值,也就是未处理的数据
            }
            else {
                // 消息没有发全,等待数据到达完全再来循环. 剩余数据不足一条完整的消息
                break;
            }
        }
        return 0;
    }

    // 响应网络消息
    virtual void OnNetMsg(SOCKET cSock, DataHeader* header) {
        if (isRun()) {
            switch (header->cmd) {
            case CMD_LOGIN: {
                //                    Login* login = (Login*)header;
                //                    std::cout << "收到客户端 < " << cSock << " >发送过来的命令:" << login->cmd << "  ,数据长度为: " << login->dataLength << "  ,Username: " << login->userName << "  ,PassWord: " << login->PassWord << std::endl;
//                LoginResult  ret;
//                SendData(cSock, &ret);
                break;
            }
            case CMD_LOGOUT: {
                //                  LogOut* loginout = (LogOut*)header;
                //                    std::cout << "收到客户端< " << cSock << " >发送过来的命令:" << loginout->cmd << "  ,数据长度为: " << loginout->dataLength << "  ,Username: " << loginout->userName << std::endl;

//                LogOutResult  ret;
//                SendData(cSock, &ret);
                break;
            }
            default: {
                std::cout << std::endl << "<soclet= " << _sock <<"> 收到服务器消息: 未定义消息 ,数据长度为" << header->dataLength << std::endl;
                                  //  DataHeader err;
                                  //  SendData(_cSock, &err);
                break;
            }
            }
        }
    }

    //发送给指定SOCKET客户端数据
    int SendData(SOCKET cSock, const DataHeader* header) {
        if (isRun() && header)
            return static_cast<int>(send(cSock, (const char*)header, header->dataLength, 0));
        else
            return SOCKET_ERROR;
    }

    // 群发消息
    void SendDataToALL(const DataHeader* header) {
        for (int n = static_cast<int>(_clients.size()) - 1; n >= 0; n--) {
            SendData(_clients.at(n)->sockfd(), header);
        }
    }
};

#endif
```





> **MessageHeader.hpp**  文件内容

```c++
#ifndef __MESSAGEHEADER_HPP__
#define __MESSAGEHEADER_HPP__

// 消息协议

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
    DataHeader():dataLength(sizeof(DataHeader)),cmd(CMD_ERROR){}
    short dataLength;   //数据长度
    short cmd;          //命令, 使用 enum CMD 枚举,来执行命令
};


// 数据包的  包体
 /* 登录数据结构 */
struct Login :public DataHeader
{
    Login(){
        DataHeader::dataLength = sizeof(Login);
        DataHeader::cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
    char data    [932];
};

/* 返回登陆是否成功的结构体 */
struct LoginResult : public DataHeader
{
    LoginResult() :result(0) {
        DataHeader::dataLength = sizeof(LoginResult);
        DataHeader::cmd = CMD_LOGIN_RESULT;
    }
    int result;    /* 登陆结果 */
    char data[992];
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

#endif
```



> **main.cpp** 文件内容

```c++
#include "EasyTcpServer.hpp"

bool g_bun = true;

void
cmdThread() {
    char cmdBuf[256] = {};
    while (g_bun) {
        std::cin >> cmdBuf;
        cmdBuf[strlen(cmdBuf)] = '\0';
        if (0 == strcmp(cmdBuf, "exit")) {
            std::cout << "退出 cmdThread 线程" << std::endl;
            g_bun = false;
        }
        else {
            std::cout << "不支持的命令,需要重新输入" << std::endl;
        }
        memset(cmdBuf, 0, sizeof(cmdBuf));
    }
}

    
int
main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, SERVER_PORT);
    server.Listen(64);
    
    std::thread t1(cmdThread);      //创建一个线程,并启动, 去执行 cmdThread 函数体的内容
    t1.detach();                             // 设置线程分离
    
    while(server.isRun() && g_bun){
        server.OnRun();
    }
    server.Close();
    std::cout << "已退出" << std::endl;
    getchar();
    return 0;
}
```







### TCP-客户端

> **MessageHeader.hpp** 文件内容

```c++
#ifndef __MESSAGEHEADER_HPP__
#define __MESSAGEHEADER_HPP__

// 消息协议

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
    DataHeader():dataLength(sizeof(DataHeader)),cmd(CMD_ERROR){}
    short dataLength;   //数据长度
    short cmd;          //命令, 使用 enum CMD 枚举,来执行命令
};


// 数据包的  包体
 /* 登录数据结构 */
struct Login :public DataHeader
{
    Login(){
        DataHeader::dataLength = sizeof(Login);
        DataHeader::cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
    char data    [932];
};

/* 返回登陆是否成功的结构体 */
struct LoginResult : public DataHeader
{
    LoginResult() :result(0) {
        DataHeader::dataLength = sizeof(LoginResult);
        DataHeader::cmd = CMD_LOGIN_RESULT;
    }
    int result;    /* 登陆结果 */
    char data[992];
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

#endif
```

### 

> **EasyTcpClient.hpp** 文件内容

```c++
#ifndef   __EASYTCPCLIENT_HPP__
#define   __EASYTCPCLIENT_HPP__


    #ifdef   _WIN32  //只有在Windows 下,才会定义这个宏. 出于移植考虑, 应该使用这种判别式, 以使用头文件和宏
    #define   WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
    #define  _WINSOCK_DEPRECATED_NO_WARNINGS
    #pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去

    #define   FD_SETSIZE  1024
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
#include "MessageHeader.hpp"

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE  10240   // 消息缓冲区长度
#endif

class EasyTcpClient {
private:
    // 缓冲区 区域最小单元长度
    SOCKET _sock;
    // 使用缓冲区接收数据, 接收缓冲区
    char  _szRecv[RECV_BUFF_SIZE] = {};
    
    //第二缓冲区, 消息缓冲区
    char  _szMsggBuf[RECV_BUFF_SIZE * 2] = {};
    int    _lastPos;    // 第二缓冲区 目前存在的数据长度
    
public:
    EasyTcpClient() :_sock(INVALID_SOCKET), _lastPos(0) {}
    
    // 虚析构函数
    virtual  ~EasyTcpClient()
    {
        Close();
    }
    
    /* =========================================================================== */
    //初始化 socket, 以及 SOCKET网络环境
    void initSocket()
    {
        // 判断一下, 是否已经建立了一个套接字, 如果建立了,应该关闭它,并断开连接.
        if (_sock != INVALID_SOCKET) {
            std::cout << "< socket = " << _sock << " >"
            << "关闭之前的套接字(或已连接的套接字),并重新建立一个新的套接字" << std::endl;
            this->Close();
        }
        
#ifdef _WIN32
        /* 建立和启动 Windows socket2.x 环境 */
        WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
        WSADATA dat;                 //数据指针
        (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
        // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。
#endif
        
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // 需求一个 TCP的流式套接字
        if (INVALID_SOCKET == _sock) {
            std::cerr << "创建套接字错误" << std::endl;
        }
        else
            std::cout << "创建套接字 < " << _sock << " > 成功" << std::endl;
    }
    
    /* =========================================================================== */
    
    
    //连接服务器,参数: IP地址, 网络字节序的端口号(unisgned short)
    int Connect(const char* ip, const u_short port)
    {
        if (INVALID_SOCKET == _sock) {
            std::cout << "!由于:<socket = INVALID_SOCKET> ,自动调用 initSocket() 进行初始化~! " << std::endl;
            initSocket();
        }
        sockaddr_in  _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);
        
        // 服务器IP地址
#ifdef _WIN32
        _sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
        inet_pton(AF_INET, ip, &_sin.sin_addr);
#endif
        
        int ret = connect(_sock, (sockaddr*)&_sin, sizeof(_sin));
        if (SOCKET_ERROR == ret) {
            std::cerr << "!(" << _sock << ")与服务器< " << ip << " : " << port << " >连接失败~!" << std::endl;
            
#ifdef  _WIN32
            closesocket(_sock);
#else
            shutdown(_sock, SHUT_RDWR);
#endif
            return (errno);
        }
        else {
            struct sockaddr_in local_sock;
#ifdef _WIN32
            int  local_sock_len = sizeof(local_sock);
            getsockname(AF_INET, (sockaddr*)&local_sock, &local_sock_len);
            const char* ip_local_arr = inet_ntoa(local_sock.sin_addr);
#else
            char ip_local_arr[INET6_ADDRSTRLEN] = {};
            socklen_t  local_sock_len = sizeof(local_sock);
            getsockname(_sock, (sockaddr*)&local_sock, &local_sock_len);
            inet_ntop(AF_INET, &local_sock.sin_addr.s_addr, ip_local_arr, INET6_ADDRSTRLEN);
#endif
            std::cout << "\t 本地IP:" << ip_local_arr << ", 本地端口:" << ntohs(local_sock.sin_port) << std::endl;
            std::cout << "\t(" << _sock << ")与服务器< " << ip << " : " << port << " >连接成功;" << std::endl;
        }
        
        return 0;
    }
    
    
    /* =========================================================================== */
    
    //关闭 socket, 并关闭已连接的套接字, 并且清除SOCKET 网络环境
    void Close()
    {
        if (INVALID_SOCKET != _sock) {
            std::cout << "清理sock< " << _sock << ">  环境和结束通讯,客户端已结束" << std::endl;
#ifdef _WIN32
            closesocket(_sock);
            /*  清除 Windows socket 环境  */
            WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
#else
            shutdown(_sock, SHUT_WR);
#endif
            _sock = INVALID_SOCKET;
        }
    }
    
    
    
    /* =========================================================================== */
    
    //处理网络消息  (应该写纯虚函数)
    virtual bool OnRun()
    {
        if (isRun()) {
            fd_set fdReads;
            fd_set fdWrites;
            fd_set fdExps;
            FD_ZERO(&fdReads);
            FD_ZERO(&fdWrites);
            FD_ZERO(&fdExps);
            FD_SET(_sock, &fdReads);
            FD_SET(_sock, &fdWrites);
            FD_SET(_sock, &fdExps);
            
            timeval t = { 0,0 };
            //macos和linux 的 select 需要的是 套接字描述符的最大值+1
            //win 的话, 无所谓, 第一个参数无用.
            int ret = select(_sock + 1, &fdReads, nullptr, nullptr, &t);
            //            std::cout << "select ret = " << ret  << " ,count = " << (++count) << std::endl;
            
            if (ret < 0 || INVALID_SOCKET == _sock || SOCKET_ERROR == _sock) {
                std::cout << "<select = " << _sock << " > 任务结束" << std::endl;
                Close();
                return false;
            }
            if (FD_ISSET(_sock, &fdReads)) {
                FD_CLR(_sock, &fdReads);
                /*--*/
                int ret = RecvData(_sock);
                if (ret == -1) {
                    std::cerr << "<select = " << _sock << " >服务器出现某些错误 ,中断连接" << std::endl;
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    
    
    /* =========================================================================== */
    // 判读套接字是否在运行中, 运行中返回true , 没有运行则返回 false
    bool isRun()
    {
        return _sock != INVALID_SOCKET && SOCKET_ERROR != _sock;
    }
    
    
    
    /* =========================================================================== */
    // 接收数据. 收数据的时候要处理 粘包,拆分包 的问题
    int    RecvData(SOCKET Csock) {
#ifdef _WIN32
        int ret;
#else
        ssize_t ret;   // ssize_t  是 long 类型,  size_t 是 unsigned long
#endif
        ret = recv(Csock, _szRecv, sizeof(_szRecv), 0);  // 接受服务器发送过来的数据
        if (ret <= 0) {
            std::cout << "<soclet= " << Csock << ">与服务器断开连接, 任务结束" << std::endl;
            return -1;
        }
        
        // 从接收缓冲区 拷贝到 消息缓冲区. (10倍差距),有多少拷贝多少.
        memcpy(_szMsggBuf + _lastPos, _szRecv, ret);
        _lastPos += ret;
        
        //判断 消息缓冲区内的数据长度大于消息头, 这时就可以知道当前消息体的长度.
        while (_lastPos >= sizeof(DataHeader)) {
            DataHeader* header = reinterpret_cast<DataHeader*>(_szMsggBuf);
            //判断 消息缓冲区的数据长度大于消息长度
            if (_lastPos >= header->dataLength) {
                //剩余未处理消息缓存区数据的长度, 先行计算
                auto nSize = _lastPos - header->dataLength;
                OnNetMsg(header);    //处理消息
                memcpy(_szMsggBuf, _szMsggBuf + header->dataLength, nSize); // 将第二缓存数据前移
                _lastPos = nSize;     // 需要交还给 第二缓冲区目前存在的数据长度 的数值,也就是未处理的数据
            }
            else {
                // 消息没有发全,等待数据到达完全再来循环. 剩余数据不足一条完整的消息
                break;
            }
        }
        return 0;
    }
    
    
    
    
    
    /* =========================================================================== */
    //发送数据
    int SendData(const DataHeader* header)
    {
        if (isRun() && header)
            return static_cast<int>(send(_sock, (const char*)header, header->dataLength, 0));
        else
            return SOCKET_ERROR;
    }
    
    
    /* =========================================================================== */
    // 响应网络消息
    virtual void OnNetMsg(DataHeader* header)
    {
        
        // * 6 处理请求
        switch (header->cmd) {
            case CMD_LOGIN_RESULT: {   // 期待服务器返回登录消息
                //                LoginResult* login = (LoginResult*)header;
                //                std::cout << std::endl << "<soclet= " << _sock << "> 收到服务器消息: CMD_LOGIN_RESULT ,数据长度为" << login->dataLength << std::endl;
                break;
            }
            case CMD_LOGOUT_RESULT: {   // 期待服务器返回 退出登录的消息
                //                LogOutResult* loginout = (LogOutResult*)header;
                //                std::cout << std::endl << "<soclet= " << _sock<< "> 收到服务器消息: CMD_LOGOUT_RESULT ,数据长度为" << loginout->dataLength << std::endl;
                break;
            }
            case CMD_NEW_USER_JOIN: {   // 有新的连接 加入到服务器的消息
                //                NewUserJoin* userJoin = (NewUserJoin*)header;
                //                std::cout << std::endl << "<soclet= " << _sock << "<soclet= " << _sock << " >收到服务器消息: CMD_NEW_USER_JOIN ,数据长度为" << userJoin->dataLength << std::endl;
                break;
            }
            case CMD_ERROR: {  // 收到了服务器错误的消息
                DataHeader* userJoin = (DataHeader*)header;
                std::cout << std::endl << "<soclet= " << _sock << "> 收到服务器消息: CMD_ERROR ,数据长度为" << userJoin->dataLength << std::endl;
                break;
            }
            default: {
                std::cout << std::endl << "<soclet= " << _sock << "> 收到服务器消息: 未定义消息 ,数据长度为" << header->dataLength << std::endl;
                break;
            }
        }
    }
    
    SOCKET ReturnSock(void)
    {
        return _sock;
        
    }
};
#endif
```





> **main.cpp**  文件内容

```c++
#include "EasyTcpClient.hpp"
#include <limits>
#include <istream>
#include <cstring>
#include <stdio.h>

    
#define MAC_IP    "10.0.0.94"
#define WIN_IP    "10.0.0.95"
#define LIN_IP    "10.0.0.1"
#define LOCAL_IP  "127.0.0.1"
#define ALY_IP    "47.240.114.70"
#define CONNECT_IP  LOCAL_IP      //服务器的IP
#define  SERVER_PORT  4567



bool g_bun = true;

void
cmdThread() {
    char cmdBuf[10] = {};
    while (g_bun) {
        std::cin >> cmdBuf;
        cmdBuf[strlen(cmdBuf)] = '\0';
        if (0 == strcmp(cmdBuf, "exit")) {
            std::cout << "退出 cmdThread 线程" << std::endl;
            g_bun = false;
        }
        else {
            std::cout << "不支持的命令,需要重新输入" << std::endl;
        }
        memset(cmdBuf, 0, sizeof(cmdBuf));
    }
}
    
int
main()
{
#ifdef _WIN32
    const int cCount = FD_SETSIZE-1;  //win 下有64个.实际可用 63个
#else
    const int cCount = FD_SETSIZE-5; // linux 下有1024 ,减去 0,1,2,3 和监听.实际可用 1024-5=1019.(以服务器为准.)
#endif
    EasyTcpClient* client = new EasyTcpClient[cCount];
    for (int n = 0; n < cCount ; n++){
        if (!g_bun){
            return 0;
        }
        client[n].initSocket();
        client[n].Connect(CONNECT_IP, SERVER_PORT);
    }
    std::thread t1(cmdThread);      //创建一个线程,并启动, 去执行 cmdThread 函数体的内容
    t1.detach();                             // 设置线程分离
    
    Login login;
#ifdef _WIN32
    strcpy_s(login.userName, sizeof(login.userName), "lyb");
    strcpy_s(login.PassWord, sizeof(login.userName), "lybpasswd");

#else
    strncpy(login.userName, "lyb", sizeof(login.userName));
    strncpy(login.PassWord, "lybpasswd", sizeof(login.PassWord));
#endif
    
    while (g_bun){
        for (int n =0; n < cCount && g_bun ;n++){
            client[n].SendData(&login);
//            client[n].OnRun();
        }
    }

    for (int n =0; n < cCount ;n++){
        client[n].Close();
    }
    delete[] client;
    return 0;
}
```

