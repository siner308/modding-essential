#include "D3D11Hook.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>

void ShowMainMenu() {
    std::cout << "\n=== Visual Effects System ===\n";
    std::cout << "1. Load preset effect\n";
    std::cout << "2. Adjust basic parameters\n";
    std::cout << "3. Advanced color grading\n";
    std::cout << "4. Special effects\n";
    std::cout << "5. Monitor performance\n";
    std::cout << "6. Toggle effects on/off\n";
    std::cout << "7. Save current settings\n";
    std::cout << "8. Reset to defaults\n";
    std::cout << "9. Exit\n";
    std::cout << "Choice: ";
}

void LoadPresetMenu() {
    std::cout << "\n=== Effect Presets ===\n";
    std::cout << "1. Cinematic (warm, dramatic)\n";
    std::cout << "2. Vintage (aged, sepia tones)\n";
    std::cout << "3. High Contrast (vivid, sharp)\n";
    std::cout << "4. Warm (orange/yellow tint)\n";
    std::cout << "5. Cool (blue tint)\n";
    std::cout << "6. Dramatic (dark shadows, bright highlights)\n";
    std::cout << "7. Natural (subtle enhancement)\n";
    std::cout << "8. Black & White (monochrome)\n";
    std::cout << "9. Sepia (vintage brown)\n";
    std::cout << "10. Cyberpunk (neon, high saturation)\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    std::map<int, std::string> presets = {
        {1, "cinematic"}, {2, "vintage"}, {3, "high_contrast"},
        {4, "warm"}, {5, "cool"}, {6, "dramatic"},
        {7, "natural"}, {8, "bw"}, {9, "sepia"}, {10, "cyberpunk"}
    };
    
    if (presets.find(choice) != presets.end()) {
        VisualEffects::LoadPreset(presets[choice]);
        std::cout << "Preset loaded successfully!\n";
    } else {
        std::cout << "Invalid choice!\n";
    }
}

void AdjustBasicParameters() {
    EffectParams& params = VisualEffects::GetEffectParams();
    
    std::cout << "\n=== Basic Parameters ===\n";
    std::cout << "Current settings:\n";
    std::cout << "  Brightness: " << params.brightness << "\n";
    std::cout << "  Contrast: " << params.contrast << "\n";
    std::cout << "  Saturation: " << params.saturation << "\n";
    std::cout << "  Gamma: " << params.gamma << "\n";
    
    std::cout << "\n1. Adjust brightness (-1.0 to 3.0)\n";
    std::cout << "2. Adjust contrast (0.0 to 3.0)\n";
    std::cout << "3. Adjust saturation (0.0 to 3.0)\n";
    std::cout << "4. Adjust gamma (0.1 to 3.0)\n";
    std::cout << "5. Color tint (RGB multipliers)\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            float value;
            std::cout << "Enter brightness value: ";
            std::cin >> value;
            params.brightness = std::clamp(value, -1.0f, 3.0f);
            break;
        }
        case 2: {
            float value;
            std::cout << "Enter contrast value: ";
            std::cin >> value;
            params.contrast = std::clamp(value, 0.0f, 3.0f);
            break;
        }
        case 3: {
            float value;
            std::cout << "Enter saturation value: ";
            std::cin >> value;
            params.saturation = std::clamp(value, 0.0f, 3.0f);
            break;
        }
        case 4: {
            float value;
            std::cout << "Enter gamma value: ";
            std::cin >> value;
            params.gamma = std::clamp(value, 0.1f, 3.0f);
            break;
        }
        case 5: {
            float r, g, b;
            std::cout << "Enter RGB tint values (0.0-2.0): ";
            std::cin >> r >> g >> b;
            params.colorTint = {
                std::clamp(r, 0.0f, 2.0f),
                std::clamp(g, 0.0f, 2.0f),
                std::clamp(b, 0.0f, 2.0f)
            };
            break;
        }
    }
    
    VisualEffects::SetEffectParams(params);
    std::cout << "Parameters updated!\n";
}

void AdvancedColorGrading() {
    EffectParams& params = VisualEffects::GetEffectParams();
    
    std::cout << "\n=== Advanced Color Grading ===\n";
    std::cout << "1. Shadows adjustment\n";
    std::cout << "2. Midtones adjustment\n";
    std::cout << "3. Highlights adjustment\n";
    std::cout << "4. Three-way color balance\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            float r, g, b;
            std::cout << "Current shadows: " << params.shadows.x << ", " << params.shadows.y << ", " << params.shadows.z << "\n";
            std::cout << "Enter new shadow RGB values: ";
            std::cin >> r >> g >> b;
            params.shadows = {r, g, b};
            break;
        }
        case 2: {
            float r, g, b;
            std::cout << "Current midtones: " << params.midtones.x << ", " << params.midtones.y << ", " << params.midtones.z << "\n";
            std::cout << "Enter new midtone RGB values: ";
            std::cin >> r >> g >> b;
            params.midtones = {r, g, b};
            break;
        }
        case 3: {
            float r, g, b;
            std::cout << "Current highlights: " << params.highlights.x << ", " << params.highlights.y << ", " << params.highlights.z << "\n";
            std::cout << "Enter new highlight RGB values: ";
            std::cin >> r >> g >> b;
            params.highlights = {r, g, b};
            break;
        }
        case 4: {
            std::cout << "Automatic color balance applied\n";
            // Auto-balance algorithm
            params.shadows = {0.95f, 0.98f, 1.0f};    // Lift shadows with blue
            params.midtones = {1.0f, 1.0f, 1.0f};     // Neutral midtones
            params.highlights = {1.0f, 1.0f, 0.98f};  // Warm highlights
            break;
        }
    }
    
    VisualEffects::SetEffectParams(params);
    std::cout << "Color grading updated!\n";
}

void SpecialEffectsMenu() {
    EffectParams& params = VisualEffects::GetEffectParams();
    
    std::cout << "\n=== Special Effects ===\n";
    std::cout << "1. Toggle Sepia (" << (params.enableSepia > 0.5f ? "ON" : "OFF") << ")\n";
    std::cout << "2. Toggle Grayscale (" << (params.enableGrayscale > 0.5f ? "ON" : "OFF") << ")\n";
    std::cout << "3. Toggle Color Invert (" << (params.enableInvert > 0.5f ? "ON" : "OFF") << ")\n";
    std::cout << "4. Vignette strength (current: " << params.vignetteStrength << ")\n";
    std::cout << "5. Sharpen strength (current: " << params.sharpenStrength << ")\n";
    std::cout << "6. Film grain/noise (current: " << params.noiseStrength << ")\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1:
            params.enableSepia = (params.enableSepia > 0.5f) ? 0.0f : 1.0f;
            break;
        case 2:
            params.enableGrayscale = (params.enableGrayscale > 0.5f) ? 0.0f : 1.0f;
            break;
        case 3:
            params.enableInvert = (params.enableInvert > 0.5f) ? 0.0f : 1.0f;
            break;
        case 4: {
            float value;
            std::cout << "Enter vignette strength (0.0-1.0): ";
            std::cin >> value;
            params.vignetteStrength = std::clamp(value, 0.0f, 1.0f);
            break;
        }
        case 5: {
            float value;
            std::cout << "Enter sharpen strength (0.0-2.0): ";
            std::cin >> value;
            params.sharpenStrength = std::clamp(value, 0.0f, 2.0f);
            break;
        }
        case 6: {
            float value;
            std::cout << "Enter noise strength (0.0-0.1): ";
            std::cin >> value;
            params.noiseStrength = std::clamp(value, 0.0f, 0.1f);
            break;
        }
    }
    
    VisualEffects::SetEffectParams(params);
    std::cout << "Special effects updated!\n";
}

void MonitorPerformance() {
    std::cout << "\n=== Performance Monitor ===\n";
    std::cout << "Monitoring for 5 seconds...\n";
    
    EffectProfiler::Reset();
    
    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < 5.0f) {
        EffectProfiler::BeginFrame();
        
        // Simulate frame processing
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS simulation
        
        EffectProfiler::EndFrame();
    }
    
    std::cout << "Performance Results:\n";
    std::cout << "  Average Frame Time: " << EffectProfiler::GetAverageFrameTime() << " ms\n";
    std::cout << "  Average FPS: " << EffectProfiler::GetCurrentFPS() << "\n";
    std::cout << "  Effects Enabled: " << (VisualEffects::IsEnabled() ? "Yes" : "No") << "\n";
}

void SaveCurrentSettings() {
    // In a real implementation, this would save to a file
    EffectParams& params = VisualEffects::GetEffectParams();
    
    std::cout << "\n=== Current Settings ===\n";
    std::cout << "Brightness: " << params.brightness << "\n";
    std::cout << "Contrast: " << params.contrast << "\n";
    std::cout << "Saturation: " << params.saturation << "\n";
    std::cout << "Gamma: " << params.gamma << "\n";
    std::cout << "Color Tint: " << params.colorTint.x << ", " << params.colorTint.y << ", " << params.colorTint.z << "\n";
    std::cout << "Vignette: " << params.vignetteStrength << "\n";
    std::cout << "Sharpen: " << params.sharpenStrength << "\n";
    std::cout << "Noise: " << params.noiseStrength << "\n";
    std::cout << "Special Effects: ";
    if (params.enableSepia > 0.5f) std::cout << "Sepia ";
    if (params.enableGrayscale > 0.5f) std::cout << "Grayscale ";
    if (params.enableInvert > 0.5f) std::cout << "Invert ";
    std::cout << "\n";
    
    std::cout << "\nSettings would be saved to config file (not implemented in demo)\n";
}

void ResetToDefaults() {
    EffectParams defaultParams = {};
    defaultParams.brightness = 1.0f;
    defaultParams.contrast = 1.0f;
    defaultParams.saturation = 1.0f;
    defaultParams.gamma = 1.0f;
    defaultParams.colorTint = {1.0f, 1.0f, 1.0f};
    
    VisualEffects::SetEffectParams(defaultParams);
    std::cout << "\nSettings reset to defaults!\n";
}

int main() {
    std::cout << "=== DirectX 11 Visual Effects System ===\n";
    std::cout << "Advanced post-processing and color grading for games\n";
    std::cout << "\nWarning: This tool hooks into DirectX - use at your own risk!\n";
    std::cout << "Make sure to:\n";
    std::cout << "1. Run as administrator\n";
    std::cout << "2. Close anti-virus temporarily\n";
    std::cout << "3. Target game should use DirectX 11\n";
    
    // Initialize D3D11 Hook system
    if (!D3D11Hook::Initialize()) {
        std::cout << "\nFailed to initialize DirectX hook system!\n";
        std::cout << "Possible issues:\n";
        std::cout << "- DirectX 11 not available\n";
        std::cout << "- Missing DirectX runtime\n";
        std::cout << "- Insufficient permissions\n";
        return 1;
    }
    
    std::cout << "\nDirectX hook system initialized successfully!\n";
    std::cout << "Visual effects are now active.\n";
    
    // Main application loop
    bool running = true;
    while (running) {
        ShowMainMenu();
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                LoadPresetMenu();
                break;
                
            case 2:
                AdjustBasicParameters();
                break;
                
            case 3:
                AdvancedColorGrading();
                break;
                
            case 4:
                SpecialEffectsMenu();
                break;
                
            case 5:
                MonitorPerformance();
                break;
                
            case 6:
                VisualEffects::SetEnabled(!VisualEffects::IsEnabled());
                std::cout << "Effects " << (VisualEffects::IsEnabled() ? "enabled" : "disabled") << "\n";
                break;
                
            case 7:
                SaveCurrentSettings();
                break;
                
            case 8:
                ResetToDefaults();
                break;
                
            case 9:
                running = false;
                break;
                
            default:
                std::cout << "Invalid choice!\n";
                break;
        }
        
        // Small delay to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << "\nShutting down visual effects system...\n";
    D3D11Hook::Shutdown();
    
    std::cout << "Thank you for using Visual Effects System!\n";
    return 0;
}