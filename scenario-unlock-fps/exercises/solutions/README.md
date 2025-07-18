# Exercise Solutions - FPS 제한 해제

이 폴더는 scenario-unlock-fps의 연습문제 해답들을 포함합니다.

## 📋 연습문제 목록

### Exercise 1: FPS 값 스캔
**문제**: 메모리에서 FPS 제한 값을 찾는 스캐너를 작성하세요.

**해답 파일**: `exercise1_fps_scanner.cpp`

### Exercise 2: 동적 주소 추적
**문제**: 게임 재시작 후에도 FPS 주소를 자동으로 찾는 시스템을 구현하세요.

**해답 파일**: `exercise2_dynamic_address.cpp`

### Exercise 3: 안전한 FPS 변경
**문제**: 게임별 안전한 FPS 범위를 확인하고 제한하는 기능을 만드세요.

**해답 파일**: `exercise3_safe_fps_change.cpp`

### Exercise 4: FPS 모니터링
**문제**: 실시간으로 FPS를 측정하고 표시하는 모니터를 작성하세요.

**해답 파일**: `exercise4_fps_monitor.cpp`

### Exercise 5: 프리셋 시스템
**문제**: 다양한 FPS 프리셋을 저장하고 불러오는 시스템을 구현하세요.

**해답 파일**: `exercise5_fps_presets.cpp`

## 📚 학습 목표

1. **메모리 패턴 매칭**: AOB 스캔 기법 습득
2. **포인터 체이싱**: 다단계 포인터 추적
3. **게임별 최적화**: 엔진 특성 이해
4. **성능 측정**: 정확한 FPS 계산
5. **데이터 관리**: 설정 저장/로드 시스템

## 🎮 지원 게임

연습문제는 다음 게임들을 대상으로 합니다:

- **Elden Ring**: FromSoftware 엔진
- **Dark Souls III**: 동일 엔진 패턴
- **Skyrim SE**: Creation Engine
- **The Witcher 3**: REDengine

## 🔧 기술 요구사항

### 필수 기술
- **메모리 스캔**: `VirtualQueryEx`, `ReadProcessMemory`
- **패턴 매칭**: AOB (Array of Bytes) 검색
- **프로세스 제어**: 프로세스 핸들 관리
- **시간 측정**: 고정밀 타이머 사용

### 고급 기술
- **시그니처 생성**: IDA Pro/Ghidra 활용
- **동적 분석**: 디버거 연동
- **예외 처리**: SEH (Structured Exception Handling)
- **멀티스레딩**: 비동기 스캔

## ⚠️ 주의사항

### 안전 가이드라인
1. **백업**: 게임 파일 백업 필수
2. **테스트**: 오프라인 모드에서 먼저 테스트
3. **범위 확인**: 안전한 FPS 범위 준수
4. **예외 처리**: 메모리 접근 실패 대응

### 성능 고려사항
```cpp
// 효율적인 스캔 범위 제한
MEMORY_BASIC_INFORMATION mbi;
while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
    // 실행 가능한 메모리만 스캔
    if (mbi.State == MEM_COMMIT && 
        (mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
        // 스캔 수행
    }
    address += mbi.RegionSize;
}
```

## 📊 평가 기준

| 항목 | 가중치 | 평가 요소 |
|------|--------|-----------|
| 정확성 | 30% | FPS 값을 정확히 찾고 변경하는가? |
| 안정성 | 25% | 게임 크래시나 오류 없이 동작하는가? |
| 효율성 | 20% | 빠르고 효율적으로 스캔하는가? |
| 사용성 | 15% | 사용자가 쉽게 사용할 수 있는가? |
| 확장성 | 10% | 다른 게임에도 적용 가능한가? |

---

**🎯 목표: FPS 제한 해제를 통해 메모리 조작의 기초를 확실히 익히기**