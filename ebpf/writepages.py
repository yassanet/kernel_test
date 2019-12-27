#!/usr/bin/python3

from bcc import BPF
import ctypes as ct
import time

bpf_text = """
#include <linux/fs.h>

struct data_t {
  unsigned long ino;
};

BPF_PERF_OUTPUT(events);

int trace_do_writepages(struct pt_regs *ctx, struct address_space *mapping) {
  struct data_t data;
  struct inode *host = mapping->host;
  if (host && host->i_ino) {
    data.ino = host->i_ino;
    events.perf_submit(ctx, &data, sizeof(data));
  }
  return 0;
};
"""

b = BPF(text=bpf_text)
b.attach_kprobe(event="do_writepages",
                fn_name="trace_do_writepages")

class Data(ct.Structure):
  _fields_ = [
    ("i_ino", ct.c_ulong)
  ]

def print_event(cpu, data, size):
  event = ct.cast(data, ct.POINTER(Data)).contents
  print("{} {}".format(time.strftime("%H:%M:%S"), event.i_ino))

b["events"].open_perf_buffer(print_event)
print("TIME     INODE")
while True:
  b.perf_buffer_poll()
