// http://deusty.blogspot.com/2007/07/gzip-compressiondecompression.html
// Thanks!

@interface NSData (DDData)

// gzip compression utilities
- (NSData *)gzipDeflate;

@end



