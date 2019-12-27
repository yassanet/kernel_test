#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <linux/kernel.h>
#include <netpacket/packet.h>
#include <net/if.h>

struct sock_filter code[] = {
    BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),  // ldh [12]
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x806, 0, 1), // jeq #0x06 jt 2 jf 3
    BPF_STMT(BPF_RET | BPF_K, -1), // ret #-1
    BPF_STMT(BPF_RET | BPF_K, 0) // ret #0
};

struct sock_fprog bpf = {
    .len = sizeof(code)/sizeof(code[0]),
    .filter = code,
};

int main(){
    int soc;
    struct ifreq ifr;
    struct sockaddr_ll sll;
    unsigned char buf[4096];

    memset(&ifr, 0, sizeof(ifr));
    memset(&sll, 0, sizeof(sll));

    soc = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    strncpy(ifr.ifr_name, "ens3", IFNAMSIZ);
    ioctl(soc, SIOCGIFINDEX, &ifr);

    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;
    bind(soc, (struct sockaddr *)&sll, sizeof(sll));

	/* */
//    struct bpf_program bpf;
//    pcap_t *handle;
//    handle = pcap_open_live("br0", 4096, 1, 1000, buf);
//    pcap_compile(handle,&bpf,"arp",1,PCAP_NETMASK_UNKNOWN);
//    setsockopt(soc, SOL_SOCKET, SO_ATTACH_FILTER, (struct sock_fprog*)&bpf, sizeof(bpf));    struct bpf_program bpf;
//    pcap_t *handle;
//    handle = pcap_open_live("br0", 4096, 1, 1000, buf);
//    pcap_compile(handle,&bpf,"arp",1,PCAP_NETMASK_UNKNOWN);
//    setsockopt(soc, SOL_SOCKET, SO_ATTACH_FILTER, (struct sock_fprog*)&bpf, sizeof(bpf));
    setsockopt(soc, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));

    while(1){
        ssize_t len = recv(soc, buf, sizeof(buf), 0);
        struct ethhdr* ethhdr = (struct ethhdr*)buf;
        int proto = ntohs(ethhdr->h_proto);
        if(len <= 0) break;
        printf("%3ld %0x %s\n", len, proto,
                proto==ETH_P_ARP ? "arp" : proto==ETH_P_IP ? "ip" : "other");
    }
    return 0;
}
