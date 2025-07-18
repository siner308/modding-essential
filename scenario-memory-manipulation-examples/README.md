# Memory Manipulation Examples (Python & TypeScript)

이 과정는 Python과 TypeScript(Node.js)를 사용하여 다른 프로세스의 메모리를 스캔하고 패치하는 기본적인 방법을 보여줍니다.

**중요 사항:**

*   이 예제들은 **Windows 운영체제**를 기준으로 작성되었습니다. macOS나 Linux에서는 메모리 접근 방식이 다르거나 보안 제약이 더 많을 수 있습니다.
*   예제 코드는 `notepad.exe` 프로세스를 대상으로 합니다. 실제 애플리케이션에 적용하려면 대상 프로세스 이름과 메모리 주소를 변경해야 합니다.
*   메모리 주소는 프로그램이 실행될 때마다 달라질 수 있는 **동적 주소**일 가능성이 높습니다. 정확한 메모리 주소를 찾기 위해서는 별도의 메모리 스캐너(예: Cheat Engine)를 사용해야 합니다.
*   메모리 주소를 찾는 방법에 대한 자세한 내용은 `getting-started/memory-scanning-guide.md`를 참조하십시오.
*   이러한 방식의 메모리 조작은 대부분의 안티-치트(Anti-Cheat) 시스템에 의해 쉽게 탐지될 수 있습니다.

## 포함된 예제:

*   `python-example/`: Python의 `pymem` 라이브러리를 사용한 메모리 조작 예제.
*   `typescript-example/`: TypeScript(Node.js)의 `memory-js` 라이브러리를 사용한 메모리 조작 예제.