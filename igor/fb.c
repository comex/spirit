#define CFCOMMON
#define IOSFC_BUILDING_IOSFC
#include "common.h"
#include <stdbool.h>
#include <IOKit/IOKitLib.h>
#include <IOSurface/IOSurface.h>
#include <IOMobileFramebuffer/IOMobileFramebuffer.h>
#include <CoreSurface/CoreSurface.h>
#include <QuartzCore/QuartzCore.h>
#include "fb.h"

#define BAR_BG 0xff9f9f9f
#define BAR_FG 0xffffffff

static IOMobileFramebufferConnection conn;
static CoreSurfaceBufferRef surface, oldsurface;
static size_t bufwidth, bufheight, barwidth;
static bool flipped;

static void draw_background(void *base) {
    CGDataProviderRef provider = CGDataProviderCreateWithFilename("/var/mobile/Media/spirit/bg.jpg");
    CGImageRef img = CGImageCreateWithJPEGDataProvider(provider, NULL, false, kCGRenderingIntentPerceptual);
    CGColorSpaceRef color = CGColorSpaceCreateDeviceRGB();
    CGContextRef cgctx = CGBitmapContextCreate(base, CGImageGetWidth(img), CGImageGetHeight(img), 8, 4*CGImageGetWidth(img), color, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(color);
    CGContextDrawImage(cgctx, CGRectMake(0, 0, CGImageGetWidth(img), CGImageGetHeight(img)), img);
    CGContextRelease(cgctx);
    CGImageRelease(img);
    CGDataProviderRelease(provider);
}

static void fb_swap() {
    if(surface == oldsurface) {
        // On iPhone, this makes it flash to black.
        // Rather than figure out why, I will be lazy and not swap
        // (because I don't have to).
        return;
    }
    TRY(fb_swap_begin, IOMobileFramebufferSwapBegin(conn, NULL));
    TRY(fb_swap_sl1, IOMobileFramebufferSwapSetLayer(conn, 0, surface));
    TRY(fb_swap_sl1, IOMobileFramebufferSwapSetLayer(conn, 1, 0));
    TRY(fb_swap_sl1, IOMobileFramebufferSwapSetLayer(conn, 2, 0));
    TRY(fb_swap_end, IOMobileFramebufferSwapEnd(conn));
}

void gasgauge_init() {
    char *names[] = { "AppleH1CLCD", "AppleM2CLCD", "AppleCLCD" };
    char *name;
    io_service_t svc;
    for(int i = 0; i < sizeof(names); i++) {
        if(svc = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(name = names[i])))
            goto done;
    }
    TRY(find_a_lcd, false);
    done: (void)0;
    
    TRY(fb_open, IOMobileFramebufferOpen(svc, mach_task_self(), 0, &conn));
    IOMobileFramebufferGetLayerDefaultSurface(conn, 0, &oldsurface);
    memset(CoreSurfaceBufferGetBaseAddress(oldsurface), 0, CoreSurfaceBufferGetAllocSize(oldsurface));

    if(!strcmp(name, "AppleCLCD")) {
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(dict, CFSTR("IOSurfaceIsGlobal"), kCFBooleanTrue);
        int val = (int) 'ARGB'; // LE
        CFDictionarySetValue(dict, CFSTR("IOSurfacePixelFormat"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        val = 768;
        CFDictionarySetValue(dict, CFSTR("IOSurfaceHeight"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        CFDictionarySetValue(dict, CFSTR("IOSurfaceBufferTileMode"), kCFBooleanFalse);
        val = 4096;
        CFDictionarySetValue(dict, CFSTR("IOSurfaceBytesPerRow"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        val = 4;
        CFDictionarySetValue(dict, CFSTR("IOSurfaceBytesPerElement"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        val = 3145728;
        CFDictionarySetValue(dict, CFSTR("IOSurfaceAllocSize"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        val = 1024;
        CFDictionarySetValue(dict, CFSTR("IOSurfaceWidth"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
        CFDictionarySetValue(dict, CFSTR("IOSurfaceMemoryRegion"), CFSTR("PurpleGfxMem"));

        AST(iosurfacecreate, surface = (CoreSurfaceBufferRef) IOSurfaceCreate(dict));

        fb_swap();

    } else {
        surface = oldsurface;
    }

    // n.b. you can cast this to an IOSurfaceRef if necessary
    // (and see IOSurfaceAPI.h for docs)
    TRY(fb_lock, CoreSurfaceBufferLock(surface));
    void *base = CoreSurfaceBufferGetBaseAddress(surface);
    memset(base, 0, CoreSurfaceBufferGetAllocSize(oldsurface));
    
    draw_background(base);

    TRY(fb_unlock, CoreSurfaceBufferUnlock(surface));

    fb_swap();
    
    bufwidth = CoreSurfaceBufferGetWidth(surface);
    bufheight = CoreSurfaceBufferGetHeight(surface);
    flipped = false;
    if(bufwidth > bufheight) {
        bufwidth = CoreSurfaceBufferGetHeight(surface);
        bufheight = CoreSurfaceBufferGetWidth(surface);
        flipped = true;
    }
    barwidth = bufwidth * 0.8;
}

static void memset32(void *buf, unsigned int val, size_t numwords) {
    unsigned int *p = (unsigned int *) buf;
    while(numwords--) *p++ = val;
}

void gasgauge_set_progress(float progress) {
    static size_t last_px = -1;

    size_t px = progress < 0 ? last_px : (progress * barwidth);
    if(px > barwidth) px = barwidth;
    if(px == last_px) return;

    //I("set_progress: px = %d", (int) px);

    TRY(fb_lock, CoreSurfaceBufferLock(surface));
    void *base = CoreSurfaceBufferGetBaseAddress(surface);
    size_t barleft = (bufwidth - barwidth) / 2;
    size_t bpr = CoreSurfaceBufferGetBytesPerRow(surface);
    size_t bpp = 4;

    if(flipped) {
        for(int y = barleft; y < barleft + barwidth; y++) {
            memset32(((char *) base) + (bpr * (bufwidth - y)) + (bpp * (bufheight/2 - 1)), y >= px ? BAR_BG : BAR_FG, 2);
        }
    } else {
        for(int y = bufheight/2 - 1; y < bufheight/2 + 1; y++) {
            memset32(((char *) base) + (bpr * y) + (barleft * bpp), BAR_FG, px);
            memset32(((char *) base) + (bpr * y) + ((barleft + px) * bpp), BAR_BG, (barwidth - px));
        }
    }
    
    TRY(fb_unlock, CoreSurfaceBufferUnlock(surface));
    fb_swap();
    
    last_px = px;
}

void gasgauge_fini() {
}
