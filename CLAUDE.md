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
└── test_echo_client.c  # UDP echo client (reference/sample code — sends an RRQ packet, prints reply)
```

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

### `server_test.c` — UDP Echo Server

A reference UDP echo server that:
1. Creates a UDP socket (`AF_INET`, `SOCK_DGRAM`)
2. Binds to `INADDR_ANY` on port `60010`
3. Loops forever: receives a datagram and echoes it back to the sender

Uses **K&R (pre-ANSI) C** function declaration style. The server never terminates.

### `test_echo_client.c` — UDP Echo Client

A reference UDP echo client that:
1. Creates a UDP socket and binds to any available port
2. Constructs a hardcoded `RRQ` struct (`optcode=1`, filename `"dsgdfgdfgdfghf"`, mode `"octet"`)
3. Sends the RRQ packet to the server at `127.0.0.1:60010`
4. Receives the echoed reply and prints the decoded `optcode`, filename, and mode
5. Closes the socket and exits

Note: The main client loop (`while (fgets(...))`) is **commented out** — the client sends one packet and exits.

---

## Building

There is **no Makefile**. Compile manually with `gcc`:

```bash
gcc -Wall -Wextra -g -o server_test server_test.c
gcc -Wall -Wextra -g -o test_echo_client test_echo_client.c
```

---

## Running the Echo Test

The server and client both use `SERV_UDP_PORT` from `tftp.h` (port **60010**).

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

- **Language:** C (targeting C89/K&R compatibility)
- **Function declarations:** K&R style used in sample files (`dg_echo(sockfd) int sockfd;`) — new code should use ANSI C prototypes
- **Boolean:** A manual `typedef int bool` with `#define true 1` / `#define false 0` is defined in `tftp.h` (do not include `<stdbool.h>` separately when using this header)
- **Network byte order:** Always use `htons()`/`htonl()` when setting port/address fields, and `ntohs()`/`ntohl()` when reading them
- **Error handling:** Use `exit()` with distinct non-zero codes for each error condition (convention already in place)
- **Buffer zeroing:** Use `memset(ptr, 0, size)` — `bzero()` is deprecated

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

## Remaining Gotchas

- **K&R function declarations** — `server_test.c` and `test_echo_client.c` use pre-ANSI K&R style (`dg_echo(sockfd) int sockfd;`). New code should use ANSI C prototypes.
- **Client loop commented out** — `test_echo_client.c`'s `while (fgets(...))` loop is commented out; the client sends exactly one packet and exits.
