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

#define print_eth_buf(buf,len) print_buf(buf,len,1)
#define print_iph_buf(buf,len) print_buf(buf,len,0)
#define print_l4_buf(buf,len) print_buf(buf,len,2)

static struct option longopts[] = {
    {"ip_hdrincl",      required_argument,  NULL,   'i'},
    {"proto",           required_argument,  NULL,   'p'},
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
char p_proto[16] = {"udp"};
ushort dport = 4567;
uint d_ip = 0;
ushort sport = 8765;
uint s_ip = 0;
uint ip_hdrincl = 0;
uint16_t my_pid;
static uint16_t icmp_echo_seq = 0;

void print_buf(uchar *buf,int len,int flag)
{
    int i,start_offset;
    uchar *ptr = buf;
    struct ethhdr *l2h;
    struct iphdr *iph;
    struct icmphdr *icmph;
    struct udphdr *udph;
    struct tcphdr *tcph;
    
    if(flag == 0){
        l2h = (struct ethhdr *)buf;
        fprintf(stdout,""MAC_FMT"->"MAC_FMT",0x%x,",
                    MAC_ARG(l2h->h_source),
                    MAC_ARG(l2h->h_dest),
                    ntohs(l2h->h_proto));
    }

    if(flag == 1){
        iph = (struct iphdr *)(buf);
        fprintf(stdout,""IP_FMT"->"IP_FMT",%u,",
                        IP_ARG(&iph->saddr),
                        IP_ARG(&iph->daddr),
                        iph->protocol);
        
        if(iph->protocol == IPPROTO_UDP || iph->protocol == IPPROTO_TCP){
            udph = (struct udphdr *)(buf + (iph->ihl << 2));
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
    printf("    -i|--ip_hdrincl     1 set IP_HDRINCL option,0 is not \n");
    printf("    -p|--proto          <tcp|udp|icmp>\n");
    printf("    -b|--destaddr       <ipaddress> destination address ex:x.x.x.x\n");
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
        c = getopt_long(argc, argv, "hi:d:o:p:a:s:",longopts, &option_index);
        if(-1 == c) break;
        
        switch(c){
        case 'p':
            strcpy(p_proto,optarg);
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
        case 'i':
            ip_hdrincl = (uint)atoi(optarg);
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
// Build IPv4 TCP pseudo-header and call checksum function.
uint16_t tcp4_checksum (struct iphdr *iph, struct tcphdr *tcph, uint8_t *payload, int payloadlen)
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
    // Copy TCP length to buf (16 bits)
    *((uint16_t *)ptr) = htons(tcph->doff << 2);
    ptr += sizeof (uint16_t);
    chksumlen += sizeof (uint16_t);

    /* 真实TCP首部 */
    // Copy TCP header to buf
    memcpy(ptr, tcph,tcph->doff << 2);
    ptr += (tcph->doff << 2);
    chksumlen += (tcph->doff << 2);

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
// Build IPv4 ICMP pseudo-header and call checksum function.
uint16_t icmp4_checksum (struct iphdr *iph, struct tcphdr *tcph, uint8_t *payload, int payloadlen)
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
    // Copy TCP length to buf (16 bits)
    *((uint16_t *)ptr) = htons(tcph->doff << 2);
    ptr += sizeof (uint16_t);
    chksumlen += sizeof (uint16_t);

    /* 真实TCP首部 */
    // Copy TCP header to buf
    memcpy(ptr, tcph,tcph->doff << 2);
    ptr += (tcph->doff << 2);
    chksumlen += (tcph->doff << 2);

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
int create_l3_raw_socket(uchar proto)
{
    int pfd;
    /*
    raw_socket = socket(AF_INET, SOCK_RAW, int protocol);
        protocol:
            IPPROTO_RAW         只能用在raw socket 发包时,表示不指定发送报文的l4协议,而是可以发送任何l4协议的报文
            IPPROTO_TCP         接收或发送TCP报文
            IPPROTO_UDP         接收或发送UDP报文
            ...             (完整支持查看:https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml)
    */
    pfd = socket(AF_INET,SOCK_RAW,proto);
    if(-1 == pfd){
        fprintf(stderr,"socket err '%s'\n",strerror(errno));
        exit(-1);
    }
    return pfd;
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
    ptr->check = checksum((uint16_t *)ptr,sizeof(struct iphdr));
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
int pack_tcp_header(char *buf)
{
    struct tcphdr *ptr = (struct tcphdr *)buf;
    ptr->source = htons (sport);
	ptr->dest = htons (dport);
	ptr->seq = 0;
	ptr->ack_seq = 0;
	ptr->doff = 5;	//tcp header size
	ptr->fin=0;
	ptr->syn=1;
	ptr->rst=0;
	ptr->psh=0;
	ptr->ack=0;
	ptr->urg=0;
	ptr->window = htons (5840); //* maximum allowed window size */
	ptr->check = 0;	            //leave checksum 0 now, filled later by pseudo header
	ptr->urg_ptr = 0;
    return ptr->doff << 2;
}
int pack_icmp_header(char *buf)
{
    struct icmphdr *ptr = (struct icmphdr *)buf;
    ptr->type = 8;
    ptr->code = 0;
    ptr->un.echo.id = htons(my_pid);
    ptr->un.echo.sequence = htons(icmp_echo_seq);
    ptr->checksum = 0;
    
    return sizeof(struct icmphdr);
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
        return 1;
    }
    return 0;
}
int l3_send(uchar proto)
{
    int ret,offset,i;
    int sd;
    uchar sendbuf[2048];
    uchar recvbuf[2048];
    struct sockaddr_in daddr;
    struct sockaddr_in saddr;
    socklen_t addrlen;
    struct iphdr *iph = NULL,*recv_iph;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL,*recv_tcph;
    struct icmphdr *icmph = NULL,*recv_icmph;
    struct send_data *data = NULL,*recv_data;
    
    sd = create_l3_raw_socket(proto);
    
    memset(&daddr,0,sizeof(daddr));
    memset(&saddr,0,sizeof(saddr));
    addrlen = sizeof(daddr);

    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(dport);
    daddr.sin_addr.s_addr = d_ip;
    addrlen = sizeof(daddr);

    offset = 0;
    memset(sendbuf,0,sizeof(sendbuf));

    /* 是否自行构造IP头*/
    if(ip_hdrincl){
        /* 设置 IP_HDRINCL 选项,需要自己构造IP头*/
        ret = set_option_IP_HDRINCL(sd);
        if(! ret){
            iph = (struct iphdr *)(sendbuf + offset);
            offset += pack_ip_header(sendbuf + offset,proto);
        }
    }

    switch(proto){
    case IPPROTO_TCP:
        tcph = (struct tcphdr *)(sendbuf + offset);
        offset += pack_tcp_header(sendbuf + offset);
        break;
    case IPPROTO_ICMP:
        icmph = (struct icmphdr *)(sendbuf + offset);
        offset += pack_icmp_header(sendbuf + offset);
        break;
    case IPPROTO_UDP:
    default:
        udph = (struct udphdr *)(sendbuf + offset);
        offset += pack_udp_header(sendbuf + offset);
        break;
    }
    
    data = (struct send_data *)(sendbuf + offset);
    if(proto != IPPROTO_TCP){
        offset += pack_data(sendbuf + offset);
    }

    if(NULL != iph && proto == IPPROTO_UDP){
        udph->check = udp4_checksum(iph,udph, (uint8_t *)data,sizeof(struct send_data));
    }else if(NULL != iph && proto == IPPROTO_TCP){
        /* 
            有个问题: 如果没有设置 IP_HDRINCL, 这里TCP头的checksum=0,这个报文
            发出去后,会checksum error。
            也就是说,内核不会替你填充 tcp的 checksum
            所以,测试TCP发包的话,还是加上 -i 1 参数,开启 IP_HDRINCL
        */
        tcph->check = tcp4_checksum(iph,tcph, (uint8_t *)data,0);
    }else if(NULL != iph && proto == IPPROTO_ICMP){
        icmph->checksum = checksum((uint16_t *)icmph,sizeof(struct icmphdr) + sizeof(struct send_data));
    }

    if(proto == IPPROTO_UDP){
        for(;;){
            ret = sendto(sd,sendbuf,offset,0,(struct sockaddr *)&daddr,addrlen);
            if(-1 == ret){
                fprintf(stderr,"sendto err '%s'\n",strerror(errno));
            }else{
                fprintf(stdout,"sendto -> "IP_FMT":%u\n",IP_ARG(&daddr.sin_addr.s_addr),ntohs(daddr.sin_port));
                print_l4_buf(sendbuf, offset);
            }
            sleep(1);
        }
    }else if(proto == IPPROTO_ICMP){
        for(;;){
            icmph->un.echo.sequence = htons(icmp_echo_seq ++);
            icmph->checksum = checksum((uint16_t *)icmph,sizeof(struct icmphdr) + sizeof(struct send_data));
            ret = sendto(sd,sendbuf,offset,0,(struct sockaddr *)&daddr,addrlen);
            if(-1 == ret){
                fprintf(stderr,"sendto err '%s'\n",strerror(errno));
            }else{
                fprintf(stdout,"sendto -> "IP_FMT":icmp request ,id(%u),seq(%u)\n",
                                IP_ARG(&daddr.sin_addr.s_addr),
                                ntohs(icmph->un.echo.id),
                                ntohs(icmph->un.echo.sequence));
                for(i = 0;i < 3;i ++){
                    memset(recvbuf,0,sizeof(recvbuf));
                    /*
                        这里有可能出现长时间阻塞.

                        ICMP报文的发包没有问题,收包有点问题,不想修改了

                        知道就行 
                    */
                    ret = recvfrom(sd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&saddr,&addrlen);
                    if(-1 == ret){
                        fprintf(stderr,"recvfrom err '%s'\n",strerror(errno));
                    }else{
                        if(saddr.sin_addr.s_addr != d_ip){
                            fprintf(stderr,"no answer \n");
                            sleep(1);
                            continue;
                        }else{
                            recv_iph = (struct iphdr *)recvbuf;
                            recv_icmph = (struct icmphdr *)(recvbuf + (recv_iph->ihl << 2));
                            if(recv_icmph->type == 0 && recv_icmph->code == 0){
                                recv_data = (struct send_data *)(recvbuf + (recv_iph->ihl << 2) + sizeof(struct icmphdr));
                                fprintf(stdout,"-> recive "IP_FMT":icmp response ,id(%u),seq(%u), %s\n",
                                                IP_ARG(&saddr.sin_addr.s_addr),
                                                ntohs(recv_icmph->un.echo.id),
                                                ntohs(recv_icmph->un.echo.sequence),
                                                recv_data->str);
                            }
                            break;
                        }
                    }
                }
            }
            sleep(1);
        }
    }else if(proto == IPPROTO_TCP){
        for(;;){

            /*
                TCP 发送一个 syn 请求,
                对端没有监听端口的话,会回复一个 rst ,看打印的话 很清晰
            */
            ret = sendto(sd,sendbuf,offset,0,(struct sockaddr *)&daddr,addrlen);
            if(-1 == ret){
                fprintf(stderr,"sendto err '%s'\n",strerror(errno));
            }else{
                fprintf(stdout,"sendto -> "IP_FMT":%u,ack(%u),rst(%u),syn(%u),seq(%u),ack_seq(%u)\n",
                                IP_ARG(&daddr.sin_addr.s_addr),
                                ntohs(daddr.sin_port),
                                tcph->ack,
                                tcph->rst,
                                tcph->syn,
                                ntohl(tcph->seq),
                                ntohl(tcph->ack_seq));
                for(i = 0;i < 3;i ++){
                    memset(recvbuf,0,sizeof(recvbuf));
                    ret = recvfrom(sd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&saddr,&addrlen);
                    if(-1 == ret){
                        fprintf(stderr,"recvfrom err '%s'\n",strerror(errno));
                    }else{
                        if(saddr.sin_addr.s_addr != d_ip){
                            fprintf(stderr,"wait tcp response \n");
                            sleep(1);
                            continue;
                        }else{
                            recv_iph = (struct iphdr *)recvbuf;
                            recv_tcph = (struct tcphdr *)(recvbuf + (recv_iph->ihl << 2));
                            fprintf(stdout,"-> recive "IP_FMT":%u,ack(%u),rst(%u),syn(%u),seq(%u),ack_seq(%u)\n",
                                            IP_ARG(&saddr.sin_addr.s_addr),
                                            ntohs(saddr.sin_port),
                                            recv_tcph->ack,
                                            recv_tcph->rst,
                                            recv_tcph->syn,
                                            ntohl(recv_tcph->seq),
                                            ntohl(recv_tcph->ack_seq));
                            break;
                        }
                    }
                }
            }
            sleep(1);
        }
    }
}

int main(int argc,char **argv)
{
    int ret;
    uchar proto;
    
    my_pid = (uint16_t)getpid();
    ret = args_handle(argc,argv);
    if(ret){
        help(argv[0]);
        return 0;
    }
    proto = proto_str2u8(p_proto);
    l3_send(proto);

    return 0;
}

