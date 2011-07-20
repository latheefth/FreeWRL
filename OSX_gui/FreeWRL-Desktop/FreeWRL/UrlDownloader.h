#import <Cocoa/Cocoa.h>


@interface UrlDownloader : NSOperation
{
    NSURL * _url;
    NSURLConnection * net_connection;
    NSInteger _statusCode;
    NSMutableData * receivedData;
    NSError * _error;
    
    BOOL _isExecuting;
    BOOL _isFinished;
}

@property (readonly, copy) NSURL * url;
@property (readonly) NSInteger statusCode;
@property (readonly, retain) NSData * data;
@property (readonly, retain) NSError * error;

@property (readonly) BOOL isExecuting;
@property (readonly) BOOL isFinished;

+ (id)urlDownloaderWithUrlString:(NSString *)urlString opFlag:(bool *)opFlag;

- (id)initWithUrl:(NSURL *)url origString:(NSString *)urlString;

@end
