import pymem
import pymem.process
import time

PROCESS_NAME = "notepad.exe"
# 이 주소는 예시이며, 실제 notepad.exe의 메모리 주소는 실행할 때마다 달라질 수 있습니다.
# 정확한 주소를 찾으려면 Cheat Engine과 같은 메모리 스캐너를 사용해야 합니다.
# 예를 들어, notepad.exe의 특정 문자열이나 값을 찾아서 그 주소를 여기에 입력해야 합니다.
# 여기서는 단순히 0x00400000을 예시로 사용합니다.
HYPOTHETICAL_ADDRESS = 0x00400000 
VALUE_TO_WRITE = b"MODDED!" # 7바이트
LENGTH_TO_READ = len(VALUE_TO_WRITE)

def main():
    try:
        pm = pymem.Pymem(PROCESS_NAME)
        print(f"'{PROCESS_NAME}' 프로세스를 찾았습니다! PID: {pm.process_id}")

        # 메모리 읽기 예제
        try:
            # HYPOTHETICAL_ADDRESS에서 LENGTH_TO_READ 만큼의 바이트를 읽습니다.
            # 이 주소에 유효한 데이터가 없을 경우 오류가 발생할 수 있습니다.
            current_bytes = pm.read_bytes(HYPOTHETICAL_ADDRESS, LENGTH_TO_READ)
            print(f"주소 {hex(HYPOTHETICAL_ADDRESS)}에서 읽은 값: {current_bytes.decode(errors='ignore')}")
        except Exception as e:
            print(f"주소 {hex(HYPOTHETICAL_ADDRESS)}에서 읽기 실패: {e}. 이 주소는 유효하지 않을 수 있습니다.")

        # 메모리 쓰기 예제
        print(f"주소 {hex(HYPOTHETICAL_ADDRESS)}에 '{VALUE_TO_WRITE.decode()}' 쓰기 시도...")
        try:
            pm.write_bytes(HYPOTHETICAL_ADDRESS, VALUE_TO_WRITE, LENGTH_TO_READ)
            print(f"주소 {hex(HYPOTHETICAL_ADDRESS)}에 성공적으로 썼습니다.")
            
            # 쓴 후에 다시 읽어서 확인
            verified_bytes = pm.read_bytes(HYPOTHETICAL_ADDRESS, LENGTH_TO_READ)
            print(f"쓰기 후 확인된 값: {verified_bytes.decode(errors='ignore')}")

        except Exception as e:
            print(f"주소 {hex(HYPOTHETICAL_ADDRESS)}에 쓰기 실패: {e}. 권한 문제 또는 유효하지 않은 주소일 수 있습니다.")

    except pymem.exception.ProcessNotFound:
        print(f"'{PROCESS_NAME}' 프로세스를 찾을 수 없습니다. '{PROCESS_NAME}'이(가) 실행 중인지 확인하세요.")
    except Exception as e:
        print(f"예상치 못한 오류 발생: {e}")

if __name__ == "__main__":
    main()
