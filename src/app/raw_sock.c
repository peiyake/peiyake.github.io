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
#include <pcap.h>



#define uint    unsigned int
#define ushort  unsigned short
#define uchar   unsigned char

#define S_SOCK_RAW      1
#define S_SOCK_PCAP     2
#define IP_FMT      "%u.%u.%u.%u"
#define IP_ARG(x)   ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2],((unsigned char *)(x))[3]

#define MAC_FMT     "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x)  ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2], \
                    ((unsigned char *)(x))[3],((unsigned char *)(x))[4],((unsigned char *)(x))[5]

extern void print_buf(uchar *buf,int len,int flag);

#define print_eth_buf(buf,len) print_buf(buf,len,1)
#define print_iph_buf(buf,len) print_buf(buf,len,0)

struct main_args{
    char dumpif[32];
    uint sock;
};
static struct option longopts[] = {
    {"interface",   required_argument,  NULL,   'i'},
    {"sock",        required_argument,  NULL,   's'},
    {"help",        no_argument,        NULL,   'h'},
    {0,0,0,0}
};

struct main_args global_arg;
void help(char *name)
{
    printf("Usage: %s [option]\n",name);
    printf(" option\n");
    printf("    -i|--interface          dump interface\n");
    printf("    -s|--sock               <raw|pcap> use raw socket or libpcap\n");
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
        c = getopt_long(argc, argv, "hs:i:",longopts, &option_index);
        if(-1 == c) break;
        
        switch(c){
        case 'h':
            help(argv[0]);
            break;
        case 'i':
            strcpy(global_arg.dumpif,optarg);
            fprintf(stdout,"arg -i : %s\n",global_arg.dumpif);
            break;
        case 's':
            if(0 == strncmp(optarg,"raw",strlen("raw"))){
                global_arg.sock = S_SOCK_RAW;
            }else{
                global_arg.sock = S_SOCK_PCAP;
            }
            break;
        case '?':
        default:
            ret = 1;
            fprintf(stdout,"option unknown\n");
            break;
        }
    }
    return ret;
}

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

char *getifmac(char *ifname,char *mac)
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
    
    ret = ioctl(fd,SIOCGIFHWADDR,&req);
    if(-1 == fd){
        close(fd);
        fprintf(stdout,"ioctl err '%s'\n",strerror(errno));
        return NULL;
    }

    memcpy(mac,req.ifr_hwaddr.sa_data,ETH_ALEN);
    close(fd);

    fprintf(stdout,"%s:"MAC_FMT"\n",ifname,MAC_ARG(mac));
    return mac;
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
    dev.sll_family = PF_PACKET;
    dev.sll_protocol = htons(ETH_P_IP);
    //dev.sll_pkttype = PACKET_HOST;
    //dev.sll_halen = ETH_ALEN;
    ret = getifmac(ifname,mac);
    if(NULL == ret){
        fprintf(stderr,"getifmac error\n");
        return 1;
    }
    //memcpy(dev.sll_addr,mac,ETH_ALEN);

    iret = bind(sock,(struct sockaddr *)&dev,sizeof(dev));
    if(-1 == iret){
        fprintf(stderr,"bind error '%s'\n",strerror(errno));
        return 1;
    }
    return 0;
}

int raw_socket_recv(void)
{
    int ret;
    int pfd;
    uchar recvbuf[2048];
    struct sockaddr_in client;
    socklen_t addrlen;
    pfd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP));
    if(-1 == pfd){
        fprintf(stderr,"socket err '%s'\n",strerror(errno));
        return 0;
    }

    bind_device(pfd, global_arg.dumpif);
    memset(&client,0,sizeof(client));
    addrlen = sizeof(client);
    for(;;){
        memset(recvbuf,0,sizeof(recvbuf));
        ret = recvfrom(pfd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&client,&addrlen);
        if(-1 == ret){
            fprintf(stderr,"recvfrom err '%s'\n",strerror(errno));
            continue;
        }
        print_eth_buf(recvbuf,ret);
        fprintf(stdout,"*****************************************************************\n");
    }
}
void packet_handler(u_char *args, const struct pcap_pkthdr *pkthdr,const u_char *packet)
{
    print_eth_buf(packet, pkthdr->len);
    fprintf(stdout,"*****************************************************************\n");
    return ;
}

int libpcap_recv(void)
{
    pcap_t *handle;
    char *dev;          /* The device to sniff on */
    char errbuf[PCAP_ERRBUF_SIZE];  /* Error string */
    struct bpf_program fp;      /* The compiled filter */
    char filter_exp[] = "udp";  /* The filter expression */
    bpf_u_int32 mask;       /* Our netmask */
    bpf_u_int32 net;        /* Our IP */
    struct pcap_pkthdr header;  /* The header that pcap gives us */
    const u_char *packet;       /* The actual packet */

    dev = global_arg.dumpif;

    #if 0   /*find default device */
    /* Define the device */
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
        fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
        return(2);
    }
    #endif
    /* Find the properties for the device */
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
        net = 0;
        mask = 0;
    }
    /* Open the session in promiscuous mode */
    handle = pcap_open_live(dev, 65535, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 1;
    }
    /* Compile and apply the filter */
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 1;
    }
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return 1;
    }

    pcap_loop(handle,0,packet_handler,NULL);
    pcap_close(handle);

    return 0;
}
int main(int argc,char **argv)
{
    int ret;
    memset(&global_arg,0,sizeof(global_arg));
    if(ret = args_handle(argc,argv)){
        help(argv[0]);
        return 0;
    }

    if(global_arg.sock == S_SOCK_RAW){
        raw_socket_recv();
    }else if(global_arg.sock == S_SOCK_PCAP){
        libpcap_recv();
    }

    return 0;
}
