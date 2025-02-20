#include <pthread.h>

#include "socket_Sever.h"
#include "control.h"
#include "tcp_socket_interface.h" 
#include "msg_queue.h"
#include "global.h"

static int s_fd = -1;

static int tcpsocket_init(void)
{
    s_fd = socket_init(MY_IP_ADDRESS, MY_PORT);
    
    return -1;
}

static void tcpsocket_final(void)
{
    close(s_fd);
    s_fd = -1;
}

static void* tcpsocket_get(void *arg)
{

    int c_fd = -1;
    int ret = -1;
    struct sockaddr_in c_addr;
    unsigned char buffer[BUF_SIZE];
    mqd_t mqd = -1;
    ctrl_info_t *ctrl_info= NULL;
    int keepalive = 1; // 开启TCP_KEEPALIVE选项
    int keepidle = 10; // 设置探测时间间隔为10秒
    int keepinterval = 5; // 设置探测包发送间隔为5秒
    int keepcount = 3; // 设置探测包发送次数为3次

    pthread_detach(pthread_self());

    printf("%s|%s|%d: s_fd = %d\n", __FILE__, __func__, __LINE__,s_fd);

    if (-1 == s_fd)
    {
        s_fd = tcpsocket_init();
        if (-1 == s_fd)
        {
            printf("tcpsocket_init failed\n");
            pthread_exit(0);
        }
    }
    if (NULL != arg)
        ctrl_info = (ctrl_info_t *)arg;
    
    if(NULL != ctrl_info)
    {
        mqd =  ctrl_info->mqd;
      
    }
    if ((mqd_t)-1 == mqd)
    {
            pthread_exit(0);
    }
 
    memset(&c_addr,0,sizeof(struct sockaddr_in));

    //4. accept
    int clen = sizeof(struct sockaddr_in);
//    printf("%s thread start\n", __func__);
    while (1)
    {
        c_fd = accept(s_fd,(struct sockaddr *)&c_addr,&clen);  
        if (-1 == c_fd)
        {
            continue;
        }

        ret = setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)); // 设置TCP_KEEPALIVE选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }

        ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)); // 设置探测时间间隔选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }

        ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepinterval, sizeof(keepinterval)); // 设置探测包发送间隔选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }

        ret = setsockopt(c_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcount, sizeof(keepcount)); // 设置探测包发送次数选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }
        
        printf("Accepted a connection from %s:%d\n", inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port)); // 打印客户端的IP地址和端口号
       
        while (1) 
        {
            memset(buffer, 0, BUF_SIZE);
            ret = recv(c_fd, buffer, BUF_SIZE, 0);
            printf("%s|%s|%d: 0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
            if (ret > 0)
            {
               
                if(buffer[0] == 0xAA && buffer[1] == 0x55
                && buffer[5] == 0xAA && buffer[4] == 0x55)
                {
                    printf("%s|%s|%d:send 0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
//                    send_message(mqd, buffer, ret);//注意，不要用strlen去计算实际的长度
                    message_queue_send(mqd, buffer, ret);
                }
            }
            else if ( -1 == ret || 0 == ret)
            {
                break;
            }
        }
        
    }
    pthread_exit(0);

}


struct control tcpsocket_control = {
    .control_name = "tcpsocket",
    .init = tcpsocket_init,
    .final = tcpsocket_final,
    .get = tcpsocket_get,
    .set = NULL,
    .next = NULL
};

struct control *add_tcp_socket_to_ctrl_list(struct control *phead)
{//头插法
    return add_control_to_ctrl_list(phead, &tcpsocket_control);
};