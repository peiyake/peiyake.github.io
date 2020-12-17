#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#define uint    unsigned int
#define ushort  unsigned short
#define uchar   unsigned char

#define IP_FMT      "%u.%u.%u.%u"
#define IP_ARG(x)   ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2],((unsigned char *)(x))[3]

#define MAC_FMT     "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x)  ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2], \
                    ((unsigned char *)(x))[3],((unsigned char *)(x))[4],((unsigned char *)(x))[5]

#define PROMISCUOUS_ENABLE      1
#define PROMISCUOUS_DISABLE     0

extern void print_buf(uchar *buf,int len,int flag);

#define print_eth_buf(buf,len) print_buf(buf,len,1)
#define print_iph_buf(buf,len) print_buf(buf,len,0)

static struct option longopts[] = {
    {"interface",       required_argument,  NULL,   'i'},
    {"promiscuous",     no_argument,        NULL,   'p'},
    {"help",            no_argument,        NULL,   'h'},
    {0,0,0,0}
};

uint promiscuous = PROMISCUOUS_DISABLE;
char device[32] = {"any"};

void print_buf(uchar *buf,int len,int flag)
{
    int i,eth_offset;
    uchar *ptr = buf;
    struct ethhdr *l2h;
    struct iphdr *iph;
    struct icmphdr *icmph;
    struct udphdr *udph;
    struct tcphdr *tcph;
    

    eth_offset = flag ? ETH_HLEN : 0;

    if(flag){
        l2h = (struct ethhdr *)buf;
        fprintf(stdout,""MAC_FMT"->"MAC_FMT",0x%x,",
                    MAC_ARG(l2h->h_source),
                    MAC_ARG(l2h->h_dest),
                    ntohs(l2h->h_proto));
    }

    iph = (struct iphdr *)(buf + eth_offset);
    fprintf(stdout,""IP_FMT"->"IP_FMT",%u,",
                    IP_ARG(&iph->saddr),
                    IP_ARG(&iph->daddr),
                    iph->protocol);

    if(iph->protocol == IPPROTO_UDP || iph->protocol == IPPROTO_TCP){
        udph = (struct udphdr *)(buf + eth_offset + (iph->ihl << 2));
        fprintf(stdout,"%s:%u->%u\n",
                    (iph->protocol == IPPROTO_UDP) ? "udp" : "tcp",
                    ntohs(udph->source),
                    ntohs(udph->dest));
    }else{
        fprintf(stdout,"\n");
    }
    
    for(i = 0;i < len;i ++){
        if((i % 32) != 31){
            fprintf(stdout,"%02X ",*ptr ++);
        }else{
            fprintf(stdout,"%02X\n",*ptr ++);
        }
    }
    if(i != 31) 
        fprintf(stdout,"\n");

    return ;
}


void help(char *name)
{
    printf("Usage: %s [option]\n",name);
    printf(" option\n");
    printf("    -i|--interface          dump interface\n");
    printf("    -p|--promiscuous        enable promiscuous\n");
    printf("    -h|--help               print help\n");
    printf("\n");
    return ;
}
int args_handle(int argc,char **argv)
{
    int c,ret;
    int option_index = 0;
    ret = 0;
    while(1){
        c = getopt_long(argc, argv, "hpi:",longopts, &option_index);
        if(-1 == c) break;
        
        switch(c){
        case 'i':
            strcpy(device,optarg);
            break;
        case 'p':
            promiscuous = PROMISCUOUS_ENABLE;
            break;
        default:
            ret = 1;
            break;
        }
    }
    return ret;
}

int set_promiscuous(int sock,char *ifname)
{
     struct packet_mreq mreq;
     int ret;

     memset(&mreq,0,sizeof(mreq));

     mreq.mr_ifindex = if_nametoindex(ifname);
    if(mreq.mr_ifindex == 0){
        fprintf(stderr,"set_promiscuous error '%s'\n",strerror(errno));
        return 1;
    }

    /* 配置接口混杂模式 */
    mreq.mr_type = PACKET_MR_PROMISC;

    /* 配置接口加入指定组播组,此时需要设置 mr_alen 和 mr_address*/
    //mreq.mr_type = PACKET_MR_MULTICAST;

    /* 接收所有组播组消息 */
    //mreq.mr_type = PACKET_MR_MULTICAST;

    /*这里我们仅设置混杂模式*/

    ret = setsockopt(sock,SOL_PACKET,PACKET_ADD_MEMBERSHIP,(void *)&mreq,sizeof(mreq));
    if(-1 == ret){
        fprintf(stderr,"add %s PROMISC fail, '%s'\n",ifname,strerror(errno));
    }

    return 0;
}

int bind_device(int sock,char *ifname)
{
    struct sockaddr_ll dev;
    uchar mac[ETH_ALEN] = {0};
    char *ret;
    int iret;
    
    memset(&dev,0,sizeof(dev));

    dev.sll_ifindex = if_nametoindex(ifname);
    if(dev.sll_ifindex == 0){
        fprintf(stderr,"bind_device error '%s'\n",strerror(errno));
        return 1;
    }
    /*默认只接收目的MAC是本机的包,如果需要接收其它包,需要开启混杂模式*/
    dev.sll_pkttype = PACKET_HOST;
    dev.sll_family = PF_PACKET;
    //dev.sll_protocol = htons(ETH_P_ALL);
    dev.sll_protocol = htons(ETH_P_IP);
    iret = bind(sock,(struct sockaddr *)&dev,sizeof(dev));
    if(-1 == iret){
        fprintf(stderr,"bind error '%s'\n",strerror(errno));
        return 1;
    }
    return 0;
}

int l2_recv(char *ifname)
{
    int ret;
    int pfd;
    uchar recvbuf[2048];
    uchar buf[2048];
    struct sockaddr_ll sll;
    socklen_t addrlen;
    /* 接收所有二层包 */
    //pfd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    /* 接收二层IP包*/
    pfd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP));
    if(-1 == pfd){
        fprintf(stderr,"socket err '%s'\n",strerror(errno));
        return 0;
    }

    if(strncmp(ifname,"any",strlen("any"))){
        bind_device(pfd, ifname);
        if(promiscuous){
            set_promiscuous(pfd,ifname);
        }
    }
    memset(&sll,0,sizeof(sll));
    addrlen = sizeof(sll);
    for(;;){
        memset(recvbuf,0,sizeof(recvbuf));
        ret = recvfrom(pfd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&sll,&addrlen);
        if(-1 == ret){
            fprintf(stderr,"recvfrom err '%s'\n",strerror(errno));
            continue;
        }
        fprintf(stdout,"recv package len (%u),"MAC_FMT"\n",ret,MAC_ARG(sll.sll_addr));
        memcpy(buf,recvbuf,sizeof(recvbuf));
        print_eth_buf(buf,ret);
        fprintf(stdout,"*****************************************************************\n");
    }
}

int main(int argc,char **argv)
{
    int ret;
    ret = args_handle(argc,argv);
    if(ret){
        help(argv[0]);
        return 0;
    }
    l2_recv(device);

    return 0;
}

