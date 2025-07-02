# x86-64 어셈블리 퀵 레퍼런스

게임 모딩에 자주 사용되는 x86-64 어셈블리 명령어들의 빠른 참조 가이드입니다.

## 📋 레지스터

### 범용 레지스터
| 64비트 | 32비트 | 16비트 | 8비트(하위) | 용도 |
|--------|--------|--------|-------------|------|
| RAX    | EAX    | AX     | AL          | 누산기, 반환값 |
| RBX    | EBX    | BX     | BL          | 베이스 레지스터 |
| RCX    | ECX    | CX     | CL          | 카운터, 첫 번째 인수 |
| RDX    | EDX    | DX     | DL          | 데이터, 두 번째 인수 |
| RSI    | ESI    | SI     | SIL         | 소스 인덱스 |
| RDI    | EDI    | DI     | DIL         | 목적지 인덱스 |
| RSP    | ESP    | SP     | SPL         | 스택 포인터 |
| RBP    | EBP    | BP     | BPL         | 베이스 포인터 |
| R8-R15 | R8D-R15D | R8W-R15W | R8B-R15B | 추가 레지스터 |

### 플래그 레지스터 (RFLAGS)
| 플래그 | 의미 | 설정 조건 |
|--------|------|----------|
| ZF | Zero Flag | 결과가 0일 때 |
| CF | Carry Flag | 자리올림/빌림 발생 |
| SF | Sign Flag | 결과가 음수일 때 |
| OF | Overflow Flag | 부호 있는 오버플로우 |
| PF | Parity Flag | 결과의 짝수 비트 |

## 🔧 데이터 이동 명령어

### 기본 이동
```assembly
mov rax, rbx        ; rax = rbx (값 복사)
mov rax, 100        ; rax = 100 (즉시값)
mov [rax], rbx      ; *rax = rbx (메모리에 저장)
mov rax, [rbx]      ; rax = *rbx (메모리에서 로드)
mov rax, [rbx+8]    ; rax = *(rbx + 8) (오프셋 접근)
```

### 주소 계산
```assembly
lea rax, [rbx+8]         ; rax = rbx + 8 (주소 계산)
lea rax, [rbx+rcx*2+10]  ; rax = rbx + rcx*2 + 10
```

### 값 교환
```assembly
xchg rax, rbx       ; rax와 rbx 값 교환
```

## ➕ 산술 연산

### 기본 연산
```assembly
add rax, rbx        ; rax += rbx
sub rax, rbx        ; rax -= rbx
inc rax             ; rax++
dec rax             ; rax--
neg rax             ; rax = -rax
```

### 곱셈/나눗셈
```assembly
mul rbx             ; rax = rax * rbx (부호 없음)
imul rbx            ; rax = rax * rbx (부호 있음)
imul rax, rbx, 10   ; rax = rbx * 10
div rbx             ; rax = rax / rbx, rdx = rax % rbx
```

### 시프트 연산
```assembly
shl rax, 2          ; rax <<= 2 (왼쪽으로 2비트)
shr rax, 2          ; rax >>= 2 (오른쪽으로 2비트, 부호 없음)
sar rax, 2          ; rax >>= 2 (오른쪽으로 2비트, 부호 있음)
```

## 🔀 논리 연산

### 비트 연산
```assembly
and rax, rbx        ; rax &= rbx
or rax, rbx         ; rax |= rbx
xor rax, rbx        ; rax ^= rbx
not rax             ; rax = ~rax
```

### 특수 XOR 패턴
```assembly
xor rax, rax        ; rax = 0 (자기 자신과 XOR)
xor eax, eax        ; rax = 0 (32비트도 64비트 클리어)
```

### 비트 테스트
```assembly
test rax, rax       ; rax가 0인지 테스트 (rax & rax)
test rax, 1         ; rax의 최하위 비트 테스트
bt rax, 5           ; rax의 5번째 비트 테스트
```

## 🔍 비교 및 조건부 점프

### 비교 명령어
```assembly
cmp rax, rbx        ; rax와 rbx 비교 (rax - rbx)
cmp rax, 0          ; rax와 0 비교
cmp byte ptr [rax], 1  ; *rax와 1 비교
```

### 무조건 점프
```assembly
jmp label           ; 항상 label로 점프
jmp rax             ; rax가 가리키는 주소로 점프
```

### 조건부 점프 (등호 기반)
```assembly
je label            ; Jump if Equal (ZF=1)
jne label           ; Jump if Not Equal (ZF=0)
jz label            ; Jump if Zero (ZF=1, je와 동일)
jnz label           ; Jump if Not Zero (ZF=0, jne와 동일)
```

### 조건부 점프 (부호 있는 비교)
```assembly
jg label            ; Jump if Greater (SF=OF, ZF=0)
jge label           ; Jump if Greater or Equal (SF=OF)
jl label            ; Jump if Less (SF≠OF)
jle label           ; Jump if Less or Equal (SF≠OF or ZF=1)
```

### 조건부 점프 (부호 없는 비교)
```assembly
ja label            ; Jump if Above (CF=0, ZF=0)
jae label           ; Jump if Above or Equal (CF=0)
jb label            ; Jump if Below (CF=1)
jbe label           ; Jump if Below or Equal (CF=1 or ZF=1)
```

### 플래그 기반 점프
```assembly
jc label            ; Jump if Carry (CF=1)
jnc label           ; Jump if Not Carry (CF=0)
jo label            ; Jump if Overflow (OF=1)
jno label           ; Jump if Not Overflow (OF=0)
js label            ; Jump if Sign (SF=1)
jns label           ; Jump if Not Sign (SF=0)
```

## 📚 스택 조작

### 기본 스택 연산
```assembly
push rax            ; 스택에 rax 저장 (rsp -= 8)
pop rbx             ; 스택에서 rbx로 로드 (rsp += 8)
```

### 다중 레지스터 저장
```assembly
pushad              ; 모든 범용 레지스터 저장 (32비트)
pushfq              ; 플래그 레지스터 저장 (64비트)
popfq               ; 플래그 레지스터 복원
```

## 🔧 함수 호출

### 기본 호출
```assembly
call function       ; function 호출 (return address를 스택에 저장)
ret                 ; 함수 복귀 (스택에서 return address 로드)
ret 16              ; 함수 복귀 후 스택에서 16바이트 제거
```

### Windows x64 호출 규약
```assembly
; 처음 4개 정수 인수: RCX, RDX, R8, R9
; 부동소수점 인수: XMM0, XMM1, XMM2, XMM3
; 5번째 이후 인수는 스택에

mov rcx, arg1       ; 첫 번째 인수
mov rdx, arg2       ; 두 번째 인수
mov r8, arg3        ; 세 번째 인수
mov r9, arg4        ; 네 번째 인수
push arg5           ; 다섯 번째 인수 (스택)
call function
add rsp, 8          ; 스택 정리 (caller clean-up)
```

## 🎮 게임 모딩 특화 패턴

### 메모리 패치용 NOP
```assembly
nop                 ; 90 (No Operation, 1바이트)
; 여러 NOP으로 긴 명령어 무력화
nop                 ; 90
nop                 ; 90  
nop                 ; 90
nop                 ; 90
nop                 ; 90
```

### 자주 보는 게임 패턴
```assembly
; 패턴 1: 게임 상태 체크
cmp byte ptr [rax+8], 0     ; 상태 확인
je skip_update              ; 일시정지면 건너뛰기
call update_function        ; 게임 업데이트

; 패턴 2: HP/MP 체크
cmp dword ptr [player+0x10], 0  ; HP 확인  
jle death_handler               ; 0 이하면 사망 처리

; 패턴 3: 아이템 개수 제한
cmp eax, 999                    ; 최대 개수 확인
jge max_reached                 ; 999개 도달시 처리
inc eax                         ; 개수 증가
```

### SIMD 명령어 (그래픽 관련)
```assembly
; XMM 레지스터 조작 (128비트 SIMD)
movups xmm0, [rax]         ; 16바이트 로드 (정렬 안된)
movaps xmm0, [rax]         ; 16바이트 로드 (16바이트 정렬)
addps xmm0, xmm1           ; 4개 float 동시 덧셈
mulps xmm0, xmm1           ; 4개 float 동시 곱셈
pxor xmm0, xmm0            ; XMM0을 0으로 클리어

; 색상/그래픽 효과 제거용
xorps xmm1, xmm1           ; XMM1을 0으로 (색상 제거)
movups [color_buffer], xmm1 ; 0으로 된 색상 데이터 저장
```

## 📏 메모리 주소 지정

### 기본 형식
```assembly
[base]                     ; 직접 주소
[base + offset]            ; 베이스 + 오프셋
[base + index]             ; 베이스 + 인덱스
[base + index*scale]       ; 베이스 + 인덱스*배율
[base + index*scale + offset] ; 복합 주소 지정
```

### 실제 예제
```assembly
mov rax, [rbx]             ; *rbx
mov rax, [rbx+8]           ; *(rbx + 8)
mov rax, [rbx+rcx]         ; *(rbx + rcx)
mov rax, [rbx+rcx*2]       ; *(rbx + rcx*2)
mov rax, [rbx+rcx*4+16]    ; *(rbx + rcx*4 + 16)
```

### 게임 오브젝트 접근 패턴
```assembly
; 플레이어 구조체 접근
mov rax, [player_ptr]      ; 플레이어 포인터 로드
mov ebx, [rax+0x10]        ; HP (오프셋 0x10)
mov ecx, [rax+0x14]        ; MP (오프셋 0x14)
mov edx, [rax+0x18]        ; 경험치 (오프셋 0x18)

; 인벤토리 배열 접근
mov rax, [inventory_ptr]   ; 인벤토리 시작 주소
mov rbx, 5                 ; 아이템 인덱스
mov rcx, [rax+rbx*8]       ; inventory[5] (각 항목 8바이트)
```

## 🔢 바이트 크기별 명령어

### 데이터 크기 지정자
```assembly
; 8비트 (1바이트)
mov al, 100                ; AL = 100
mov byte ptr [rax], 50     ; *(char*)rax = 50

; 16비트 (2바이트)  
mov ax, 1000               ; AX = 1000
mov word ptr [rax], 500    ; *(short*)rax = 500

; 32비트 (4바이트)
mov eax, 100000            ; EAX = 100000
mov dword ptr [rax], 999   ; *(int*)rax = 999

; 64비트 (8바이트)
mov rax, 0x123456789ABCDEF ; RAX = 큰 수
mov qword ptr [rbx], rax   ; *(long long*)rbx = rax
```

## 🎯 모딩별 주요 명령어

### 무적 모드 구현
```assembly
; 원본: 데미지 적용
sub [player_hp], eax       ; HP -= 데미지
jz death_handler           ; HP가 0이면 사망

; 패치: 데미지 무시
nop                        ; SUB 명령어 무력화
nop
nop  
nop
jmp continue_game          ; 항상 게임 계속
```

### 무한 아이템 구현
```assembly
; 원본: 아이템 감소
dec dword ptr [item_count] ; 아이템 개수 감소
cmp dword ptr [item_count], 0
je no_items

; 패치: 아이템 고정
nop                        ; DEC 명령어 무력화 
nop
nop
nop
jmp use_item              ; 항상 사용 가능
```

### 스피드 핵 구현
```assembly
; 원본: 속도 제한
mov eax, 100              ; 최대 속도 100
cmp [player_speed], eax
jle speed_ok

; 패치: 속도 증가
mov eax, 1000             ; 최대 속도 1000으로 변경
nop                       ; 비교 생략
nop
jmp speed_ok              ; 항상 통과
```

## 📖 16진수 바이트 코드

### 자주 사용되는 명령어의 바이트 코드
```
명령어                    바이트 코드
mov eax, 0               B8 00 00 00 00
mov eax, ebx             89 D8
nop                      90
je (짧은 점프)           74 XX
jne (짧은 점프)          75 XX
jmp (짧은 점프)          EB XX
call (상대 주소)         E8 XX XX XX XX
ret                      C3
push eax                 50
pop eax                  58
```

### AOB 패턴 작성 예제
```
원본 어셈블리:
  mov eax, [rbx+8]       ; 8B 43 08
  cmp eax, 0             ; 83 F8 00  
  je short loc_401234    ; 74 XX

AOB 패턴:
  8B 43 08 83 F8 00 74 ?

와일드카드 사용:
  8B ? ? 83 F8 00 74 ?   (오프셋이 다를 수 있음)
  8B 43 ? 83 F8 ? 74 ?   (더 유연한 매칭)
```

---

**💡 사용 팁**:

1. **패턴 분석 시**: 고정된 부분과 가변적인 부분을 구분
2. **패치 적용 시**: 최소한의 바이트만 수정하여 안정성 확보  
3. **디버깅 시**: 레지스터와 플래그 상태를 주의 깊게 관찰
4. **최적화 시**: 캐시 친화적이고 CPU 파이프라인을 고려한 코드 작성

이 레퍼런스는 실제 게임 모딩 작업에서 빠른 참조용으로 활용하세요!