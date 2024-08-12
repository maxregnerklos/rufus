/*
 * Rufus: The Reliable USB Formatting Utility
 * Copyright © 2011-2024 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(disable: 4996)  // Ignore deprecated
#pragma warning(disable: 6258)  // Using TerminateThread
#pragma warning(disable: 26451) // Arithmetic overflow
#pragma warning(disable: 28159) // Consider using GetVersionEx
#endif

// Constants
constexpr auto KB = 1024LL;
constexpr auto MB = 1048576LL;
constexpr auto GB = 1073741824LL;
constexpr auto TB = 1099511627776LL;
constexpr auto PB = 1125899906842624LL;

#define APPLICATION_NAME "Rufus"
#define COMPANY_NAME "Akeo Consulting"
#define STR_NO_LABEL "NO_LABEL"

// Unicode markers
const std::wstring LEFT_TO_RIGHT_MARK = L"\u200E";
const std::wstring RIGHT_TO_LEFT_MARK = L"\u200F";
const std::wstring LEFT_TO_RIGHT_EMBEDDING = L"\u202A";
const std::wstring RIGHT_TO_LEFT_EMBEDDING = L"\u202B";
const std::wstring POP_DIRECTIONAL_FORMATTING = L"\u202C";
const std::wstring LEFT_TO_RIGHT_OVERRIDE = L"\u202D";
const std::wstring RIGHT_TO_LEFT_OVERRIDE = L"\u202E";

// Timing constants
constexpr auto DRIVE_ACCESS_TIMEOUT = 15000;  // ms
constexpr auto DRIVE_ACCESS_RETRIES = 150;

// Size constants
constexpr auto MIN_DRIVE_SIZE = 8 * MB;
constexpr auto MIN_EXTRA_PART_SIZE = 1 * MB;
constexpr auto MIN_EXT_SIZE = 256 * MB;

// Enums
enum class HashType {
    MD5,
    SHA1,
    SHA256,
    SHA512,
    MAX
};

enum class FileIOType {
    Read,
    Write,
    Append
};

// Structs
struct WindowsVersion {
    uint32_t Major;
    uint32_t Minor;
    uint32_t Build;
    uint32_t Revision;
};

// Function prototypes
std::string WindowsErrorString();
void UpdateProgress(int op, float percent);
bool CreateTaskbarList();
bool SetTaskbarProgressState(TASKBAR_PROGRESS_FLAGS tbpFlags);
bool SetTaskbarProgressValue(ULONGLONG ullCompleted, ULONGLONG ullTotal);

// Hash functions
using HashInitFunc = std::function<void(void*)>;
using HashWriteFunc = std::function<void(void*, const uint8_t*, size_t)>;
using HashFinalFunc = std::function<void(void*)>;

extern std::array<HashInitFunc, static_cast<size_t>(HashType::MAX)> hash_init;
extern std::array<HashWriteFunc, static_cast<size_t>(HashType::MAX)> hash_write;
extern std::array<HashFinalFunc, static_cast<size_t>(HashType::MAX)> hash_final;

// Utility functions
template<typename T>
bool IsPowerOfTwo(T x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

template<typename T>
void IgnoreReturnValue(T&&) {}

template<typename T, size_t N>
constexpr size_t ArraySize(T (&)[N]) {
    return N;
}

#define STRINGIFY(x) #x
#define PERCENTAGE(percent, value) ((1ULL * (percent) * (value)) / 100ULL)

// Safe string operations
template<typename... Args>
std::string SafeSprintf(const char* format, Args... args) {
    int size = snprintf(nullptr, 0, format, args...);
    std::string result(size + 1, '\0');
    snprintf(&result[0], size + 1, format, args...);
    return result;
}

// Modern C++ alternatives to macros
template<typename T>
void SafeFree(T*& ptr) {
    delete ptr;
    ptr = nullptr;
}

template<typename T>
void SafeArrayFree(T*& ptr) {
    delete[] ptr;
    ptr = nullptr;
}

// Thread-safe singleton for global state
class GlobalState {
public:
    static GlobalState& Instance() {
        static GlobalState instance;
        return instance;
    }

    // Add your global variables here
    HWND hMainDialog = nullptr;
    HWND hLogDialog = nullptr;
    // ... other global variables

private:
    GlobalState() = default;
    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;
};

// Use GlobalState::Instance().variableName to access global variables
// Hash context using modern C++ features
struct HashContext {
    std::array<uint8_t, 128> buf;  // Using MAX_BLOCKSIZE
    std::array<uint64_t, 8> state;
    uint64_t bytecount;
};

// Modern C++ wrapper for Windows API functions
class WindowsApi {
public:
    static HWND CreateDialogWrapper(HINSTANCE hInstance, int DialogId, HWND hWndParent, DLGPROC lpDialogFunc) {
        return CreateDialogParam(hInstance, MAKEINTRESOURCE(DialogId), hWndParent, lpDialogFunc, 0);
    }

    static INT_PTR DialogBoxWrapper(HINSTANCE hInstance, int DialogId, HWND hWndParent, DLGPROC lpDialogFunc) {
        return DialogBoxParam(hInstance, MAKEINTRESOURCE(DialogId), hWndParent, lpDialogFunc, 0);
    }

    static void CenterDialog(HWND hDlg, HWND hParent) {
        RECT rc, rcDlg, rcParent;
        GetWindowRect(hParent, &rcParent);
        GetWindowRect(hDlg, &rcDlg);
        CopyRect(&rc, &rcParent);

        OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
        OffsetRect(&rc, -rc.left, -rc.top);
        OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

        SetWindowPos(hDlg,
                     HWND_TOP,
                     rcParent.left + (rc.right / 2),
                     rcParent.top + (rc.bottom / 2),
                     0, 0,
                     SWP_NOSIZE);
    }
};

// Modern file operations
class FileOperations {
public:
    static std::vector<uint8_t> ReadFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + path);
        }

        auto size = file.tellg();
        std::vector<uint8_t> buffer(size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        return buffer;
    }

    static void WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file: " + path);
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
};

// Modern string operations
class StringOperations {
public:
    static std::wstring Utf8ToWide(const std::string& utf8) {
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        std::wstring wide(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
        return wide;
    }

    static std::string WideToUtf8(const std::wstring& wide) {
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size, nullptr, nullptr);
        return utf8;
    }
};

// Modern process and thread management
class ProcessManager {
public:
    static DWORD RunCommand(const std::string& command, const std::string& directory) {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        
        if (!CreateProcessA(nullptr, const_cast<LPSTR>(command.c_str()), nullptr, nullptr, FALSE, 0, nullptr, 
                            directory.empty() ? nullptr : directory.c_str(), &si, &pi)) {
            throw std::runtime_error("Failed to create process: " + WindowsErrorString());
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return exitCode;
    }

    static void SetThreadAffinity(std::vector<DWORD_PTR>& threadAffinity) {
        DWORD_PTR processAffinityMask, systemAffinityMask;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask)) {
            throw std::runtime_error("Failed to get process affinity mask: " + WindowsErrorString());
        }

        for (size_t i = 0; i < threadAffinity.size(); ++i) {
            threadAffinity[i] &= processAffinityMask;
            if (threadAffinity[i] == 0) {
                throw std::runtime_error("Invalid thread affinity specified");
            }
        }

        SetThreadAffinityMask(GetCurrentThread(), threadAffinity[0]);
    }
};

// Modern error handling
class ErrorHandler {
public:
    static void ThrowLastError(const std::string& message) {
        throw std::runtime_error(message + ": " + WindowsErrorString());
    }

    static void CheckApiError(BOOL result, const std::string& message) {
        if (!result) {
            ThrowLastError(message);
        }
    }
};

// Modernized global functions
std::string GetExecutablePath() {
    std::vector<char> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0 || size == buffer.size()) {
        ErrorHandler::ThrowLastError("Failed to get executable path");
    }
    return std::string(buffer.data(), size);
}

void InitializeApplication() {
    // Initialize global state, load configuration, etc.
    auto& globalState = GlobalState::Instance();
    globalState.hMainInstance = GetModuleHandle(nullptr);
    // ... other initialization code
}

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    try {
        InitializeApplication();
        // Main application logic here
        return 0;
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
        return 1;
    }
}
