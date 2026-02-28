# CLAUDE.md — CSE 123 UCSD Networking Project

## Project Overview

This is a **C networking project** from UCSD's CSE 123 (Computer Networks, Fall 2012). The goal is to implement a simplified **TFTP-like (Trivial File Transfer Protocol) client/server** over UDP sockets using POSIX socket APIs.

The provided sample code demonstrates UDP echo client/server communication and lays the foundation for building a full TFTP implementation.

---

## Repository Structure

```
CSE123/
├── tftp.h              # TFTP protocol definitions: packet structs, opcodes, error codes, constants
├── server_test.c       # UDP echo server (reference/sample code)
├── test_echo_client.c  # UDP echo client (reference/sample code — sends an RRQ packet, prints reply)
├── server_test         # Pre-compiled binary (macOS Mach-O x86_64 — does NOT run on Linux)
└── test_echo_client    # Pre-compiled binary (macOS Mach-O x86_64 — does NOT run on Linux)
```

> **Note:** The committed binaries (`server_test`, `test_echo_client`) are **macOS Mach-O executables** and will not run on Linux. Recompile from source (see below).

---

## Key File Details

### `tftp.h` — Protocol Header

Defines all TFTP protocol constants and packet structures:

| Constant | Value | Description |
|---|---|---|
| `SERV_UDP_PORT` | 60010 | Server UDP port for TFTP |
| `SERV_HOST_ADDR` | `"127.0.0.1"` | Default server address (localhost) |
| `MAX_TFTP_CLIENTS` | 100 | Maximum concurrent clients |
| `MAX_TFTP_TIMEOUTS` | 10 | Retry limit per transfer |
| `TIMEOUT_DURATION` | 5 | Seconds before timeout |
| `MAX_BUFF_SIZE` | 2048 | Maximum UDP packet size |
| `MAX_DATA_SIZE` | 512 | TFTP data block size (standard) |
| `DEBUG` | true | Enable debug output |

**TFTP Opcodes:**

| Constant | Value | Meaning |
|---|---|---|
| `OPTCODE_RRQ` | 1 | Read Request |
| `OPTCODE_WRQ` | 2 | Write Request |
| `OPTCODE_DATA` | 3 | Data packet |
| `OPTCODE_ACK` | 4 | Acknowledgment |
| `OPTCODE_ERR` | 5 | Error |

**TFTP Error Codes** (`ERRCODE_*`): `UNDEFINED(0)`, `FILE_NOT_FOUND(1)`, `ACCESS_VIOLATION(2)`, `DISK_FULL(3)`, `ILLEGAL_OPERATION(4)`, `UNKNOWN_TRANSFER_ID(5)`, `FILE_ALREADY_EXISTS(6)`, `NO_SUCH_USER(7)`.

**Packet structs** (in `tftp.h`):
- `RRQ` / `WRQ` — optcode + FileName + Mode
- `DATA` — optcode(3) + block number + 512-byte data array
- `ACK` — optcode(4) + block number
- `ERROR` — optcode(5) + error code + error message string

> **Known issue in `tftp.h`:** The struct member initializers (`char FileName[...] = "..."`) are C++ syntax and are **not valid in C**. The file must be compiled as C++ (`g++ -x c++`) or the initializers must be removed and set in code instead.

### `server_test.c` — UDP Echo Server

A reference UDP echo server that:
1. Creates a UDP socket (`AF_INET`, `SOCK_DGRAM`)
2. Binds to `INADDR_ANY` on port `12345`
3. Loops forever: receives a datagram and echoes it back to the sender

Uses **K&R (pre-ANSI) C** function declaration style. The server never terminates.

### `test_echo_client.c` — UDP Echo Client

A reference UDP echo client that:
1. Creates a UDP socket and binds to any available port
2. Constructs a hardcoded `RRQ` struct (`optcode=1`, filename `"dsgdfgdfgdfghf"`, mode `"octet"`)
3. Sends the RRQ packet to the server at `127.0.0.1:12345`
4. Receives the echoed reply and prints the decoded `optcode`, filename, and mode
5. Closes the socket and exits

Note: The main client loop (`while (fgets(...))`) is **commented out** — the client sends one packet and exits.

---

## Building

There is **no Makefile**. Compile manually with `gcc`. Note the C++ struct initializer issue in `tftp.h` — if included, compile with `g++`:

```bash
# Compile the echo server
gcc -o server_test server_test.c

# Compile the echo client (includes tftp.h indirectly via test_echo_client.c if applicable)
gcc -o test_echo_client test_echo_client.c

# If tftp.h's C++ initializers cause issues, use g++:
g++ -o server_test server_test.c
g++ -o test_echo_client test_echo_client.c
```

Recommended flags for development:
```bash
gcc -Wall -Wextra -g -o <output> <source>.c
```

---

## Running the Echo Test

The server and client use **port 12345** (different from the `SERV_UDP_PORT 60010` in `tftp.h`).

```bash
# Terminal 1: start the server (runs indefinitely)
./server_test

# Terminal 2: run the client (sends one RRQ, prints echoed reply, exits)
./test_echo_client
```

Expected client output:
```
1
dsgdfgdfgdfghf
octet
```

---

## Code Style and Conventions

- **Language:** C (targeting C89/K&R compatibility, though `tftp.h` uses C++ features)
- **Function declarations:** K&R style used in sample files (`dg_echo(sockfd) int sockfd;`) — new code should use ANSI C prototypes
- **Boolean:** A manual `typedef int bool` with `#define true 1` / `#define false 0` is defined in `tftp.h` (do not include `<stdbool.h>` separately when using this header)
- **Network byte order:** Always use `htons()`/`htonl()` when setting port/address fields, and `ntohs()`/`ntohl()` when reading them
- **Error handling:** Use `exit()` with distinct non-zero codes for each error condition (convention already in place)
- **Buffer zeroing:** Use `bzero()` (as in existing code) or `memset(..., 0, ...)` — both are acceptable

---

## Development Workflow

### Branch
All changes go to the active development branch. Do not commit directly to `master`.

### Typical workflow
```bash
git checkout claude/add-claude-documentation-hNQXx   # or your feature branch
# ... make changes ...
git add <files>
git commit -m "descriptive message"
git push -u origin <branch-name>
```

### What to implement next (expected TFTP functionality)
Based on the provided header and stubs, the project requires implementing:
1. **RRQ handler** — server receives read request, opens file, sends DATA packets in 512-byte blocks
2. **WRQ handler** — server receives write request, stores DATA packets received from client
3. **ACK/retransmit logic** — sender waits for ACK; retransmits on timeout (up to `MAX_TFTP_TIMEOUTS`)
4. **ERROR handling** — send ERROR packets for bad requests, missing files, access violations
5. **Multi-client support** — server must handle up to `MAX_TFTP_CLIENTS` concurrent transfers (consider `fork()` or separate ephemeral ports per RFC 1350)

---

## Known Issues and Gotchas

1. **`tftp.h` C++ initializers** — `char FileName[...] = "..."` inside a `struct` is invalid C; compile as C++ or remove the initializers.
2. **Port mismatch** — `server_test.c` / `test_echo_client.c` use port `12345`; `tftp.h` defines `SERV_UDP_PORT 60010`. Align these when building the actual TFTP server.
3. **Pre-built binaries are macOS binaries** — `server_test` and `test_echo_client` are Mach-O executables and cannot run on Linux. Always recompile from source.
4. **`recvfrom` with `clilen` type** — `clilen` is declared as `int` in `server_test.c` but `recvfrom` expects `socklen_t *`. This may cause warnings; use `socklen_t` in new code.
5. **Duplicate includes** — `test_echo_client.c` includes `<arpa/inet.h>` and `<stdlib.h>` twice; harmless but worth cleaning up.
6. **`bzero` is deprecated** — prefer `memset(ptr, 0, size)` in new code.
