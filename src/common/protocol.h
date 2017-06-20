#ifndef PROTOCOL_H
#define PROTOCOL_H

#define LSTF "LSTF"
#define LSTR "LSTR"
#define ENCR "ENCR"
#define DECR "DECR"

#define RETOK_INT 200
#define RETMORE_INT 300
#define RETERR_INT 400
#define RETERRTRANS_INT 500
#define RETOK "200"
#define RETMORE "300"
#define RETERR "400"
#define RETERRTRANS "500"

#define CMD_LEN 4 //length of cmd str
#define UINT_LEN 10 //max length of 32 bit integer in base 10

#define DEFAULT_PORT 8888 //the server default port


#endif
