#include <iostream>
#include <Windows.h>
#include <TlHelp32.h> // For PROCESSENTRY32
#include <string>

// Function to get process ID by name
DWORD GetProcessIdByName(const wchar_t* processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"CreateToolhelp32Snapshot failed." << std::endl;
        return 0;
    }

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

int main() {
    const wchar_t* processName = L"notepad.exe";
    // This is a hypothetical address. In a real scenario, you would find this address
    // using a memory scanner like Cheat Engine, or by analyzing the target process.
    // The actual address for a specific value in notepad.exe will vary with each run.
    // For demonstration, we'll use a common base address for executables.
    uintptr_t hypotheticalAddress = 0x00400000; 
    const char* valueToWrite = "MODDED!";
    size_t lengthToWrite = strlen(valueToWrite) + 1; // +1 for null terminator

    DWORD pid = GetProcessIdByName(processName);

    if (pid == 0) {
        std::wcerr << L"Process '" << processName << L"' not found. Please ensure it is running." << std::endl;
        return 1;
    }

    std::wcout << L"Found process '" << processName << L"' with PID: " << pid << std::endl;

    // Open the process with necessary access rights
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        std::wcerr << L"Failed to open process. Error: " << GetLastError() << std::endl;
        return 1;
    }

    // Read memory example
    char readBuffer[256]; // Buffer to store read data
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)hypotheticalAddress, readBuffer, sizeof(readBuffer) - 1, &bytesRead)) {
        readBuffer[bytesRead] = '\0'; // Null-terminate the string
        std::cout << "Read " << bytesRead << " bytes from 0x" << std::hex << hypotheticalAddress << ": \"" << readBuffer << "\"" << std::endl;
    } else {
        std::cerr << "Failed to read memory from 0x" << std::hex << hypotheticalAddress << ". Error: " << GetLastError() << std::endl;
    }

    // Write memory example
    SIZE_T bytesWritten;
    std::cout << "Attempting to write \"" << valueToWrite << "\" to 0x" << std::hex << hypotheticalAddress << "..." << std::endl;
    if (WriteProcessMemory(hProcess, (LPVOID)hypotheticalAddress, valueToWrite, lengthToWrite, &bytesWritten)) {
        std::cout << "Successfully wrote " << bytesWritten << " bytes to 0x" << std::hex << hypotheticalAddress << std::endl;

        // Verify by reading again
        char verifyBuffer[256];
        if (ReadProcessMemory(hProcess, (LPCVOID)hypotheticalAddress, verifyBuffer, lengthToWrite, &bytesRead)) {
            verifyBuffer[bytesRead] = '\0';
            std::cout << "Verified value after write: \"" << verifyBuffer << "\"" << std::endl;
        } else {
            std::cerr << "Failed to verify write. Error: " << GetLastError() << std::endl;
        }
    } else {
        std::cerr << "Failed to write memory to 0x" << std::hex << hypotheticalAddress << ". Error: " << GetLastError() << std::endl;
    }

    CloseHandle(hProcess);
    return 0;
}