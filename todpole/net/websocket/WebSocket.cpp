#include "todpole/net/websocket/WebSocket.h"
#include "todpole/utility/SHA1.h"
#include "todpole/utility/Base64.h"
// #include "muduo/base/Logger.h"
#include "todpole/base/StringUtil.h"
namespace muduo
{
namespace net
{
namespace ws
{

    uint16_t ntoh16(uint16_t x)
    {
        return static_cast<uint16_t>((x & 0x00ff) << 8) | ((x & 0xff00) >> 8);
    }
    uint16_t hton16(uint16_t x)
    {
        return static_cast<uint16_t>((x & 0x00ff) << 8) | ((x & 0xff00) >> 8);
    }
    uint64_t ntoh64(uint64_t x)
    {
        return (x >> 56) |
            ((x << 40) & 0x00ff000000000000LL) |
            ((x << 24) & 0x0000ff0000000000LL) |
            ((x << 8) & 0x000000ff00000000LL) |
            ((x >> 8) & 0x00000000ff000000LL) |
            ((x >> 24) & 0x0000000000ff0000LL) |
            ((x >> 40) & 0x000000000000ff00LL) |
            (x << 56);
    }
    uint64_t hton64(uint64_t x)
    {
        return (x >> 56) |
            ((x << 40) & 0x00ff000000000000LL) |
            ((x << 24) & 0x0000ff0000000000LL) |
            ((x << 8) & 0x000000ff00000000LL) |
            ((x >> 8) & 0x00000000ff000000LL) |
            ((x >> 24) & 0x0000000000ff0000LL) |
            ((x >> 40) & 0x000000000000ff00LL) |
            (x << 56);
    }

    static void generateHash(char buffer[], size_t bufferlen)
    {
        /*byte*/int8_t bytes[16] = {0};
        for(int i = 0; i < 16; i++)
        {
            bytes[i] = static_cast<int8_t>(random() % 255);
        }
        muduo::util::base64Encode(reinterpret_cast<char *>(bytes), 16, buffer);
    }

    std::string makeHandshakeRequest(const std::string& url)
    {
        std::string buffer;
        buffer.reserve(4096);
        muduo::base::stringFormatAppend(&buffer, "GET /%s HTTP/1.1\r\n", url.c_str());
        //zl::base::stringFormatAppend(&buffer, "Host: %s:%d\r\n", url.c_str());
        muduo::base::stringFormatAppend(&buffer, "Upgrade: websocket\r\n");
        muduo::base::stringFormatAppend(&buffer, "Connection: Upgrade\r\n");

        char hash[45] = {0};
        generateHash(hash, 45);
        muduo::base::stringFormatAppend(&buffer, "Sec-WebSocket-Key: %s\r\n", hash);
        muduo::base::stringFormatAppend(&buffer, "Connection: Upgrade\r\n");

        //zl::base::stringFormatAppend(&buffer, "Sec-WebSocket-Protocol: %s\r\n", "");
        muduo::base::stringFormatAppend(&buffer, "Sec-WebSocket-Version: %d\r\n", 13);  /// 钦定了版本号

        muduo::base::stringFormatAppend(&buffer, "\r\n");
        return buffer;
    }

    std::string makeHandshakeResponse(const char* seckey)
    {
        std::string answer("");
        answer += "HTTP/1.1 101 Switching Protocols\r\n";
        answer += "Upgrade: WebSocket\r\n";
        answer += "Connection: Upgrade\r\n";
        answer += "Sec-WebSocket-Version: 13\r\n";
        if(seckey)
        {
            std::string key(seckey);
            key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            char shakey[20] = {0};
            muduo::util::SHA1 sha1;
            sha1.update(key);
            sha1.final(shakey);
            // LOG_WARN("get SHA1 : %s, %s", key.c_str(), shakey);
            key = muduo::util::base64Encode(shakey, 20);
            // LOG_WARN("get base64 : %s", key.c_str());
            answer += ("Sec-WebSocket-Accept: "+ key + "\r\n");
        }
        answer += "\r\n";
        return answer;
    }

    /*
           0                   1                   2                   3
           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
           +-+-+-+-+-------+-+-------------+-------------------------------+
           |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
           |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
           |N|V|V|V|       |S|             |   (if payload len==126/127)   |
           | |1|2|3|       |K|             |                               |
           +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
           |     Extended payload length continued, if payload len == 127  |
           + - - - - - - - - - - - - - - - +-------------------------------+
           |                               |Masking-key, if MASK set to 1  |
           +-------------------------------+-------------------------------+
           | Masking-key (continued)       |          Payload Data         |
           +-------------------------------- - - - - - - - - - - - - - - - +
           :                     Payload Data continued ...                :
           + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
           |                     Payload Data continued ...                |
           +---------------------------------------------------------------+

            opcode:
                 *  %x0 denotes a continuation frame
                 *  %x1 denotes a text frame
                 *  %x2 denotes a binary frame
                 *  %x3-7 are reserved for further non-control frames
                 *  %x8 denotes a connection close
                 *  %x9 denotes a ping
                 *  %xA denotes a pong
                 *  %xB-F are reserved for further control frames

           Payload length:  7 bits, 7+16 bits, or 7+64 bits

           Masking-key:  0 or 4 bytes
    */
    WsFrameType decodeFrame(const char* inbuf, int inlength, std::vector<char>* outbuf)
    {
        if(inlength < 3) return WS_INCOMPLETE_FRAME;

        const unsigned char* inp = reinterpret_cast<const unsigned char*>(inbuf);

        unsigned char msg_opcode = inp[0] & 0x0F;
        unsigned char msg_fin = (inp[0] >> 7) & 0x01;
        unsigned char msg_masked = (inp[1] >> 7) & 0x01;

        int payload_length = 0;
        int pos = 2;
        int length_field = inp[1] & (~0x80);
        unsigned int mask = 0;

        if(length_field <= PAYLOAD_SIZE_BASIC)
        {
            payload_length = length_field;
        }
        else if(length_field == 126)  //msglen is 16bit!
        {
            payload_length = (inp[2] << 8) + inp[3];
            pos += 2;
        }
        else if(length_field == 127)  //msglen is 64bit!
        {
            payload_length = static_cast<int>(static_cast<unsigned long long>(inp[2]) << 56) + static_cast<int>(static_cast<unsigned long long>(inp[3]) << 48) +
                    static_cast<int>(static_cast<unsigned long long>(inp[4]) << 40) + static_cast<int>(static_cast<unsigned long long>(inp[5]) << 32) +
                    static_cast<int>(static_cast<unsigned long long>(inp[6]) << 24) + static_cast<int>(static_cast<unsigned long long>(inp[7]) << 16) +
                    static_cast<int>(static_cast<unsigned long long>(inp[8]) << 8)  + static_cast<int>(static_cast<unsigned long long>(inp[9]) << 0);
            payload_length = payload_length / 2;
            pos += 8;
        }

        if(inlength < payload_length+pos)
        {
            return WS_INCOMPLETE_FRAME;
        }
        else if(static_cast<unsigned int>(payload_length) > outbuf->size())
        {
            outbuf->resize(payload_length + 1);
        }

        if(msg_masked)
        {
            // mask = *((unsigned int*)(inp+pos));
            mask = *(reinterpret_cast<const unsigned int*>(inp+pos));
            pos += 4;
            memcpy(&*outbuf->begin(), reinterpret_cast<const void *>(inp+pos), payload_length);
            // unmask data:
            char* c = &*outbuf->begin();
            for(int i = 0; i < payload_length; i++)
            {
                c[i] = static_cast<char>(c[i] ^ (reinterpret_cast<unsigned char*>(&mask))[i%4]);
            }
        }
        (*outbuf)[payload_length] = 0;

        if(0)
        {
            printf("IN:");
            for(int i=0; i < 20; i++)
                printf("%02x ",inp[i]);
            printf("\n");
            printf("OPCODE : %d\n", msg_opcode);
            printf("PAYLOAD_LEN: %08x\n", payload_length);
            printf("MASK: %08x\n", mask);
            printf("TEXT: %s\n", outbuf->data());
        }
        if(msg_opcode == 0x0) return (msg_fin) ? WS_TEXT_FRAME : WS_INCOMPLETE_TEXT_FRAME; // continuation frame ?
        if(msg_opcode == 0x1) return (msg_fin) ? WS_TEXT_FRAME : WS_INCOMPLETE_TEXT_FRAME;
        if(msg_opcode == 0x2) return (msg_fin) ? WS_BINARY_FRAME : WS_INCOMPLETE_BINARY_FRAME;
        if(msg_opcode == 0x8) return WS_CLOSE_FRAME;
        if(msg_opcode == 0x9) return WS_PING_FRAME;
        if(msg_opcode == 0xA) return WS_PONG_FRAME;

        return WS_ERROR_FRAME;
    }

    int encodeFrame(WsFrameType frame_type, const char* msg, int msg_length, char* outbuf, size_t bufsize)
    {
        //std::cout << "makeFrame : " <<  frame_type << "\t" << msg << "\t" << msg_length << "\n";
        int pos = 0;
        int size = msg_length;
        outbuf[pos++] = static_cast<unsigned char>(frame_type); // text frame
        if(size <= PAYLOAD_SIZE_BASIC)  // 125
        {
            outbuf[pos++] = static_cast<char>(size);
        }
        else if(size <= PAYLOAD_SIZE_EXTENDED)   // 65535
        {
            outbuf[pos++] = 126; //16 bit length
            outbuf[pos++] = static_cast<char>((size >> 8) & 0xFF); // rightmost first
            outbuf[pos++] = static_cast<char>(size & 0xFF);
        }
        else  // >2^16-1
        {
            outbuf[pos++] = 127; //64 bit length

            // write 8 bytes length (significant first)
            // since msg_length is int it can be no longer than 4 bytes = 2^32-1
            // padd zeroes for the first 4 bytes
            for(int i=3; i>=0; i--)
            {
                outbuf[pos++] = 0;
            }
            // write the actual 32bit msg_length in the next 4 bytes
            for(int i=3; i>=0; i--)
            {
                outbuf[pos++] = static_cast<char>((size >> 8*i) & 0xFF);
            }
        }
        memcpy(reinterpret_cast<void*>(outbuf+pos), msg, size);
        //printf("makeFrame [%s]\n", outbuf);
        //printf("makeFrame [%s]\n", outbuf + 2);
        return (size+pos);
    }

}  }  }  // namespace muduo { namespace net { namespace ws {