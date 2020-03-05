## 目录

- [windows下vs2019配置](#windows下vs2019配置)
  - [_DEBUG调试宏](#_DEBUG调试宏)
- [windows下socket通信模版](#windows下socket通信模版)
  - [一对一阻塞式 仅限于windows](#一对一阻塞式)
    - [TCP-服务端一对一阻塞式](#TCP-服务端一对一阻塞式)
    - [TCP-客户端一对一阻塞式](#TCP-客户端一对一阻塞式)
  - [select模型 全平台](#select模型)
    - 









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





## windows下socket通信模版

## 一对一阻塞式

### TCP-服务端一对一阻塞式

```c++

#define WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <iostream>
#include <WinSock2.h>
#include <Windows.h>     // 这两个头文件的文位置不同，会造成编译失败

//#pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去

// 命令
enum CMD
{
    CMD_LOGIN,     /* 登录 */
    CMD_LOGIN_RESULT,    /* LOGIN 返回值,返回消息*/
    CMD_LOGOUT,    /* 登出 */
    CMD_LOGOUT_RESULT,  /* LOGOUT 返回值,返回消息*/
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
    Login() {
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
    LogOut(){
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

constexpr size_t MAXLINE = 128;
const int  DataHeaderLen = sizeof(DataHeader);

int main()
{
    /* 建立和启动 Windows socket2.x 环境 */
    WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
    WSADATA dat;                 //数据指针
    (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
                            // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。

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
    _sin.sin_addr.S_un.S_addr = htonl(ADDR_ANY);         
   // _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 这里的初始化和linux有些区别

    if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR) // Win下特有的错误对比
        std::cout << "bind Not failure" << std::endl;
    else
        std::cout << "bind success" << std::endl;


   //  *3. listen  监听网络端口
    if (listen(_sock, 5) == SOCKET_ERROR )
        std::cout << "listen Not failure" << std::endl;
    else
        std::cout << "listen success" << std::endl;
    

   // *4. accept  等待接收客户端链接
    sockaddr_in clientAddr = {};
    int clientAddrLen = sizeof(clientAddr);
    SOCKET _cSock = INVALID_SOCKET;           // 该宏可以用来初始化和表示  无效的 SOCK套接字
 

    _cSock = accept(_sock, (struct sockaddr*) & clientAddr, &clientAddrLen);
    if (_cSock == INVALID_SOCKET) {
        std::cout << "接收得到无效的客户端" << std::endl;
        return errno;
    }

    std::cout << "新客户端加入 \t "
        << "客户端的IP地址为: " << inet_ntoa(clientAddr.sin_addr) //将sin_addr 结构体转换成字符串。
        << std::endl
        << "客户端的端口为: " << ntohs(clientAddr.sin_port)
        << std::endl;


    // 使用缓冲区接收数据.
    char  szRecv[1024] = {};
    size_t ret;
    while (true) {
        ret = recv(_cSock, szRecv, sizeof(DataHeader), 0);  // 接受客户端发送过来的数据
        if (ret <= 0) {
            std::cout << "客户端发送了终止符,或已退出" << std::endl;
            break;
        }
        DataHeader* header = (DataHeader*)szRecv;
      

 // * 6 处理请求
        switch (header->cmd) {
            case CMD_LOGIN: {
                ret = recv(_cSock, szRecv +sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                Login* login = (Login*)szRecv;
                // 忽略判断用户名和密码
                if (ret <= 0)
                    std::cerr << "对方没有发送任何数据 或关闭了客户端, 这不符合流程,进行强制中断" << std::endl;

                std::cout << "收到客户端发送过来的命令:" << header->cmd
                          << "  ,数据长度为: "<< header->dataLength
                          << "  ,Username: " << login->userName 
                          << "  ,PassWord: " << login->PassWord <<  std::endl;

                LoginResult  ret;
                send(_cSock, (char*)&ret, sizeof(ret), 0);
                break;
            }
            case CMD_LOGOUT: {

                ret = recv(_cSock,  szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
                LogOut* loginout = (LogOut*)szRecv;

                // 忽略判断用户名和密码
                if (ret <= 0)
                    std::cerr << "对方没有发送任何数据 或关闭了客户端, 这不符合流程,进行强制中断" << std::endl;

                std::cout << "收到客户端发送过来的命令:" << header->cmd
                          << "  ,数据长度为: " << header->dataLength
                          << "  ,Username: " << loginout->userName << std::endl;
              
                LogOutResult  ret;
                send(_cSock, (char*)&ret, sizeof(ret), 0);
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
    }

    std::cout << "结束通讯" << std::endl;
//    *7. 关闭套接字 closesocket
    closesocket(_cSock);
    closesocket(_sock);
     /*  清除 Windows socket 环境  */
    WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
    Sleep(10000);
    return 0;
}

```



### TCP-客户端一对一阻塞式

```c++

#define  WIN32_LEAN_AND_MEAN   //  这个宏的作用是： 会尽量避免引用早期会引起冲突的库
#define  _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>     // 这两个头文件的文位置不同，会造成编译失败

//#pragma  comment( lib, "ws2_32.lib")  //明确的指出需要一个动态链接库， 或者在项目属性页->连接器->输入->添加依赖项,将 ws2_32.lib加入进去
// 命令
enum CMD
{
    CMD_LOGIN,     /* 登录 */
    CMD_LOGIN_RESULT,    /* LOGIN 返回值,返回消息*/
    CMD_LOGOUT,    /* 登出 */
    CMD_LOGOUT_RESULT,  /* LOGOUT 返回值,返回消息*/
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
    Login() {
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
    LogOut()  {
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

constexpr size_t MAXLINE = 128;
const int  DataHeaderLen = sizeof(DataHeader);




int main()
{
    /* 建立和启动 Windows socket2.x 环境 */
    WORD ver = MAKEWORD(2, 2);   //创建版本号  2.x的版本， 2是大版本 x是小版本
    WSADATA dat;                 //数据指针
    (void)WSAStartup(ver, &dat);  // winsocket的启动函数 (版本号,  ）;  调用的是动态库，应该加入动态链接库
                            // LP开头的类型，只要把LP去掉就会得到真正的类型，并传入指针即可。

    /*----------
     * 用Socket API 建立简易TCP客户端
     * 1. 建立scoket
     * 2. 连接服务器  connect
     * 3. 接收服务器信息 recv
     * 4. 关闭套接字  closescokets
     *------
     */

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
    _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    // _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 这里的初始化和linux有些区别

    int ret = connect(_sock, (sockaddr*)&_sin, sizeof(_sin));
    if (SOCKET_ERROR == ret) {
        std::cerr << "与服务器连接错误" << std::endl;
        closesocket(_sock);
        return (errno);
    }
    else
        std::cout << "与服务器连接成功" << std::endl;

    
    char buff[MAXLINE] = {};
 
    while (true) {
        std::cin >> buff;
        if (0 == strcmp(buff, "exit")) {
            std::cout << "收到退出命令" << std::endl;
            break;
        }
        else if (0 == strcmp(buff, "login")) {
            // 向服务器发送数据,请求登录
            Login login;
            strcpy_s(login.userName, sizeof(login.userName),"lyd");
            strcpy_s(login.PassWord,sizeof(login.PassWord),  "lydmima");

            send(_sock, (char*)&login, sizeof(login), 0);
            
            //接收服务器返回的数据
            LoginResult loginRet;
            recv(_sock, (char*)&loginRet, sizeof(loginRet), 0);

            // 打印收到的返回数据
            std::cout << "LoginResult: " << loginRet.result << std::endl;
        }
        else if (0 == strcmp(buff, "logout")){
            //向服务器发送数据, 请求退出登录
            LogOut logout ;
            strcpy_s(logout.userName, sizeof(logout.userName),"lyd");
            send(_sock, (char*)&logout, sizeof(logout) , 0);

            // 接收服务器返回的消息.
            LogOutResult logoutRet;
            recv(_sock, (char*)&logoutRet, sizeof(logoutRet), 0);

            // 打印收到的返回数据
            std::cout << "LoginResult: " << logoutRet.result << std::endl;
        }
        else{
            std::cout << "不支持的命令,需要重新输入 : " << buff << std::endl;
            continue;
        }
    }

    std::cout << "客户端已结束" << std::endl;


    closesocket(_sock);
    /*  清除 Windows socket 环境  */
    WSACleanup(); //与WSAStartup() 相匹配， 一个是开，一个是关
    Sleep(10000);
    return 0;
}

```





## select模型

[关于select的详细说明](https://github.com/solo1d/TCP-IP/blob/master/套接字函数和信号处理以及多进程.md)

### server服务端

```c++

```



### client客户端

```c++

```

