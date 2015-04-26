#include "socast.h"

long unsigned long get_tsf(int *fd){
  if(*fd >= 0){
      char buffer[64];
	  bzero(buffer,64);
	  read(*fd,buffer,64);
#ifdef DEBUG
 	  printf("read tsf = %s tsflen = %d\n",buffer,strlen(buffer));
#endif
      return strtoul(buffer, NULL, 16);
   }
   return 0;
}

long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void print_timestamp(){
  struct timeval tv;
  int ret;
  ret = gettimeofday(&tv,NULL);
  #ifdef DEBUG
    if(!ret){
      printf("seconds = %ld ,useconds = %ld\n",(long) tv.tv_sec, (long)tv.tv_usec);
    }
  #endif
}

void close_playback(snd_pcm_t *PCM_HANDLE){
    snd_pcm_prepare(PCM_HANDLE); 
    snd_pcm_drain(PCM_HANDLE); 
    snd_pcm_close(PCM_HANDLE);
    #ifdef DEBUG
       printf("close_playback\n");
    #endif
}


void Die(char *mess) { perror(mess); exit(1); }


int get_nio_nums(){
   int fd;
  int if_len;     /* 接口数量 */
  struct ifreq buf[MAXINTERFACES];    /* ifreq结构数组 */
  struct ifconf ifc;   
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
        close(fd);
        return -1;
    }
 
    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */
    close(fd);
    return if_len;
}

int get_network_io(so_network_io *buffer[MAXINTERFACES]){
    int fd;
    int if_len;     /* 接口数量 */
    struct ifreq buf[MAXINTERFACES];    /* ifreq结构数组 */
    struct ifconf ifc;   
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
        close(fd);
        return -1;
    }
 
    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */
    int index = 0;
    for(index=0;index<if_len;index++)
    {
        buffer[index] = (so_network_io *)malloc(sizeof(so_network_io));
        strncpy(buffer[index]->name,buf[index].ifr_name,strlen(buf[index].ifr_name));

        printf("interface name：%s\n", buf[index].ifr_name); /* 接口名称 */
 
        /* 获得接口标志 */
        if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf[index])))
        {
            /* 接口状态 */
            if (buf[index].ifr_flags & IFF_UP)
            {
              buffer[index]->is_up = 1;
              printf("status: UP\n");
            }
            else
            {
              buffer[index]->is_up = 0;
              printf("status: DOWN\n");
            }
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFFLAGS ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
 
        /* IP地址 */
        if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf[index])))
        {
            buffer[index]->ip = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;

            printf("IP :%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
        /* 子网掩码 */
        if (!(ioctl(fd, SIOCGIFNETMASK, (char *) &buf[index])))
        {
            buffer[index]->netmask = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;

            printf("NETMASK :%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
        /* 广播地址 */
        if (!(ioctl(fd, SIOCGIFBRDADDR, (char *) &buf[index])))
        {
            buffer[index]->broadcast = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;
            printf("broadcast:%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
    }//–while end
 
    //关闭socket
    close(fd);
    return 0;
}

int get_ip(char *name,struct ifreq *ifr){
     int inet_sock;
      inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
      printf("interface name = %s strlen = %d\n",name,strlen(name));

      strcpy(ifr->ifr_name, name);
      //SIOCGIFADDR标志代表获取接口地址
      if (ioctl(inet_sock, SIOCGIFADDR, ifr) <  0){
          perror("ioctl");
          return -1;
      }
      printf("%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
      return 0;
}
