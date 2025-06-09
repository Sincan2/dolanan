#!/usr/bin/env python3
import r2pipe
import sys

def find_gadgets(r2, instructions):
    gadgets = {}
    for instr in instructions:
        cmd = f"/a {instr}"
        result = r2.cmdj(cmd)
        if result:
            for res in result:
                addr = res['offset']
                gadgets.setdefault(instr, []).append(addr)
    return gadgets

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <kernel_binary>")
        return

    binary = sys.argv[1]
    r2 = r2pipe.open(binary)
    r2.cmd("aaa")  # Analyze all

    # Gadget instruksi untuk dicari
    instructions = [
        "pop rdi; ret",
        "ret",
        "swapgs; ret",
        "pop r15; ret",
        "push rbx; pop rsp; ret"
    ]

    gadgets = {}
    for instr in instructions:
        # Pencarian gadget dengan cmd '/a' dan filter instruksi
        out = r2.cmd(f"/a {instr}")
        # Parsing sederhana
        lines = out.splitlines()
        addresses = []
        for line in lines:
            parts = line.strip().split()
            if parts:
                try:
                    addr = int(parts[0], 16)
                    addresses.append(addr)
                except:
                    pass
        gadgets[instr] = addresses

    for instr, addrs in gadgets.items():
        print(f"[+] Gadgets for '{instr}':")
        for addr in addrs:
            print(f"    0x{addr:x}")

if __name__ == "__main__":
    main()
