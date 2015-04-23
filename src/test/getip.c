#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
 
#define MAXINTERFACES 16    /* 最大接口数 */
 
int fd;         /* 套接字 */
int if_len;     /* 接口数量 */
struct ifreq buf[MAXINTERFACES];    /* ifreq结构数组 */
struct ifconf ifc;                  /* ifconf结构 */
 
int main(int argc, char **argv)
{
    /* 建立IPv4的UDP套接字fd */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket(AF_INET, SOCK_DGRAM, 0)");
        return -1;
    }
 
    /* 初始化ifconf结构 */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
 
    /* 获得接口列表 */
    if (ioctl(fd, SIOCGIFCONF, (char *) &ifc) == -1)
    {
        perror("SIOCGIFCONF ioctl");
        return -1;
    }
 
    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */
    printf("接口数量:%d\n\n", if_len);
 
    while (if_len > 0) /* 遍历每个接口 */
    {
    	if_len--;
        printf("接口：%s\n", buf[if_len].ifr_name); /* 接口名称 */
 
        /* 获得接口标志 */
        if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf[if_len])))
        {
            /* 接口状态 */
            if (buf[if_len].ifr_flags & IFF_UP)
            {
                printf("接口状态: UP\n");
            }
            else
            {
                printf("接口状态: DOWN\n");
            }
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFFLAGS ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }
 
 
        /* IP地址 */
        if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf[if_len])))
        {
            printf("IP地址:%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }
 
        /* 子网掩码 */
        if (!(ioctl(fd, SIOCGIFNETMASK, (char *) &buf[if_len])))
        {
            printf("子网掩码:%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }
 
        /* 广播地址 */
        if (!(ioctl(fd, SIOCGIFBRDADDR, (char *) &buf[if_len])))
        {
            printf("广播地址:%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }
 
    }//–while end
 
    //关闭socket
    close(fd);
    return 0;
}