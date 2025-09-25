#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/OpenGL.h>
#include <mutex>
#include <atomic>
#include <cmath>
#include <cstring>

@interface INGOQOpenGLView : NSOpenGLView
{
    float* displayBuffer;
    int bufferSize;
    float brightness;
    float angle;
    float phase;
    float frequency;
    std::mutex* bufferMutex;
}

- (void)updateWithBuffer:(float*)buffer size:(int)size brightness:(float)b 
                   angle:(float)a phase:(float)p frequency:(float)f;

@end

@implementation INGOQOpenGLView

- (instancetype)initWithFrame:(NSRect)frameRect {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
        0
    };
    
    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pixelFormat) {
        NSLog(@"Failed to create pixel format");
        return nil;
    }
    
    self = [super initWithFrame:frameRect pixelFormat:pixelFormat];
    if (self) {
        bufferSize = 1024;
        displayBuffer = new float[bufferSize];
        memset(displayBuffer, 0, bufferSize * sizeof(float));
        bufferMutex = new std::mutex();
        brightness = 1.0f;
        angle = 0.0f;
        phase = 0.0f;
        frequency = 440.0f;
    }
    return self;
}

- (void)dealloc {
    delete[] displayBuffer;
    delete bufferMutex;
    [super dealloc];
}

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];
    
    // Setup OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
}

- (void)updateWithBuffer:(float*)buffer size:(int)size brightness:(float)b 
                   angle:(float)a phase:(float)p frequency:(float)f {
    bufferMutex->lock();
    if (buffer && size == bufferSize) {
        memcpy(displayBuffer, buffer, size * sizeof(float));
    }
    brightness = b;
    angle = a;
    phase = p;
    frequency = f;
    bufferMutex->unlock();
}

- (void)drawRect:(NSRect)dirtyRect {
    [[self openGLContext] makeCurrentContext];
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Setup orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    bufferMutex->lock();
    
    // Calculate color from frequency (exactly matching Metal version)
    float logFreq = log10f(fmaxf(20.0f, frequency));
    float logMin = log10f(20.0f);
    float logMax = log10f(20000.0f);
    float hue = ((logFreq - logMin) / (logMax - logMin)) * 360.0f;
    hue = fmodf(hue, 360.0f);
    
    // HSV to RGB conversion (exactly matching Metal version)
    float c = 1.0f;
    float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
    float m = 0.0f;
    float r, g, b;
    
    if (hue < 60) {
        r = c; g = x; b = 0;
    } else if (hue < 120) {
        r = x; g = c; b = 0;
    } else if (hue < 180) {
        r = 0; g = c; b = x;
    } else if (hue < 240) {
        r = 0; g = x; b = c;
    } else if (hue < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    // Apply brightness to base color (matching Metal)
    r = (r + m) * brightness;
    g = (g + m) * brightness;
    b = (b + m) * brightness;
    
    // Calculate rotation and scaling (matching Metal)
    float angleRad = angle * M_PI / 180.0f;
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);
    float scale = 1.0f;
    
    if (fabsf(angle) > 0.01f) {
        float cosAbs = fabsf(cosA);
        float sinAbs = fabsf(sinA);
        float scaleX = cosAbs + sinAbs;
        float scaleY = sinAbs + cosAbs;
        scale = fmaxf(scaleX, scaleY);
    }
    
    // Draw vertical bars (exactly matching Metal logic)
    int barCount = bufferSize;
    float barWidth = 2.0f / (float)barCount;
    
    glBegin(GL_QUADS);
    
    for (int i = 0; i < barCount; i++) {
        // Apply phase shift (exactly matching Metal)
        int shiftedIndex = (i + (int)(phase * bufferSize)) % bufferSize;
        float voltage = displayBuffer[shiftedIndex];
        
        // Map voltage to brightness (exactly matching Metal)
        float normalizedVoltage = (voltage + 10.0f) * 0.05f;
        normalizedVoltage = fmaxf(0.0f, fminf(1.0f, normalizedVoltage));
        
        // Calculate bar color with voltage-based brightness
        float barR = r * normalizedVoltage;
        float barG = g * normalizedVoltage;
        float barB = b * normalizedVoltage;
        
        // Calculate bar position
        float x1 = (float)i / (float)barCount * 2.0f - 1.0f;
        float x2 = x1 + barWidth;
        float y1 = -1.0f;
        float y2 = 1.0f;
        
        // Apply rotation and scaling to all 4 corners (matching Metal)
        float corners[4][2] = {
            {x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}
        };
        
        glColor4f(barR, barG, barB, 1.0f);
        
        for (int j = 0; j < 4; j++) {
            float rotX = (corners[j][0] * cosA - corners[j][1] * sinA) * scale;
            float rotY = (corners[j][0] * sinA + corners[j][1] * cosA) * scale;
            glVertex2f(rotX, rotY);
        }
    }
    
    glEnd();
    
    bufferMutex->unlock();
    
    [[self openGLContext] flushBuffer];
}

@end

// Window Controller
@interface INGOQWindowController : NSWindowController
@property (nonatomic, strong) INGOQOpenGLView* glView;
@property (nonatomic, strong) NSTimer* renderTimer;
@end

@implementation INGOQWindowController

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled |
                                                             NSWindowStyleMaskClosable |
                                                             NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    self = [super initWithWindow:window];
    if (self) {
        [window setTitle:@"INGOQ - OpenGL Display"];
        [window center];
        
        self.glView = [[INGOQOpenGLView alloc] initWithFrame:frame];
        [window setContentView:self.glView];
        
        // Setup render timer (120 FPS)
        self.renderTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/120.0
                                                           target:self.glView
                                                         selector:@selector(setNeedsDisplay:)
                                                         userInfo:nil
                                                          repeats:YES];
    }
    return self;
}

- (void)dealloc {
    if (self.renderTimer) {
        [self.renderTimer invalidate];
        self.renderTimer = nil;
    }
    [super dealloc];
}

@end

// C++ Interface
extern "C" {
    void* createINGOQWindow(int width, int height) {
        @autoreleasepool {
            INGOQWindowController* controller = [[INGOQWindowController alloc] init];
            [controller showWindow:nil];
            return (__bridge_retained void*)controller;
        }
    }
    
    void destroyINGOQWindow(void* window) {
        @autoreleasepool {
            INGOQWindowController* controller = (__bridge_transfer INGOQWindowController*)window;
            if (controller.renderTimer) {
                [controller.renderTimer invalidate];
            }
            [controller close];
        }
    }
    
    void openINGOQWindow(void* window) {
        @autoreleasepool {
            INGOQWindowController* controller = (__bridge INGOQWindowController*)window;
            [controller showWindow:nil];
        }
    }
    
    void closeINGOQWindow(void* window) {
        @autoreleasepool {
            INGOQWindowController* controller = (__bridge INGOQWindowController*)window;
            [controller close];
        }
    }
    
    bool isINGOQWindowOpen(void* window) {
        @autoreleasepool {
            INGOQWindowController* controller = (__bridge INGOQWindowController*)window;
            return controller.window.isVisible;
        }
    }
    
    void updateINGOQWindow(void* window, const float* buffer, int size,
                          float brightness, float angle, float phase, float frequency) {
        @autoreleasepool {
            INGOQWindowController* controller = (__bridge INGOQWindowController*)window;
            if (controller.glView) {
                [controller.glView updateWithBuffer:(float*)buffer size:size brightness:brightness 
                                              angle:angle phase:phase frequency:frequency];
            }
        }
    }
}