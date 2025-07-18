# 🟢 FromSoftware 게임 모딩

**난이도**: 초급-중급 | **학습 시간**: 2-3주 | **접근법**: 커뮤니티 도구 기반

## 🎯 학습 목표

FromSoftware 게임들(Dark Souls, Elden Ring, Sekiro 등)의 독특한 모딩 생태계를 이해하고, 전문 커뮤니티 도구들을 활용한 모딩 기법을 익힙니다.

### 핵심 학습 포인트
- ✅ Dantelion/Katana 엔진 구조 이해
- ✅ DSMapStudio를 통한 맵/파라미터 편집
- ✅ UXM + Mod Engine 2 워크플로우
- ✅ FromSoft 특화 파일 포맷 이해
- ✅ SoulsModding 커뮤니티 리소스 활용

## 📚 이론 학습: FromSoft 엔진 아키텍처

### Dantelion 엔진 (메인 엔진)
```
Dantelion 엔진 게임들:
├── Demon's Souls (PS3) - 초기 버전
├── Dark Souls (2011) - 정립
├── Bloodborne (2015) - PS4 최적화
├── Dark Souls 3 (2016) - 현대화
├── Sekiro (2019) - 액션 특화
└── Elden Ring (2022) - 오픈월드 확장
```

**핵심 특징**:
- 🏗️ **완전 자체 개발**: 문서화되지 않은 독점 엔진
- 🔧 **미들웨어 활용**: Havok(물리), FMOD(사운드), Bink(동영상)
- 📈 **점진적 진화**: 각 게임이 이전 버전을 기반으로 발전
- 🔐 **역공학 의존**: 모든 모딩 도구가 커뮤니티 제작

### Katana 엔진 (Dark Souls 2 전용)
```
Dark Souls 2의 특별한 경우:
- 메인 Dantelion 브랜치에서 분기
- Armored Core 계열과의 혼합 추정
- dantelion2 문자열도 발견됨
- 독특한 구조로 별도 도구 필요
```

## 🛠️ 필수 모딩 도구

### 1. **Smithbox** ⭐ (최신 권장)
```bash
기능: 멀티게임 GUI 도구
지원: DS1/2/3, BB, Sekiro, Elden Ring, AC6
용도:
- 맵 편집 (적/아이템 배치)
- 파라미터 수정 (스탯/밸런스)
- 텍스트 편집 (대사/설명)
```

### 2. **DSMapStudio** (레거시 but 강력)
```bash
기능: 맵/레벨 에디터
요구사항: Vulkan 1.3 지원 GPU
특징:
- 모드 프로젝트 시스템
- 원본 파일 보호
- 실시간 3D 편집
```

### 3. **WitchyBND** 
```bash
기능: 파일 압축/해제 도구
대체: Yabber (구버전, 사용 금지)
용도:
- .bnd/.bdt 아카이브 해제
- 암호화된 파일 접근
- 파일 리패키징
```

### 4. **Mod Engine 2**
```bash
기능: 모드 로더/관리자
특징:
- 원본 파일 수정 없음
- modoverridedirectory 시스템
- 다중 모드 지원
```

## 🔍 실습 과정

### 1단계: 환경 설정

#### **게임 준비** (Elden Ring 예시)
```bash
Elden Ring/
├── Game/
│   ├── eldenring.exe
│   └── Data0.bdt              # 메인 데이터 아카이브
├── ModEngine/                 # Mod Engine 2 설치
│   ├── modengine2_launcher.exe
│   └── config_eldenring.toml
└── Mods/                      # 모드 파일들
    ├── regulation.bin         # 파라미터 파일
    ├── menu/                  # UI 수정
    └── map/                   # 맵 데이터
```

#### **도구 설치**
```bash
1. Smithbox 다운로드 및 압축 해제
2. Mod Engine 2 설치
3. WitchyBND 설치 (필요시)
4. 게임 백업 생성 (중요!)
```

### 2단계: 첫 모드 만들기 - 적 체력 수정

#### **Smithbox로 파라미터 편집**
```bash
1. Smithbox 실행
2. 게임 선택 (Elden Ring)
3. Param Editor 탭 열기
4. NpcParam 찾기
5. 원하는 적 찾아서 HP 수정
6. Save 후 regulation.bin 생성
```

#### **실제 편집 예시**
```
NpcParam → ID: 10010100 (기본 병사)
├── HP: 100 → 500 (5배 증가)
├── Stamina: 50 → 200 (4배 증가)
└── Soul Drop: 25 → 100 (4배 증가)
```

### 3단계: 맵 편집 - 적 배치 변경

#### **Map Editor 사용법**
```bash
1. Smithbox → Map Editor 탭
2. 맵 선택 (예: m60_00_00_00 - 림그레이브)
3. Entity Groups 확인
4. Enemy 선택하여 다른 적으로 교체
5. 위치/수량 조정
6. Save
```

#### **고급 맵 편집**
```bash
새로운 적 추가:
1. 빈 엔티티 슬롯 찾기
2. Entity ID 설정
3. 위치 좌표 입력 (X, Y, Z)
4. 회전값 설정
5. Think Parameter 연결
```

### 4단계: 모드 테스트

#### **Mod Engine 2 설정**
```toml
# config_eldenring.toml
[modengine]
debug = false
external_dlls = []

[extension.mod_loader]
enabled = true
loose_params = true
mod_dir = "\\Mods"
```

#### **테스트 실행**
```bash
1. modengine2_launcher.exe 실행
2. 게임 시작
3. 수정된 내용 확인
4. 문제 시 백업으로 복구
```

## 📋 실전 실습 과제

### 과제 1: 기본 밸런스 조정 (Smithbox - Param Editor)
- [ ] **CharaInitParam**: 플레이어 초기 스탯 변경
- [ ] **WeaponParam**: 무기 데미지/스케일링 수정
- [ ] **ItemLotParam**: 아이템 드롭률 조정
- [ ] **SpEffectParam**: 스펠/스킬 효과 변경

### 과제 2: 맵 커스터마이징 (Smithbox - Map Editor)
- [ ] **Enemy Placement**: 특정 구역의 적 배치 변경
- [ ] **Item Lot**: 보물상자/아이템 위치 수정
- [ ] **Asset Placement**: 새로운 오브젝트 배치
- [ ] **Collision**: 숨겨진 통로/벽 생성

### 과제 3: 이벤트 스크립팅 (EMEVD)
- [ ] **RegisterLadder**: 새로운 사다리 상호작용
- [ ] **RegisterBonfire**: 커스텀 모닥불 생성
- [ ] **TreasureBox**: 보물상자 아이템 설정
- [ ] **TalkScript**: NPC 대화 이벤트

### 과제 4: 모브셋/애니메이션 (고급)
- [ ] **Character Files**: .flver 모델 교체
- [ ] **Animation**: .hkx 애니메이션 수정
- [ ] **Sound Integration**: 사운드 효과 연동
- [ ] **Havok Behavior**: 물리 시뮬레이션 조정

### 과제 5: UI/사운드 커스터마이징
- [ ] **Menu Folder**: HUD 요소 및 아이콘 수정
- [ ] **Msg Folder**: 텍스트/대사 번역/수정
- [ ] **Sound Folder**: BGM/효과음 교체 (DSSI 활용)
- [ ] **Language Files**: 다국어 지원 추가

## 🔧 고급 기법 (SoulsModding Wiki 기반)

### 1. EMEVD 이벤트 스크립팅

#### **기본 구조 이해**
```python
# EMEVD 이벤트 예제 (Python-like pseudocode)
def Event_12345():
    """커스텀 보물상자 이벤트"""
    # 플레이어가 상자 근처에 있는지 확인
    if player_near_entity(1234567):
        # 상자 열기 애니메이션
        play_animation(1234567, "chest_open")
        # 아이템 지급
        give_item_lot(12340)
        # 이벤트 종료
        end_event()
```

#### **실전 EMEVD 편집**
```bash
# Elden Ring EMEVD 워크플로우
1. Smithbox → Event Editor 열기
2. 맵별 EMEVD 파일 선택 (m60_00_00_00.emevd)
3. 새 이벤트 ID 할당 (10000000~19999999)
4. 이벤트 로직 작성:
   - RegisterLadder(ladder_id, start_pos, end_pos)
   - RegisterBonfire(bonfire_id, map_area)
   - TreasureBox(chest_id, item_lot_id, flag_id)
```

### 2. 맵핑 미니 튜토리얼 모음

#### **새로운 사다리 생성**
```bash
1. Asset 배치: 사다리 모델을 맵에 배치
2. EntityID 할당: 고유 ID 부여 (예: 1234567)
3. EMEVD 이벤트: RegisterLadder 이벤트 추가
   RegisterLadder(1234567, 
                  start_position={X:100, Y:0, Z:200},
                  end_position={X:100, Y:50, Z:200})
```

#### **보물상자 커스터마이징**
```bash
1. TreasureBox Entity 생성
2. ItemLotParam 설정:
   - 아이템 ID 지정
   - 드롭 확률 설정
   - 수량 지정
3. EMEVD 연동: TreasureBox 이벤트 등록
```

#### **엘리베이터 설치**
```bash
1. Platform Asset 배치
2. Collision Box 설정
3. Movement Path 정의
4. Activation Trigger 생성
```

### 3. 모브셋 모딩 (Muffin's Knowledge Compendium)

#### **새로운 모브셋 추가 과정**
```bash
# 캐릭터 파일 구조
chr/c1234.chrbnd.dcx
├── c1234.flver        # 3D 모델
├── c1234.hkx          # 애니메이션
├── c1234_l.tpf        # 저해상도 텍스처
└── c1234_h.tpf        # 고해상도 텍스처

# 모브셋 복제 과정
1. 기존 적의 chr 파일 복사
2. 새로운 번호로 리네임 (c5678)
3. 모델/텍스처 수정 (옵션)
4. NpcParam에서 새 적 등록
5. 맵에서 EntityID 할당
```

#### **애니메이션 시스템**
```bash
# Havok Behavior 편집
1. .hkx 추출 (WitchyBND)
2. hkxconv로 TagXML 변환
3. 애니메이션 데이터 수정
4. 다시 .hkx로 컴파일
5. chr 파일에 리패킹
```

### 4. 사운드 시스템 (DSSI 활용)

#### **배경음악 교체**
```bash
# Dark Souls Sound Inserter (DSSI) 워크플로우
1. sound/*.fsb 파일 추출
2. DSSI로 .fsb 열기
3. 원하는 트랙을 .wav로 교체
4. 샘플레이트/포맷 맞추기
5. .fsb 재생성 및 리패킹
```

#### **효과음 커스터마이징**
```bash
# FMOD Designer 활용 (고급)
1. .fev/.fsb 파일 분석
2. FMOD Designer에서 이벤트 편집
3. 새로운 사운드 에셋 임포트
4. 게임 내 사운드 트리거 연동
```

### 5. UI/UX 모딩

#### **HUD 요소 수정**
```bash
# menu/ 폴더 구조
menu/
├── 00_solo/           # 메인 HUD
├── 01_common/         # 공통 UI
├── 02_system/         # 시스템 메뉴
└── 80_achievements/   # 업적 UI

# 텍스처 교체 과정
1. .tpf 파일 추출 (WitchyBND)
2. DDS 텍스처 수정
3. 새 이미지를 DDS로 변환
4. .tpf 재패킹
```

#### **다국어 지원**
```bash
# msg/ 폴더 활용
msg/
├── engus/            # 영어
├── jpnjp/            # 일본어
├── frafr/            # 프랑스어
└── custom/           # 커스텀 언어

# 텍스트 수정 과정
1. .msgbnd 파일 추출
2. Yabber로 .fmg 파일 추출
3. 텍스트 편집 (UTF-8)
4. 다시 패킹하여 적용
```

### 6. 고급 파일 포맷 작업

#### **Regulation.bin 고급 병합**
```bash
# Smithbox Param Merger 활용
1. Base Regulation 로드
2. Mod Regulations 순차 로드
3. Conflict Resolution:
   - Override: 나중 모드가 덮어씀
   - Merge: 가능한 경우 병합
   - Skip: 충돌 시 건너뜀
4. Final Regulation 생성
```

#### **BND/BDT 아카이브 관리**
```bash
# WitchyBND 고급 활용
1. Selective Extraction: 필요한 파일만 추출
2. Compression Options: 압축 레벨 조정
3. Encryption Handling: DCX 압축 관리
4. Batch Processing: 다중 파일 일괄 처리
```

## 🎯 실전 워크샵: 단계별 가이드

### Workshop 1: 첫 번째 파라미터 모드 만들기 (초급)

#### **목표**: 기본 적의 체력을 2배로 늘리기

```bash
# 준비물
- Elden Ring (업데이트 최신)
- Smithbox 최신 버전
- Mod Engine 2

# 단계별 과정
1. 게임 백업 생성
2. Smithbox 실행 → Elden Ring 선택
3. Param Editor → NpcParam 열기
4. ID 10010100 (기본 병사) 찾기
5. HP 값: 100 → 200 변경
6. Save → regulation.bin 생성
7. Mod Engine 2로 테스트
```

#### **예상 소요시간**: 30분
#### **학습 포인트**: 파라미터 편집 기초, Mod Engine 사용법

### Workshop 2: 커스텀 보물상자 생성하기 (중급)

#### **목표**: 특정 위치에 레어 아이템이 든 보물상자 추가

```bash
# 과정
1. Map Editor에서 림그레이브(m60_00_00_00) 열기
2. 빈 Entity 슬롯 찾기 (ID: 1234567)
3. Model ID를 보물상자로 설정
4. 위치 좌표 입력 (게이트프론트 근처)
5. ItemLotParam에서 아이템 설정
6. EMEVD에서 TreasureBox 이벤트 등록
7. 테스트 및 디버깅
```

#### **예상 소요시간**: 1-2시간  
#### **학습 포인트**: 맵 편집, 이벤트 스크립팅, 좌표 시스템

### Workshop 3: BGM 교체 프로젝트 (중급-고급)

#### **목표**: 특정 지역의 배경음악을 커스텀 트랙으로 교체

```bash
# 과정
1. WitchyBND로 sound 폴더 추출
2. 해당 지역의 .fsb 파일 식별
3. DSSI로 .fsb 파일 열기
4. 원본 트랙 분석 (샘플레이트, 길이 등)
5. 새 음악을 같은 포맷으로 변환
6. DSSI로 트랙 교체
7. .fsb 재생성 및 패킹
8. 게임에서 테스트
```

#### **예상 소요시간**: 2-3시간
#### **학습 포인트**: 사운드 편집, 파일 포맷 이해, DSSI 활용

### Workshop 4: 새로운 적 추가하기 (고급)

#### **목표**: 기존 적을 복사하여 새로운 적 생성

```bash
# 과정
1. 기존 적의 chr 파일 식별 (c1234.chrbnd.dcx)
2. WitchyBND로 파일 추출 및 복사
3. 새 ID로 리네임 (c5678)
4. 모델/텍스처 수정 (선택사항)
5. NpcParam에서 새 적 등록:
   - HP, 공격력, AI 설정
   - 드롭 아이템 설정
6. ThinkParam에서 AI 행동 패턴 설정
7. 맵에서 스폰 위치 지정
8. 테스트 및 밸런싱
```

#### **예상 소요시간**: 4-6시간
#### **학습 포인트**: 파일 구조 이해, AI 시스템, 종합적 모딩

## 🔬 고급 주제: 역공학 이해하기

### FromSoft 파일 포맷 분석

#### **BND/BDT 아카이브 구조**
```
.bnd/.bdt 파일 = "바인더" 아카이브
├── Header: 파일 정보
├── File Entries: 포함된 파일 목록  
├── File Names: 파일명 테이블
└── File Data: 실제 파일 데이터

DCX = 압축된 BND (LZ4/ZLIB)
```

#### **EMEVD 바이트코드 구조**
```python
# EMEVD 파일 분석 (의사코드)
class EMEVDFile:
    def __init__(self):
        self.header = Header()
        self.string_table = []
        self.events = []
        self.instructions = []
    
    def parse_instruction(self, opcode):
        # 명령어별 파라미터 파싱
        if opcode == 0x2000403:  # IF 조건문
            return IfStatement(params)
        elif opcode == 0x1014000: # EndEvent
            return EndEvent()
```

### 커뮤니티 도구 개발 과정

#### **도구 개발 흐름**
```bash
1. 리버스 엔지니어링
   - 게임 파일 구조 분석
   - 바이너리 패턴 식별
   - 포맷 스펙 추출

2. 파서 개발
   - C#/Python으로 파일 리더 작성
   - 구조체 정의
   - 검증 로직 구현

3. GUI 도구 제작
   - WPF/WinForms 인터페이스
   - 데이터 편집 기능
   - 저장/로드 시스템

4. 커뮤니티 피드백
   - 베타 테스트
   - 버그 수정
   - 기능 개선
```

## 🌐 커뮤니티 리소스 및 발전 방향

### SoulsModding Wiki 활용
```bash
주요 섹션:
├── Tools Database - 도구 목록 및 설명
├── Tutorials - 단계별 가이드
├── File Formats - 포맷 문서화
├── References - 레퍼런스 자료
└── Community - 디스코드 링크
```

### 추천 학습 자료
- 📚 **SoulsModding Discord**: 실시간 도움말
- 📖 **Nexus Mods**: 기존 모드 분석
- 🛠️ **GitHub SoulsMods**: 오픈소스 도구들
- 📺 **YouTube 튜토리얼**: 비주얼 가이드

## 🎓 학습 정리

### FromSoft 모딩의 독특함
1. **커뮤니티 주도**: 모든 도구가 팬 제작
2. **역공학 기반**: 문서화되지 않은 포맷 해독
3. **전문화된 도구**: 게임별 특화 솔루션
4. **강력한 수정 범위**: 거의 모든 게임 요소 변경 가능

### 다른 모딩과의 차이점
- 🔄 **학습 곡선**: 초기에 가파름, 나중에 매우 강력
- 🛡️ **안전성**: 원본 보호 시스템 잘 구축
- 🌟 **품질**: 상용 도구 수준의 모딩 환경
- 🤝 **커뮤니티**: 매우 활발하고 전문적

## 🔗 관련 자료

- [SoulsModding Wiki](http://soulsmodding.wikidot.com/) - 공식 커뮤니티 위키
- [Smithbox](https://github.com/vawser/Smithbox) - 최신 모딩 도구
- [Mod Engine 2](https://github.com/soulsmods/ModEngine2) - 모드 로더
- [SoulsMods Discord](https://discord.gg/mT2JJjx) - 커뮤니티

## 💡 트러블슈팅

### 자주 발생하는 문제

**Q: 게임이 크래시돼요**
A: regulation.bin 문제일 가능성. 백업으로 복구 후 단계별 적용

**Q: 모드가 적용되지 않아요**
A: Mod Engine 2 설정 확인, 파일 경로 점검

**Q: 온라인에서 밴당했어요**
A: FromSoft 게임은 EAC 적용, 모드 사용 시 오프라인만 권장

**Q: Smithbox가 실행되지 않아요**
A: .NET 런타임 설치, GPU 드라이버 업데이트 확인

---

**다음 학습**: [PAK 로더 구현](../scenario-pak-loader/) | **이전**: [PAK 모딩 체험](../scenario-pak-modding/)