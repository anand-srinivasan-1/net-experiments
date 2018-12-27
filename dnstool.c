#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

typedef enum {
    A = 1,
    NS = 2,
    CNAME = 5,
    MX = 15,
    TXT = 16
} rr_type;

char *rr_type_to_str(rr_type rt) {
    switch(rt) {
    case A:
        return "A";
    case NS:
        return "NS";
    case CNAME:
        return "CNAME";
    case MX:
        return "MX";
    case TXT:
        return "TXT";
    default:
        return "unknown record";
    }
}

char *class_to_str(uint16_t class) {
    switch(class) {
    case 1:
        return "IN";
    default:
        return "unknown class";
    }
}

void write_byte(uint8_t *dst, uint8_t data, int *idx, int maxlen) {
    if(maxlen - *idx >= 1) {
        dst[*idx] = data;
        (*idx)++;
    }
}

void write_short(uint8_t *dst, uint16_t data, int *idx, int maxlen) {
    write_byte(dst, data >> 8, idx, maxlen);
    write_byte(dst, data & 255, idx, maxlen);
}

void write_name(uint8_t *dst, char *data, int *idx, int maxlen) {
    int dlen = strlen(data);
    int ctr = 0;
    while(ctr < dlen) {
        int label_size = 0;
        while(data[ctr+label_size] != '.' && data[ctr+label_size] != '\0') {
            label_size++;
        }

        write_byte(dst, label_size, idx, maxlen);
        for(int i = 0; i < label_size; i++) {
            write_byte(dst, data[ctr+i], idx, maxlen);
        }

        ctr += label_size+1;
    }

    write_byte(dst, 0, idx, maxlen);
}

void print_name(uint8_t *data, int *idx, int len) {
    if(*idx >= len - 1) {
        return;
    }

    int i;
    if((data[*idx] & 0xC0) != 0) {
        i = ((data[*idx] & 0x3F) << 8) | data[*idx+1];
    } else {
        i = *idx;
    }

    while(i < len) {
        uint8_t label_size = data[i];
        i++;

        if(label_size == 0 || i + label_size >= len) {
            break;
        }

        for(int j = 0; j < label_size; j++) {
            printf("%c", data[i+j]);
        }
        printf(".");

        i += label_size;
    }

    if((data[*idx] & 0xC0) != 0) {
        *idx += 2;
    } else {
        *idx += i;
    }
}

void create_query(uint8_t *query, uint16_t *qlen, char *domain, rr_type t) {
    int i = 0;
    write_short(query, 0x1234, &i, 512); // ID
    write_short(query, 0, &i, 512); // various fields
    write_short(query, 1, &i, 512); // QDCOUNT
    write_short(query, 0, &i, 512); // ANCOUNT
    write_short(query, 0, &i, 512); // NSCOUNT
    write_short(query, 0, &i, 512); // ARCOUNT

    write_name(query, domain, &i, 512);
    write_short(query, (uint16_t) t, &i, 512);
    write_short(query, 1, &i, 512); /* IN */

    *qlen = i;
}

void print_section(uint8_t *data, int *idx, int len) {
    print_name(data, idx, len);

    printf(" ");
    uint16_t rt;
    if(*idx < len-2) {
        rt = data[*idx]*256 + data[*idx+1];
    } else {
        rt = 0;
    }
    *idx += 2;

    uint16_t class;
    if(*idx < len-2) {
        class = data[*idx]*256 + data[*idx+1];
    } else {
        class = 0;
    }
    *idx += 2; // class

    uint32_t ttl;
    if(*idx < len-4) {
        ttl = (data[*idx] << 24) | (data[*idx+1] << 16) | (data[*idx+2] << 8) | data[*idx+3];
    } else {
        ttl = 0;
    }
    *idx += 4;

    uint16_t rdlength;
    if(*idx < len-2) {
        rdlength = data[*idx]*256 + data[*idx+1];
    } else {
        rdlength = 0;
    }
    *idx += 2;

    int ip[4];
    if(rt == A) {
        ip[0] = data[*idx];
        ip[1] = data[*idx+1];
        ip[2] = data[*idx+2];
        ip[3] = data[*idx+3];
    }

    *idx += rdlength;

    printf("type: %d (%s), class: %d (%s), ttl: %d", rt, rr_type_to_str(rt), class, class_to_str(class), ttl);

    if(rt == A) {
        printf(" (%d.%d.%d.%d)\n", ip[0], ip[1], ip[2], ip[3]);
    } else {
        printf("\n");
    }
}

void print_response(uint8_t *data, int len) {
    int aa = (data[2] >> 2) & 1;
    int tc = (data[2] >> 1) & 1;
    int rcode = data[3] & 15;

    int qdcount = data[4]*256 + data[5];
    int ancount = data[6]*256 + data[7];
    int nscount = data[8]*256 + data[9];
    int arcount = data[10]*256 + data[11];

    printf("AA: %d\nTC: %d\nRCODE: %d\n", aa, tc, rcode);
    printf("QDCOUNT: %d\nANCOUNT: %d\nNSCOUNT: %d\nARCOUNT: %d\n", qdcount, ancount, nscount, arcount);
    puts("");

    int idx = 12;

    // consume question section
    for(int i = 0; i < qdcount; i++) {
        while(data[idx] != 0 && idx < len) {
            idx++;
        }
        idx++;
        idx += 4;
    }

    puts("answer section:");
    for(int i = 0; i < ancount; i++) {
        print_section(data, &idx, len);
    }
    puts("");

    puts("authority section:");
    for(int i = 0; i < nscount; i++) {
        print_section(data, &idx, len);
    }
    puts("");

    puts("additional section:");
    for(int i = 0; i < arcount; i++) {
        print_section(data, &idx, len);
    }
    puts("");
}

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("usage: %s <recordtype> <domain> <server>\n", argv[0]);
        return 1;
    }

    rr_type rt;
    if(strcasecmp(argv[1], "a") == 0) {
        rt = A;
    } else if(strcasecmp(argv[1], "ns") == 0) {
        rt = NS;
    } else if(strcasecmp(argv[1], "cname") == 0) {
        rt = CNAME;
    } else if(strcasecmp(argv[1], "mx") == 0) {
        rt = MX;
    } else if(strcasecmp(argv[1], "txt") == 0) {
        rt = TXT;
    } else {
        puts("unknown record type");
        return 1;
    }

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s == -1) {
        puts("socket() failed");
        return 1;
    }

    uint8_t query[512];
    uint16_t qlen;
    create_query(query, &qlen, argv[2], rt);

    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = inet_addr(argv[3]),
    };
    sendto(s, query, qlen, 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    uint8_t response[1024];
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
        .sin_addr.s_addr = inet_addr(argv[3]),
    };
    socklen_t tmp = sizeof(server_addr);
    int rlen = recvfrom(s, response, 1024, 0, (struct sockaddr*) &server_addr, &tmp);
    printf("received %d bytes\n", rlen);
    print_response(response, rlen);

    close(s);

    return 0;
}
