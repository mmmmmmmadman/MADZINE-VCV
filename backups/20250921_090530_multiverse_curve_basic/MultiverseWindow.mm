#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <OpenGL/OpenGL.h>
#include <mutex>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

// Display dimensions
static const int MULTIVERSE_WIDTH = 1024;
static const int MULTIVERSE_HEIGHT = 512;
static const int AUDIO_BUFFER_SIZE = 256;  // Small audio buffer per channel

// Vertex shader - simple pass-through
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader - all visual processing happens here
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

// Audio data as 1D textures
uniform sampler1D audioChannel0;
uniform sampler1D audioChannel1;
uniform sampler1D audioChannel2;
uniform sampler1D audioChannel3;

// Channel parameters
uniform vec4 curves;
uniform vec4 phases;
uniform vec4 angles;
uniform vec4 intensities;
uniform vec4 frequencies;

// Global parameters
uniform float mixMode;
uniform float time;

// Octave-based frequency to hue
vec3 freqToRGB(float freq) {
    float baseFreq = 261.63;  // C4 - middle C for better distribution
    float octavePos = mod(log2(freq / baseFreq), 1.0);
    float hue = octavePos * 360.0;  // Map to full spectrum

    // HSV to RGB conversion
    float c = 1.0;
    float h = hue / 60.0;
    float x = c * (1.0 - abs(mod(h, 2.0) - 1.0));

    vec3 rgb;
    if (h < 1.0) rgb = vec3(c, x, 0);
    else if (h < 2.0) rgb = vec3(x, c, 0);
    else if (h < 3.0) rgb = vec3(0, c, x);
    else if (h < 4.0) rgb = vec3(0, x, c);
    else if (h < 5.0) rgb = vec3(x, 0, c);
    else rgb = vec3(c, 0, x);

    return rgb;
}

// Rotation transformation
vec2 rotate2D(vec2 pos, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return vec2(pos.x * c - pos.y * s, pos.x * s + pos.y * c);
}

// Blend modes - continuous interpolation between 4 modes
vec3 blendColors(vec3 base, vec3 blend, float mode) {
    // mode 0-1: Add to Difference
    // mode 1-2: Difference to Screen
    // mode 2-3: Screen to Lighten

    vec3 result;

    if (mode <= 1.0) {
        // Interpolate between Add and Difference
        vec3 addResult = min(vec3(1.0), base + blend);
        vec3 diffResult = abs(base - blend);
        result = mix(addResult, diffResult, mode);
    }
    else if (mode <= 2.0) {
        // Interpolate between Difference and Screen
        vec3 diffResult = abs(base - blend);
        vec3 screenResult = vec3(1.0) - (vec3(1.0) - base) * (vec3(1.0) - blend);
        result = mix(diffResult, screenResult, mode - 1.0);
    }
    else {
        // Interpolate between Screen and Lighten
        vec3 screenResult = vec3(1.0) - (vec3(1.0) - base) * (vec3(1.0) - blend);
        vec3 lightenResult = max(base, blend);
        result = mix(screenResult, lightenResult, mode - 2.0);
    }

    return result;
}

void main() {
    vec2 uv = TexCoord;
    vec3 finalColor = vec3(0.0);

    // Process each channel
    for (int ch = 0; ch < 4; ch++) {
        float curve = ch == 0 ? curves.x : (ch == 1 ? curves.y : (ch == 2 ? curves.z : curves.w));
        float phase = ch == 0 ? phases.x : (ch == 1 ? phases.y : (ch == 2 ? phases.z : phases.w));
        float angle = ch == 0 ? angles.x : (ch == 1 ? angles.y : (ch == 2 ? angles.z : angles.w));
        float intensity = ch == 0 ? intensities.x : (ch == 1 ? intensities.y : (ch == 2 ? intensities.z : intensities.w));
        float freq = ch == 0 ? frequencies.x : (ch == 1 ? frequencies.y : (ch == 2 ? frequencies.z : frequencies.w));

        // Determine sampling position based on curve
        float xSample = uv.x;

        if (curve > 0.001) {
            // Gradually bend from straight line to circle
            vec2 center = vec2(0.5, 0.5);
            vec2 fromCenter = uv - center;
            float dist = length(fromCenter);
            float pixelAngle = atan(fromCenter.y, fromCenter.x);

            // Map angle to 0-1 range for sampling
            float angleSample = (pixelAngle + 3.14159) / (2.0 * 3.14159);

            // Interpolate between x-based and angle-based sampling
            xSample = mix(uv.x, angleSample, curve);
        }

        // Apply phase offset
        xSample = mod(xSample + phase, 1.0);

        // Apply rotation if needed (only in straight mode)
        if (abs(angle) > 0.001 && curve < 0.001) {
            float angleRad = -angle * 3.14159 / 180.0;
            vec2 rotatedUV = rotate2D((vec2(uv.x, uv.y) - 0.5), angleRad) + 0.5;
            xSample = mod(rotatedUV.x + phase, 1.0);
        }

        // Sample audio waveform at this position
        float audioValue = 0.0;
        if (ch == 0) audioValue = texture(audioChannel0, xSample).r;
        else if (ch == 1) audioValue = texture(audioChannel1, xSample).r;
        else if (ch == 2) audioValue = texture(audioChannel2, xSample).r;
        else if (ch == 3) audioValue = texture(audioChannel3, xSample).r;

        // Normalize audio value (already normalized in CPU to -1..1)
        // Reduce brightness - max brightness is 0.8 when intensity is at max (1.5)
        float normalizedVoltage = clamp((audioValue + 1.0) * 0.5 * intensity * 0.1067, 0.0, 0.8);

        // Create the waveform display
        float brightness = normalizedVoltage;

        // Get color from frequency
        vec3 channelColor = freqToRGB(freq) * brightness;

        // Blend with previous channels
        finalColor = blendColors(finalColor, channelColor, mixMode);
    }

    FragColor = vec4(finalColor, 1.0);
}
)";

@interface MultiverseOpenGLView : NSOpenGLView
{
    // Audio buffers - much smaller than pixel buffers
    float audioBuffers[4][AUDIO_BUFFER_SIZE];

    // Channel parameters
    float curve[4];
    float phase[4];
    float angle[4];
    float intensity[4];
    float frequency[4];

    // Global parameters
    float mixMode;
    float time;

    // Thread safety
    std::mutex* bufferMutex;
    std::atomic<bool> needsDisplay;

    // OpenGL resources
    GLuint shaderProgram;
    GLuint VAO, VBO;
    GLuint audioTextures[4];

    // Uniform locations
    GLint curveUniform, phaseUniform, angleUniform, intensityUniform, frequencyUniform;
    GLint mixModeUniform, timeUniform;
}

- (void)updateChannel:(int)channel buffer:(const float*)buffer size:(int)size;
- (void)updateChannelParams:(int)channel
                       curve:(float)c
                       phase:(float)p
                       angle:(float)a
                   intensity:(float)i
                   frequency:(float)f;
- (void)updateGlobalParams:(float)mix crossMod:(float)xmod;
@end

@implementation MultiverseOpenGLView

- (instancetype)initWithFrame:(NSRect)frameRect {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,  // Use modern OpenGL
        0
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pixelFormat) {
        NSLog(@"Failed to create pixel format");
        return nil;
    }

    self = [super initWithFrame:frameRect pixelFormat:pixelFormat];
    if (self) {
        // Initialize audio buffers
        for (int i = 0; i < 4; i++) {
            memset(audioBuffers[i], 0, AUDIO_BUFFER_SIZE * sizeof(float));
            curve[i] = 0.0f;
            phase[i] = 0.0f;
            angle[i] = 0.0f;
            intensity[i] = 1.0f;
            frequency[i] = 440.0f;
        }

        mixMode = 0.0f;
        time = 0.0f;
        bufferMutex = new std::mutex();
        needsDisplay = false;
    }
    return self;
}

- (void)dealloc {
    [[self openGLContext] makeCurrentContext];

    if (audioTextures[0]) {
        glDeleteTextures(4, audioTextures);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }

    delete bufferMutex;
    [super dealloc];
}

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create fullscreen quad
    float vertices[] = {
        // positions    // texture coords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create 1D textures for audio data
    glGenTextures(4, audioTextures);
    for (int i = 0; i < 4; i++) {
        glBindTexture(GL_TEXTURE_1D, audioTextures[i]);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, AUDIO_BUFFER_SIZE, 0, GL_RED, GL_FLOAT, audioBuffers[i]);
    }

    // Get uniform locations
    glUseProgram(shaderProgram);
    curveUniform = glGetUniformLocation(shaderProgram, "curves");
    phaseUniform = glGetUniformLocation(shaderProgram, "phases");
    angleUniform = glGetUniformLocation(shaderProgram, "angles");
    intensityUniform = glGetUniformLocation(shaderProgram, "intensities");
    frequencyUniform = glGetUniformLocation(shaderProgram, "frequencies");
    mixModeUniform = glGetUniformLocation(shaderProgram, "mixMode");
    timeUniform = glGetUniformLocation(shaderProgram, "time");

    // Set texture uniforms
    glUniform1i(glGetUniformLocation(shaderProgram, "audioChannel0"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "audioChannel1"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "audioChannel2"), 2);
    glUniform1i(glGetUniformLocation(shaderProgram, "audioChannel3"), 3);
}

- (void)reshape {
    [super reshape];
    NSRect bounds = [self bounds];
    [[self openGLContext] makeCurrentContext];
    glViewport(0, 0, (GLsizei)bounds.size.width, (GLsizei)bounds.size.height);
}

- (void)drawRect:(NSRect)dirtyRect {
    [[self openGLContext] makeCurrentContext];

    // Update viewport in case it changed
    NSRect bounds = [self bounds];
    glViewport(0, 0, (GLsizei)bounds.size.width, (GLsizei)bounds.size.height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    bufferMutex->lock();

    // Update audio textures
    for (int i = 0; i < 4; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_1D, audioTextures[i]);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, AUDIO_BUFFER_SIZE, GL_RED, GL_FLOAT, audioBuffers[i]);
    }

    // Update uniforms
    glUniform4f(curveUniform, curve[0], curve[1], curve[2], curve[3]);
    glUniform4f(phaseUniform, phase[0], phase[1], phase[2], phase[3]);
    glUniform4f(angleUniform, angle[0], angle[1], angle[2], angle[3]);
    glUniform4f(intensityUniform, intensity[0], intensity[1], intensity[2], intensity[3]);
    glUniform4f(frequencyUniform, frequency[0], frequency[1], frequency[2], frequency[3]);
    glUniform1f(mixModeUniform, mixMode);
    glUniform1f(timeUniform, time);

    bufferMutex->unlock();

    // Draw fullscreen quad
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    [[self openGLContext] flushBuffer];

    time += 0.016f;  // ~60fps
}

- (void)updateChannel:(int)channel buffer:(const float*)buffer size:(int)size {
    if (channel < 0 || channel >= 4 || !buffer) return;

    bufferMutex->lock();
    // Copy only what fits in our small buffer
    int copySize = std::min(size, AUDIO_BUFFER_SIZE);
    memcpy(audioBuffers[channel], buffer, copySize * sizeof(float));
    bufferMutex->unlock();

    needsDisplay = true;
}

- (void)updateChannelParams:(int)channel
                       curve:(float)c
                       phase:(float)p
                       angle:(float)a
                   intensity:(float)i
                   frequency:(float)f {
    if (channel < 0 || channel >= 4) return;

    bufferMutex->lock();
    curve[channel] = c;
    phase[channel] = p;
    angle[channel] = a * 360.0f;  // Convert to degrees
    intensity[channel] = i;
    frequency[channel] = f;
    bufferMutex->unlock();

    needsDisplay = true;
}

- (void)updateGlobalParams:(float)mix crossMod:(float)xmod {
    bufferMutex->lock();
    mixMode = mix;
    bufferMutex->unlock();

    needsDisplay = true;
}

- (void)animate {
    if (needsDisplay.exchange(false)) {
        [self setNeedsDisplay:YES];
    }
}

@end

// Window controller
@interface MultiverseWindowController : NSWindowController
{
    MultiverseOpenGLView* glView;
    NSTimer* displayTimer;
}
@end

@implementation MultiverseWindowController

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, MULTIVERSE_WIDTH, MULTIVERSE_HEIGHT);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled |
                                                             NSWindowStyleMaskClosable |
                                                             NSWindowStyleMaskMiniaturizable |
                                                             NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];

    self = [super initWithWindow:window];
    if (self) {
        [window setTitle:@"Multiverse - GPU Rendering"];
        [window center];

        // Set window size constraints
        [window setMinSize:NSMakeSize(512, 256)];  // Minimum size
        [window setMaxSize:NSMakeSize(3840, 2160)];  // Maximum 4K resolution

        // Enable full screen support
        [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

        glView = [[MultiverseOpenGLView alloc] initWithFrame:frame];
        [glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [window setContentView:glView];

        // 60fps display timer
        displayTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                         target:glView
                                                       selector:@selector(animate)
                                                       userInfo:nil
                                                        repeats:YES];
    }
    return self;
}

- (void)dealloc {
    [displayTimer invalidate];
    [glView release];
    [super dealloc];
}

@end

// C interface functions
extern "C" {
    void* createMultiverseWindow() {
        @autoreleasepool {
            MultiverseWindowController* controller = [[MultiverseWindowController alloc] init];
            return (__bridge_retained void*)controller;
        }
    }

    void destroyMultiverseWindow(void* window) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge_transfer MultiverseWindowController*)window;
            [controller close];
            [controller release];
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
            MultiverseOpenGLView* view = (MultiverseOpenGLView*)controller.window.contentView;
            [view updateChannel:channel buffer:buffer size:size];
        }
    }

    void updateMultiverseChannelParams(void* window, int channel,
                                      float curve, float phase, float angle,
                                      float intensity, float frequency) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            MultiverseOpenGLView* view = (MultiverseOpenGLView*)controller.window.contentView;
            [view updateChannelParams:channel curve:curve phase:phase angle:angle intensity:intensity frequency:frequency];
        }
    }

    void updateMultiverseGlobalParams(void* window, float mixMode, float crossMod) {
        @autoreleasepool {
            MultiverseWindowController* controller = (__bridge MultiverseWindowController*)window;
            MultiverseOpenGLView* view = (MultiverseOpenGLView*)controller.window.contentView;
            [view updateGlobalParams:mixMode crossMod:crossMod];
        }
    }
}