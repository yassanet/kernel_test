#!/usr/bin/python3

from bcc import BPF

bpf_text = """
int trace_sys_clone(struct pt_regs *ctx) {
  bpf_trace_printk("Hello, World!\\n");
  return 0;
}
"""

b = BPF(text=bpf_text)
b.attach_kprobe(event="__x64_sys_clone", fn_name="trace_sys_clone")

b.trace_print()
