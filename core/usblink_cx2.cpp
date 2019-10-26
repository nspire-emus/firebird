#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <sys/time.h>

#include "emu.h"
#include "usblink.h"
#include "usblink_cx2.h"
#include "usb_cx2.h"

// #define DEBUG

enum Address {
    AddrAll		= 0xFF,
    AddrMe		= 0xFE,
    AddrCalc	= 0x01
};

enum Service {
    AddrReqService  = 0x01,
    TimeService     = 0x02,
    EchoService     = 0x03,
    StreamService   = 0x04,
    TransmitService = 0x05,
    LoopbackService = 0x06,
    StatsService    = 0x07,
    UnknownService  = 0x08,
    AckFlag         = 0x80
};

// Big endian!
struct NNSEMessage {
    uint8_t 	misc;		// Unused?
    uint8_t		service;	// Service number. If bit 7 set, an ACK
    uint8_t     src;		// Address of the source
    uint8_t     dest;		// Address of the destination
    uint8_t     unknown;	// No idea
    uint8_t     reqAck;		// 0x1: Whether an ack is expected, 0x9: Not the first try
    uint16_t    length;		// Length of the packet, including this header
    uint16_t    seqno;		// Sequence number. Increases by one for every non-ACK packet.
    uint16_t    csum;		// Checksum. Inverse of the 16bit modular sum with carry added.

    uint8_t     data[0];
} __attribute__((packed));

struct NNSEMessage_AddrReq {
    NNSEMessage hdr;
    uint8_t     code; // 00
    uint8_t     clientID[64];
} __attribute__((packed));

struct NNSEMessage_AddrResp {
    NNSEMessage hdr;
    uint8_t     addr;
} __attribute__((packed));

struct NNSEMessage_UnkResp {
    NNSEMessage hdr;
    uint8_t     noidea[2]; // 80 03
} __attribute__((packed));

struct NNSEMessage_TimeReq {
    NNSEMessage hdr;
    uint8_t     code;
} __attribute__((packed));

struct NNSEMessage_TimeResp {
    NNSEMessage hdr;
    uint8_t     noidea; // 80
    uint32_t    sec;
    uint64_t    frac;
    uint32_t    frac2;
} __attribute__((packed));

struct usblink_cx2_state usblink_cx2_state;

#ifdef DEBUG
static void dumpPacket(const NNSEMessage *message)
{
    printf("Misc:   \t%02x\n", message->misc);
    printf("Service:\t%02x\n", message->service);
    printf("Src:    \t%02x\n", message->src);
    printf("Dest:   \t%02x\n", message->dest);
    printf("Unknown:\t%02x\n", message->unknown);
    printf("ReqAck: \t%02x\n", message->reqAck);
    printf("Length: \t%04x\n", ntohs(message->length));
    printf("SeqNo:  \t%04x\n", ntohs(message->seqno));
    printf("Csum:   \t%04x\n", ntohs(message->csum));

    auto datalen = ntohs(message->length) - sizeof(NNSEMessage);
    for(auto i = 0u; i < datalen; ++i)
        printf("%02x ", message->data[i]);

    printf("\n");
}
#endif

static uint16_t compute_checksum(const uint8_t *data, uint32_t size)
{
    uint32_t acc = 0;

    if (size > 0)
    {
        for (uint32_t i = 0; i < size - 1; i += 2)
        {
            uint16_t cur = (((uint16_t)data[i]) << 8) | data[i + 1];
            acc += cur;
        }

        if (size & 1)
            acc += ((uint16_t)data[size - 1]) << 8;
    }

    while (acc >> 16)
        acc = (acc >> 16) + uint16_t(acc);

    return acc;
}

static bool writePacket(NNSEMessage *message)
{
    auto length = ntohs(message->length);

    message->csum = 0;
    message->csum = htons(compute_checksum(reinterpret_cast<uint8_t*>(message), length) ^ 0xFFFF);

    if(compute_checksum(reinterpret_cast<uint8_t*>(message), length) != 0xFFFF)
    {
        error("Failed to compute checksum\n");
        return false;
    }

#ifdef DEBUG
    printf("Sending packet:\n");
    dumpPacket(message);
#endif

    return usb_cx2_packet_to_calc(1, reinterpret_cast<uint8_t*>(message), length);
}

static uint16_t nextSeqno()
{
    return usblink_cx2_state.seqno++;
}

template <typename T> bool sendMessage(T &message)
{
    message.hdr.src = AddrMe;
    message.hdr.dest = AddrCalc;
    message.hdr.length = htons(sizeof(T));
    message.hdr.seqno = htons(nextSeqno());

    return writePacket(&message.hdr);
}

template <typename T> T* messageCast(NNSEMessage *message)
{
    if(ntohs(message->length) < sizeof(T))
        return nullptr;

    return reinterpret_cast<T*>(message);
}

template <typename T> const T* messageCast(const NNSEMessage *message)
{
    if(ntohs(message->length) < sizeof(T))
        return nullptr;

    return reinterpret_cast<const T*>(message);
}

static void handlePacket(const NNSEMessage *message, const uint8_t **streamdata = nullptr, int *streamsize = nullptr)
{
    if(message->dest != AddrMe && message->dest != AddrAll)
    {
#ifdef DEBUG
        printf("Not for me?\n");
#endif
        return;
    }

    if(message->service & AckFlag)
    {
#ifdef DEBUG
        printf("Got ack for %02x\n", ntohs(message->seqno));
#endif

        if((message->service & (~AckFlag)) == StreamService)
        {
            // Tell usblink that an ack arrived
            usblink_received_packet(nullptr, 0);
        }
        return;
    }

    if(message->reqAck & 1)
    {
        NNSEMessage ack = {
            .misc = message->misc,
            .service = uint8_t(message->service | AckFlag),
            .src = message->dest,
            .dest = message->src,
            .unknown = message->unknown,
            .reqAck = uint8_t(message->reqAck & ~1),
            .length = htons(sizeof(NNSEMessage)),
            .seqno = message->seqno,
        };

        if(!writePacket(&ack))
            printf("Failed to ack\n");
    }

    if(message->reqAck & 8)
    {
        // There's no proper seqid tracking, but shouldn't be necessary
        printf("Got packet with failed ack flag (seqid %d) - ignoring\n", ntohs(message->seqno));
        return;
    }

    switch(message->service & ~AckFlag)
    {
    case AddrReqService:
    {
        const NNSEMessage_AddrReq *req = messageCast<NNSEMessage_AddrReq>(message);
        if(!req || req->code != 0)
            goto drop;

#ifdef DEBUG
        printf("Got request from client %s (product id %c%c)\n", &req->clientID[12], req->clientID[10], req->clientID[11]);
#endif
/*      Apparently this isn't absolutely necessary.
        For some reason sending this breaks the usb stack enough that the time request
        doesn't get sent.

        NNSEMessage_AddrResp resp = {
            .hdr = {
                .service = message->service,
            },
            .addr = AddrCalc,
        };

        if(!sendMessage(resp))
            printf("Failed to send message\n");
*/
        NNSEMessage_AddrResp resp2 = {
            .hdr = {
                .service = message->service,
            },
            .addr = 0x80, // No idea
        };

        if(!sendMessage(resp2))
            printf("Failed to send message\n");

        break;
    }
    case TimeService:
    {
        const NNSEMessage_TimeReq *req = messageCast<NNSEMessage_TimeReq>(message);
        if(!req || req->code != 0)
            goto drop;

#ifdef DEBUG
        printf("Got time request\n");
#endif

        struct timeval val;
        gettimeofday(&val, nullptr);

        NNSEMessage_TimeResp resp = {
            .hdr = {
                .service = message->service,
            },
            .noidea = 0x80,
            .sec = htonl(uint32_t(val.tv_sec)),
            .frac = 0,
        };

        if(!sendMessage(resp))
            printf("Failed to send message\n");

        usblink_cx2_state.handshake_complete = true;

        gui_status_printf("usblink connected.");
        usblink_connected = true;
        gui_usblink_changed(true);

        break;
    }
    case UnknownService:
    {
        if(ntohs(message->length) != sizeof(NNSEMessage) + 1 || message->data[0] != 0x01)
            goto drop;

#ifdef DEBUG
        printf("Got packet for unknown service\n");
#endif

        NNSEMessage_UnkResp resp = {
            .hdr = {
                .service = message->service,
            },
            .noidea = {0x81, 0x03},
        };

        if(!sendMessage(resp))
            printf("Failed to send message\n");

        break;
    }
    case StreamService:
    {
        if(streamdata)
            *streamdata = message->data;
        if(streamsize)
            *streamsize = ntohs(message->length) - sizeof(NNSEMessage);

        break;
    }
    default:
        printf("Unhandled service %02x\n", message->service & ~AckFlag);
    }

    return;

drop:
    printf("Ignoring packet.\n");
}

bool usblink_cx2_send_navnet(const uint8_t *data, uint16_t size)
{
    if(!usblink_cx2_state.handshake_complete)
        return false;

    int len = sizeof(NNSEMessage) + size;
    NNSEMessage *msg = reinterpret_cast<NNSEMessage*>(malloc(len));
    *msg = {
        .service = StreamService,
        .src = AddrMe,
        .dest = AddrCalc,
        .reqAck = 1,
        .length = htons(len),
        .seqno = htons(nextSeqno()),
    };
    memcpy(msg->data, data, size);

    bool ret = writePacket(msg);

    free(msg);

    return ret;
}

bool usblink_cx2_handle_packet(const uint8_t *data, uint16_t size)
{
    if(size < sizeof(NNSEMessage))
        return false;

    const NNSEMessage *message = reinterpret_cast<const NNSEMessage*>(data);

    const auto completeLength = ntohs(message->length);

    // The nspire code has if(size & 0x3F == 0) ++size; for some reason
    if(size != completeLength + (completeLength % 64 == 0))
    {
        error("Got too small or too big packet");
        return false;
    }

#ifdef DEBUG
    printf("Got packet:\n");
    dumpPacket(message);
#endif

    if(compute_checksum(reinterpret_cast<const uint8_t*>(message), completeLength) != 0xFFFF)
        return false;

    const uint8_t *streamdata = nullptr;
    int streamsize = 0;
    handlePacket(message, &streamdata, &streamsize);

    if(streamsize)
        usblink_received_packet(streamdata, streamsize);

    return true;
}

void usblink_cx2_reset()
{
    usblink_cx2_state.seqno = 0;
    usblink_cx2_state.handshake_complete = false;
    usblink_reset();
}
