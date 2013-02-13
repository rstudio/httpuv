#include <stdint.h>
#include <string>
#include <vector>

/*
 * Translation Table as described in RFC1113
 */
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * encodeblock
 *
 * encode 3 8-bit binary bytes as 4 '6-bit' characters
 */
void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
   out[0] = cb64[ in[0] >> 2 ];
   out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
   out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
   out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

std::string b64encode(const std::vector<uint8_t>& data)
{
   unsigned char in[3], out[4];
   int i, len;

   size_t data_len = data.size();
   int data_elem = 0;
   std::string databuf;

   while( data_elem < data_len ) {
      len = 0;
      for( i = 0; i < 3; i++ ) {
         if( data_elem < data_len ) {
            in[i] = (unsigned char) data[data_elem++];
            len++;
         } else {
            in[i] = 0;
         }
      }
      if( len ) {
         encodeblock( in, out, len );
         for( i = 0; i < 4; i++ ) {
            databuf.push_back(out[i]);
         }
      }
   }

   return databuf;
}
