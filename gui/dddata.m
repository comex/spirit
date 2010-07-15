#include <zlib.h>

@implementation NSData (DDData)

- (NSData *)gzipDeflate
{
 if ([self length] == 0) return self;
 
 z_stream strm;
 
 strm.zalloc = Z_NULL;
 strm.zfree = Z_NULL;
 strm.opaque = Z_NULL;
 strm.total_out = 0;
 strm.next_in=(Bytef *)[self bytes];
 strm.avail_in = [self length];
 
 // Compresssion Levels:
 //   Z_NO_COMPRESSION
 //   Z_BEST_SPEED
 //   Z_BEST_COMPRESSION
 //   Z_DEFAULT_COMPRESSION
 
 if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) return nil;
 
 NSMutableData *compressed = [NSMutableData dataWithLength:16384];  // 16K chunks for expansion
 
 do {
  
  if (strm.total_out >= [compressed length])
   [compressed increaseLengthBy: 16384];
  
  strm.next_out = [compressed mutableBytes] + strm.total_out;
  strm.avail_out = [compressed length] - strm.total_out;
  
  deflate(&strm, Z_FINISH);  
  
 } while (strm.avail_out == 0);
 
 deflateEnd(&strm);
 
 [compressed setLength: strm.total_out];
 return [NSData dataWithData:compressed];
}

@end
