## C++ Example: Memory Scan and Patch

이 폴더에는 C++과 Windows API를 사용하여 메모리를 스캔하고 패치하는 예제 코드가 포함되어 있습니다.

### 준비물:

1.  **Visual Studio** (또는 MinGW 등 C++ 컴파일러) 설치
    *   Windows SDK가 포함되어 있어야 합니다.

### 예제 코드 (`main.cpp`):

`main.cpp` 파일은 `notepad.exe` 프로세스를 대상으로 메모리 스캔 및 패치를 시도합니다. 이 예제는 특정 문자열을 찾아 다른 문자열로 변경하는 방법을 보여줍니다.

**주의:** `notepad.exe`의 특정 문자열 주소는 실행 환경에 따라 달라질 수 있습니다. 이 예제는 개념 증명용이며, 실제 사용 시에는 정확한 주소를 찾아야 합니다.

### 컴파일 및 실행 방법:

1.  `notepad.exe`를 실행합니다.
2.  Visual Studio 개발자 명령 프롬프트 또는 PowerShell에서 다음 명령어를 사용하여 컴파일합니다:
    ```bash
    cl main.cpp /EHsc
    ```
    또는 Visual Studio 프로젝트를 생성하여 빌드합니다.
3.  컴파일 후 생성된 실행 파일 (`main.exe`)을 실행합니다:
    ```bash
    .\main.exe
    ```

코드가 성공적으로 실행되면 `notepad.exe`의 특정 메모리 영역에 있는 문자열이 변경될 것입니다.