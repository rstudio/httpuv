#ifndef BASE64_H
#define BASE64_H

void encodeblock( unsigned char in[3], unsigned char out[4], int len );

template <class InputIterator>
std::string b64encode(InputIterator begin, InputIterator end)
{
   unsigned char in[3], out[4];
   int i, len;

   std::string databuf;

   while( begin != end ) {
      len = 0;
      for( i = 0; i < 3; i++ ) {
         if( begin != end ) {
            in[i] = static_cast<unsigned char>( *begin++ );
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

#endif
