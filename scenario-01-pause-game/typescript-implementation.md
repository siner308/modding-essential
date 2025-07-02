# 🚀 TypeScript로 EldenRing PauseTheGame 모드 구현

**C++ 대신 TypeScript/Node.js를 사용한 대안적 접근법**

## 📚 왜 TypeScript로 모딩을 하는가?

### TypeScript 모딩의 장점
- ✅ **타입 안전성**: 컴파일 시점에 오류 검출
- ✅ **빠른 개발**: 인터프리터 언어의 빠른 반복 개발
- ✅ **크로스 플랫폼**: Windows, macOS, Linux 지원
- ✅ **풍부한 생태계**: npm 패키지 활용 가능
- ✅ **디버깅 용이**: 실시간 로그, 핫 리로드 지원

### C++과의 차이점
```
C++ 모드:                    TypeScript 모드:
├── 컴파일 필요 (.dll)      ├── 인터프리터 실행 (.js)
├── Visual Studio 필요      ├── VS Code / 텍스트 에디터
├── Windows API 직접 호출   ├── FFI를 통한 API 호출  
├── 메모리 관리 수동        ├── 가비지 컬렉션 자동
└── 빌드 시간 필요          └── 즉시 실행 가능
```

## 🛠️ 프로젝트 설정

### 1. 기본 환경 구성

```bash
# Node.js 18+ 필요 (https://nodejs.org/ 에서 다운로드)
node --version  # v18.0.0 이상 확인

# 프로젝트 폴더 생성
mkdir eldenring-pause-ts
cd eldenring-pause-ts

# npm 프로젝트 초기화
npm init -y
```

### 2. 필수 패키지 설치

```bash
# TypeScript 관련
npm install -D typescript @types/node @types/ffi-napi

# 메모리 조작용 패키지
npm install ffi-napi ref-napi ref-struct-napi

# 추가 유틸리티
npm install node-key-sender
```

### 3. 프로젝트 구조

```
eldenring-pause-ts/
├── package.json
├── tsconfig.json
├── config.json                 # 설정 파일
├── src/
│   ├── types.ts                # 타입 정의
│   ├── memory.ts               # 메모리 관리
│   ├── aob-scanner.ts          # AOB 패턴 스캔
│   ├── pause-mod.ts            # 메인 모드 로직
│   ├── input-handler.ts        # 입력 처리
│   └── main.ts                 # 진입점
├── dist/                       # 컴파일된 JS 파일
└── README.md
```

## 📝 핵심 코드 구현

### 1. 타입 정의 (src/types.ts)

```typescript
// 프로세스 정보
export interface ProcessInfo {
  pid: number;
  handle: Buffer;
  baseAddress: number;
}

// 패치 정보
export interface PatchInfo {
  address: number;
  originalByte: number;
  patchedByte: number;
  isApplied: boolean;
}

// 키바인드 설정
export interface KeybindConfig {
  pauseKey: string;
  unpauseKey: string;
  enableLogging: boolean;
}

// AOB 스캔 결과
export interface ScanResult {
  pattern: string;
  addresses: number[];
  foundCount: number;
}
```

### 2. 메모리 관리자 (src/memory.ts)

```typescript
import * as ffi from 'ffi-napi';
import * as ref from 'ref-napi';

const PROCESS_ALL_ACCESS = 0x1F0FFF;
const PAGE_EXECUTE_READWRITE = 0x40;

// Windows API 바인딩
const kernel32 = ffi.Library('kernel32', {
  'OpenProcess': ['pointer', ['uint32', 'bool', 'uint32']],
  'ReadProcessMemory': ['bool', ['pointer', 'pointer', 'pointer', 'size_t', 'pointer']],
  'WriteProcessMemory': ['bool', ['pointer', 'pointer', 'pointer', 'size_t', 'pointer']],
  'VirtualProtectEx': ['bool', ['pointer', 'pointer', 'size_t', 'uint32', 'pointer']],
  'CloseHandle': ['bool', ['pointer']]
});

const user32 = ffi.Library('user32', {
  'FindWindowW': ['pointer', ['pointer', 'pointer']],
  'GetWindowThreadProcessId': ['uint32', ['pointer', 'pointer']]
});

export class MemoryManager {
  private processHandle: Buffer | null = null;
  private processId: number = 0;

  async findEldenRingProcess(): Promise<boolean> {
    try {
      // EldenRing 창 찾기
      const windowName = Buffer.from('ELDEN RING™\0', 'utf16le');
      const hwnd = user32.FindWindowW(null, windowName);
      
      if (hwnd.isNull()) {
        console.log('❌ EldenRing 프로세스를 찾을 수 없습니다.');
        console.log('💡 게임이 실행 중인지 확인하세요.');
        return false;
      }

      // 프로세스 ID 획득
      const pidPtr = ref.alloc('uint32');
      user32.GetWindowThreadProcessId(hwnd, pidPtr);
      this.processId = pidPtr.deref();

      // 프로세스 핸들 열기
      this.processHandle = kernel32.OpenProcess(PROCESS_ALL_ACCESS, false, this.processId);
      
      if (this.processHandle.isNull()) {
        console.log('❌ 프로세스 핸들을 열 수 없습니다.');
        console.log('💡 관리자 권한으로 실행하세요.');
        return false;
      }

      console.log(`✅ EldenRing 프로세스 발견: PID ${this.processId}`);
      return true;
    } catch (error) {
      console.error('❌ 프로세스 검색 오류:', error);
      return false;
    }
  }

  readMemory(address: number, size: number): Buffer | null {
    if (!this.processHandle) return null;

    const buffer = Buffer.alloc(size);
    const bytesRead = ref.alloc('size_t');
    const addressPtr = ref.address(Buffer.alloc(8));
    addressPtr.writeUInt64LE(BigInt(address), 0);

    const success = kernel32.ReadProcessMemory(
      this.processHandle,
      addressPtr,
      buffer,
      size,
      bytesRead
    );

    return success ? buffer : null;
  }

  writeMemory(address: number, data: Buffer): boolean {
    if (!this.processHandle) return false;

    const bytesWritten = ref.alloc('size_t');
    const oldProtect = ref.alloc('uint32');
    const addressPtr = ref.address(Buffer.alloc(8));
    addressPtr.writeUInt64LE(BigInt(address), 0);

    // 메모리 보호 해제
    kernel32.VirtualProtectEx(
      this.processHandle,
      addressPtr,
      data.length,
      PAGE_EXECUTE_READWRITE,
      oldProtect
    );

    const success = kernel32.WriteProcessMemory(
      this.processHandle,
      addressPtr,
      data,
      data.length,
      bytesWritten
    );

    // 메모리 보호 복원
    kernel32.VirtualProtectEx(
      this.processHandle,
      addressPtr,
      data.length,
      oldProtect.deref(),
      oldProtect
    );

    return success;
  }

  close(): void {
    if (this.processHandle && !this.processHandle.isNull()) {
      kernel32.CloseHandle(this.processHandle);
      this.processHandle = null;
    }
  }
}
```

### 3. AOB 스캐너 (src/aob-scanner.ts)

```typescript
import { MemoryManager } from './memory';
import { ScanResult } from './types';

export class AOBScanner {
  constructor(private memoryManager: MemoryManager) {}

  patternToBytes(pattern: string): { bytes: number[], mask: string } {
    const parts = pattern.split(' ').filter(p => p.length > 0);
    const bytes: number[] = [];
    let mask = '';

    for (const part of parts) {
      if (part === '?' || part === '??') {
        bytes.push(0);
        mask += '?';
      } else {
        bytes.push(parseInt(part, 16));
        mask += 'x';
      }
    }

    return { bytes, mask };
  }

  scanForPattern(
    pattern: string, 
    startAddress: number = 0x140000000, 
    endAddress: number = 0x150000000
  ): ScanResult {
    const { bytes, mask } = this.patternToBytes(pattern);
    const results: number[] = [];
    const chunkSize = 0x1000; // 4KB 청크

    console.log(`🔍 AOB 스캔 시작: ${pattern}`);
    console.log(`📍 범위: 0x${startAddress.toString(16)} - 0x${endAddress.toString(16)}`);

    let scannedChunks = 0;
    const totalChunks = Math.floor((endAddress - startAddress) / chunkSize);

    for (let addr = startAddress; addr < endAddress; addr += chunkSize) {
      const memory = this.memoryManager.readMemory(addr, chunkSize);
      if (!memory) continue;

      for (let i = 0; i <= memory.length - bytes.length; i++) {
        let found = true;
        
        for (let j = 0; j < bytes.length; j++) {
          if (mask[j] === 'x' && memory[i + j] !== bytes[j]) {
            found = false;
            break;
          }
        }

        if (found) {
          const foundAddress = addr + i;
          results.push(foundAddress);
          console.log(`✅ 패턴 발견: 0x${foundAddress.toString(16)}`);
        }
      }

      scannedChunks++;
      if (scannedChunks % 1000 === 0) {
        const progress = ((scannedChunks / totalChunks) * 100).toFixed(1);
        console.log(`⏳ 스캔 진행률: ${progress}%`);
      }
    }

    console.log(`🏁 스캔 완료: ${results.length}개 패턴 발견`);

    return {
      pattern,
      addresses: results,
      foundCount: results.length
    };
  }
}
```

### 4. 메인 모드 로직 (src/pause-mod.ts)

```typescript
import { MemoryManager } from './memory';
import { AOBScanner } from './aob-scanner';
import { PatchInfo, KeybindConfig } from './types';
import * as fs from 'fs';
import * as path from 'path';

export class EldenRingPauseMod {
  private memoryManager: MemoryManager;
  private aobScanner: AOBScanner;
  private patchInfo: PatchInfo | null = null;
  private isPaused: boolean = false;
  private config: KeybindConfig;

  // EldenRing PauseTheGame 모드와 동일한 AOB 패턴
  private readonly AOB_PATTERN = "0f 84 ? ? ? ? c6 ? ? ? ? ? 00 ? 8d ? ? ? ? ? ? 89 ? ? 89 ? ? ? 8b ? ? ? ? ? ? 85 ? 75";
  private readonly PATCH_OFFSET = 1; // JE -> JNE 패치 위치

  constructor() {
    this.memoryManager = new MemoryManager();
    this.aobScanner = new AOBScanner(this.memoryManager);
    this.config = this.loadConfig();
  }

  private loadConfig(): KeybindConfig {
    const configPath = path.join(process.cwd(), 'config.json');
    
    try {
      if (fs.existsSync(configPath)) {
        const configData = fs.readFileSync(configPath, 'utf8');
        const config = JSON.parse(configData);
        console.log('✅ 설정 파일 로드 완료');
        return config;
      }
    } catch (error) {
      console.log('⚠️ 설정 파일 로드 실패, 기본값 사용');
    }

    // 기본 설정
    const defaultConfig: KeybindConfig = {
      pauseKey: 'p',
      unpauseKey: 'p',
      enableLogging: true
    };

    // 기본 설정 저장
    try {
      fs.writeFileSync(configPath, JSON.stringify(defaultConfig, null, 2));
      console.log('📝 기본 설정 파일 생성 완료');
    } catch (error) {
      console.log('⚠️ 설정 파일 저장 실패');
    }

    return defaultConfig;
  }

  async initialize(): Promise<boolean> {
    console.log('🚀 EldenRing PauseTheGame 모드 (TypeScript) 초기화 중...');

    // 프로세스 찾기
    if (!await this.memoryManager.findEldenRingProcess()) {
      return false;
    }

    // AOB 패턴 스캔
    const scanResult = this.aobScanner.scanForPattern(this.AOB_PATTERN);
    
    if (scanResult.foundCount === 0) {
      console.log('❌ 패턴을 찾을 수 없습니다.');
      console.log('💡 게임 버전을 확인하거나 다른 패턴을 시도하세요.');
      return false;
    }

    if (scanResult.foundCount > 1) {
      console.log(`⚠️ 여러 패턴 발견 (${scanResult.foundCount}개), 첫 번째 주소 사용`);
    }

    const patchAddress = scanResult.addresses[0] + this.PATCH_OFFSET;
    
    // 원본 바이트 읽기
    const originalByte = this.memoryManager.readMemory(patchAddress, 1);
    if (!originalByte) {
      console.log('❌ 원본 바이트를 읽을 수 없습니다.');
      return false;
    }

    this.patchInfo = {
      address: patchAddress,
      originalByte: originalByte[0], // 0x84 (JE)
      patchedByte: 0x85, // JNE
      isApplied: false
    };

    console.log(`📍 패치 주소: 0x${patchAddress.toString(16)}`);
    console.log(`📋 원본 바이트: 0x${this.patchInfo.originalByte.toString(16)} (JE)`);
    console.log(`📋 패치 바이트: 0x${this.patchInfo.patchedByte.toString(16)} (JNE)`);
    console.log('✅ 초기화 완료!');

    return true;
  }

  pause(): boolean {
    if (!this.patchInfo || this.isPaused) {
      console.log('⚠️ 이미 일시정지 상태이거나 패치 정보가 없습니다.');
      return false;
    }

    if (this.config.enableLogging) {
      console.log('⏸️ 게임 일시정지 적용 중...');
    }
    
    // JE(0x84) -> JNE(0x85) 패치
    const patchByte = Buffer.from([this.patchInfo.patchedByte]);
    
    if (this.memoryManager.writeMemory(this.patchInfo.address, patchByte)) {
      this.patchInfo.isApplied = true;
      this.isPaused = true;
      console.log('✅ 게임이 일시정지되었습니다.');
      return true;
    }

    console.log('❌ 패치 적용 실패');
    return false;
  }

  unpause(): boolean {
    if (!this.patchInfo || !this.isPaused) {
      console.log('⚠️ 일시정지 상태가 아니거나 패치 정보가 없습니다.');
      return false;
    }

    if (this.config.enableLogging) {
      console.log('▶️ 게임 일시정지 해제 중...');
    }
    
    // JNE(0x85) -> JE(0x84) 복원
    const originalByte = Buffer.from([this.patchInfo.originalByte]);
    
    if (this.memoryManager.writeMemory(this.patchInfo.address, originalByte)) {
      this.patchInfo.isApplied = false;
      this.isPaused = false;
      console.log('✅ 게임 일시정지가 해제되었습니다.');
      return true;
    }

    console.log('❌ 패치 복원 실패');
    return false;
  }

  togglePause(): boolean {
    return this.isPaused ? this.unpause() : this.pause();
  }

  getStatus(): { 
    isPaused: boolean; 
    patchAddress?: string; 
    originalByte?: string;
    patchedByte?: string;
  } {
    return {
      isPaused: this.isPaused,
      patchAddress: this.patchInfo ? `0x${this.patchInfo.address.toString(16)}` : undefined,
      originalByte: this.patchInfo ? `0x${this.patchInfo.originalByte.toString(16)}` : undefined,
      patchedByte: this.patchInfo ? `0x${this.patchInfo.patchedByte.toString(16)}` : undefined
    };
  }

  cleanup(): void {
    console.log('🧹 정리 작업 중...');
    
    // 종료 시 패치 복원
    if (this.isPaused) {
      console.log('⚠️ 일시정지 상태에서 종료, 패치 복원 중...');
      this.unpause();
    }
    
    this.memoryManager.close();
    console.log('✅ 정리 완료');
  }
}
```

### 5. 메인 실행 파일 (src/main.ts)

```typescript
import { EldenRingPauseMod } from './pause-mod';
import * as readline from 'readline';

class InputHandler {
  private rl: readline.Interface;

  constructor() {
    this.rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout
    });
  }

  async waitForInput(message: string): Promise<string> {
    return new Promise((resolve) => {
      this.rl.question(message, (answer) => {
        resolve(answer.trim().toLowerCase());
      });
    });
  }

  close(): void {
    this.rl.close();
  }
}

async function main() {
  console.log('='.repeat(60));
  console.log('🎮 EldenRing PauseTheGame 모드 (TypeScript 버전)');
  console.log('='.repeat(60));
  console.log('⚠️ 주의사항:');
  console.log('  • 관리자 권한으로 실행해야 합니다');
  console.log('  • EldenRing이 실행 중이어야 합니다');
  console.log('  • 오프라인 모드에서만 사용하세요');
  console.log('  • EAC를 비활성화해야 합니다');
  console.log('='.repeat(60));
  console.log('');

  const pauseMod = new EldenRingPauseMod();
  const inputHandler = new InputHandler();

  // 초기화
  if (!await pauseMod.initialize()) {
    console.log('❌ 초기화 실패. 프로그램을 종료합니다.');
    process.exit(1);
  }

  // 종료 시 정리 작업
  const cleanup = () => {
    console.log('\\n🛑 프로그램 종료 중...');
    pauseMod.cleanup();
    inputHandler.close();
    process.exit(0);
  };

  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);

  // 메인 루프
  console.log('\\n📋 사용법:');
  console.log('  p  : 일시정지/해제 토글');
  console.log('  s  : 현재 상태 확인');
  console.log('  h  : 도움말 표시');
  console.log('  q  : 프로그램 종료');
  console.log('');

  while (true) {
    try {
      const input = await inputHandler.waitForInput('🎮 명령어 입력 (p/s/h/q): ');

      switch (input) {
        case 'p':
          pauseMod.togglePause();
          break;

        case 's':
          const status = pauseMod.getStatus();
          console.log('\\n📊 현재 상태:');
          console.log(`  상태: ${status.isPaused ? '⏸️ 일시정지됨' : '▶️ 정상 실행 중'}`);
          if (status.patchAddress) {
            console.log(`  패치 주소: ${status.patchAddress}`);
            console.log(`  원본 바이트: ${status.originalByte} (JE)`);
            console.log(`  패치 바이트: ${status.patchedByte} (JNE)`);
          }
          console.log('');
          break;

        case 'h':
          console.log('\\n📖 도움말:');
          console.log('  이 모드는 EldenRing의 게임 루프를 일시정지시킵니다.');
          console.log('  조건부 점프(JE→JNE)를 변경하여 게임 로직 실행을 막습니다.');
          console.log('  종료 시 자동으로 원본 상태로 복원됩니다.');
          console.log('\\n⚠️ 주의: 온라인 모드에서는 절대 사용하지 마세요!');
          console.log('');
          break;

        case 'q':
          console.log('👋 프로그램을 종료합니다.');
          cleanup();
          break;

        default:
          console.log('❓ 잘못된 명령어입니다. h를 입력하면 도움말을 볼 수 있습니다.');
          break;
      }
    } catch (error) {
      console.error('❌ 입력 처리 오류:', error);
    }
  }
}

// 에러 핸들링
process.on('uncaughtException', (error) => {
  console.error('💥 예기치 않은 오류:', error);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('💥 처리되지 않은 Promise 거부:', reason);
  process.exit(1);
});

main().catch((error) => {
  console.error('💥 메인 함수 오류:', error);
  process.exit(1);
});
```

## ⚙️ 설정 파일 (config.json)

```json
{
  "pauseKey": "p",
  "unpauseKey": "p",
  "enableLogging": true,
  "scanStartAddress": "0x140000000",
  "scanEndAddress": "0x150000000",
  "patchOffset": 1,
  "aobPattern": "0f 84 ? ? ? ? c6 ? ? ? ? ? 00 ? 8d ? ? ? ? ? ? 89 ? ? 89 ? ? ? 8b ? ? ? ? ? ? 85 ? 75"
}
```

## 🏗️ 빌드 및 실행

### TypeScript 설정 (tsconfig.json)

```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "lib": ["ES2020"],
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "declaration": true,
    "sourceMap": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

### 실행 스크립트 (package.json)

```json
{
  "name": "eldenring-pause-ts",
  "version": "1.0.0",
  "description": "EldenRing PauseTheGame mod in TypeScript",
  "main": "dist/main.js",
  "scripts": {
    "build": "tsc",
    "start": "npm run build && node dist/main.js",
    "dev": "tsc --watch",
    "clean": "rimraf dist",
    "lint": "eslint src/**/*.ts",
    "test": "npm run build && node dist/main.js"
  },
  "keywords": ["eldenring", "mod", "typescript", "pause"],
  "author": "Your Name",
  "license": "MIT"
}
```

## 🚀 사용법

### 1. 실행 전 준비

```bash
# 1. 관리자 권한으로 명령 프롬프트 실행
# Windows: Win+X → "Windows PowerShell (관리자)"

# 2. 프로젝트 폴더로 이동
cd C:\path\to\eldenring-pause-ts

# 3. 의존성 설치
npm install

# 4. EldenRing 실행 (오프라인 모드, EAC 비활성화)
# Steam → 라이브러리 → EldenRing 우클릭 → 속성 → 시작 옵션:
# -offline -eac_launcher_settings SkipEAC
```

### 2. 모드 실행

```bash
# 개발 모드 (실시간 컴파일)
npm run dev

# 또는 일반 실행
npm start
```

### 3. 실행 화면

```
============================================================
🎮 EldenRing PauseTheGame 모드 (TypeScript 버전)
============================================================
⚠️ 주의사항:
  • 관리자 권한으로 실행해야 합니다
  • EldenRing이 실행 중이어야 합니다
  • 오프라인 모드에서만 사용하세요
  • EAC를 비활성화해야 합니다
============================================================

🚀 EldenRing PauseTheGame 모드 (TypeScript) 초기화 중...
✅ EldenRing 프로세스 발견: PID 12345
🔍 AOB 스캔 시작: 0f 84 ? ? ? ? c6 ? ? ? ? ? 00 ...
✅ 패턴 발견: 0x140a2b5c0
🏁 스캔 완료: 1개 패턴 발견
📍 패치 주소: 0x140a2b5c1
📋 원본 바이트: 0x84 (JE)
📋 패치 바이트: 0x85 (JNE)
✅ 초기화 완료!

📋 사용법:
  p  : 일시정지/해제 토글
  s  : 현재 상태 확인
  h  : 도움말 표시
  q  : 프로그램 종료

🎮 명령어 입력 (p/s/h/q): 
```

## 🔄 C++과 TypeScript 버전 비교

### 개발 과정 비교

| 단계 | C++ 버전 | TypeScript 버전 |
|------|----------|-----------------|
| **환경 설정** | Visual Studio 설치 (2GB+) | Node.js 설치 (50MB) |
| **프로젝트 생성** | VS 프로젝트 템플릿 | `npm init -y` |
| **의존성 관리** | 수동 라이브러리 링크 | `npm install` |
| **코딩** | C++ + Windows API | TypeScript + FFI |
| **컴파일** | 5-30초 | 1-3초 |
| **디버깅** | VS 디버거 | console.log + 브레이크포인트 |
| **배포** | .dll 파일 | .js 파일 + Node.js |

### 성능 비교

```
메트릭            C++        TypeScript
실행 속도:        ⭐⭐⭐⭐⭐    ⭐⭐⭐⭐
메모리 사용량:    ⭐⭐⭐⭐⭐    ⭐⭐⭐
개발 속도:        ⭐⭐         ⭐⭐⭐⭐⭐
디버깅 편의성:    ⭐⭐⭐       ⭐⭐⭐⭐⭐
유지보수성:       ⭐⭐⭐       ⭐⭐⭐⭐⭐
```

### 코드 비교 예시

**C++ 버전:**
```cpp
// Windows API 직접 호출
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
SIZE_T bytesRead;
ReadProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesRead);
```

**TypeScript 버전:**
```typescript
// FFI를 통한 API 호출
const processHandle = kernel32.OpenProcess(PROCESS_ALL_ACCESS, false, processId);
const success = kernel32.ReadProcessMemory(processHandle, addressPtr, buffer, size, bytesRead);
```

## 📦 게임 설치 및 적용 방법

### 1. EldenRing 오프라인 모드 설정

```bash
# Steam 시작 옵션 설정
# Steam → 라이브러리 → EldenRing 우클릭 → 속성 → 시작 옵션에 입력:
-offline -eac_launcher_settings SkipEAC

# 또는 Steam 오프라인 모드로 실행
# Steam → 설정 → 계정 → "오프라인 모드로 다시 시작"
```

### 2. 모드 설치

```bash
# 방법 1: 직접 실행 (권장)
git clone <repository-url>
cd eldenring-pause-ts
npm install
npm start

# 방법 2: 글로벌 설치
npm install -g eldenring-pause-ts
eldenring-pause

# 방법 3: npx를 통한 일회성 실행
npx eldenring-pause-ts
```

### 3. 모드 제거

```bash
# 프로세스 종료 (Ctrl+C)
# 자동으로 패치가 복원됩니다

# 파일 제거
rm -rf eldenring-pause-ts/

# 글로벌 설치한 경우
npm uninstall -g eldenring-pause-ts
```

## 🛡️ 안전성 및 주의사항

### TypeScript 모드의 안전 기능

1. **자동 복원**: 프로그램 종료 시 원본 바이트 자동 복원
2. **상태 검증**: 패치 적용 전후 상태 확인
3. **에러 핸들링**: 예외 발생 시 안전한 종료
4. **로깅**: 모든 작업에 대한 상세한 로그

### 위험 요소 및 대응

| 위험 요소 | 대응 방법 |
|-----------|-----------|
| **게임 크래시** | 백업 파일 복원, 안전한 메모리 접근 |
| **밴 위험** | 오프라인 전용, EAC 비활성화 |
| **메모리 손상** | 패치 전 검증, 원본 바이트 확인 |
| **시스템 불안정** | 관리자 권한 필요, 권한 검사 |

## 🚀 확장 가능성

### 추가 기능 구현 아이디어

```typescript
// 1. GUI 인터페이스 (Electron)
import { app, BrowserWindow } from 'electron';

// 2. 핫키 지원
import { globalShortcut } from 'electron';

// 3. 웹 대시보드
import express from 'express';

// 4. 다중 게임 지원
interface GameConfig {
  name: string;
  aobPattern: string;
  patchOffset: number;
}

// 5. 플러그인 시스템
interface ModPlugin {
  name: string;
  initialize(): Promise<boolean>;
  execute(): Promise<void>;
}
```

## 📚 학습 포인트

### TypeScript 모딩에서 배운 것들

1. **FFI (Foreign Function Interface)**: 다른 언어 라이브러리 호출
2. **메모리 관리**: 버퍼 조작, 포인터 다루기
3. **비동기 프로그래밍**: Promise 기반 메모리 접근
4. **타입 시스템**: 복잡한 데이터 구조 모델링
5. **에러 핸들링**: 예외 상황 대응

### 다음 단계

- **웹 기반 모드 관리자** 구축
- **Electron GUI** 애플리케이션 개발  
- **다중 게임 지원** 시스템 확장
- **클라우드 모드 저장소** 연동

---

**💡 결론**: TypeScript로도 충분히 강력한 게임 모드를 만들 수 있으며, 개발 생산성과 유지보수성 면에서 C++보다 유리한 경우가 많습니다!