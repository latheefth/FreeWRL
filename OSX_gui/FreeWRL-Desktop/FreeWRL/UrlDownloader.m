#import "UrlDownloader.h"
#import "/freeWRL/freewrl/freex3d/src/lib/libFreeWRL.h"

bool *opFlagPtr;

// For when the files are local...
char *cString = NULL;
int cStringLen = 0;
NSString *net_url = nil;


@interface UrlDownloader ()

- (void)finish;

@end

@implementation UrlDownloader

@synthesize url = _url;
@synthesize statusCode = _statusCode;
@synthesize data = receivedData;
@synthesize error = _error;
@synthesize isExecuting = _isExecuting;
@synthesize isFinished = _isFinished;

- (void) awakeFromNib
{
    //NSLog (@"URL: awakeFromNib");
}


+ (id)urlDownloaderWithUrlString:(NSString *)urlString opFlag:(bool *)opFlag
{
    
    NSURL * url = [NSURL URLWithString:urlString];
    UrlDownloader * operation = [[self alloc] initWithUrl:url origString:urlString];
    // save the pointer to the "we are downloading already" flag
    opFlagPtr = opFlag;
    
    return [operation autorelease];
}

- (id)initWithUrl:(NSURL *)url origString:urlString;
{
    //NSLog (@"URL: initWithUrl");
    
    self = [super init];
    if (self == nil)
        return nil;
    
    _url = [url copy];
    net_url = [urlString copy];
    //NSLog (@"URL: _url %p",_url);
    //NSLog (@"URL: net_url %s",[net_url UTF8String]);
    
    _isExecuting = NO;
    _isFinished = NO;
    
    return self;
}

- (void)dealloc
{
    //NSLog (@"URL = dealloc");
    [_url release];
    [net_connection release];
    [receivedData release];
    [_error release];
    [super dealloc];
}

- (BOOL)isConcurrent
{
    //NSLog (@"URL = isConcurrent");
    return YES;
    //return NO;
}

- (void)start
{
    //NSLog (@"URL - start");
    if (![NSThread isMainThread])
    {
        [self performSelectorOnMainThread:@selector(start) withObject:nil waitUntilDone:NO];
        return;
    }
    
    // NSLog(@"opeartion for <%@> started.", _url);
    
    //if (*opFlagPtr) {
    //    NSLog (@"UrlDownloaderStart - flag says downloading");
    //    
    //} else {
    //    NSLog (@"UrlDownloaderStart - flag says not downloading");
    //}
    
    [self willChangeValueForKey:@"isExecuting"];
    _isExecuting = YES;
    [self didChangeValueForKey:@"isExecuting"];

    // set up connection to be nil
    net_connection = nil;
    cString = NULL;
    cStringLen = 0;
    
    
    // is this a local file, or a network file?
    char *ascii_url = (char *)[net_url UTF8String];
    //NSLog (@"URL: ascii_url %s",ascii_url);
    
    // call to library
    if (checkNetworkFile(ascii_url)) {
        //NSLog (@" this is a netowrk file");
        
        NSURLRequest * request = [NSURLRequest requestWithURL:_url];
        net_connection = [[NSURLConnection alloc] initWithRequest:request
                                                         delegate:self];
    } else {
        //NSLog (@"URL: url is not a network url, get it locally");
         //NSError *error;
        
        // just open the file, if it exists
        NSData *texData = [[NSData alloc] initWithContentsOfFile:net_url];
        
        
        if (texData == nil) {
            //NSLog (@"texData nil, returning zero");
        } else {
            cStringLen = (int)[texData length];
            cString = malloc(cStringLen+2); // pad it out by 2 chars...
            [texData getBytes:cString];
        
            //NSLog (@"got data, returning length %d",cStringLen);
        }
        
    }

    
    
    //NSLog (@"at end, the connection is %p",net_connection);
    
    if (net_connection == nil)
        [self finish];
}

- (void)finish
{
    //NSLog(@"operation for <%@> finished. "
      //    @"status code: %d, error: %@, data size: %u",
        //  _url, _statusCode, _error, [receivedData length]);
    
    [net_connection release];
    net_connection = nil;
    
    [self willChangeValueForKey:@"isExecuting"];
    [self willChangeValueForKey:@"isFinished"];

    _isExecuting = NO;
    _isFinished = YES;

    [self didChangeValueForKey:@"isExecuting"];
    [self didChangeValueForKey:@"isFinished"];
    
    //if (*opFlagPtr) {
      //  NSLog (@"UrlDownloaderFinish - flag says downloading");
    //    
    //} else {
      //  NSLog (@"UrlDownloaderFinish - flag says not downloading");
    //}

    
    if (_error != nil) {
        //NSLog (@"URL: have error here");
        
        // tell front end that things failed
        fwg_frontEndReturningData(NULL,0);
        
    }else {
        // what is the status code? if it is 404, we have an error...
        if (_statusCode == 404) {
            //NSLog (@"URL: 404 ");
            fwg_frontEndReturningData(NULL,0);
            
        } else {
            //NSLog (@"URL: good download, here's the data");
            
            if (cStringLen != 0) {
                fwg_frontEndReturningData((unsigned char*)cString, cStringLen);
                if (cString != NULL)
                    free (cString);
            } else {
                int len;
                unsigned char *myData;
                len = (int)[receivedData length];
                myData = malloc (len+2); // pad it out by 2 chars
                [receivedData getBytes:myData];
                
                //printf ("URL: mydata %s\n",myData);
        
                fwg_frontEndReturningData((unsigned char*)myData,len);
        
                free (myData);
            }
        }
    }

    // tell gui that we can get another file
    *opFlagPtr = false;

}

#pragma mark -
#pragma mark NSURLConnection delegate

- (void)connection:(NSURLConnection *)connection
didReceiveResponse:(NSURLResponse *)response
{
    [receivedData release];
    receivedData = [[NSMutableData alloc] init];

    NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *)response;
    _statusCode = [httpResponse statusCode];
}

- (void)connection:(NSURLConnection *)connection
    didReceiveData:(NSData *)data
{
    [receivedData appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    [self finish];
}

- (void)connection:(NSURLConnection *)connection
  didFailWithError:(NSError *)error
{
    _error = [error copy];
    [self finish];
}

@end
