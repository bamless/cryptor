#ifndef PROTOCOL_H
#define PROTOCOL_H

#define LSTF "LSTF"
#define LSTR "LSTR"
#define ENCR "ENCR"
#define DECR "DECR"

#define CMD_LEN 4 //length of cmd str
#define UINT_LEN 10 //max length of 32 bit integer in base 10
#define MAX_PROT_PATH 512 //the maximum path length the protocol can handle

#define MAX_CMDLINE_LEN (CMD_LEN + UINT_LEN + MAX_PROT_PATH + 2) //max lenght of a command line. The +2 is for the 2 spaces

#define DEFAULT_PORT 8888 //the server default port


#endif
