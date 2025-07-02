#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <regex>
#include <iomanip>
#include <cmath> // For std::log2
#include <codecvt>
#include <locale>

/**
 * Exercise 5: 난독화 해제 시스템
 * 
 * 목표: 다양한 난독화 기법으로 보호된 코드/데이터를 분석하고 복원
 * 
 * 구현 내용:
 * 1. XOR 암호화 문자열 탐지 및 복호화
 * 2. Base64 인코딩 탐지 및 디코딩
 * 3. ROT13/Caesar 암호 해독
 * 4. 간단한 치환 암호 분석
 * 5. API 이름 난독화 해제
 * 6. 제어 흐름 난독화 분석
 * 7. 패턴 기반 자동 키 추출
 */

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

class Deobfuscator {
private:
    struct ObfuscatedString {
        std::vector<BYTE> data;
        size_t offset;
        std::string decrypted;
        std::string method;
        int confidence;
    };

    struct DecryptionResult {
        std::string plaintext;
        std::string method;
        std::vector<BYTE> key;
        int confidence;
    };

public:
    // 1. XOR 암호화 문자열 탐지 및 복호화
    static std::vector<DecryptionResult> DetectAndDecryptXOR(const std::vector<BYTE>& data, 
                                                           size_t minLength = 4) {
        std::wcout << L"[+] XOR 암호화 문자열 탐지 중..." << std::endl;
        
        std::vector<DecryptionResult> results;
        
        // 단일 바이트 XOR 키 브루트포스 (1-255)
        for (int key = 1; key < 256; key++) {
            auto decrypted = XORDecrypt(data, static_cast<BYTE>(key));
            auto strings = ExtractPrintableStrings(decrypted, minLength);
            
            for (const auto& str : strings) {
                if (IsMeaningfulString(str)) {
                    DecryptionResult result;
                    result.plaintext = str;
                    result.method = "XOR (단일 바이트)";
                    result.key = {static_cast<BYTE>(key)};
                    result.confidence = CalculateStringConfidence(str);
                    
                    if (result.confidence > 70) {
                        results.push_back(result);
                    }
                }
            }
        }
        
        // 다중 바이트 XOR 키 (2-16 바이트)
        auto multiByteResults = DetectMultiByteXOR(data, minLength);
        results.insert(results.end(), multiByteResults.begin(), multiByteResults.end());
        
        std::wcout << L"[+] XOR 복호화 결과: " << results.size() << L"개 문자열" << std::endl;
        return results;
    }

    // 2. Base64 디코딩
    static std::vector<DecryptionResult> DetectAndDecodeBase64(const std::vector<BYTE>& data) {
        std::wcout << L"[+] Base64 인코딩 탐지 중..." << std::endl;
        
        std::vector<DecryptionResult> results;
        std::string dataStr(data.begin(), data.end());
        
        // Base64 패턴 정규식
        std::regex base64Pattern(R"([A-Za-z0-9+/]{4,}={0,2})");
        std::sregex_iterator iter(dataStr.begin(), dataStr.end(), base64Pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string encoded = iter->str();
            
            if (encoded.length() >= 4 && encoded.length() % 4 == 0) {
                try {
                    auto decoded = Base64Decode(encoded);
                    
                    if (!decoded.empty()) {
                        std::string decodedStr(decoded.begin(), decoded.end());
                        
                        if (IsPrintableString(decodedStr) && IsMeaningfulString(decodedStr)) {
                            DecryptionResult result;
                            result.plaintext = decodedStr;
                            result.method = "Base64";
                            result.confidence = CalculateStringConfidence(decodedStr);
                            
                            if (result.confidence > 60) {
                                results.push_back(result);
                            }
                        }
                    }
                } catch (...) {
                    // Base64 디코딩 실패는 무시
                }
            }
        }
        
        std::wcout << L"[+] Base64 디코딩 결과: " << results.size() << L"개 문자열" << std::endl;
        return results;
    }

    // 3. ROT13/Caesar 암호 해독
    static std::vector<DecryptionResult> DetectAndDecryptROT(const std::vector<BYTE>& data) {
        std::wcout << L"[+] ROT/Caesar 암호 탐지 중..." << std::endl;
        
        std::vector<DecryptionResult> results;
        std::string dataStr(data.begin(), data.end());
        
        // ROT 1-25 시도
        for (int shift = 1; shift < 26; shift++) {
            std::string decrypted = CaesarDecrypt(dataStr, shift);
            auto strings = ExtractWordsFromString(decrypted);
            
            for (const auto& str : strings) {
                if (str.length() >= 4 && IsMeaningfulString(str)) {
                    DecryptionResult result;
                    result.plaintext = str;
                    result.method = "Caesar (shift " + std::to_string(shift) + ")";
                    result.confidence = CalculateStringConfidence(str);
                    
                    if (result.confidence > 75) {
                        results.push_back(result);
                    }
                }
            }
        }
        
        std::wcout << L"[+] Caesar 해독 결과: " << results.size() << L"개 문자열" << std::endl;
        return results;
    }

    // 4. API 이름 난독화 해제
    static std::vector<std::string> DetectObfuscatedAPIs(const std::vector<BYTE>& data) {
        std::wcout << L"[+] 난독화된 API 이름 탐지 중..." << std::endl;
        
        std::vector<std::string> apis;
        
        // 일반적인 Windows API 목록
        std::vector<std::string> commonAPIs = {
            "GetProcAddress", "LoadLibraryA", "LoadLibraryW", "GetModuleHandleA", "GetModuleHandleW",
            "VirtualAlloc", "VirtualProtect", "VirtualFree", "CreateFileA", "CreateFileW",
            "ReadFile", "WriteFile", "CloseHandle", "CreateProcessA", "CreateProcessW",
            "OpenProcess", "TerminateProcess", "GetCurrentProcess", "GetCurrentThread",
            "Sleep", "GetTickCount", "QueryPerformanceCounter", "RegOpenKeyA", "RegOpenKeyW",
            "RegQueryValueA", "RegQueryValueW", "RegCloseKey", "MessageBoxA", "MessageBoxW"
        };
        
        // XOR로 난독화된 API 이름 탐지
        for (int key = 1; key < 256; key++) {
            auto decrypted = XORDecrypt(data, static_cast<BYTE>(key));
            std::string decryptedStr(decrypted.begin(), decrypted.end());
            
            for (const auto& api : commonAPIs) {
                if (decryptedStr.find(api) != std::string::npos) {
                    apis.push_back(api + " (XOR key: " + std::to_string(key) + ")");
                }
            }
        }
        
        // 스택 문자열 탐지 (문자 단위로 푸시되는 API 이름)
        apis.insert(apis.end(), DetectStackStrings(data).begin(), DetectStackStrings(data).end());
        
        std::wcout << L"[+] 발견된 API: " << apis.size() << L"개" << std::endl;
        return apis;
    }

    // 5. 자동 키 추출 (빈도 분석 기반)
    static std::vector<BYTE> ExtractXORKeyByFrequency(const std::vector<BYTE>& ciphertext, 
                                                     size_t keyLength = 0) {
        std::wcout << L"[+] 빈도 분석을 통한 XOR 키 추출..." << std::endl;
        
        if (keyLength == 0) {
            // 키 길이 추정 (반복 패턴 분석)
            keyLength = EstimateKeyLength(ciphertext);
        }
        
        if (keyLength == 0 || keyLength > 16) {
            keyLength = 1; // 기본값
        }
        
        std::vector<BYTE> key(keyLength);
        
        // 각 키 바이트별로 빈도 분석
        for (size_t keyPos = 0; keyPos < keyLength; keyPos++) {
            std::map<BYTE, int> frequency;
            
            for (size_t i = keyPos; i < ciphertext.size(); i += keyLength) {
                frequency[ciphertext[i]]++;
            }
            
            // 가장 빈번한 바이트를 공백(0x20)과 XOR하여 키 추출
            BYTE mostFrequent = 0;
            int maxCount = 0;
            for (const auto& pair : frequency) {
                if (pair.second > maxCount) {
                    maxCount = pair.second;
                    mostFrequent = pair.first;
                }
            }
            
            key[keyPos] = mostFrequent ^ 0x20; // 공백 문자 가정
        }
        
        std::wcout << L"[+] 추출된 키 길이: " << keyLength << L" 바이트" << std::endl;
        return key;
    }

    // 6. 패턴 기반 문자열 추출
    static std::vector<std::string> ExtractPatternBasedStrings(const std::vector<BYTE>& data) {
        std::wcout << L"[+] 패턴 기반 문자열 추출 중..." << std::endl;
        
        std::vector<std::string> results;
        
        // URL 패턴
        std::string dataStr(data.begin(), data.end());
        std::regex urlPattern(R"(https?://[^\s<>"'{}|\\^`\[\]]+)");
        std::sregex_iterator iter(dataStr.begin(), dataStr.end(), urlPattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            results.push_back("URL: " + iter->str());
        }
        
        // 이메일 패턴
        std::regex emailPattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        iter = std::sregex_iterator(dataStr.begin(), dataStr.end(), emailPattern);
        
        for (; iter != end; ++iter) {
            results.push_back("Email: " + iter->str());
        }
        
        // 파일 경로 패턴
        std::regex pathPattern(R"([C-Z]:\\[^<>:"|?*\r\n]+)");
        iter = std::sregex_iterator(dataStr.begin(), dataStr.end(), pathPattern);
        
        for (; iter != end; ++iter) {
            results.push_back("Path: " + iter->str());
        }
        
        // IP 주소 패턴
        std::regex ipPattern(R"(\b(?:[0-9]{1,3}\.){3}[0-9]{1,3}\b)");
        iter = std::sregex_iterator(dataStr.begin(), dataStr.end(), ipPattern);
        
        for (; iter != end; ++iter) {
            results.push_back("IP: " + iter->str());
        }
        
        std::wcout << L"[+] 패턴 기반 추출 결과: " << results.size() << L"개" << std::endl;
        return results;
    }

    // 7. 종합 분석 실행
    static void AnalyzeFile(const std::string& filePath) {
        std::wcout << L"=== 난독화 해제 분석 시작 ===" << std::endl;
        std::wcout << L"파일: " << StringToWString(filePath) << std::endl;
        
        // 파일 읽기
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::wcout << L"[-] 파일 열기 실패" << std::endl;
            return;
        }
        
        std::vector<BYTE> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
        file.close();
        
        std::wcout << L"[+] 파일 크기: " << data.size() << L" 바이트" << std::endl;
        
        // 각종 해독 시도
        auto xorResults = DetectAndDecryptXOR(data);
        auto base64Results = DetectAndDecodeBase64(data);
        auto rotResults = DetectAndDecryptROT(data);
        auto apiResults = DetectObfuscatedAPIs(data);
        auto patternResults = ExtractPatternBasedStrings(data);
        
        // 결과 출력
        PrintResults("XOR 복호화", xorResults);
        PrintResults("Base64 디코딩", base64Results);
        PrintResults("Caesar 해독", rotResults);
        PrintStringResults("API 탐지", apiResults);
        PrintStringResults("패턴 추출", patternResults);
        
        // 고신뢰도 결과만 별도 출력
        std::wcout << L"\n=== 고신뢰도 결과 (90% 이상) ===" << std::endl;
        PrintHighConfidenceResults({xorResults, base64Results, rotResults});
    }

private:
    // 헬퍼 함수들
    static std::vector<BYTE> XORDecrypt(const std::vector<BYTE>& data, BYTE key) {
        std::vector<BYTE> result(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            result[i] = data[i] ^ key;
        }
        return result;
    }
    
    static std::vector<BYTE> XORDecrypt(const std::vector<BYTE>& data, const std::vector<BYTE>& key) {
        std::vector<BYTE> result(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            result[i] = data[i] ^ key[i % key.size()];
        }
        return result;
    }

    static std::vector<std::string> ExtractPrintableStrings(const std::vector<BYTE>& data, size_t minLength = 4) {
        std::vector<std::string> strings;
        std::string current;
        
        for (BYTE b : data) {
            if (b >= 32 && b <= 126) { // 인쇄 가능한 ASCII
                current += static_cast<char>(b);
            } else {
                if (current.length() >= minLength) {
                    strings.push_back(current);
                }
                current.clear();
            }
        }
        
        if (current.length() >= minLength) {
            strings.push_back(current);
        }
        
        return strings;
    }

    static bool IsMeaningfulString(const std::string& str) {
        if (str.length() < 3) return false;
        
        // 영어 단어 패턴 확인
        static const std::vector<std::string> commonWords = {
            "the", "and", "for", "are", "but", "not", "you", "all", "can", "had", "her", "was", "one", "our",
            "out", "day", "get", "has", "him", "his", "how", "its", "may", "new", "now", "old", "see", "two",
            "who", "boy", "did", "man", "end", "few", "got", "let", "put", "say", "she", "too", "use",
            "error", "file", "system", "user", "password", "login", "admin", "config", "data", "temp",
            "windows", "microsoft", "program", "process", "memory", "address", "function", "library"
        };
        
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        for (const auto& word : commonWords) {
            if (lower.find(word) != std::string::npos) {
                return true;
            }
        }
        
        // 특수 패턴 확인
        if (str.find("http") != std::string::npos ||
            str.find("www.") != std::string::npos ||
            str.find(".exe") != std::string::npos ||
            str.find(".dll") != std::string::npos ||
            str.find("C:\\") != std::string::npos ||
            str.find("HKEY_") != std::string::npos) {
            return true;
        }
        
        return false;
    }

    static int CalculateStringConfidence(const std::string& str) {
        int confidence = 0;
        
        // 길이 점수
        if (str.length() >= 8) confidence += 20;
        else if (str.length() >= 4) confidence += 10;
        
        // 문자 구성 점수
        int letters = 0, digits = 0, spaces = 0, special = 0;
        for (char c : str) {
            if (isalpha(c)) letters++;
            else if (isdigit(c)) digits++;
            else if (c == ' ') spaces++;
            else special++;
        }
        
        float letterRatio = static_cast<float>(letters) / str.length();
        if (letterRatio > 0.6f) confidence += 30;
        else if (letterRatio > 0.4f) confidence += 20;
        
        // 공백 비율 (자연스러운 텍스트)
        float spaceRatio = static_cast<float>(spaces) / str.length();
        if (spaceRatio > 0.1f && spaceRatio < 0.3f) confidence += 20;
        
        // 의미있는 단어 포함
        if (IsMeaningfulString(str)) confidence += 30;
        
        return std::min(confidence, 100);
    }

    static std::vector<DecryptionResult> DetectMultiByteXOR(const std::vector<BYTE>& data, size_t minLength) {
        std::vector<DecryptionResult> results;
        
        // 2-8 바이트 키 시도
        for (size_t keyLen = 2; keyLen <= 8; keyLen++) {
            auto estimatedKey = EstimateXORKey(data, keyLen);
            if (!estimatedKey.empty()) {
                auto decrypted = XORDecrypt(data, estimatedKey);
                auto strings = ExtractPrintableStrings(decrypted, minLength);
                
                for (const auto& str : strings) {
                    if (IsMeaningfulString(str)) {
                        DecryptionResult result;
                        result.plaintext = str;
                        result.method = "XOR (다중 바이트, 길이: " + std::to_string(keyLen) + ")";
                        result.key = estimatedKey;
                        result.confidence = CalculateStringConfidence(str);
                        
                        if (result.confidence > 70) {
                            results.push_back(result);
                        }
                    }
                }
            }
        }
        
        return results;
    }

    static std::vector<BYTE> EstimateXORKey(const std::vector<BYTE>& data, size_t keyLength) {
        if (data.size() < keyLength * 4) return {}; // 데이터가 너무 작음
        
        std::vector<BYTE> key(keyLength);
        
        for (size_t pos = 0; pos < keyLength; pos++) {
            std::map<BYTE, int> frequency;
            
            for (size_t i = pos; i < data.size(); i += keyLength) {
                frequency[data[i]]++;
            }
            
            // 가장 빈번한 바이트를 공백(0x20)과 XOR
            BYTE mostFrequent = 0;
            int maxCount = 0;
            for (const auto& pair : frequency) {
                if (pair.second > maxCount) {
                    maxCount = pair.second;
                    mostFrequent = pair.first;
                }
            }
            
            key[pos] = mostFrequent ^ 0x20; // 공백 문자 가정
        }
        
        return key;
    }

    static std::vector<BYTE> Base64Decode(const std::string& encoded) {
        static const std::string chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::vector<BYTE> result;
        int val = 0, valb = -8;
        
        for (char c : encoded) {
            if (chars.find(c) == std::string::npos) break;
            val = (val << 6) + chars.find(c);
            valb += 6;
            if (valb >= 0) {
                result.push_back(static_cast<BYTE>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        
        return result;
    }

    static std::string CaesarDecrypt(const std::string& ciphertext, int shift) {
        std::string result;
        for (char c : ciphertext) {
            if (isalpha(c)) {
                char base = islower(c) ? 'a' : 'A';
                result += static_cast<char>((c - base - shift + 26) % 26 + base);
            } else {
                result += c;
            }
        }
        return result;
    }

    static std::vector<std::string> ExtractWordsFromString(const std::string& text) {
        std::vector<std::string> words;
        std::string current;
        
        for (char c : text) {
            if (isalpha(c)) {
                current += c;
            } else {
                if (current.length() >= 3) {
                    words.push_back(current);
                }
                current.clear();
            }
        }
        
        if (current.length() >= 3) {
            words.push_back(current);
        }
        
        return words;
    }

    static bool IsPrintableString(const std::string& str) {
        for (char c : str) {
            if (c < 32 || c > 126) {
                return false;
            }
        }
        return true;
    }

    static size_t EstimateKeyLength(const std::vector<BYTE>& data) {
        // Index of Coincidence를 사용한 키 길이 추정
        std::map<size_t, double> icScores;
        
        for (size_t keyLen = 1; keyLen <= 16; keyLen++) {
            double totalIC = 0.0;
            
            for (size_t pos = 0; pos < keyLen; pos++) {
                std::vector<int> frequency(256, 0);
                int count = 0;
                
                for (size_t i = pos; i < data.size(); i += keyLen) {
                    frequency[data[i]]++;
                    count++;
                }
                
                // IC 계산
                double ic = 0.0;
                if (count > 1) {
                    for (int freq : frequency) {
                        ic += freq * (freq - 1);
                    }
                    ic /= (count * (count - 1));
                }
                
                totalIC += ic;
            }
            
            icScores[keyLen] = totalIC / keyLen;
        }
        
        // 가장 높은 IC 점수를 가진 키 길이 반환
        size_t bestKeyLen = 1;
        double bestScore = 0.0;
        for (const auto& pair : icScores) {
            if (pair.second > bestScore) {
                bestScore = pair.second;
                bestKeyLen = pair.first;
            }
        }
        
        return bestKeyLen;
    }

    static std::vector<std::string> DetectStackStrings(const std::vector<BYTE>& data) {
        // 스택 문자열 패턴 탐지 (PUSH 명령어로 문자 단위 삽입)
        std::vector<std::string> results;
        
        // 68 XX XX XX XX (PUSH imm32) 패턴 찾기
        for (size_t i = 0; i < data.size() - 20; i++) {
            if (data[i] == 0x68) { // PUSH imm32
                std::string stackString;
                size_t j = i;
                
                // 연속된 PUSH 명령어 확인
                while (j < data.size() - 5 && data[j] == 0x68) {
                    DWORD value = *reinterpret_cast<const DWORD*>(&data[j + 1]);
                    
                    // 4바이트 값을 문자로 변환
                    for (int k = 0; k < 4; k++) {
                        BYTE b = (value >> (k * 8)) & 0xFF;
                        if (b >= 32 && b <= 126) {
                            stackString = static_cast<char>(b) + stackString; // 역순으로 추가
                        } else if (b == 0) {
                            break;
                        }
                        else {
                            stackString.clear();
                            break;
                        }
                    }
                    
                    j += 5; // 다음 PUSH 명령어로
                }
                
                if (stackString.length() >= 4 && IsMeaningfulString(stackString)) {
                    results.push_back("Stack String: " + stackString);
                }
            }
        }
        
        return results;
    }

    // 결과 출력 함수들
    static void PrintResults(const std::string& title, const std::vector<DecryptionResult>& results) {
        std::wcout << L"\n=== " << StringToWString(title) << L" ===" << std::endl;
        
        if (results.empty()) {
            std::wcout << L"결과 없음" << std::endl;
            return;
        }
        
        for (const auto& result : results) {
            std::wcout << L"[" << result.confidence << L"%] " << StringToWString(result.method) 
                      << L": " << StringToWString(result.plaintext) << std::endl;
        }
    }

    static void PrintStringResults(const std::string& title, const std::vector<std::string>& results) {
        std::wcout << L"\n=== " << StringToWString(title) << L" ===" << std::endl;
        
        if (results.empty()) {
            std::wcout << L"결과 없음" << std::endl;
            return;
        }
        
        for (const auto& result : results) {
            std::wcout << L"  " << StringToWString(result) << std::endl;
        }
    }

    static void PrintHighConfidenceResults(const std::vector<std::vector<DecryptionResult>>& allResults) {
        for (const auto& results : allResults) {
            for (const auto& result : results) {
                if (result.confidence >= 90) {
                    std::wcout << L"[" << result.confidence << L"%] " << StringToWString(result.method) 
                              << L": " << StringToWString(result.plaintext) << std::endl;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    std::wcout << L"고급 난독화 해제 시스템 v1.0" << std::endl;
    std::wcout << L"교육 및 연구 목적으로만 사용하세요." << std::endl;
    std::wcout << L"===================================" << std::endl;

    if (argc != 2) {
        std::wcout << L"사용법: " << StringToWString(argv[0]) << L" <분석할파일>" << std::endl;
        std::wcout << L"예제: " << StringToWString(argv[0]) << L" obfuscated.exe" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    Deobfuscator::AnalyzeFile(filePath);

    std::wcout << L"\n계속하려면 Enter를 누르세요..." << std::endl;
    std::wcin.get();

    return 0;
}

/*
 * 컴파일 방법:
 * cl /EHsc exercise5_deobfuscation.cpp
 * 
 * 사용 방법:
 * exercise5_deobfuscation.exe target_file.exe
 * 
 * 지원 난독화 기법:
 * 1. XOR 암호화 (단일/다중 바이트)
 * 2. Base64 인코딩
 * 3. ROT13/Caesar 암호
 * 4. 스택 문자열
 * 5. API 이름 난독화
 * 
 * 분석 기능:
 * - 자동 키 추출 (빈도 분석)
 * - 패턴 기반 문자열 탐지
 * - 신뢰도 기반 결과 필터링
 * - 다양한 인코딩 방식 지원
 * 
 * 출력 정보:
 * - 복호화된 문자열
 * - 사용된 복호화 방법
 * - 신뢰도 점수
 * - 추출된 키 정보
 * 
 * 학습 포인트:
 * - 암호화 분석 기법
 * - 빈도 분석 알고리즘
 * - 패턴 매칭 기술
 * - 자동화된 문자열 추출
 * - 휴리스틱 분석 방법
 * 
 * 실제 적용:
 * - 악성코드 분석
 * - 리버스 엔지니어링
 * - 포렌식 조사
 * - 보안 연구
 */