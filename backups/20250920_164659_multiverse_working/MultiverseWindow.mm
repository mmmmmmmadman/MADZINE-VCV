#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/OpenGL.h>
#include <mutex>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

// Multiverse window display dimensions - same as internal
static const int MULTIVERSE_WIDTH = 1024;
static const int MULTIVERSE_HEIGHT = 512;

@interface MultiverseOpenGLView : NSOpenGLView
{
    // 4 channel buffers - same structure as internal
    float* displayBuffers[4];
    int bufferSize;
    
    // Channel parameters
    float phase[4];
    float ratio[4];
    float angle[4];
    float intensity[4];
    float frequency[4];
    
    // Global parameters
    float mixMode;
    float crossMod;
    
    // Pixel buffer for compositing
    std::vector<unsigned char>* pixelData;
    GLuint textureID;
    
    std::mutex* bufferMutex;
}

- (void)updateChannel:(int)channel buffer:(float*)buffer size:(int)size;
- (void)updateChannelParams:(int)channel phase:(float)p ratio:(float)r 
                       angle:(float)a intensity:(float)i frequency:(float)f;
- (void)updateGlobalParams:(float)mix crossMod:(float)xmod;

@end

@implementation MultiverseOpenGLView

- (instancetype)initWithFrame:(NSRect)frameRect {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
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
        bufferSize = MULTIVERSE_WIDTH;
        
        // Initialize 4 channel buffers
        for (int i = 0; i < 4; i++) {
            displayBuffers[i] = new float[bufferSize];
            memset(displayBuffers[i], 0, bufferSize * sizeof(float));
            phase[i] = 0.0f;
            ratio[i] = 0.5f;
            angle[i] = 0.0f;
            intensity[i] = 1.0f;
            frequency[i] = 440.0f;
        }
        
        mixMode = 0.0f;
        crossMod = 0.0f;
        bufferMutex = new std::mutex();
        
        // Initialize pixel buffer
        size_t pixelCount = (size_t)MULTIVERSE_WIDTH * (size_t)MULTIVERSE_HEIGHT * 4;
        pixelData = new std::vector<unsigned char>(pixelCount, 0);
        textureID = 0;
    }
    return self;
}

- (void)dealloc {
    for (int i = 0; i < 4; i++) {
        delete[] displayBuffers[i];
    }
    delete bufferMutex;
    delete pixelData;
    
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
    
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
    
    // Create texture for pixel buffer
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

- (void)updateChannel:(int)channel buffer:(float*)buffer size:(int)size {
    if (channel < 0 || channel >= 4) return;
    
    bufferMutex->lock();
    if (buffer && size == bufferSize) {
        memcpy(displayBuffers[channel], buffer, size * sizeof(float));
    }
    bufferMutex->unlock();
}

- (void)updateChannelParams:(int)channel phase:(float)p ratio:(float)r 
                       angle:(float)a intensity:(float)i frequency:(float)f {
    if (channel < 0 || channel >= 4) return;
    
    bufferMutex->lock();
    phase[channel] = p;
    ratio[channel] = r;
    angle[channel] = a;
    intensity[channel] = i;
    frequency[channel] = f;
    bufferMutex->unlock();
}

- (void)updateGlobalParams:(float)mix crossMod:(float)xmod {
    bufferMutex->lock();
    mixMode = mix;
    crossMod = xmod;
    bufferMutex->unlock();
}

// HSV to RGB conversion
- (void)hsvToRGB:(float)h s:(float)s v:(float)v r:(float*)r g:(float*)g b:(float*)b {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    if (h < 60) {
        *r = c; *g = x; *b = 0;
    } else if (h < 120) {
        *r = x; *g = c; *b = 0;
    } else if (h < 180) {
        *r = 0; *g = c; *b = x;
    } else if (h < 240) {
        *r = 0; *g = x; *b = c;
    } else if (h < 300) {
        *r = x; *g = 0; *b = c;
    } else {
        *r = c; *g = 0; *b = x;
    }
    
    *r += m;
    *g += m;
    *b += m;
}

// Get hue from frequency
- (float)getHueFromFrequency:(float)freq {
    // Octave-based color mapping - each octave cycles through full spectrum
    // This creates a musical relationship where same notes in different octaves have similar colors

    // Clamp frequency to reasonable range
    freq = fminf(fmaxf(freq, 20.0f), 20000.0f);

    // Reference frequency (can be adjusted - using A1 = 55Hz as base)
    const float baseFreq = 55.0f;

    // Calculate position within octave using logarithm
    // log2(freq/baseFreq) tells us how many octaves above base
    // The fractional part tells us position within current octave
    float octavePosition = fmodf(log2f(freq / baseFreq), 1.0f);

    // Handle negative values (frequencies below baseFreq)
    if (octavePosition < 0) {
        octavePosition += 1.0f;
    }

    // Map octave position (0-1) to full hue range (0-360°)
    float hue = octavePosition * 360.0f;

    return hue;
}

// Blend two colors based on mix mode
- (void)blendColors:(float)r1 g1:(float)g1 b1:(float)b1 a1:(float)a1
                    r2:(float)r2 g2:(float)g2 b2:(float)b2 a2:(float)a2
                 mode:(int)mode result_r:(float*)rr result_g:(float*)rg 
              result_b:(float*)rb result_a:(float*)ra {
    
    switch (mode) {
        case 0: // Add
            *rr = fminf(1.0f, r1 + r2);
            *rg = fminf(1.0f, g1 + g2);
            *rb = fminf(1.0f, b1 + b2);
            *ra = fminf(1.0f, a1 + a2);
            break;
            
        case 1: // Screen
            *rr = 1.0f - (1.0f - r1) * (1.0f - r2);
            *rg = 1.0f - (1.0f - g1) * (1.0f - g2);
            *rb = 1.0f - (1.0f - b1) * (1.0f - b2);
            *ra = 1.0f - (1.0f - a1) * (1.0f - a2);
            break;
            
        case 2: // Difference
            *rr = fabsf(r1 - r2);
            *rg = fabsf(g1 - g2);
            *rb = fabsf(b1 - b2);
            *ra = fmaxf(a1, a2);
            break;
            
        case 3: // Color Dodge
            *rr = (r2 < 0.999f) ? fminf(1.0f, r1 / fmaxf(0.001f, 1.0f - r2)) : 1.0f;
            *rg = (g2 < 0.999f) ? fminf(1.0f, g1 / fmaxf(0.001f, 1.0f - g2)) : 1.0f;
            *rb = (b2 < 0.999f) ? fminf(1.0f, b1 / fmaxf(0.001f, 1.0f - b2)) : 1.0f;
            *ra = fmaxf(a1, a2);
            break;
            
        default:
            *rr = r1; *rg = g1; *rb = b1; *ra = a1;
            break;
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    [[self openGLContext] makeCurrentContext];
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Setup orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    bufferMutex->lock();
    
    // Clear pixel data
    std::fill(pixelData->begin(), pixelData->end(), 0);
    
    int mixModeInt = (int)roundf(mixMode);
    
    // Process each layer
    for (int layer = 0; layer < 4; layer++) {
        // Check if channel has signal
        bool hasSignal = false;
        for (int i = 0; i < bufferSize; i++) {
            if (fabsf(displayBuffers[layer][i]) > 0.001f) {
                hasSignal = true;
                break;
            }
        }
        if (!hasSignal) continue;
        
        // Get frequency for color
        float hue = [self getHueFromFrequency:frequency[layer]];
        
        // Convert HSV to RGB
        float r, g, b;
        [self hsvToRGB:hue s:1.0f v:1.0f r:&r g:&g b:&b];
        
        // Apply phase shift
        float phaseOffset = phase[layer] * MULTIVERSE_WIDTH;
        
        // Create temporary layer buffer
        std::vector<float> layerBuffer(MULTIVERSE_WIDTH * MULTIVERSE_HEIGHT, 0.0f);
        
        // Fill layer buffer with voltage data
        for (int x = 0; x < MULTIVERSE_WIDTH; x++) {
            int shiftedX = (x + (int)phaseOffset + MULTIVERSE_WIDTH) % MULTIVERSE_WIDTH;
            float voltage = displayBuffers[layer][shiftedX];
            
            // Map voltage to brightness
            float normalizedVoltage = (voltage + 10.0f) * 0.05f;
            normalizedVoltage = fmaxf(0.0f, fminf(1.0f, normalizedVoltage)) * intensity[layer];
            
            // Fill entire column
            for (int y = 0; y < MULTIVERSE_HEIGHT; y++) {
                size_t idx = (size_t)y * MULTIVERSE_WIDTH + (size_t)x;
                if (idx < layerBuffer.size()) {
                    layerBuffer[idx] = normalizedVoltage;
                }
            }
        }
        
        // Apply rotation and blend
        float angleRad = angle[layer] * 2.0f * M_PI;
        if (fabsf(angleRad) > 0.01f) {
            // With rotation - calculate scale to fill frame
            float cosA = cosf(angleRad);
            float sinA = sinf(angleRad);
            
            float w = MULTIVERSE_WIDTH;
            float h = MULTIVERSE_HEIGHT;
            float absCosA = fabsf(cosA);
            float absSinA = fabsf(sinA);
            float scaleX = (w * absCosA + h * absSinA) / w;
            float scaleY = (w * absSinA + h * absCosA) / h;
            float scale = fmaxf(scaleX, scaleY);
            
            int centerX = MULTIVERSE_WIDTH / 2;
            int centerY = MULTIVERSE_HEIGHT / 2;
            
            for (int y = 0; y < MULTIVERSE_HEIGHT; y++) {
                for (int x = 0; x < MULTIVERSE_WIDTH; x++) {
                    // Calculate source position (inverse rotation with scaling)
                    float dx = (x - centerX) / scale;
                    float dy = (y - centerY) / scale;
                    int srcX = (int)(centerX + dx * cosA + dy * sinA);
                    int srcY = (int)(centerY - dx * sinA + dy * cosA);
                    
                    if (srcX >= 0 && srcX < MULTIVERSE_WIDTH && 
                        srcY >= 0 && srcY < MULTIVERSE_HEIGHT) {
                        size_t srcIdx = (size_t)srcY * MULTIVERSE_WIDTH + (size_t)srcX;
                        if (srcIdx < layerBuffer.size() && layerBuffer[srcIdx] > 0.0f) {
                            size_t dstIdx = ((size_t)y * MULTIVERSE_WIDTH + (size_t)x) * 4;
                            if (dstIdx + 3 < pixelData->size()) {
                                // Get existing color
                                float existingR = (*pixelData)[dstIdx] / 255.0f;
                                float existingG = (*pixelData)[dstIdx + 1] / 255.0f;
                                float existingB = (*pixelData)[dstIdx + 2] / 255.0f;
                                float existingA = (*pixelData)[dstIdx + 3] / 255.0f;
                                
                                // New layer color
                                float newR = r * layerBuffer[srcIdx];
                                float newG = g * layerBuffer[srcIdx];
                                float newB = b * layerBuffer[srcIdx];
                                float newA = layerBuffer[srcIdx];
                                
                                // Blend colors
                                float blendedR, blendedG, blendedB, blendedA;
                                [self blendColors:existingR g1:existingG b1:existingB a1:existingA
                                              r2:newR g2:newG b2:newB a2:newA
                                           mode:mixModeInt result_r:&blendedR result_g:&blendedG 
                                        result_b:&blendedB result_a:&blendedA];
                                
                                (*pixelData)[dstIdx + 0] = (unsigned char)(blendedR * 255);
                                (*pixelData)[dstIdx + 1] = (unsigned char)(blendedG * 255);
                                (*pixelData)[dstIdx + 2] = (unsigned char)(blendedB * 255);
                                (*pixelData)[dstIdx + 3] = 255;
                            }
                        }
                    }
                }
            }
        } else {
            // No rotation - direct copy
            for (size_t i = 0; i < layerBuffer.size(); i++) {
                if (layerBuffer[i] > 0.0f) {
                    size_t idx = i * 4;
                    if (idx + 3 < pixelData->size()) {
                        // Get existing color
                        float existingR = (*pixelData)[idx] / 255.0f;
                        float existingG = (*pixelData)[idx + 1] / 255.0f;
                        float existingB = (*pixelData)[idx + 2] / 255.0f;
                        float existingA = (*pixelData)[idx + 3] / 255.0f;
                        
                        // New layer color  
                        float newR = r * layerBuffer[i];
                        float newG = g * layerBuffer[i];
                        float newB = b * layerBuffer[i];
                        float newA = layerBuffer[i];
                        
                        // Blend colors
                        float blendedR, blendedG, blendedB, blendedA;
                        [self blendColors:existingR g1:existingG b1:existingB a1:existingA
                                      r2:newR g2:newG b2:newB a2:newA
                                   mode:mixModeInt result_r:&blendedR result_g:&blendedG 
                                result_b:&blendedB result_a:&blendedA];
                        
                        (*pixelData)[idx + 0] = (unsigned char)(blendedR * 255);
                        (*pixelData)[idx + 1] = (unsigned char)(blendedG * 255);
                        (*pixelData)[idx + 2] = (unsigned char)(blendedB * 255);
                        (*pixelData)[idx + 3] = 255;
                    }
                }
            }
        }
    }
    
    bufferMutex->unlock();
    
    // Upload pixel data to texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MULTIVERSE_WIDTH, MULTIVERSE_HEIGHT, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData->data());
    
    // Draw texture as full screen quad
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(0, 1);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    
    [[self openGLContext] flushBuffer];
}

@end

// Window Controller
@interface MultiverseWindowController : NSWindowController
@property (nonatomic, strong) MultiverseOpenGLView* glView;
@property (nonatomic, strong) NSTimer* renderTimer;
@end

@implementation MultiverseWindowController

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 1024, 512);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled |
                                                             NSWindowStyleMaskClosable |
                                                             NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    self = [super initWithWindow:window];
    if (self) {
        [window setTitle:@"Multiverse - External Display"];
        [window center];
        
        self.glView = [[MultiverseOpenGLView alloc] initWithFrame:frame];
        [window setContentView:self.glView];
        
        // Setup render timer (60 FPS)
        self.renderTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
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
    void* createMultiverseWindow() {
        @autoreleasepool {
            MultiverseWindowController* controller = [[MultiverseWindowController alloc] init];
            [controller showWindow:nil];
            return (__bridge_retained void*)controller;
        }
    }
    
    void destroyMultiverseWindow(void* window) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge_transfer MultiverseWindowController*)window;
            if (controller.renderTimer) {
                [controller.renderTimer invalidate];
            }
            [controller close];
        }
    }
    
    void openMultiverseWindow(void* window) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            [controller showWindow:nil];
        }
    }
    
    void closeMultiverseWindow(void* window) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            [controller close];
        }
    }
    
    bool isMultiverseWindowOpen(void* window) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            return controller.window.isVisible;
        }
    }
    
    void updateMultiverseChannel(void* window, int channel, const float* buffer, int size) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            if (controller.glView) {
                [controller.glView updateChannel:channel buffer:(float*)buffer size:size];
            }
        }
    }
    
    void updateMultiverseChannelParams(void* window, int channel, 
                                      float phase, float ratio, float angle, 
                                      float intensity, float frequency) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            if (controller.glView) {
                [controller.glView updateChannelParams:channel phase:phase ratio:ratio 
                                                 angle:angle intensity:intensity frequency:frequency];
            }
        }
    }
    
    void updateMultiverseGlobalParams(void* window, float mixMode, float crossMod) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            if (controller.glView) {
                [controller.glView updateGlobalParams:mixMode crossMod:crossMod];
            }
        }
    }
}