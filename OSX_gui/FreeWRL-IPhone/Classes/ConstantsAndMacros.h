//
//  ConstantsAndMacros.h
//  Particles
//

// How many times a second to refresh the screen
#if TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
#define kRenderingFrequency             15.0
#define kInactiveRenderingFrequency     5.0
#else
#define kRenderingFrequency             30.0
#define kInactiveRenderingFrequency     3.0
#endif

// Defines whether to setup and use a depth buffer
#define USE_DEPTH_BUFFER                1
// Set to 1 if you want it to attempt to create a 2.0 context
