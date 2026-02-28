/* tftp_client.c — minimal TFTP client (RRQ)
 * CSE 123 - UCSD - Fall 2012
 *
 * Usage: ./tftp_client <filename>
 *
 * Sends an RRQ for <filename> to SERV_HOST_ADDR:SERV_UDP_PORT, receives
 * DATA blocks from the server's ephemeral transfer socket, ACKs each one,
 * and writes the file contents to stdout.
 *
 * The server reply comes from a new ephemeral port (not 60010); subsequent
 * ACKs are directed to that transfer port so the server's dispatch loop
 * stays free for new requests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tftp.h"

static char *progname;

/* Send an ACK for block to addr. */
static void send_ack(int sockfd,
                     struct sockaddr *addr, socklen_t addrlen,
                     unsigned short block)
{
    ACK pkt;
    pkt.optcode  = htons(OPTCODE_ACK);
    pkt.No_Block = htons(block);
    sendto(sockfd, &pkt, sizeof(pkt), 0, addr, addrlen);
}

/* Send RRQ for filename to the server and receive the file, writing its
 * contents to stdout.  Returns 0 on success, -1 on error. */
static int get_file(int sockfd,
                    struct sockaddr *serv_addr, socklen_t servlen,
                    const char *filename)
{
    RRQ  rrq;
    DATA data_pkt;
    struct sockaddr xfer_addr; /* server's ephemeral transfer address  */
    socklen_t xfer_len;
    int  n, data_bytes;
    unsigned short block;

    /* Build and send the RRQ. */
    memset(&rrq, 0, sizeof(rrq));
    rrq.optcode = htons(OPTCODE_RRQ);
    strncpy(rrq.FileName, filename, MAX_STRING_SIZE);
    strncpy(rrq.Mode, "octet", MAX_MODE_SIZE);

    if (sendto(sockfd, &rrq, sizeof(rrq), 0,
               serv_addr, servlen) != (int)sizeof(rrq)) {
        fprintf(stderr, "%s: sendto RRQ failed\n", progname);
        return -1;
    }

    /* Receive DATA blocks until the last (short) block arrives. */
    block = 1;
    do {
        xfer_len = sizeof(xfer_addr);
        n = (int)recvfrom(sockfd, &data_pkt, sizeof(data_pkt), 0,
                          &xfer_addr, &xfer_len);
        if (n < 0) {
            fprintf(stderr, "%s: recvfrom error\n", progname);
            return -1;
        }

        /* Check for an ERROR packet from the server. */
        if (ntohs(data_pkt.optcode) == OPTCODE_ERR) {
            ERROR *err = (ERROR *)&data_pkt;
            fprintf(stderr, "%s: server error %u: %s\n",
                    progname,
                    ntohs(err->ErrorCode),
                    err->ErrMsg);
            return -1;
        }

        if (ntohs(data_pkt.optcode) != OPTCODE_DATA ||
            ntohs(data_pkt.No_Block) != block) {
            fprintf(stderr,
                    "%s: unexpected packet (opcode=%u block=%u, "
                    "expected DATA block %u)\n",
                    progname,
                    ntohs(data_pkt.optcode),
                    ntohs(data_pkt.No_Block),
                    block);
            return -1;
        }

        /* Wire bytes minus the 4-byte DATA header = payload size. */
        data_bytes = n - (int)(sizeof(data_pkt.optcode) +
                               sizeof(data_pkt.No_Block));

        /* ACK this block to the server's transfer port. */
        send_ack(sockfd, &xfer_addr, xfer_len, block);

        if (DEBUG)
            fprintf(stderr, "[RRQ] received block %u (%d bytes), ACKed\n",
                    block, data_bytes);

        fwrite(data_pkt.data, 1, data_bytes, stdout);

        block++;
    } while (data_bytes == MAX_DATA_SIZE);
    /* A block shorter than MAX_DATA_SIZE (including 0 bytes) signals
     * end of transfer.                                               */

    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in cli_addr, serv_addr;

    progname = argv[0];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", progname);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "%s: can't open socket\n", progname);
        exit(2);
    }

    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family      = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port        = htons(0); /* OS picks a free port */

    if (bind(sockfd,
             (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
        fprintf(stderr, "%s: can't bind socket\n", progname);
        exit(3);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port        = htons(SERV_UDP_PORT);

    if (get_file(sockfd,
                 (struct sockaddr *)&serv_addr, sizeof(serv_addr),
                 argv[1]) != 0) {
        close(sockfd);
        exit(4);
    }

    close(sockfd);
    return 0;
}
