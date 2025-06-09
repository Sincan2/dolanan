#!/usr/bin/env python3

def find_binsh_offset(libc_path):
    target = b"/bin/sh\x00"
    with open(libc_path, "rb") as f:
        data = f.read()
        offset = data.find(target)
        if offset == -1:
            print("[!] String '/bin/sh' tidak ditemukan di", libc_path)
            return None
        print("[+] Ditemukan '/bin/sh' di offset 0x{:x}".format(offset))
        return offset

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: {} /path/to/libc.so".format(sys.argv[0]))
        exit(1)
    libc_path = sys.argv[1]
    find_binsh_offset(libc_path)
