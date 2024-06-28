#include <iostream>
#include <windows.h>
#include <string>
#include <TlHelp32.h>
#include <vector>

enum positionStates {
    MINE = 0x8F,
    MINEFLAG = 0x8E,
    MINEQUESTION = 0x8D,
    NOMINE = 0x0F,
    NOMINEFLAG = 0x0E,
    NOMINEQUESTION = 0x0D
};

DWORD findProcessId(const std::wstring& processName) {
    DWORD processId = 0;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(hProcessSnap, &processEntry)) {
        do {
            if (processName == processEntry.szExeFile) {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(hProcessSnap, &processEntry));
    }

    CloseHandle(hProcessSnap);
    return processId;
}

bool readMemory(HANDLE hProcess, uintptr_t address, void* buffer, SIZE_T size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) && bytesRead == size;
}

int main() {
    DWORD processID = findProcessId(L"WINMINE.EXE");

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, false, processID);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process, quitting!" << std::endl;
        exit(-1);
    }

    uintptr_t addressOfMatrix = 0x01005361;
    uintptr_t widthAddress = 0x01005334;
    uintptr_t heightAddress = 0x01005338;

    int gridWidth, gridHeight;
    if (!readMemory(hProcess, widthAddress, &gridWidth, sizeof(gridWidth))) {
        std::cerr << "Failed to read gridWidth, quitting!" << std::endl;
        exit(-2);
    }
    if (!readMemory(hProcess, heightAddress, &gridHeight, sizeof(gridHeight))) {
        std::cerr << "Failed to read gridHeight, quitting!" << std::endl;
        exit(-3);
    }

    uint8_t positionState;

    for (int i = 0; i < gridHeight; i++) {
        uintptr_t rowAddress = addressOfMatrix + (i * 32);
        for (int j = 0; j < gridWidth; j++) {
            if (!readMemory(hProcess, rowAddress + j, &positionState, sizeof(positionState))) {
                std::cerr << "Failed to read positionState, quitting!" << std::endl;
                exit(-4);
            }
            switch (positionState) {
            case positionStates::MINE:
                std::cout << "X ";
                break;
            case positionStates::NOMINE:
                std::cout << ". ";
                break;
            default:
                std::cout << "? ";
                break;
            }
        }
        std::cout << "\n";
    }

    std::cout << "To close the window, press F12.\n";

    // Keep the console open until F12 is pressed
    while (true) {
        if (GetAsyncKeyState(VK_F12)) {
            break;
        }
        Sleep(100); // Sleep to reduce CPU usage
    }

    CloseHandle(hProcess);

    return 0;
}