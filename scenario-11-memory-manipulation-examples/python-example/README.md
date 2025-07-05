## Python Example: Memory Scan and Patch

이 폴더에는 Python의 `pymem` 라이브러리를 사용하여 메모리를 스캔하고 패치하는 예제 코드가 포함되어 있습니다.

### 준비물:

1.  **Python 3.x** 설치
2.  `pymem` 라이브러리 설치:
    ```bash
    pip install pymem
    ```

### 예제 코드 (`main.py`):

`main.py` 파일은 `notepad.exe` 프로세스를 대상으로 메모리 스캔 및 패치를 시도합니다. 이 예제는 특정 문자열을 찾아 다른 문자열로 변경하는 방법을 보여줍니다.

**주의:** `notepad.exe`의 특정 문자열 주소는 실행 환경에 따라 달라질 수 있습니다. 이 예제는 개념 증명용이며, 실제 사용 시에는 정확한 주소를 찾아야 합니다.

### 실행 방법:

1.  `notepad.exe`를 실행합니다.
2.  `main.py` 파일을 실행합니다:
    ```bash
    python main.py
    ```

코드가 성공적으로 실행되면 `notepad.exe`의 특정 메모리 영역에 있는 문자열이 변경될 것입니다.