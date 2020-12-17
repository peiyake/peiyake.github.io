#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
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

#define BIND_METHOD_bind                1
#define BIND_METHOD_setsockopt          2

extern void print_buf(uchar *buf,int len,int flag);

#define print_eth_buf(buf,len) print_buf(buf,len,1)
#define print_iph_buf(buf,len) print_buf(buf,len,0)

static struct option longopts[] = {
    {"interface",       required_argument,  NULL,   'i'},
    {"bind_method",       required_argument,  NULL,   'b'},
    {"help",            no_argument,        NULL,   'h'},
    {0,0,0,0}
};

char device[32] = {"any"};
int bind_method = BIND_METHOD_bind;
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
    printf("    -b|--bind_method        <1|2> method of bind interface 1:bind,2:setsockopt\n");
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
        c = getopt_long(argc, argv, "hb:i:",longopts, &option_index);
        if(-1 == c) break;
        
        switch(c){
        case 'i':
            strcpy(device,optarg);
            break;
        case 'b':
            bind_method = (atoi(optarg) == 1) ? BIND_METHOD_bind : BIND_METHOD_setsockopt;
            break;
        default:
            ret = 1;
            break;
        }
    }
    return ret;
}


char *get_addrin_byname(char *ifname,struct sockaddr_in *in)
{
    struct ifreq req;
    int fd,ret;

    memset(&req,0,sizeof(req));

    fd = socket(AF_INET,SOCK_DGRAM,0);
    if(-1 == fd){
        fprintf(stdout,"socket err '%s'\n",strerror(errno));
        return NULL;
    }

    strncpy(req.ifr_name,ifname,IFNAMSIZ);
    
    ret = ioctl(fd,SIOCGIFADDR,&req);
    if(-1 == fd){
        close(fd);
        fprintf(stdout,"ioctl err '%s'\n",strerror(errno));
        return NULL;
    }

    if(req.ifr_addr.sa_family == AF_INET){
        memcpy(in,(struct sockaddr_in *)(&req.ifr_addr),sizeof(struct sockaddr_in));
    }else{
        in = NULL;
    }
    close(fd);

    return (char *)in;
}

int bind_device(int sock,char *ifname)
{
    struct sockaddr_in bind_addr;
    uchar mac[ETH_ALEN] = {0};
    char *ret;
    int iret;
    char opt[32];
    socklen_t optlen = sizeof(opt);
    memset(&bind_addr,0,sizeof(bind_addr));
    /*
        注意: 
            raw socket 没有端口的概念,所以 bind_addr.sin_port 无需关注
    */

    if(bind_method == BIND_METHOD_bind){
        ret = get_addrin_byname(ifname,&bind_addr);
        if(NULL == ret){
            fprintf(stdout,"get_addrin_byname err '%s'\n",ifname);
            return 1;
        }
        
        iret = bind(sock,(struct sockaddr *)&bind_addr,sizeof(bind_addr));
        if(-1 == iret){
            fprintf(stderr,"bind error '%s'\n",strerror(errno));
            return 1;
        }else{
            fprintf(stdout,"bind socket to address "IP_FMT" of %s\n",IP_ARG(&bind_addr.sin_addr.s_addr),ifname);
        }
    }else{
        memset(opt,0,sizeof(opt));
        sprintf(opt,"%s",ifname);
        iret = setsockopt(sock,SOL_SOCKET,SO_BINDTODEVICE,(void *)opt,optlen);
        if(-1 == iret){
            fprintf(stderr,"setsockopt SO_BINDTODEVICE to %s fail '%s'\n",opt,strerror(errno));
        }else{
            fprintf(stdout,"bind socket to interface %s\n",opt);
        }
    }
    return 0;
}

int l3_recv(char *ifname)
{
    int ret;
    int pfd;
    uchar recvbuf[2048];
    struct sockaddr_in saddr;
    socklen_t addrlen;

    /*
    raw_socket = socket(AF_INET, SOCK_RAW, int protocol);
        protocol:
            IPPROTO_RAW         只能用在raw socket 发包时,表示不指定发送报文的l4协议,而是可以发送任何l4协议的报文
            IPPROTO_TCP         接收或发送TCP报文
            IPPROTO_UDP         接收或发送UDP报文
            ...             (完整支持查看:https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml)
    */
    pfd = socket(AF_INET,SOCK_RAW,IPPROTO_UDP);
    if(-1 == pfd){
        fprintf(stderr,"socket err '%s'\n",strerror(errno));
        return 0;
    }

    if(strncmp(ifname,"any",strlen("any"))){
        bind_device(pfd, ifname);
    }
    memset(&saddr,0,sizeof(saddr));
    addrlen = sizeof(saddr);
    for(;;){
        memset(recvbuf,0,sizeof(recvbuf));
        ret = recvfrom(pfd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&saddr,&addrlen);
        if(-1 == ret){
            fprintf(stderr,"recvfrom err '%s'\n",strerror(errno));
            continue;
        }
        fprintf(stdout,"recv package len (%u),"IP_FMT"\n",ret,IP_ARG(&saddr.sin_addr.s_addr));
        print_iph_buf(recvbuf,ret);
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
    l3_recv(device);

    return 0;
}

