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
#include <linux/icmp.h>
#include <arpa/inet.h>


#define uint    unsigned int
#define ushort  unsigned short
#define uchar   unsigned char

#define IP_FMT      "%u.%u.%u.%u"
#define IP_ARG(x)   ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2],((unsigned char *)(x))[3]

#define MAC_FMT     "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x)  ((unsigned char *)(x))[0],((unsigned char *)(x))[1],((unsigned char *)(x))[2], \
                    ((unsigned char *)(x))[3],((unsigned char *)(x))[4],((unsigned char *)(x))[5]


extern void print_buf(uchar *buf,int len,int flag);

#define print_eth_buf(buf,len) print_buf(buf,len,0)
#define print_iph_buf(buf,len) print_buf(buf,len,1)
#define print_l4_buf(buf,len) print_buf(buf,len,2)

static struct option longopts[] = {
    {"socket_type",     required_argument,  NULL,   'e'},
    {"mac",             required_argument,  NULL,   'm'},
    {"interface",       required_argument,  NULL,   'i'},
    {"destaddr",        required_argument,  NULL,   'd'},
    {"dport",           required_argument,  NULL,   'o'},
    {"sourceaddr",      required_argument,  NULL,   'a'},
    {"sport",           required_argument,  NULL,   's'},
    {"help",            no_argument,        NULL,   'h'},
    {0,0,0,0}
};

struct send_data{
    char str[64];
    uint num;
};
char    p_proto[16] = {"udp"};
ushort  dport = 4567;
uint    d_ip = 0;
ushort  sport = 8765;
uint    s_ip = 0;
uint    ip_hdrincl = 0;

uint8_t s_mac[6];
uint8_t d_mac[6];
uint32_t socket_type = SOCK_RAW;
char device[32] = {"null"};
void print_buf(uchar *buf,int len,int flag)
{
    int i,start_offset;
    uchar *ptr = buf;
    struct ethhdr *l2h;
    struct iphdr *iph;
    struct icmphdr *icmph;
    struct udphdr *udph;
    struct tcphdr *tcph;

    start_offset = 0;
    if(flag == 0){
        l2h = (struct ethhdr *)buf;
        fprintf(stdout,""MAC_FMT"->"MAC_FMT",0x%x,",
                    MAC_ARG(l2h->h_source),
                    MAC_ARG(l2h->h_dest),
                    ntohs(l2h->h_proto));
        start_offset = ETH_HLEN;
    }

    if(flag != 2){
        iph = (struct iphdr *)(buf + start_offset);
        fprintf(stdout,""IP_FMT"->"IP_FMT",%u,",
                        IP_ARG(&iph->saddr),
                        IP_ARG(&iph->daddr),
                        iph->protocol);
        
        if(iph->protocol == IPPROTO_UDP || iph->protocol == IPPROTO_TCP){
            udph = (struct udphdr *)(buf + start_offset + (iph->ihl << 2));
            fprintf(stdout,"%s:%u->%u\n",
                        (iph->protocol == IPPROTO_UDP) ? "udp" : "tcp",
                        ntohs(udph->source),
                        ntohs(udph->dest));
        }else{
            fprintf(stdout,"\n");
        }
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
    printf("    -e|--socket_type    set AF_PACKET socket_type 0:SOCK_RAW,1:SOCK_DGRAM\n");
    printf("    -i|--interface      set send interface \n");
    printf("    -m|--mac            set dest mac address \n");
    printf("    -d|--destaddr       <ipaddress> destination address ex:x.x.x.x\n");
    printf("    -o|--dport          <port> destination port\n");
    printf("    -a|--sourceaddr     <ipaddress> source address ex:x.x.x.x\n");
    printf("    -s|--sport          <port> source port\n");
    printf("    -h|--help           print help\n");
    printf("\n");
    return ;
}
int args_handle(int argc,char **argv)
{
    int c,ret;
    int option_index = 0;
    ret = 0;
    while(1){
        c = getopt_long(argc, argv, "he:i:d:o:a:s:m:",longopts, &option_index);
        if(-1 == c) break;
        
        switch(c){
        case 'e':
            socket_type = atoi(optarg) ? SOCK_DGRAM : SOCK_RAW;
            break;
        case 'd':
            if(1 != inet_pton(AF_INET,optarg,&d_ip)){
                printf("'%s' invalid\n",optarg);
                ret = 1;
            }
            break;
        case 'o':
            dport = (ushort)atoi(optarg);
            break;
        case 'a':
            if(1 != inet_pton(AF_INET,optarg,&s_ip)){
                printf("'%s' invalid\n",optarg);
                ret = 1;
            }
            break;
        case 's':
            sport = (ushort)atoi(optarg);
            break;
        case 'm':
            if(6 != sscanf(optarg,"%x:%x:%x:%x:%x:%x",&d_mac[0],&d_mac[1],&d_mac[2],&d_mac[3],&d_mac[4],&d_mac[5])){
                printf("-m '%s' invalid\n",optarg);
                ret = 1;
            }
            fprintf(stdout,"dest mac: "MAC_FMT"\n",MAC_ARG(d_mac));
            break;
        case 'i':
            strcpy(device,optarg);
            break;
        default:
            ret = 1;
            break;
        }
    }
    return ret;
}

uchar proto_str2u8(char *strp)
{
    switch(strp[0]){
    case 'u':
        return IPPROTO_UDP;
    case 't':
        return IPPROTO_TCP;
    case 'i':
        return IPPROTO_ICMP;
    default:
        return IPPROTO_UDP;
    }
}

ushort getprotolen(uchar proto)
{
    switch(proto){
    case IPPROTO_UDP:
        return sizeof(struct udphdr);
    case IPPROTO_TCP:
        return sizeof(struct tcphdr);
        
    case IPPROTO_ICMP:
        return sizeof(struct icmphdr);
    default:
        return 0;
    }
}
uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}
// Build IPv4 UDP pseudo-header and call checksum function.
uint16_t udp4_checksum (struct iphdr *iph, struct udphdr *udph, uint8_t *payload, int payloadlen)
{
    char buf[1500] = {0};
    char *ptr;
    int chksumlen = 0;
    int i;

    ptr = &buf[0];  // ptr points to beginning of buffer buf

    /* 构造 伪首部 */
    // Copy source IP address into buf (32 bits)
    memcpy (ptr, &iph->saddr, sizeof (iph->saddr));
    ptr += sizeof (iph->saddr);
    chksumlen += sizeof (iph->saddr);
    // Copy destination IP address into buf (32 bits)
    memcpy (ptr, &iph->daddr, sizeof (iph->daddr));
    ptr += sizeof (iph->daddr);
    chksumlen += sizeof (iph->daddr);
    // Copy zero field to buf (8 bits)
    *ptr = 0; ptr++;
    chksumlen += 1;
    // Copy transport layer protocol to buf (8 bits)
    memcpy (ptr, &iph->protocol, sizeof (iph->protocol));
    ptr += sizeof (iph->protocol);
    chksumlen += sizeof (iph->protocol);
    // Copy UDP length to buf (16 bits)
    memcpy (ptr, &udph->len, sizeof (udph->len));
    ptr += sizeof (udph->len);
    chksumlen += sizeof (udph->len);

    /* 真实UDP首部 */
    // Copy UDP source port to buf (16 bits)
    memcpy (ptr, &udph->source, sizeof (udph->source));
    ptr += sizeof (udph->source);
    chksumlen += sizeof (udph->source);
    // Copy UDP destination port to buf (16 bits)
    memcpy (ptr, &udph->dest, sizeof (udph->dest));
    ptr += sizeof (udph->dest);
    chksumlen += sizeof (udph->dest);
    // Copy UDP length again to buf (16 bits)
    memcpy (ptr, &udph->len, sizeof (udph->len));
    ptr += sizeof (udph->len);
    chksumlen += sizeof (udph->len);
    // Copy UDP checksum to buf (16 bits)
    // Zero, since we don't know it yet
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 2;

    /* 数据部分 */
    // Copy payload to buf
    memcpy (ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // Pad to the next 16-bit boundary
    for (i=0; i<payloadlen%2; i++, ptr++) {
        *ptr = 0;
        ptr++;
        chksumlen++;
    }

    return checksum ((uint16_t *) buf, chksumlen);
}

int create_l2_raw_socket(int s_type)
{
    int pfd;
    /*
    raw_socket = socket(AF_PACKET, socket_type, int protocol);
        socket_type:
            SOCK_RAW:   收包时,包括 以太网头
                        发包时,发送报文需填充以太网头
            SOCK_DGRAM: 收包时, 不包括 以太网头
                        发包时, 不需要填充以太网头, 但要通过 struct sockaddr_ll   来设置 
        protocol:
            ETH_P_ALL:  支持接收或发送所有二层协议报文
            ETH_P_IP:   接收或发送 IP 数据报文
    */
    pfd = socket(AF_PACKET,s_type,htons(ETH_P_IP));
    if(-1 == pfd){
        fprintf(stderr,"socket err '%s'\n",strerror(errno));
        exit(-1);
    }
    return pfd;
}

int pack_eth_header(char *buf)
{
    ushort tmps;
    struct ethhdr *ptr = (struct ethhdr *)buf;

    memcpy(ptr->h_dest,d_mac,ETH_ALEN);
    memcpy(ptr->h_source,s_mac,ETH_ALEN);
    ptr->h_proto = htons(ETH_P_IP);
    return sizeof(struct ethhdr);
}

int pack_ip_header(char *buf,uchar l4proto)
{
    ushort tmps;
    struct iphdr *ptr = (struct iphdr *)buf;

    tmps = getprotolen(l4proto);
    ptr->ihl = 5;
    ptr->version = 4;
    ptr->tos = 0;
    ptr->tot_len = htons(sizeof (struct iphdr) + tmps + sizeof(struct send_data));
    ptr->id = 0;            //Id of this packet
    ptr->frag_off = 0;
    ptr->ttl = 255;
    ptr->protocol = l4proto;
    ptr->saddr = s_ip;          //Spoof the source ip address
    ptr->daddr = d_ip;
    ptr->check = 0;
    tmps = checksum((uint16_t *)ptr,sizeof(struct iphdr));
    //ptr->check = htons(tmps);
    ptr->check = tmps;
    return sizeof(struct iphdr);
}
int pack_udp_header(char *buf)
{
    struct udphdr *ptr = (struct udphdr *)buf;
    ptr->dest = htons(dport);
    ptr->source = htons(sport);
    ptr->len = htons(8 + sizeof(struct send_data));
    ptr->check = 0;
    return sizeof(struct udphdr);
}
int pack_data(char *buf)
{
    struct send_data *ptr = (struct send_data *)buf;
    memset(ptr,0,sizeof(struct send_data));
    sprintf(ptr->str,"Hi baby, Sayyy love me ~~ !!");
    ptr->num = htonl(18);
    return sizeof(struct send_data);
}

int set_option_IP_HDRINCL(int sock)
{
    int on = 1;

    if(setsockopt(sock,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on)) < 0){
        fprintf(stderr,"setsockopt IP_HDRINCL error '%s'\n",strerror(errno));
        return 1;
    }
    return 0;
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

    return mac;
}
int l2_send(uchar proto)
{
    int ret,offset,i;
    int sd;
    uchar sendbuf[2048];
    uchar recvbuf[2048];
    struct sockaddr_ll sll;
    struct sockaddr_ll dll;
    socklen_t addrlen;
    struct ethhdr *l2h = NULL;
    struct iphdr *iph = NULL;
    struct udphdr *udph = NULL;
    struct send_data *data = NULL;
    
    sd = create_l2_raw_socket(socket_type);

    getifmac(device,s_mac);

    fprintf(stdout,"src  mac: "MAC_FMT"\n",MAC_ARG(s_mac));
    memset(&sll,0,sizeof(sll));
    memset(&dll,0,sizeof(dll));

    offset = 0;
    memset(sendbuf,0,sizeof(sendbuf));

    if(socket_type == SOCK_RAW){
        l2h = (struct ethhdr *)(sendbuf + offset);
        offset += pack_eth_header(sendbuf + offset);
    }
    
    iph = (struct iphdr *)(sendbuf + offset);
    offset += pack_ip_header(sendbuf + offset,proto);

    switch(proto){
    case IPPROTO_TCP:
        /* TO DO */
        break;
    case IPPROTO_ICMP:
        /* TO DO */
        break;
    case IPPROTO_UDP:
    default:
        udph = (struct udphdr *)(sendbuf + offset);
        offset += pack_udp_header(sendbuf + offset);
        break;
    }
    
    data = (struct send_data *)(sendbuf + offset);
    offset += pack_data(sendbuf + offset);

    if(NULL != iph && proto == IPPROTO_UDP){
        udph->check = udp4_checksum(iph,udph, (uint8_t *)data,sizeof(struct send_data));
    }else if(NULL != iph && proto == IPPROTO_TCP){
        /* TO DO */
    }else if(NULL != iph && proto == IPPROTO_ICMP){
        /* TO DO */
    }

    dll.sll_ifindex = if_nametoindex(device);
    if(dll.sll_ifindex == 0){
        fprintf(stderr,"if_nametoindex error '%s'\n",strerror(errno));
        return 1;
    }
    dll.sll_family = PF_PACKET;
    dll.sll_protocol = htons(ETH_P_IP);
    dll.sll_halen = ETH_ALEN;
    memcpy(dll.sll_addr,d_mac,ETH_ALEN);
    
    if(proto == IPPROTO_UDP){
        for(;;){
            ret = sendto(sd,sendbuf,offset,0,(struct sockaddr *)&dll,sizeof(dll));
            if(-1 == ret){
                fprintf(stderr,"sendto err '%s'\n",strerror(errno));
            }else{
                if(socket_type == SOCK_RAW){
                    print_eth_buf(sendbuf, offset);
                }else{
                    print_iph_buf(sendbuf, offset);
                }
            }
            sleep(1);
        }
    }else if(proto == IPPROTO_ICMP){
        /* TO DO */
    }else if(proto == IPPROTO_TCP){
        /* TO DO*/
    }

    return 0;
}

int main(int argc,char **argv)
{
    int ret;
    uchar proto;
    
    ret = args_handle(argc,argv);
    if(ret){
        help(argv[0]);
        return 0;
    }
    proto = proto_str2u8(p_proto);
    l2_send(proto);

    return 0;
}

