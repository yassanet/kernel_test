import time
from bcc import BPF

def main():
    b = BPF(src_file="./sockex1.c", debug=0)
    f = b.load_func("bpf_prog", BPF.SOCKET_FILTER)
    BPF.attach_raw_socket(f, "lo")
    my_map = b.get_table("my_map")

    ICMP = 1
    TCP = 6
    UDP = 17
    for i in range(5):
        print("TCP {} UDP {} ICMP {} bytes".format(my_map[TCP], my_map[UDP],
                                                   my_map[ICMP]))
        time.sleep(1)


if __name__ == "__main__":
    main()
