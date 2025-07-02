#include "CameraSystem.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <iomanip>

void ShowMainMenu() {
    std::cout << "\n=== Advanced Camera System ===\n";
    std::cout << "1. Free Camera Mode\n";
    std::cout << "2. FOV Adjustment\n";
    std::cout << "3. Camera Transitions\n";
    std::cout << "4. Cinematic Mode\n";
    std::cout << "5. Photo Mode\n";
    std::cout << "6. Camera Tracking\n";
    std::cout << "7. Save/Load Presets\n";
    std::cout << "8. Camera Status\n";
    std::cout << "9. Settings\n";
    std::cout << "0. Exit\n";
    std::cout << "Choice: ";
}

void ShowFreeCameraMenu() {
    std::cout << "\n=== Free Camera Controls ===\n";
    std::cout << "Movement:\n";
    std::cout << "  WASD - Move forward/back/left/right\n";
    std::cout << "  QE - Move down/up\n";
    std::cout << "  Right Mouse - Look around\n";
    std::cout << "  Shift - Move faster\n";
    std::cout << "  Ctrl - Move slower\n";
    std::cout << "\n1. Enable Free Camera\n";
    std::cout << "2. Disable Free Camera\n";
    std::cout << "3. Adjust Camera Speed\n";
    std::cout << "4. Adjust Mouse Sensitivity\n";
    std::cout << "5. Toggle Y-Axis Invert\n";
    std::cout << "6. Set Position Manually\n";
    std::cout << "7. Set Rotation Manually\n";
    std::cout << "0. Back to Main Menu\n";
    std::cout << "Choice: ";
}

void HandleFreeCameraMenu(CameraSystem& camera) {
    while (true) {
        ShowFreeCameraMenu();
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                if (camera.EnableFreeCamera(true)) {
                    std::cout << "Free camera enabled! Use WASD + mouse to move around.\n";
                } else {
                    std::cout << "Failed to enable free camera.\n";
                }
                break;
                
            case 2:
                camera.EnableFreeCamera(false);
                std::cout << "Free camera disabled. Original camera restored.\n";
                break;
                
            case 3: {
                float speed;
                std::cout << "Enter camera speed (0.1 - 100.0): ";
                std::cin >> speed;
                camera.SetFreeCameraSpeed(speed);
                std::cout << "Camera speed set to " << speed << "\n";
                break;
            }
            
            case 4: {
                CameraState state;
                camera.GetCameraState(state);
                float sensitivity;
                std::cout << "Current sensitivity: " << state.sensitivity << "\n";
                std::cout << "Enter new sensitivity (0.1 - 10.0): ";
                std::cin >> sensitivity;
                state.sensitivity = std::clamp(sensitivity, 0.1f, 10.0f);
                camera.SetCameraState(state);
                std::cout << "Mouse sensitivity set to " << sensitivity << "\n";
                break;
            }
            
            case 5: {
                CameraState state;
                camera.GetCameraState(state);
                state.invertY = !state.invertY;
                camera.SetCameraState(state);
                std::cout << "Y-axis invert " << (state.invertY ? "enabled" : "disabled") << "\n";
                break;
            }
            
            case 6: {
                float x, y, z;
                std::cout << "Enter position (X Y Z): ";
                std::cin >> x >> y >> z;
                if (camera.SetCameraPosition({x, y, z})) {
                    std::cout << "Position set to (" << x << ", " << y << ", " << z << ")\n";
                } else {
                    std::cout << "Failed to set position (invalid or unsafe)\n";
                }
                break;
            }
            
            case 7: {
                float pitch, yaw, roll;
                std::cout << "Enter rotation in degrees (Pitch Yaw Roll): ";
                std::cin >> pitch >> yaw >> roll;
                
                XMFLOAT3 rotation = {
                    CameraUtils::DegreesToRadians(pitch),
                    CameraUtils::DegreesToRadians(yaw),
                    CameraUtils::DegreesToRadians(roll)
                };
                
                if (camera.SetCameraRotation(rotation)) {
                    std::cout << "Rotation set to (" << pitch << "°, " << yaw << "°, " << roll << "°)\n";
                } else {
                    std::cout << "Failed to set rotation\n";
                }
                break;
            }
            
            case 0:
                return;
                
            default:
                std::cout << "Invalid choice!\n";
                break;
        }
    }
}

void HandleFOVMenu(CameraSystem& camera) {
    std::cout << "\n=== FOV Adjustment ===\n";
    
    float currentFOV;
    if (camera.GetFOV(currentFOV)) {
        std::cout << "Current FOV: " << currentFOV << "°\n";
    } else {
        std::cout << "Could not read current FOV\n";
    }
    
    std::cout << "1. Set specific FOV\n";
    std::cout << "2. Increase FOV (+5°)\n";
    std::cout << "3. Decrease FOV (-5°)\n";
    std::cout << "4. Reset to default (60°)\n";
    std::cout << "5. Ultra-wide FOV (110°)\n";
    std::cout << "6. Cinematic FOV (35°)\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            float fov;
            std::cout << "Enter FOV (30-120): ";
            std::cin >> fov;
            if (camera.SetFOV(fov)) {
                std::cout << "FOV set to " << fov << "°\n";
            } else {
                std::cout << "Failed to set FOV (invalid range or address not found)\n";
            }
            break;
        }
        case 2:
            camera.AdjustFOV(5.0f);
            break;
        case 3:
            camera.AdjustFOV(-5.0f);
            break;
        case 4:
            camera.SetFOV(60.0f);
            break;
        case 5:
            camera.SetFOV(110.0f);
            break;
        case 6:
            camera.SetFOV(35.0f);
            break;
    }
}

void HandleTransitionMenu(CameraSystem& camera) {
    std::cout << "\n=== Camera Transitions ===\n";
    std::cout << "1. Smooth transition to position\n";
    std::cout << "2. Cinematic sweep\n";
    std::cout << "3. Quick snap\n";
    std::cout << "4. Elastic bounce\n";
    std::cout << "5. Stop current transition\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            float x, y, z, duration;
            std::cout << "Enter target position (X Y Z): ";
            std::cin >> x >> y >> z;
            std::cout << "Enter duration (seconds): ";
            std::cin >> duration;
            
            CameraState targetState;
            camera.GetCameraState(targetState);
            targetState.position = {x, y, z};
            
            camera.StartCameraTransition(targetState, duration, CameraTransition::EaseType::EaseInOut);
            std::cout << "Started smooth transition\n";
            break;
        }
        
        case 2: {
            // Cinematic sweep - move in an arc
            CameraState currentState;
            camera.GetCameraState(currentState);
            
            CameraState targetState = currentState;
            targetState.position.x += 10.0f;
            targetState.position.y += 5.0f;
            targetState.rotation.y += CameraUtils::DegreesToRadians(45.0f);
            
            camera.StartCameraTransition(targetState, 3.0f, CameraTransition::EaseType::EaseInOut);
            std::cout << "Started cinematic sweep\n";
            break;
        }
        
        case 3: {
            CameraState currentState;
            camera.GetCameraState(currentState);
            
            CameraState targetState = currentState;
            targetState.position.z += 5.0f;
            
            camera.StartCameraTransition(targetState, 0.5f, CameraTransition::EaseType::Linear);
            std::cout << "Started quick snap\n";
            break;
        }
        
        case 4: {
            CameraState currentState;
            camera.GetCameraState(currentState);
            
            CameraState targetState = currentState;
            targetState.position.y += 3.0f;
            targetState.fov = 80.0f;
            
            camera.StartCameraTransition(targetState, 2.0f, CameraTransition::EaseType::Elastic);
            std::cout << "Started elastic bounce\n";
            break;
        }
        
        case 5:
            camera.StopTransition();
            std::cout << "Transition stopped\n";
            break;
    }
}

void HandleCinematicMenu(CameraSystem& camera) {
    static CinematicCamera* cinematicCamera = nullptr;
    
    if (!cinematicCamera) {
        cinematicCamera = new CinematicCamera(&camera);
    }
    
    std::cout << "\n=== Cinematic Mode ===\n";
    std::cout << "Waypoints: " << cinematicCamera->GetWaypointCount() << "\n";
    std::cout << "Status: " << (cinematicCamera->IsPlaying() ? "Playing" : "Stopped") << "\n";
    
    std::cout << "1. Add current position as waypoint\n";
    std::cout << "2. Add custom waypoint\n";
    std::cout << "3. Clear all waypoints\n";
    std::cout << "4. Play sequence\n";
    std::cout << "5. Stop playback\n";
    std::cout << "6. Set looping\n";
    std::cout << "7. Set playback speed\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            CameraState currentState;
            camera.GetCameraState(currentState);
            float duration;
            std::cout << "Enter duration for this waypoint (seconds): ";
            std::cin >> duration;
            
            cinematicCamera->AddWaypoint(currentState, duration);
            std::cout << "Added waypoint " << cinematicCamera->GetWaypointCount() << "\n";
            break;
        }
        
        case 2: {
            float x, y, z, pitch, yaw, roll, fov, duration;
            std::cout << "Enter position (X Y Z): ";
            std::cin >> x >> y >> z;
            std::cout << "Enter rotation in degrees (Pitch Yaw Roll): ";
            std::cin >> pitch >> yaw >> roll;
            std::cout << "Enter FOV: ";
            std::cin >> fov;
            std::cout << "Enter duration (seconds): ";
            std::cin >> duration;
            
            CameraState waypoint;
            waypoint.position = {x, y, z};
            waypoint.rotation = {
                CameraUtils::DegreesToRadians(pitch),
                CameraUtils::DegreesToRadians(yaw),
                CameraUtils::DegreesToRadians(roll)
            };
            waypoint.fov = fov;
            
            cinematicCamera->AddWaypoint(waypoint, duration);
            std::cout << "Added custom waypoint\n";
            break;
        }
        
        case 3:
            cinematicCamera->ClearWaypoints();
            std::cout << "All waypoints cleared\n";
            break;
            
        case 4:
            cinematicCamera->Play();
            std::cout << "Cinematic sequence started\n";
            break;
            
        case 5:
            cinematicCamera->Stop();
            std::cout << "Cinematic sequence stopped\n";
            break;
            
        case 6: {
            char loop;
            std::cout << "Enable looping? (y/n): ";
            std::cin >> loop;
            cinematicCamera->SetLooping(loop == 'y' || loop == 'Y');
            break;
        }
        
        case 7: {
            float speed;
            std::cout << "Enter playback speed (0.1 - 5.0): ";
            std::cin >> speed;
            cinematicCamera->SetPlaybackSpeed(speed);
            break;
        }
    }
}

void HandlePhotoMode(CameraSystem& camera) {
    static PhotoMode* photoMode = nullptr;
    
    if (!photoMode) {
        photoMode = new PhotoMode(&camera);
    }
    
    std::cout << "\n=== Photo Mode ===\n";
    std::cout << "Status: " << (photoMode->IsActive() ? "Active" : "Inactive") << "\n";
    
    std::cout << "1. Enter Photo Mode\n";
    std::cout << "2. Exit Photo Mode\n";
    std::cout << "3. Adjust Depth of Field\n";
    std::cout << "4. Adjust Exposure\n";
    std::cout << "5. Toggle Orthographic Mode\n";
    std::cout << "6. Hide/Show UI\n";
    std::cout << "7. Take Screenshot\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1:
            if (photoMode->EnterPhotoMode()) {
                std::cout << "Entered Photo Mode. Use free camera controls for positioning.\n";
            } else {
                std::cout << "Failed to enter Photo Mode\n";
            }
            break;
            
        case 2:
            photoMode->ExitPhotoMode();
            std::cout << "Exited Photo Mode\n";
            break;
            
        case 3: {
            float dof;
            std::cout << "Enter depth of field (0.0 - 10.0): ";
            std::cin >> dof;
            photoMode->SetDepthOfField(dof);
            break;
        }
        
        case 4: {
            float exposure;
            std::cout << "Enter exposure (-2.0 to 2.0): ";
            std::cin >> exposure;
            photoMode->SetExposure(exposure);
            break;
        }
        
        case 5: {
            char ortho;
            std::cout << "Enable orthographic mode? (y/n): ";
            std::cin >> ortho;
            photoMode->SetOrthographicMode(ortho == 'y' || ortho == 'Y');
            break;
        }
        
        case 6: {
            char hide;
            std::cout << "Hide UI? (y/n): ";
            std::cin >> hide;
            photoMode->SetUIVisibility(!(hide == 'y' || hide == 'Y'));
            break;
        }
        
        case 7: {
            std::string filename;
            std::cout << "Enter filename (without extension): ";
            std::cin >> filename;
            photoMode->TakeScreenshot(filename);
            std::cout << "Screenshot saved as " << filename << ".png\n";
            break;
        }
    }
}

void ShowCameraStatus(CameraSystem& camera) {
    std::cout << "\n=== Camera Status ===\n";
    
    CameraState state;
    if (camera.GetCameraState(state)) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Position: (" << state.position.x << ", " << state.position.y << ", " << state.position.z << ")\n";
        std::cout << "Rotation: (" 
                  << CameraUtils::RadiansToDegrees(state.rotation.x) << "°, "
                  << CameraUtils::RadiansToDegrees(state.rotation.y) << "°, "
                  << CameraUtils::RadiansToDegrees(state.rotation.z) << "°)\n";
        std::cout << "FOV: " << state.fov << "°\n";
        std::cout << "Speed: " << state.speed << "\n";
        std::cout << "Sensitivity: " << state.sensitivity << "\n";
        std::cout << "Y-Axis Inverted: " << (state.invertY ? "Yes" : "No") << "\n";
    } else {
        std::cout << "Could not read camera state\n";
    }
    
    std::cout << "Free Camera: " << (camera.IsFreeCameraEnabled() ? "Enabled" : "Disabled") << "\n";
    std::cout << "Transition Active: " << (camera.IsTransitionActive() ? "Yes" : "No") << "\n";
    std::cout << "Safety Mode: " << (camera.GetSafetyMode() ? "Enabled" : "Disabled") << "\n";
}

void HandleSettings(CameraSystem& camera) {
    std::cout << "\n=== Settings ===\n";
    std::cout << "1. Toggle Safety Mode\n";
    std::cout << "2. Set FOV Limits\n";
    std::cout << "3. Reset to Original Camera\n";
    std::cout << "4. Camera Information\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1:
            camera.SetSafetyMode(!camera.GetSafetyMode());
            std::cout << "Safety mode " << (camera.GetSafetyMode() ? "enabled" : "disabled") << "\n";
            std::cout << "Safety mode prevents extreme camera positions and invalid values.\n";
            break;
            
        case 2: {
            float minFOV, maxFOV;
            std::cout << "Enter minimum FOV (10-90): ";
            std::cin >> minFOV;
            std::cout << "Enter maximum FOV (60-179): ";
            std::cin >> maxFOV;
            camera.SetFOVLimits(minFOV, maxFOV);
            std::cout << "FOV limits set to " << minFOV << "° - " << maxFOV << "°\n";
            break;
        }
        
        case 3:
            camera.RestoreOriginalCamera();
            std::cout << "Camera restored to original state\n";
            break;
            
        case 4:
            std::cout << "Camera System Information:\n";
            std::cout << "  Initialized: " << (camera.IsInitialized() ? "Yes" : "No") << "\n";
            std::cout << "  Safety Mode: " << (camera.GetSafetyMode() ? "Enabled" : "Disabled") << "\n";
            std::cout << "  Features: Free Camera, FOV Control, Transitions, Cinematic Mode\n";
            break;
    }
}

int main() {
    std::cout << "=== Advanced Camera System for Game Modding ===\n";
    std::cout << "Enhanced camera control with cinematic features\n";
    std::cout << "\nSupported games: Elden Ring, Dark Souls series, Skyrim, and more\n";
    std::cout << "\nWarning: Use responsibly and respect game developers!\n";
    
    // Get target process
    std::wstring processName;
    std::cout << "\nEnter game executable name (e.g., eldenring.exe): ";
    std::wcin >> processName;
    
    // Initialize camera system
    CameraSystem camera;
    
    if (!camera.Initialize(processName)) {
        std::cout << "\nFailed to initialize camera system!\n";
        std::cout << "Make sure:\n";
        std::cout << "1. Game is running\n";
        std::cout << "2. Running as administrator\n";
        std::cout << "3. Game uses a supported engine\n";
        std::cout << "4. Anti-cheat is not blocking access\n";
        return 1;
    }
    
    std::cout << "\nCamera system initialized successfully!\n";
    std::cout << "You can now control the camera using the menu options.\n";
    
    // Main application loop
    bool running = true;
    while (running) {
        // Update camera system (handles transitions, input, etc.)
        camera.Update();
        
        ShowMainMenu();
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                HandleFreeCameraMenu(camera);
                break;
                
            case 2:
                HandleFOVMenu(camera);
                break;
                
            case 3:
                HandleTransitionMenu(camera);
                break;
                
            case 4:
                HandleCinematicMenu(camera);
                break;
                
            case 5:
                HandlePhotoMode(camera);
                break;
                
            case 6:
                std::cout << "Camera tracking not implemented in this demo\n";
                break;
                
            case 7:
                std::cout << "Preset system not implemented in this demo\n";
                break;
                
            case 8:
                ShowCameraStatus(camera);
                break;
                
            case 9:
                HandleSettings(camera);
                break;
                
            case 0:
                running = false;
                break;
                
            default:
                std::cout << "Invalid choice!\n";
                break;
        }
        
        // Small delay to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Cleanup
    std::cout << "\nShutting down camera system...\n";
    camera.Shutdown();
    
    std::cout << "Thank you for using Advanced Camera System!\n";
    return 0;
}