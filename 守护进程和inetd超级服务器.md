## 目录

- [关键概念](#关键概念)
- [进程组和会话](#进程组和会话)
- [syslogd 守护进程](#syslogd守护进程)
- [syslog函数](#syslog函数)
- [daemon_init自定义函数](#daemon_init自定义函数)
- [作为守护进程运行的时间获取服务器程序](#作为守护进程运行的时间获取服务器程序)
- [守护进程范例:将时间信息写入文件](#守护进程范例:将时间信息写入文件)
- 







- **`/etc/services` 文件中定义的是服务字段**
- **`/etc/protocols` 文件中定义的是互联网(IP)协议**
- **`/etc/rsyslog.conf`  日志syslogd 守护进程的配置文件**
- 

## 关键概念

- **守护进程(daemon)**
  - **是在后台运行且不与任何控制终端关联的进程**
- **守护进程没有控制终端, 通常源于它们由系统初始化脚本启动.**
  - **也可以亲自脱离与控制终端的关联, 从而避免与作业控制, 终端会话管理, 终端产生信号等发生任何不期望的交互, 也可以避免在后台运行的守护进程非预期地输出到终端.**

- ==**守护进程的启动方法:**==

  - ***1).在系统启动阶段, 许多守护进程由系统初始化脚本启动, 拥有超级用户特权***

    - ==**系统的关键服务的脚本都放在这里`/lib/systemd/system/`**==
      - ==**如果是开机启动的话会创建软连接到`/etc/systemd/system/multi-user.target.wants/` 目录下.**==
        - ==**并且会传递参数给 `/lib/systemd/systemd-sysv-install disable 服务` 这个脚本文件格式`(systemctl 命令来做)`**==
    - ==**用户自定义的脚本应该放在 `/usr/lib/systemd/system/` 目录下**==
      - **如果想开机启动,那么就创建软连接到 `/etc/systemd/system/multi-user.target.wants/`这个目录下**

    - ==**`/etc/systemd/system` 目录下的内容都是 `/lib/systemd/system/` 的软连接, 开机启动**==

  - ***2).许多网络服务器由 `inetd超级服务器`启动.***

    - **inetd由上面的某个开机自动执行脚本启动**
    - **inter 监听网络请求`(Telenet,FTP等)`, 每当有一个请求到达时,启动相应的 实际服务器`(FTP服务器)`**

  - ***3).`cron` 守护进程按照规则定期执行一些程序, 而由它启动执行的程序同样作为守护进程运行.***

    - **循环执行的例行性工作调度 `( 命令 crontab -e )`,说明:[cron 命令和工作方式](https://github.com/solo1d/Linux/blob/master/例行性工作调度 (crontab).md)**
    - **`cron` 自身由 某个开机启动的脚本启动**

  - ***4).守护进程还可以从用户终端或在前台或在后台启动***

    - **这么做往往是为了测试守护程序或重启因某种原因儿终止了的某个守护进程.**

>
>
>==**守护进程的消息应该使用 `syslog 函数` 来输出, 该函数就把消息发送给 `syslogd 守护进程`, 让这个守护进程来处理.**==



- **守护进程(一般称为服务)的特点:**
  - 后台服务进程. 不需要和它进行交互.
  - 独立于控制终端. 不需要终端. 变成会长, 脱离终端, 守护进程.
  - 周期性执行某任务. 执行某个特定操作,行为单一,不会很复杂. 一般为其他进程提供相应的服务.
  - 不受用户登陆注销影响.
  - 一般采用以 d 结尾的名字(服务).



## 进程组和会话

- **进程组**

  - **进程的组长:**
    - **组内的第一个进程 就是组长.**
    - ==**进程组ID == 进程组的组长的ID .**==

- **会话**

  - **多个进程组**

  - ==**创建一个会话注意事项:**==

    - **已经是一个进程组的组长是绝对不能创建会话. 也就是父进程不可以创建会话,但是子进程可以.**
    - **创建会话的进程 将成为新进程组的组长.**
    - **有些linux 版本系统 需要使用 root 权限来执行此操作.**
    - **创建出的新会话 会丢弃原有的控制终端.**
    - **会话 不受用户登陆注销影响, 除非关机和手动杀死进程才行.**

  - ==**一般创建步骤:**==

    - **先 `fork()`,创建出第一个子进程**

    - **杀掉父进程,留下第一个子进程**

    - **第一个子进程调用 `setsid()` 创建会话**

    - **捕获 `SIGHUP`信号, 并忽略它. ( `signal(SIGHUP, SIG_IGN);` )**

    - **再次 `fork()` , 创建出第二个子进程**

    - **杀掉第一个子进程**

    - **第二个子进程切换工作目录 `chdir("/");`** 

    - **重置文件掩码. `umask(0)`**

    - **关闭无用描述符. (  `for (int i =0; i < 64 ; i++) close (i);`  )**

    - **将stdin，stdout和stderr重定向到 /dev/null**

      - ```c
            open ("/dev/null", O_RDONLY);
            open ("/dev/null", O_RDWR);
            open ("/dev/null", O_RDWR);
        ```

    - **开始记录日志. `openlog(pname, LOG_PID, facility);`**
    - **使用第二个子进程去做真正的守护进程的核心工作**
    - 创建完成.

```c
获得进程所属的会话ID:
        pid_t getsid(pid_t pid);

创建一个会话:
        pid_t setsid(void);
```



## syslogd守护进程

> 详细信息可以查看 [分析登陆文件syslog内容](https://github.com/solo1d/Linux/blob/master/认识与分析登陆文件.md)

- **Unix系统中的 `syslogd 守护进程` 通常由某个系统初始化脚本启动的,并且在系统工作期间一直运行**
  - **使用命令 `man rsyslogd ` 可以看到该守护进程的帮助文档**
    - **这个命令是通过 `sudo systemctl status  syslog` 得到的输出中的 `Docs` 选项显示的**
- ==**`syslogd 守护进程` 启动时执行以下步骤**==
  - **1).读取配置文件.`(/etc/rsyslog.conf)`**
    - 该配置文件指定 `syslogd` 守护进程 可能收取的各种日志消息`(log message)` 应该如何处理.
    - **这些消息可能被添加到一个文件  (`/dev/console` 文件是一个特例,它把消息写到控制台上),或别写到指定用户的登录窗口,或转发给另一个主机上的 `syslogd` 进程.**
  - **2).创建一个`Unix域 数据报套接字`, 把它捆绑路径名 `/var/run/log`,或 `/dev/log`**
  - **3).创建一个UPD套接字, 把它捆绑端口 514**
  - **4).打开路径名 `/dev/klog` .来自内核中的任何出错消息看着像是这个设备的输出.**
- ==**`syslogd` 守护进程启动后 就进入一个无限的循环中运行**==
  - **调用 `select` 以等待它的3个描述符(分别来自2,3,4步) 之一变为可读.**
  - **读入日志消息, 并按照配置文件进行处理.** 
  - **如果收到`SIGHUP` 信号,则重读配置文件.`(sudo kill -8 守护进程PID)`**

> **向 `syslogd` 发现消息的方法**
>
> - **创建一个 `Unix 域套接字` ,通过往 `syslogd` 绑定的路径名发生我们的消息以达到发送日志消息的目的.**
> - ==**使用 `syslog函数` 更加便捷**==
> - (弃用):**创建一个UDP套接字, 通过环回地址和端口 514发送我们的消息以达到发送日志消息的目的.**

- ==**可使用 `logger` 命令发送日志消息到 syslogd守护进程, 适合于 shell 脚本**==

## syslog函数

**守护进程没有控制终端, 它们不能把消息 `fprintf` 到 `stderr` 上.**

[这里会有详细的额外内容](https://github.com/solo1d/Linux/blob/master/认识与分析登陆文件.md)

==当某个 系统函数 调用失败时,就可以使用守护进程调用下面的函数来执行错误输出.==

==**当 `syslog 函数`被应用进程首次调用时, 它将创建一个Unix 域数据报套接字,然后调用 connect 连接到由 `syslogd 守护进程` 创建的 Unix域 数据报套接字的众所周知路径名`(例如 /var/run/log)`, 这个头啊家诶字将一直打开,知道进程终止为止.**==

**进程也可以调用`openlog() 和 closelog()` 来替换 `syslog()` 函数**

```c
#include <syslong.h>
#include <unistd.h>
void openlog (const char* ident, int options, int facility); //可以在首次调用 syslog前调用
void closelog(void); //程序不再发送日志消息时调用

void syslog (int priority, const char* message, ... );  // 这个函数会占用一个 Unix域套接字

参数:  ident: 由syslog() 函数冠以每个日志消息之前的字符串,它的值通常是 程序名,
              传入的指针必须是在 堆或全局设定,不可释放. (openlog函数并不保存传入的字符串内容)
    options: 行为, 参数列表如下:
              LOG_CONS    /* 若无法发送到syslogd 守护进程则登记到控制台. */
              LOG_NDELAY  /* 不延迟打开, 立即创建套接字 */
              LOG_PERROR  /* 即发送到 syslogd守护进程, 又登记到 标准错误输出(stderr) */
              LOG_PID     /* 随每个日志消息 登记进程PID */
   facility: 为没有指定的后续 syslog函数 调用指定一个默认值, 通常是指定设施值 (facility)
                可以让 syslog 更加灵活,只需要传入 级别(level)就可以了.
                 
		  priority: 级别(level) 和设施(facility) 两者的组合.  具体参数写在下面两个表格中.
                 级别可随错误性质改变.
                 设施: 用于标识消息发送进程类型的设施.
                     这个选项标识了 该日志会写入的哪个文件中.
                 默认值是   LOG_NOTICE|LOG_USER
       message: 出错消息,和 printf()函数相同,后面还支持 %d 等规范输出. ("pp%d",a)

范例:
 假设 rname() 函数调用意外失败.
   syslog( LOG_INFO|LOG_LOCAL2 , "rename(%s, %s): %m", file1, file2);
                        // %m 代表: 打印strerror（errno）的输出。不需要参数。(linux独有)
```

==`facility` 和 `level` 的目的在于, 允许 `/etc/rsyslog.conf` 文件中统一配置来自同一给定设施的所有消息, 或者统一配置有相同级别的所有消息.==

- **假设`/etc/rsyslog.conf` 配置文件内有如下内容:   ([这里记录了详细的内容](https://github.com/solo1d/Linux/blob/master/认识与分析登陆文件.md))**

  - ```bash
    kern.*        /dev/console
    local7.debyg  /var/log/cisco.log
    
    说明: 这两行指定了所有内核消息登记到控制台, 
          将来自 local7 设施的所有debug 消息添加到文件/var/log/sico.log 的末尾
    ```

|   level 级别   |  值  |                  说明                   |
| :------------: | :--: | :-------------------------------------: |
|   LOG_EMERG    |  0   | 系统不可用,一般由硬件引起  (最高优先级) |
|   LOG_ALERT    |  1   |            必须立即采取行动             |
|    LOG_CRIT    |  2   |                临界条件                 |
|    LOG_ERR     |  3   |                出错条件                 |
|  LOG_WARNING   |  4   |                警告条件                 |
| **LOG_NOTICE** |  5   |     **正常然而重要的条件( 默认值)**     |
|    LOG_INFO    |  6   |                通告消息                 |
|   LOG_DEBUG    |  7   |         调试级消息 (最低优先级)         |

| facility 设施 |            说明             | facility 设施 |          说明           |
| :-----------: | :-------------------------: | :-----------: | :---------------------: |
|   LOG_AUTH    |        安全/授权消息        |  LOG_LOCAL0   |        本地使用         |
| LOG_AUTHPRIV  |     安全/授权消息(私用)     |  LOG_LOCAL1   |        本地使用         |
|   LOG_CRON    |        cron守护进程         |  LOG_LOCAL2   |        本地使用         |
|  LOG_DAEMON   |        系统守护进程         |  LOG_LOCAL3   |        本地使用         |
|    LOG_FTP    |         FTP守护进程         |  LOG_LOCAL4   |        本地使用         |
|   LOG_KERN    |          内核消息           |  LOG_LOCAL5   |        本地使用         |
|    LOG_LPR    |       行式打印机系统        |  LOG_LOCAL6   |        本地使用         |
|   LOG_MAIL    |          邮件系统           |  LOG_LOCAL7   |        本地使用         |
|   LOG_NEWS    |        网络新闻系统         |  LOG_SYSLOG   | 由syslogd内部产生的消息 |
| **LOG_USER**  | **任意的用户级消息 (默认)** |   LOG_UUCP    |        UUCP系统         |

```c
#include <stdio.h>
#include <syslog.h>
int
main(int argc, char* argv[]){
    openlog(argv[0], LOG_CONS|LOG_PID, LOG_MAIL); // 设置 消息类型是 MAIN 邮件系统
    int count = 0;
    while (count < 5) {
        syslog(LOG_INFO, "%d, log info test...", count); // 写入到文件 /var/log/mail.info 中
        ++count;
    }
    closelog();
    return 0;
}
```



## daemon_init自定义函数

==**fork 两次的 目的就是防止终端产生的一些信号让进程退出, 以及确保本守护进程将来即使打开了一个终端设备,也不会自动获得控制终端**==

- ==**守护进程在没有控制终端的环境下运行, 它绝对不会收到来自内核的 `SIGHUP` 信号**==
  - **许多守护进程会将 `SIGHUP` 信号作为来自系统管理员的一个通知, 表示其配置文件已经发生改动, 守护进程应该重新读入其配置文件.**
- ==**守护进程也不会收到来自内核的 `SIGINT`信号 和 `SIGWINCH` 信号.**==
  - **这些信号也可以安全地用作系统管理员的通知手段, 指示守护进程应该做出反应的某种变动已经发生**

```c
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAXFD   64

int  daemon_proc;

int
daemon_init (const char* pname, int facility){
    int    i;
    pid_t  pid;
    
    if ( (pid = fork()) < 0)
        return (-1);     // 没有出现子进程, 出错了
    else if (pid)      // 父进程终止, 因为父进程会得到正数值
        exit(0);
    
    /* 第一个子进程 ,  子进程继承了父进程的 进程组ID,保证了可以调用 setsid()创建会话  */
    if (setsid()  < 0)   // setsid() 创建一个会话,只有子进程才可创建会话, 会话包含多个进程组. 当前进程变成新会话的会话头以及新进程组的进程组头进程 ,从而不再有控制终端
        return (-1);    // 创建进程组失败
    
    signal(SIGHUP, SIG_IGN);  // 获取 信号,并忽略它, 因为会话头进程终止时,其会话中所有进程都会收到 SIGHUP信号.
    
    
    if ( (pid = fork()) < 0)
        return (-1);           //再次创建一个子进程, 第二个子进程
    else if (pid)
        exit(0);              // 第一个子进程退出
    /* fork 两次的 目的就是防止终端产生的一些信号让进程退出, 以及确保本守护进程将来即使打开了一个终端设备,也不会自动获得控制终端 */
    
    /* 第二个子进程 */
    
    daemon_proc = 1;
    chdir("/");   // 切换工作目录, 参数必须是是绝对路径,  fchdir() 参数是相对路径
    
    /* 关闭无用的描述符 */
    for (i =0; i < MAXFD ; i++)
        close(i);
    
    /* 将stdin，stdout和stderr重定向到 /dev/null */
    open ("/dev/null", O_RDONLY);
    open ("/dev/null", O_RDWR);
    open ("/dev/null", O_RDWR);

    openlog(pname, LOG_PID, facility);
    
    /* 进来的是父进程,  出去的是第二个子进程, 并且拥有一个会话*/
    return  0;
}



int main(int argc, const char * argv[]) {

    return 0;
}

```





## 作为守护进程运行的时间获取服务器程序

```c
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/un.h>
#include <signal.h>

#define MAXLINE 1024
#define LISTENQ 1024


#define MAXFD   64

int   daemon_proc;
int   Tcp_listen  (const char *, const char *, socklen_t *);
int   daemon_init (const char* , int );
char* sock_ntop (const struct sockaddr *sa, socklen_t salen);

int
main(int argc, const char * argv[]) {
    int         listenfd, connfd;   // 监听套接字和连接套接字
    socklen_t   addrlen,  len;
    struct  sockaddr* cliaddr;
    char  buff[MAXLINE] = { [0 ... MAXLINE-1] = '\0' };
    time_t  ticks;
    
    if (argc < 2 || argc > 3){
        fprintf(stderr, "usage: daytimetcpsrv2 [ <host> ] <service or port> \n");
        exit(errno);
    }
    
    daemon_init(argv[0], 0);
    
    if (argc == 2 )
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    else
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    
    cliaddr = malloc(addrlen);
    
    for (; ;){
        len = addrlen;
        connfd = accept(listenfd, cliaddr, &len);
        fprintf( stdout, "connection from %s", sock_ntop(cliaddr, len));
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(connfd, buff, strlen(buff));
        close(connfd);
    }
    return 0;
}


int
Tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
    int                listenfd, n;
    const int          on = 1;
    struct addrinfo    hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0){
        fprintf( stderr ,"tcp_listen error for %s, %s: %s",
                 host, serv, gai_strerror(n));
        exit(errno);
    }
    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue;        /* error, try next one */

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break;            /* success */

        close(listenfd);    /* bind error, close and try next one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL){    /* errno from final socket() or bind() */
        fprintf( stderr ,"tcp_listen error for %s, %s", host, serv);
        exit(errno);
    }

    listen(listenfd, LISTENQ);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;    /* return size of protocol address */

    freeaddrinfo(ressave);

    return(listenfd);
}
    
int
daemon_init (const char* pname, int facility){
    int    i;
    pid_t  pid;
    
    if ( (pid = fork()) < 0)
        return (-1);     // 没有出现子进程, 出错了
    else if (pid)      // 父进程终止, 因为父进程会得到正数值
        exit(0);
    printf ("父进程死亡, 第一个子进程存活 PID= %d\n", getpid());
    /* 第一个子进程 ,  子进程继承了父进程的 进程组ID,保证了可以调用 setsid()创建会话  */
    if (setsid()  < 0)   // setsid() 创建一个会话,只有子进程才可创建会话, 会话包含多个进程组. 当前进程变成新会话的会话头以及新进程组的进程组头进程 ,从而不再有控制终端
        return (-1);    // 创建进程组失败
    
    signal(SIGHUP, SIG_IGN);  // 获取 信号,并忽略它, 因为会话头进程终止时,其会话中所有进程都会收到 SIGHUP信号.
    
    
    if ( (pid = fork()) < 0)
        return (-1);           //再次创建一个子进程, 第二个子进程
    else if (pid)
        exit(0);              // 第一个子进程退出
    /* fork 两次的 目的就是防止终端产生的一些信号让进程退出, 以及确保本守护进程将来即使打开了一个终端设备,也不会自动获得控制终端 */
    
    /* 第二个子进程 */
    printf("第一个子进程死亡, 第二个子进程存存活, PID=%d\n", getpid());
    daemon_proc = 1;
    chdir("/home/pi/temp");   // 切换工作目录, 参数必须是是绝对路径,  fchdir() 参数是相对路径
    
    /* 关闭无用的描述符 */
    for (i =0; i < MAXFD ; i++)
        close(i);
    
    /* 将stdin，stdout和stderr重定向到 /dev/null */
    open ("in", O_RDONLY);
    open ("out", O_RDWR);
    open ("err", O_RDWR);
    
    openlog(pname, LOG_PID, facility);
    
    
    /* 进来的是父进程,  出去的是第二个子进程, 并且拥有一个会话*/
    return  0;
}

char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char        portstr[8];
    static char str[128];        /* Unix domain is largest */

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in    *sin = (struct sockaddr_in *) sa;

        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
            return(NULL);
        if (ntohs(sin->sin_port) != 0) {
            snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
            strcat(str, portstr);
        }
        return(str);
    }
/* end sock_ntop */

#ifdef    IPV6
    case AF_INET6: {
        struct sockaddr_in6    *sin6 = (struct sockaddr_in6 *) sa;

        str[0] = '[';
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
            return(NULL);
        if (ntohs(sin6->sin6_port) != 0) {
            snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
            strcat(str, portstr);
            return(str);
        }
        return (str + 1);
    }
#endif

#ifdef    AF_UNIX
    case AF_UNIX: {
        struct sockaddr_un    *unp = (struct sockaddr_un *) sa;

            /* OK to have no pathname bound to the socket: happens on
               every connect() unless client calls bind() first. */
        if (unp->sun_path[0] == 0)
            strcpy(str, "(no pathname bound)");
        else
            snprintf(str, sizeof(str), "%s", unp->sun_path);
        return(str);
    }
#endif

#ifdef    HAVE_SOCKADDR_DL_STRUCT
    case AF_LINK: {
        struct sockaddr_dl    *sdl = (struct sockaddr_dl *) sa;

        if (sdl->sdl_nlen > 0)
            snprintf(str, sizeof(str), "%*s (index %d)",
                     sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
        else
            snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
        return(str);
    }
#endif
    default:
        snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
                 sa->sa_family, salen);
        return(str);
    }
    return (NULL);
}

```



## 守护进程范例:将时间信息写入文件

- 每2秒向 `/Users/ns/time/time.txt` 文件写入当前时间.  Fri jan 25 12:02:44 2019
- 其中包括 : 
  - **获取当前时间, 时间格式化输出 ,打开文件和写入文件, 定时器, 信号捕捉, 信号处理, 文件描述符操作.**

```c
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <semaphore.h>


void dowork(int on){
    // 得到当前的时间  / 将时间写入文件
    time_t curtime;
    time(&curtime);
    // 将时间格式化
    char* pt = ctime(&curtime);
    //将时间写入文件
    int fd = open("/Users/ns/temp/time.txt",O_RDWR| O_APPEND);
    write(fd, pt, strlen(pt)+1);
    close(fd);
}


int main(int argc, char* argv[]){
    pid_t pid = fork();
    if(pid > 0){
        exit(0);
    }else if ( pid < 0)
        fprintf(stderr, "创建第一个子进程失败\n");
		
          // 注册信号捕捉
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = dowork;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
    signal(SIGHUP, SIG_IGN);
  
    pid = fork();
        if(pid > 0){
        exit(0);
    }else if ( pid < 0)
        fprintf(stderr, "创建第二个子进程失败\n");


    if(pid == 0){

        setsid();
        chdir("/Users/ns/temp");  // 修改目录
        umask(0);   // 重置文件掩码

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        // 关闭无用的文件描述符

        // 保证程序处于运行状态.
        
        // 核心操作

        
        struct itimerval val;
        
        // 循环触发时间间隔设定
        val.it_value.tv_usec= 0;
        val.it_value.tv_sec = 2;
        
        //第一次触发时间设定
        val.it_interval.tv_usec = 0;
        val.it_interval.tv_sec = 1;

        // 定时器完成
        setitimer(ITIMER_REAL, &val,NULL);
        

        while(1);

    }else{
        perror("fork error");
        exit(1);
    }


    return 0;

```









