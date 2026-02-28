/* tftp_server.c — TFTP server with RRQ (read request) support
 * CSE 123 - UCSD - Fall 2012
 *
 * Listens on SERV_UDP_PORT for TFTP requests.  Handles RRQ (opcode 1):
 * opens the named file and sends it to the client as a sequence of DATA
 * packets (up to MAX_DATA_SIZE bytes each).  Waits for an ACK after each
 * block; retransmits up to MAX_TFTP_TIMEOUTS times on timeout.  Sends an
 * ERROR packet for missing/unreadable files and unsupported opcodes.
 *
 * Transfer socket design (per RFC 1350 §4):
 *   The initial RRQ arrives on the well-known port.  All DATA/ACK
 *   traffic then uses a fresh ephemeral socket so the well-known port
 *   remains free for new requests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "tftp.h"

char *progname;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Send a TFTP ERROR packet to cli_addr over sockfd. */
static void send_error(int sockfd,
                       struct sockaddr *cli_addr, socklen_t clilen,
                       unsigned short errcode, const char *msg)
{
    ERROR pkt;
    pkt.optcode   = htons(OPTCODE_ERR);
    pkt.ErrorCode = htons(errcode);
    strncpy(pkt.ErrMsg, msg, MAX_STRING_SIZE - 1);
    pkt.ErrMsg[MAX_STRING_SIZE - 1] = '\0';
    sendto(sockfd, &pkt, sizeof(pkt), 0, cli_addr, clilen);
}

/* ------------------------------------------------------------------ */
/* RRQ handler                                                         */
/* ------------------------------------------------------------------ */

/* Open filename and stream it to cli_addr as numbered DATA packets.
 * A fresh ephemeral socket is used for the transfer so the well-known
 * port stays free for new requests.  Each DATA block must be ACKed
 * before the next is sent; on timeout the block is retransmitted up to
 * MAX_TFTP_TIMEOUTS times.  The transfer ends when the last block
 * contains fewer than MAX_DATA_SIZE bytes (including zero). */
static void handle_rrq(struct sockaddr *cli_addr, socklen_t clilen,
                       const char *filename)
{
    FILE  *fp;
    int    xfer_sock;
    struct sockaddr_in xfer_addr;
    struct timeval tv;
    DATA   data_pkt;
    ACK    ack_pkt;
    unsigned short block;
    int    bytes_read, pkt_len, n, retries;

    /* Open the requested file in binary mode. */
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        /* Use a temporary socket to send the error reply before we
         * have opened the transfer socket.                          */
        int err_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (err_sock >= 0) {
            unsigned short code = (errno == EACCES)
                                  ? ERRCODE_ACCESS_VIOLATION
                                  : ERRCODE_FILE_NOT_FOUND;
            const char *emsg = (code == ERRCODE_ACCESS_VIOLATION)
                               ? "Access violation" : "File not found";
            send_error(err_sock, cli_addr, clilen, code, emsg);
            close(err_sock);
        }
        fprintf(stderr, "%s: RRQ '%s': %s\n",
                progname, filename, strerror(errno));
        return;
    }

    /* Open and bind the ephemeral transfer socket. */
    xfer_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (xfer_sock < 0) {
        fprintf(stderr, "%s: can't open transfer socket\n", progname);
        fclose(fp);
        return;
    }

    memset(&xfer_addr, 0, sizeof(xfer_addr));
    xfer_addr.sin_family      = AF_INET;
    xfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    xfer_addr.sin_port        = htons(0); /* OS picks a free port */

    if (bind(xfer_sock,
             (struct sockaddr *)&xfer_addr, sizeof(xfer_addr)) < 0) {
        fprintf(stderr, "%s: can't bind transfer socket\n", progname);
        close(xfer_sock);
        fclose(fp);
        return;
    }

    /* Set a receive timeout so recvfrom returns on ACK loss. */
    tv.tv_sec  = TIMEOUT_DURATION;
    tv.tv_usec = 0;
    setsockopt(xfer_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* Send DATA blocks until the file is exhausted. */
    block = 1;
    do {
        bytes_read = (int)fread(data_pkt.data, 1, MAX_DATA_SIZE, fp);
        if (bytes_read < 0) {
            send_error(xfer_sock, cli_addr, clilen,
                       ERRCODE_UNDEFINED, "File read error");
            goto done;
        }

        data_pkt.optcode  = htons(OPTCODE_DATA);
        data_pkt.No_Block = htons(block);
        /* Wire size: 2-byte opcode + 2-byte block + data bytes */
        pkt_len = (int)(sizeof(data_pkt.optcode) +
                        sizeof(data_pkt.No_Block) + bytes_read);

        retries = 0;
        for (;;) {
            if (sendto(xfer_sock, &data_pkt, pkt_len, 0,
                       cli_addr, clilen) != pkt_len) {
                fprintf(stderr, "%s: sendto error on block %u\n",
                        progname, block);
                goto done;
            }

            if (DEBUG)
                printf("[RRQ] sent DATA block %u (%d bytes)\n",
                       block, bytes_read);

            /* Wait for ACK. */
            n = (int)recvfrom(xfer_sock, &ack_pkt, sizeof(ack_pkt),
                              0, NULL, NULL);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (++retries > MAX_TFTP_TIMEOUTS) {
                        fprintf(stderr,
                                "%s: too many timeouts on block %u, "
                                "aborting\n", progname, block);
                        goto done;
                    }
                    fprintf(stderr,
                            "%s: timeout waiting for ACK %u "
                            "(retry %d/%d)\n",
                            progname, block, retries, MAX_TFTP_TIMEOUTS);
                    continue; /* retransmit */
                }
                fprintf(stderr, "%s: recvfrom error\n", progname);
                goto done;
            }

            if (ntohs(ack_pkt.optcode)  == OPTCODE_ACK &&
                ntohs(ack_pkt.No_Block) == block) {
                if (DEBUG)
                    printf("[RRQ] ACK block %u\n", block);
                break; /* advance to next block */
            }
            /* Duplicate or out-of-order ACK — discard and re-wait. */
        }

        block++;
    } while (bytes_read == MAX_DATA_SIZE);
    /* When bytes_read < MAX_DATA_SIZE the last (possibly empty) block
     * has been sent and ACKed, signalling end of transfer.           */

done:
    close(xfer_sock);
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/* Main dispatch loop                                                  */
/* ------------------------------------------------------------------ */

/* Wait for incoming TFTP packets on sockfd and dispatch them.
 * Currently only RRQ is handled; all other opcodes receive an ERROR. */
static void serve(int sockfd)
{
    char   buf[MAX_BUFF_SIZE];
    struct sockaddr cli_addr;
    socklen_t clilen;
    int    n;
    unsigned short opcode;
    RRQ    rrq;

    for (;;) {
        clilen = sizeof(cli_addr);
        n = (int)recvfrom(sockfd, buf, sizeof(buf), 0,
                          &cli_addr, &clilen);
        if (n < 0) {
            fprintf(stderr, "%s: recvfrom error\n", progname);
            continue;
        }

        /* Peek at the opcode (first two bytes, network byte order). */
        memcpy(&opcode, buf, sizeof(opcode));
        opcode = ntohs(opcode);

        if (opcode == OPTCODE_RRQ) {
            memcpy(&rrq, buf, sizeof(rrq));
            rrq.FileName[MAX_STRING_SIZE] = '\0';
            if (DEBUG)
                printf("[RRQ] request: file='%s' mode='%s'\n",
                       rrq.FileName, rrq.Mode);
            handle_rrq(&cli_addr, clilen, rrq.FileName);
        } else {
            send_error(sockfd, &cli_addr, clilen,
                       ERRCODE_ILLEGAL_OPERATION, "Unsupported opcode");
            fprintf(stderr, "%s: rejected opcode %u\n",
                    progname, opcode);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;

    progname = argv[0];
    (void)argc;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "%s: can't open datagram socket\n", progname);
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(SERV_UDP_PORT);

    if (bind(sockfd,
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "%s: can't bind local address\n", progname);
        exit(2);
    }

    printf("%s: listening on port %d\n", progname, SERV_UDP_PORT);
    serve(sockfd);

    close(sockfd);
    return 0;
}
