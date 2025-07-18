# Exercise Solutions - 카메라 시스템 수정

이 폴더는 scenario-camera-system의 연습문제 해답들을 포함합니다.

## 📋 연습문제 목록

### Exercise 1: 카메라 주소 찾기
**문제**: 게임의 카메라 위치와 회전 정보가 저장된 메모리 주소를 찾는 스캐너를 작성하세요.

**해답 파일**: `exercise1_camera_scanner.cpp`

### Exercise 2: 기본 자유 카메라
**문제**: WASD 키로 카메라를 이동하고 마우스로 시점을 변경하는 시스템을 구현하세요.

**해답 파일**: `exercise2_free_camera.cpp`

### Exercise 3: FOV 조정
**문제**: 마우스 휠이나 키보드로 실시간 FOV 조정이 가능한 시스템을 만드세요.

**해답 파일**: `exercise3_fov_control.cpp`

### Exercise 4: 카메라 애니메이션
**문제**: 두 지점 사이를 부드럽게 이동하는 카메라 전환 시스템을 구현하세요.

**해답 파일**: `exercise4_camera_transition.cpp`

### Exercise 5: 포토 모드
**문제**: 게임을 일시정지하고 카메라를 자유롭게 조작할 수 있는 포토 모드를 만드세요.

**해답 파일**: `exercise5_photo_mode.cpp`

## 🎥 학습 목표

### 3D 수학
1. **벡터 연산**: 위치, 방향, 거리 계산
2. **행렬 변환**: 회전, 이동, 스케일
3. **쿼터니언**: 부드러운 회전 보간
4. **투영 변환**: 원근/직교 투영

### 카메라 시스템
1. **뷰 행렬**: 월드 좌표계에서 뷰 좌표계로 변환
2. **투영 행렬**: FOV, 종횡비, 클리핑 평면
3. **뷰포트**: 화면 좌표계 변환
4. **카메라 제어**: 다양한 카메라 모드

## 🔧 핵심 기술

### 메모리 구조 분석
```cpp
// 일반적인 카메라 구조체
struct CameraData {
    XMFLOAT3 position;      // 0x00: 카메라 위치
    XMFLOAT3 rotation;      // 0x0C: 오일러 각도
    float fov;              // 0x18: 시야각
    float nearPlane;        // 0x1C: 근거리 클리핑
    float farPlane;         // 0x20: 원거리 클리핑
    XMFLOAT4X4 viewMatrix;  // 0x24: 뷰 행렬
    XMFLOAT4X4 projMatrix;  // 0x64: 투영 행렬
};
```

### 게임 엔진별 패턴
```cpp
// Unreal Engine 카메라 패턴
namespace UE4Camera {
    const char* POSITION_PATTERN = "F3 0F 11 40 ? F3 0F 11 48 ? F3 0F 11 50 ?";
    const char* ROTATION_PATTERN = "F3 0F 10 40 ? F3 0F 10 48 ? F3 0F 10 50 ?";
    const char* FOV_PATTERN = "F3 0F 11 81 ? ? ? ? 8B 81 ? ? ? ?";
}

// Unity 카메라 패턴
namespace UnityCamera {
    const char* TRANSFORM_PATTERN = "48 8B 80 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ?";
    const char* CAMERA_PATTERN = "48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9";
}
```

### 수학 유틸리티
```cpp
// 오일러 각도를 방향 벡터로 변환
XMFLOAT3 EulerToDirection(const XMFLOAT3& euler) {
    float pitch = euler.x;
    float yaw = euler.y;
    
    return {
        cos(pitch) * cos(yaw),
        sin(pitch),
        cos(pitch) * sin(yaw)
    };
}

// 부드러운 카메라 이동
XMFLOAT3 SmoothMove(const XMFLOAT3& current, const XMFLOAT3& target, float speed, float deltaTime) {
    XMVECTOR currentVec = XMLoadFloat3(&current);
    XMVECTOR targetVec = XMLoadFloat3(&target);
    XMVECTOR direction = XMVectorSubtract(targetVec, currentVec);
    float distance = XMVectorGetX(XMVector3Length(direction));
    
    if (distance < 0.01f) return target;
    
    direction = XMVectorScale(XMVector3Normalize(direction), speed * deltaTime);
    XMVECTOR result = XMVectorAdd(currentVec, direction);
    
    XMFLOAT3 output;
    XMStoreFloat3(&output, result);
    return output;
}
```

## 🎮 실습 환경

### 권장 테스트 게임
- **Elden Ring**: 복잡한 카메라 시스템
- **Dark Souls III**: 타겟 락온 시스템
- **Skyrim SE**: 1인칭/3인칭 전환
- **The Witcher 3**: 동적 FOV 시스템

### 개발 도구
```cpp
// 카메라 디버그 정보 표시
void RenderCameraDebugInfo(const CameraData& camera) {
    char debugText[1024];
    sprintf_s(debugText, 
        "Camera Debug Info\n"
        "Position: (%.2f, %.2f, %.2f)\n"
        "Rotation: (%.2f°, %.2f°, %.2f°)\n"
        "FOV: %.2f°\n"
        "Near/Far: %.2f / %.2f",
        camera.position.x, camera.position.y, camera.position.z,
        XMConvertToDegrees(camera.rotation.x),
        XMConvertToDegrees(camera.rotation.y),
        XMConvertToDegrees(camera.rotation.z),
        XMConvertToDegrees(camera.fov),
        camera.nearPlane, camera.farPlane
    );
    
    // 화면에 텍스트 출력 (ImGui 또는 DirectWrite 사용)
    RenderText(debugText, 10, 10);
}
```

## 🎯 성능 최적화

### 효율적인 업데이트
```cpp
class CameraController {
private:
    float updateInterval = 1.0f / 60.0f;  // 60 FPS
    float lastUpdateTime = 0.0f;
    
public:
    void Update(float currentTime) {
        if (currentTime - lastUpdateTime < updateInterval) {
            return;  // 업데이트 스킵
        }
        
        // 카메라 업데이트 로직
        UpdateCameraPosition();
        UpdateCameraRotation();
        
        lastUpdateTime = currentTime;
    }
};
```

### 메모리 최적화
```cpp
// 카메라 상태 캐싱
class CameraCache {
private:
    CameraData cachedData;
    bool isDirty = true;
    
public:
    const CameraData& GetCameraData() {
        if (isDirty) {
            ReadCameraFromMemory(cachedData);
            isDirty = false;
        }
        return cachedData;
    }
    
    void InvalidateCache() {
        isDirty = true;
    }
};
```

## ⚠️ 주의사항

### 안전 가이드라인
```cpp
// 안전한 카메라 위치 검증
bool IsPositionSafe(const XMFLOAT3& position) {
    // 극단적인 값 검증
    if (abs(position.x) > 1000000.0f || 
        abs(position.y) > 1000000.0f || 
        abs(position.z) > 1000000.0f) {
        return false;
    }
    
    // NaN/Inf 검증
    if (!isfinite(position.x) || !isfinite(position.y) || !isfinite(position.z)) {
        return false;
    }
    
    // 게임별 안전 영역 검증
    return IsWithinGameBounds(position);
}
```

### 게임별 제한사항
- **온라인 게임**: 카메라 조작 감지 가능
- **컷신**: 자동 카메라 복원 필요
- **로딩 화면**: 카메라 데이터 무효화
- **메뉴**: UI 카메라와 게임 카메라 구분

## 📊 평가 기준

| 항목 | 가중치 | 평가 요소 |
|------|--------|-----------|
| 정확성 | 25% | 카메라 데이터를 정확히 읽고 쓰는가? |
| 사용성 | 25% | 직관적이고 편리한 조작감인가? |
| 안정성 | 20% | 게임 크래시나 오작동이 없는가? |
| 성능 | 15% | FPS 저하 없이 부드럽게 동작하는가? |
| 기능성 | 15% | 요구된 모든 기능을 구현했는가? |

## 🔍 검증 방법

### 기능 테스트
```cpp
// 자동화된 카메라 테스트
void TestCameraSystem() {
    CameraController camera;
    
    // 1. 위치 이동 테스트
    XMFLOAT3 startPos = camera.GetPosition();
    XMFLOAT3 targetPos = {startPos.x + 10, startPos.y, startPos.z};
    camera.SetPosition(targetPos);
    
    assert(Distance(camera.GetPosition(), targetPos) < 0.1f);
    
    // 2. 회전 테스트
    XMFLOAT3 rotation = {0, XM_PIDIV4, 0};  // 45도 회전
    camera.SetRotation(rotation);
    
    // 3. FOV 테스트
    camera.SetFOV(XMConvertToRadians(90.0f));
    assert(abs(camera.GetFOV() - XMConvertToRadians(90.0f)) < 0.01f);
}
```

---

**📷 목표: 3D 카메라 시스템의 이해와 실시간 조작 기술 습득**