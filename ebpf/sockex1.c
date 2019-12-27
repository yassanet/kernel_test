#include <bcc/proto.h>

BPF_ARRAY(my_map, long, 256);

int bpf_prog(struct __sk_buff *skb)
{
    u8* cursor = 0;
    struct ethernet_t *ethhdr = cursor_advance(cursor, sizeof(*ethhdr));
    if(!(ethhdr->type == 0x0800)){ // not ipv4
        return -1;
    }
    struct ip_t *iphdr = cursor_advance(cursor, sizeof(*iphdr));
    u32 index = iphdr->nextp;

    if (skb->pkt_type != PACKET_OUTGOING)
        return -1;

    long *value = my_map.lookup(&index);
    if (value)
        lock_xadd(value, skb->len);

    return -1;
}
