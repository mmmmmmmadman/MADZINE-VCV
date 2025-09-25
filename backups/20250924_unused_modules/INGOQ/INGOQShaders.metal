#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float brightness [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

struct Uniforms {
    float4x4 modelMatrix;
    float3 baseColor;
    float phase;
};

vertex VertexOut vertexShader(VertexIn in [[stage_in]],
                              constant Uniforms& uniforms [[buffer(1)]],
                              uint vid [[vertex_id]]) {
    VertexOut out;
    
    // Apply transformation
    float4 pos = float4(in.position, 0.0, 1.0);
    out.position = uniforms.modelMatrix * pos;
    
    // Apply color with brightness
    out.color = float4(uniforms.baseColor * in.brightness, 1.0);
    
    return out;
}

fragment float4 fragmentShader(VertexOut in [[stage_in]]) {
    return in.color;
}

// Compute shader for preparing vertex data
kernel void prepareVertexData(device float2* positions [[buffer(0)]],
                              device float* brightness [[buffer(1)]],
                              constant float* voltageBuffer [[buffer(2)]],
                              constant Uniforms& uniforms [[buffer(3)]],
                              uint index [[thread_position_in_grid]]) {
    if (index >= 1024) return;
    
    // Calculate phase-shifted index
    int shiftedIndex = (index + int(uniforms.phase * 1024)) % 1024;
    float voltage = voltageBuffer[shiftedIndex];
    
    // Map voltage to brightness
    float normalizedVoltage = (voltage + 10.0) * 0.05;
    normalizedVoltage = clamp(normalizedVoltage, 0.0, 1.0);
    
    // Set position for this bar (4 vertices per bar for quad)
    float xPos = float(index) / 1024.0 * 2.0 - 1.0;
    float barWidth = 2.0 / 1024.0;
    
    uint baseIdx = index * 4;
    positions[baseIdx + 0] = float2(xPos, -1.0);
    positions[baseIdx + 1] = float2(xPos + barWidth, -1.0);
    positions[baseIdx + 2] = float2(xPos, 1.0);
    positions[baseIdx + 3] = float2(xPos + barWidth, 1.0);
    
    // Set brightness for all 4 vertices
    brightness[baseIdx + 0] = normalizedVoltage;
    brightness[baseIdx + 1] = normalizedVoltage;
    brightness[baseIdx + 2] = normalizedVoltage;
    brightness[baseIdx + 3] = normalizedVoltage;
}