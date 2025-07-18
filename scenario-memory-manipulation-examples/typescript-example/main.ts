import * as memory from 'memory-js';

const PROCESS_NAME = "notepad.exe";
// 이 주소는 예시이며, 실제 notepad.exe의 메모리 주소는 실행할 때마다 달라질 수 있습니다.
// 정확한 주소를 찾으려면 Cheat Engine과 같은 메모리 스캐너를 사용해야 합니다.
// 예를 들어, notepad.exe의 특정 문자열이나 값을 찾아서 그 주소를 여기에 입력해야 합니다.
// 여기서는 단순히 0x00400000을 예시로 사용합니다.
const HYPOTHETICAL_ADDRESS = 0x00400000; 
const VALUE_TO_WRITE = "MODDED!"; // 문자열

function main() {
    try {
        // 프로세스 찾기 및 열기
        const processObject = memory.openProcess(PROCESS_NAME);
        console.log(`'${PROCESS_NAME}' 프로세스를 찾았습니다! PID: ${processObject.th32ProcessID}`);

        // 메모리 읽기 예제
        try {
            // HYPOTHETICAL_ADDRESS에서 문자열을 읽습니다. 길이는 VALUE_TO_WRITE의 길이로 가정합니다.
            // memory-js는 직접 문자열 길이를 지정하기 어렵기 때문에, 바이트 배열로 읽고 디코딩합니다.
            // 실제 사용 시에는 읽을 데이터의 정확한 크기를 알아야 합니다.
            const buffer = memory.readMemory(processObject.handle, HYPOTHETICAL_ADDRESS, memory.TYPES.LPCBYTE, VALUE_TO_WRITE.length);
            const currentString = Buffer.from(buffer as number[]).toString('utf8').replace(/\0/g, ''); // null 문자 제거
            console.log(`주소 ${HYPOTHETICAL_ADDRESS.toString(16)}에서 읽은 값: '${currentString}'`);
        } catch (e: any) {
            console.error(`주소 ${HYPOTHETICAL_ADDRESS.toString(16)}에서 읽기 실패: ${e.message}. 이 주소는 유효하지 않을 수 있습니다.`);
        }

        // 메모리 쓰기 예제
        console.log(`주소 ${HYPOTHETICAL_ADDRESS.toString(16)}에 '${VALUE_TO_WRITE}' 쓰기 시도...`);
        try {
            // 문자열을 바이트 배열로 변환하여 씁니다.
            // memory-js는 문자열을 직접 쓰는 함수가 없으므로, 바이트 배열로 변환하여 씁니다.
            const bytesToWrite = Buffer.from(VALUE_TO_WRITE + '\0'); // null-terminated string
            memory.writeMemory(processObject.handle, HYPOTHETICAL_ADDRESS, bytesToWrite, memory.TYPES.LPCBYTE, bytesToWrite.length);
            console.log(`주소 ${HYPOTHETICAL_ADDRESS.toString(16)}에 성공적으로 썼습니다.`);

            // 쓴 후에 다시 읽어서 확인
            const verifiedBuffer = memory.readMemory(processObject.handle, HYPOTHETICAL_ADDRESS, memory.TYPES.LPCBYTE, bytesToWrite.length);
            const verifiedString = Buffer.from(verifiedBuffer as number[]).toString('utf8').replace(/\0/g, '');
            console.log(`쓰기 후 확인된 값: '${verifiedString}'`);

        } catch (e: any) {
            console.error(`주소 ${HYPOTHETICAL_ADDRESS.toString(16)}에 쓰기 실패: ${e.message}. 권한 문제 또는 유효하지 않은 주소일 수 있습니다.`);
        }

        // 프로세스 핸들 닫기
        memory.closeProcess(processObject.handle);

    } catch (e: any) {
        if (e.message.includes("Could not find process")) {
            console.error(`'${PROCESS_NAME}' 프로세스를 찾을 수 없습니다. '${PROCESS_NAME}'이(가) 실행 중인지 확인하세요.`);
        } else {
            console.error(`예상치 못한 오류 발생: ${e.message}`);
        }
    }
}

main();
