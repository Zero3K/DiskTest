/*
 * DiskTest - Windows port of MS-DOS Disk performance tester
 * Originally by James Pearce, ported to Windows/Visual Studio 2019
 * 
 * For info on how to use it, see https://www.lo-tech.co.uk/wiki/DOS_Disk_Tester
 * 
 * IOMeter type performance tests for Windows PCs.
 * Used for the development of the Dangerous Prototype XT-IDE board, and
 * subsequently the lo-tech XT-CF board.
 * 
 * Includes pattern tests for testing interface reliability and
 * to generate patterns to check with a scope attached.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <conio.h>
#include <stdarg.h>

const char* VERSION = "2.5 (Windows)";
const long DEFAULT_TEST_SIZE = 4194304; // 4MB
const char* DEFAULT_FILENAME = "TEST$$$.FIL";
const int DEFAULT_SEEKS = 256;
const int PATTERN_TESTS = 10;
const int DISPLAY_CODES_COUNT = 4;
const char DISPLAY_CODES[4] = {'-', '\\', '|', '/'};

// Pattern test modes
const int PAT_READ = 1;
const int PAT_READ_CONTINUOUS = 2;
const int PAT_WRITE = 4;
const int PAT_WRITE_CONTINUOUS = 8;
const int PAT_VERIFY = 16;
const int PAT_PROMPT = 32;

// Test patterns
const unsigned short PATTERNS[PATTERN_TESTS] = {
    0x0000, 0xFFFF, 0xFF00, 0xF00F, 0xAA55, 0xA55A, 0x18E7, 0xE718, 0x0001, 0xFFFE
};

const int PATTERN_CYCLE[PATTERN_TESTS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

const char* PATTERN_NAMES[PATTERN_TESTS] = {
    "", "", "", "", "", "", "", "", "Walking 1s", "Walking 0s"
};

const unsigned short POWER_PATTERNS[2] = {0x55AA, 0xAA55};

// Global variables
long TestSize = DEFAULT_TEST_SIZE;
char FName[256];
int Seeks = DEFAULT_SEEKS;
bool QUIT = false;
bool noprogress = false;
LARGE_INTEGER frequency;
LARGE_INTEGER startTime;

// Function declarations
void StartClock();
double StopClock();
double CreateFile();
double ReadTestFile();
double RandomTest(int transfersize, int readpercent);
void PurgeTestFile();
void DeleteTestFile();
long CheckTestFile();
void MediaTest();
void SignalTest();
bool ParamSpecified(const char* param);
const char* GetParam(const char* param);
long StringToValue(const char* s);
void ShowHelp();
long GetDiskFreeSpace();

int main(int argc, char* argv[]) {
    printf("DiskTest, by James Pearce & Foone Turing. Windows Version %s\n", VERSION);
    
    // Initialize high-resolution timer
    if (!QueryPerformanceFrequency(&frequency)) {
        fprintf(stderr, "High-resolution timer not available\n");
        return 1;
    }
    
    strcpy_s(FName, sizeof(FName), DEFAULT_FILENAME);
    
    // Check for help parameters
    if (ParamSpecified("/h") || ParamSpecified("-h") || 
        ParamSpecified("/?") || ParamSpecified("-?")) {
        ShowHelp();
        return 0;
    }
    
    printf("\n");
    bool TestDone = false;
    bool Readonly = ParamSpecified("readonly");
    noprogress = ParamSpecified("noprogress");
    
    if (!Readonly) {
        TestDone = true;
        printf("Preparing drive...");
        PurgeTestFile();
        
        // Check if specific test size was specified
        if (ParamSpecified("size=")) {
            TestSize = StringToValue(GetParam("size="));
        }
        
        // Check disk space and reduce TestSize accordingly
        long freeSpace = GetDiskFreeSpace();
        if (freeSpace < TestSize || ParamSpecified("maxsize")) {
            TestSize = (freeSpace >> 15) << 15; // Truncate to 32K boundary
        }
        
        printf("\n");
        
        if (ParamSpecified("mediatest")) {
            MediaTest();
        } else if (ParamSpecified("signaltest")) {
            SignalTest();
        } else {
            TestDone = false;
        }
    }
    
    if (!TestDone) {
        // Check for seek command line options
        if (ParamSpecified("maxseeks")) Seeks = 4096;
        if (ParamSpecified("highseeks")) Seeks = 1024;
        if (ParamSpecified("lowseeks")) Seeks = 128;
        if (ParamSpecified("minseeks")) Seeks = 32;
        
        if (Readonly) {
            printf("Read-only test mode; checking for existing test file...");
            TestSize = CheckTestFile();
            if (TestSize == 0) {
                printf(" file not found.\n");
                return 1;
            } else {
                printf(" OK\n");
            }
        }
        
        // Print test summary
        printf("Configuration: %ld KB test file, %d IOs in random tests.\n\n", 
               TestSize / 1024, Seeks);
        
        double WriteSpeed = 0, ReadSpeed = 0, IOPS = 0;
        
        if (!Readonly) {
            printf("Write Speed         : ");
            WriteSpeed = CreateFile();
            printf("%.2f KB/s\n", WriteSpeed);
        }
        
        printf("Read Speed          : ");
        ReadSpeed = ReadTestFile();
        printf("%.2f KB/s\n", ReadSpeed);
        
        if (Readonly) {
            printf("8K random read      : ");
            IOPS = RandomTest(8192, 100);
        } else {
            printf("8K random, 70%% read : ");
            IOPS = RandomTest(8192, 70);
        }
        printf("%.1f IOPS\n", IOPS);
        
        printf("Sector random read  : ");
        IOPS = RandomTest(512, 100);
        printf("%.1f IOPS\n", IOPS);
        
        printf("\n");
        printf("Average access time (includes latency and file system overhead), is %.0f ms.\n", 
               1000.0 / IOPS);
        printf("\n");
    }
    
    if (!Readonly) {
        DeleteTestFile();
    }
    
    return 0;
}

void StartClock() {
    QueryPerformanceCounter(&startTime);
}

double StopClock() {
    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);
    
    double elapsed = (double)(endTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;
    return elapsed > 0 ? elapsed : 0.01; // Prevent division by zero
}

double CreateFile() {
    HANDLE hFile = CreateFileA(FName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to create test file\n");
        return 0;
    }
    
    const int BUFFER_SIZE = 32768; // 32KB
    std::vector<char> buffer(BUFFER_SIZE);
    int max = TestSize / BUFFER_SIZE;
    int mark = 1;
    
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    coord.X = csbi.dwCursorPosition.X;
    coord.Y = csbi.dwCursorPosition.Y;
    
    StartClock();
    
    for (int i = 1; i <= max; i++) {
        DWORD bytesWritten;
        if (!::WriteFile(hFile, buffer.data(), BUFFER_SIZE, &bytesWritten, NULL)) {
            fprintf(stderr, "Write error\n");
            break;
        }
        
        if (!noprogress) {
            mark++;
            if (mark > DISPLAY_CODES_COUNT) mark = 1;
            printf("%c", DISPLAY_CODES[mark - 1]);
            SetConsoleCursorPosition(hConsole, coord);
        }
    }
    
    CloseHandle(hFile);
    return (TestSize / 1024.0) / StopClock();
}

double ReadTestFile() {
    HANDLE hFile = CreateFileA(FName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open test file for reading\n");
        return 0;
    }
    
    const int BUFFER_SIZE = 32768; // 32KB
    std::vector<char> buffer(BUFFER_SIZE);
    int max = TestSize / BUFFER_SIZE;
    int mark = 1;
    
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    coord.X = csbi.dwCursorPosition.X;
    coord.Y = csbi.dwCursorPosition.Y;
    
    StartClock();
    
    for (int i = 1; i <= max; i++) {
        DWORD bytesRead;
        if (!::ReadFile(hFile, buffer.data(), BUFFER_SIZE, &bytesRead, NULL)) {
            fprintf(stderr, "Read error\n");
            break;
        }
        
        if (!noprogress) {
            mark++;
            if (mark > DISPLAY_CODES_COUNT) mark = 1;
            printf("%c", DISPLAY_CODES[mark - 1]);
            SetConsoleCursorPosition(hConsole, coord);
        }
    }
    
    CloseHandle(hFile);
    return (TestSize / 1024.0) / StopClock();
}

double RandomTest(int transfersize, int readpercent) {
    HANDLE hFile = CreateFileA(FName, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open test file for random access\n");
        return 0;
    }
    
    std::vector<char> buffer(32768); // Max transfer size
    std::vector<long> positions(Seeks);
    
    // Initialize random number generator
    srand(GetTickCount());
    
    // Generate random positions
    long max = TestSize - transfersize;
    for (int i = 0; i < Seeks; i++) {
        long pos = (long)(((double)rand() / RAND_MAX) * max);
        pos = pos & 0xFFFFFE00; // Sector align
        positions[i] = pos;
    }
    
    int n = 1;
    int mark = 1;
    int limit = readpercent / 10;
    
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    coord.X = csbi.dwCursorPosition.X;
    coord.Y = csbi.dwCursorPosition.Y;
    
    StartClock();
    
    for (int i = 0; i < Seeks; i++) {
        if (!noprogress) {
            mark++;
            if (mark > DISPLAY_CODES_COUNT) mark = 1;
            printf("%c", DISPLAY_CODES[mark - 1]);
            SetConsoleCursorPosition(hConsole, coord);
        }
        
        // Seek to position
        SetFilePointer(hFile, positions[i], NULL, FILE_BEGIN);
        
        DWORD bytesTransferred;
        if (n <= limit) {
            // Read operation
            ::ReadFile(hFile, buffer.data(), transfersize, &bytesTransferred, NULL);
        } else {
            // Write operation
            ::WriteFile(hFile, buffer.data(), transfersize, &bytesTransferred, NULL);
        }
        
        n++;
        if (n > 10) n = 1;
    }
    
    CloseHandle(hFile);
    return Seeks / StopClock();
}

void PurgeTestFile() {
    HANDLE hFile = CreateFileA(FName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

void DeleteTestFile() {
    printf("Deleting %s.\n", FName);
    DeleteFileA(FName);
}

long CheckTestFile() {
    HANDLE hFile = CreateFileA(FName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    return fileSize;
}

long GetDiskFreeSpace() {
    ULARGE_INTEGER freeBytesAvailable;
    if (GetDiskFreeSpaceExA(".", &freeBytesAvailable, NULL, NULL)) {
        return (long)freeBytesAvailable.QuadPart;
    }
    return 0;
}

bool ParamSpecified(const char* param) {
    for (int i = 1; i < __argc; i++) {
        if (_strnicmp(__argv[i], param, strlen(param)) == 0) {
            return true;
        }
    }
    return false;
}

const char* GetParam(const char* param) {
    static char result[256];
    result[0] = '\0';
    
    for (int i = 1; i < __argc; i++) {
        if (_strnicmp(__argv[i], param, strlen(param)) == 0) {
            strcpy_s(result, sizeof(result), __argv[i] + strlen(param));
            break;
        }
    }
    return result;
}

long StringToValue(const char* s) {
    if (!s || !*s) return DEFAULT_TEST_SIZE;
    
    char* endptr;
    long value = strtol(s, &endptr, 10);
    
    if (*endptr == 'K' || *endptr == 'k') {
        value *= 1024;
    } else if (*endptr == 'M' || *endptr == 'm') {
        value *= 1024 * 1024;
    }
    
    if (value < 65536) {
        printf("Size parameter must be 64K or more. Using default.\n");
        return DEFAULT_TEST_SIZE;
    }
    
    return value;
}

void ShowHelp() {
    printf("Disk and interface performance and reliability testing.\n\n");
    printf("With no command line parameters, the utility will perform a file-system based\n");
    printf("performance test with a test file size of 4MB and 256 seeks, with file size\n");
    printf("truncated to available free space if it is less.\n\n");
    printf("Performance test specific command line options:\n\n");
    printf("  * maxseeks  - 4096 seeks (default is 256)\n");
    printf("  * highseeks - 1024 seeks\n");
    printf("  * lowseeks  - 128 seeks\n");
    printf("  * minseeks  - 32 seeks (use for floppy drives)\n");
    printf("  * size=x    - specify the test file size, which will be truncated to\n");
    printf("                available free space. To use all free space use 'maxsize'\n");
    printf("                instead. Value is in bytes, specify K or M as required.\n");
    printf("                examples: size=4M (default), size=16M, size=300K\n\n");
    printf("Example: disktest size=8M maxseeks\n\n");
}

// Helper function to convert value to hex string
std::string InHex(unsigned short value) {
    char buffer[8];
    sprintf_s(buffer, sizeof(buffer), "0x%04X", value);
    return std::string(buffer);
}

// Helper function to format two digits
std::string TwoDigit(int number) {
    char buffer[4];
    sprintf_s(buffer, sizeof(buffer), "%02d", number);
    return std::string(buffer);
}

// Memory comparison function (equivalent to Pascal's COMPSW)
int CompareWords(const void* source, const void* destination, int words) {
    const unsigned short* src = (const unsigned short*)source;
    const unsigned short* dst = (const unsigned short*)destination;
    
    for (int i = 0; i < words; i++) {
        if (src[i] != dst[i]) {
            return words - i; // Return number of differing words
        }
    }
    return 0; // No differences
}

// Pattern test function
long PatternTest(std::vector<unsigned short>& writeBlock, std::vector<unsigned short>& readBlock, 
                const std::string& displayStr, int mode) {
    HANDLE hFile = CreateFileA(FName, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open test file for pattern test\n");
        return -1;
    }
    
    long totalErrors = 0;
    long errCount = 0;
    int max = TestSize / (32 * 1024); // Number of 32KB blocks
    int readmax = max;
    
    std::string testStr = displayStr + " - Writing: ";
    int dots = (78 - testStr.length() - 12) / 2; // Space for progress dots
    
    printf("%s", testStr.c_str());
    
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    coord.X = csbi.dwCursorPosition.X;
    coord.Y = csbi.dwCursorPosition.Y;
    
    // Write phase
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    int currentDot = 0;
    
    for (int io = 1; io <= max; io++) {
        DWORD bytesWritten;
        ::WriteFile(hFile, writeBlock.data(), writeBlock.size() * sizeof(unsigned short), 
                 &bytesWritten, NULL);
        
        // Update progress
        int next = (io * dots) / max;
        if (next > currentDot) {
            while (currentDot < next) {
                printf(".");
                currentDot++;
            }
        }
        
        // Check for user interrupt
        if (_kbhit()) {
            char ch = _getch();
            if (ch == ' ' || ch == 's' || ch == 'S') {
                readmax = io;
                break;
            }
            if (ch == 'q' || ch == 'Q') {
                QUIT = true;
                readmax = 0;
                break;
            }
        }
    }
    
    // Read and verify phase
    if (readmax > 0 && (mode & PAT_READ)) {
        printf(" Comparing: ");
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        currentDot = 0;
        
        COORD coord2;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        coord2.X = csbi.dwCursorPosition.X;
        coord2.Y = csbi.dwCursorPosition.Y;
        
        for (int io = 1; io <= readmax; io++) {
            DWORD bytesRead;
            ::ReadFile(hFile, readBlock.data(), readBlock.size() * sizeof(unsigned short), 
                    &bytesRead, NULL);
            
            // Compare if verify mode is enabled
            if (mode & PAT_VERIFY) {
                errCount += CompareWords(writeBlock.data(), readBlock.data(), 
                                       writeBlock.size());
            }
            
            // Update progress
            int next = (io * dots) / readmax;
            if (next > currentDot) {
                while (currentDot < next) {
                    if (errCount == 0) {
                        printf("√"); // Success mark
                    } else {
                        printf("!");  // Error mark
                    }
                    currentDot++;
                }
                
                if (errCount != 0) {
                    totalErrors++;
                    errCount = 0;
                }
            }
            
            // Check for user interrupt
            if (_kbhit()) {
                char ch = _getch();
                if (ch == 'q' || ch == 'Q') {
                    QUIT = true;
                    break;
                }
            }
        }
    }
    
    printf("\n");
    CloseHandle(hFile);
    return totalErrors;
}

void MediaTest() {
    printf("Pattern testing with %d patterns over ", PATTERN_TESTS);
    if (TestSize > 1048576) {
        printf("%.1f MB.\n", TestSize / 1048576.0);
    } else {
        printf("%ld KB.\n", TestSize / 1024);
    }
    printf("Press any key to skip on, S to skip test completely, Q to quit.\n\n");
    
    std::vector<unsigned short> writeBlock(16384); // 32KB block
    std::vector<unsigned short> readBlock(16384);
    long errors = 0;
    QUIT = false;
    
    LARGE_INTEGER testStart;
    QueryPerformanceCounter(&testStart);
    
    for (int test = 1; test <= PATTERN_TESTS; test++) {
        // Fill array with pattern
        if (PATTERN_CYCLE[test - 1] == 1) {
            // Walking pattern test
            unsigned short pattern = PATTERNS[test - 1];
            for (int i = 0; i < 16384; i++) {
                writeBlock[i] = (pattern << (i % 16)) | (pattern >> (16 - (i % 16)));
            }
        } else {
            // Static pattern
            for (int i = 0; i < 16384; i++) {
                writeBlock[i] = PATTERNS[test - 1];
            }
        }
        
        // Copy to read buffer for comparison
        readBlock = writeBlock;
        
        // Get test name
        std::string displayStr;
        if (PATTERN_CYCLE[test - 1] == 1) {
            displayStr = PATTERN_NAMES[test - 1];
        } else {
            displayStr = "Pattern " + InHex(PATTERNS[test - 1]);
        }
        
        // Check RAM blocks for errors first
        if (CompareWords(writeBlock.data(), readBlock.data(), 16384) != 0) {
            printf("RAM Error detected with %s.\n", displayStr.c_str());
            printf("Memory test failed - cannot continue pattern testing.\n");
            return;
        }
        
        // Run the pattern test
        long testErrors = PatternTest(writeBlock, readBlock, displayStr, 
                                    PAT_READ | PAT_WRITE | PAT_VERIFY);
        if (testErrors > 0) {
            errors += testErrors;
        }
        
        if (QUIT) break;
    }
    
    // Calculate test time
    LARGE_INTEGER testEnd;
    QueryPerformanceCounter(&testEnd);
    double testTime = (double)(testEnd.QuadPart - testStart.QuadPart) / frequency.QuadPart;
    
    int hours = (int)(testTime / 3600);
    int minutes = ((int)testTime % 3600) / 60;
    int seconds = (int)testTime % 60;
    
    printf("\n");
    printf("Test ran for %s:%s:%s. ", 
           TwoDigit(hours).c_str(), TwoDigit(minutes).c_str(), TwoDigit(seconds).c_str());
    
    if (errors == 0) {
        printf("No");
    } else {
        printf("%ld 32K", errors);
    }
    printf(" blocks had errors.\n");
}

void SignalTest() {
    printf("XT/IDE Development Pattern Tests - using %ld MB test file.\n", TestSize / 1048576);
    
    std::vector<unsigned short> writeBlock(16384);
    std::vector<unsigned short> readBlock(16384);
    bool endOfTest = false;
    long errors = 0;
    
    while (!endOfTest) {
        printf("\n");
        printf("Test 1 - For testing at DD7. Flips the bit continually, all others\n");
        printf("         will be low. Line DD7 has a 10k pull-down at the interface.\n\n");
        printf("Test 2 - For testing at DD11. Holds the bit low and flips all other bits\n");
        printf("         continually. Enables measurement of cross-talk as the line serving\n");
        printf("         this bit is in the middle of the data lines on the 40-pin connector.\n\n");
        printf("Test 3 - For testing on the ISA Bus at data bit 4 (ISA slot pin A5). To enable\n");
        printf("         assessment of ISA bus signal quality, flips this bit repeatedly.\n\n");
        printf("Test 4 - For measuring peak power consumption of the interface under read and\n");
        printf("         write workloads. Total power consumption will be affected by the\n");
        printf("         system (and bus) speed, since faster switching will use more power.\n");
        printf("         Test patterns are %s and %s.\n\n", 
               InHex(POWER_PATTERNS[0]).c_str(), InHex(POWER_PATTERNS[1]).c_str());
        printf("Test 5 - As test 4, except that the read part of the test is a one-pass verify\n");
        printf("         This will run much slower, but will confirm, after a heavy write test\n");
        printf("         that the signals were intact.\n\n");
        printf("Enter Test (1-5) or E to end: ");
        
        char ch;
        do {
            ch = _getch();
            ch = toupper(ch);
        } while (ch != '1' && ch != '2' && ch != '3' && ch != '4' && ch != '5' && 
                 ch != 'E' && ch != 'Q');
        
        printf("%c\n", ch);
        
        if (ch == 'E' || ch == 'Q') {
            endOfTest = true;
        } else {
            // Fill buffer based on choice
            switch (ch) {
                case '1':
                    for (int i = 0; i < 16384; i += 2) {
                        writeBlock[i] = 0x0080;
                        writeBlock[i + 1] = 0x0000;
                    }
                    break;
                case '2':
                    for (int i = 0; i < 16384; i += 2) {
                        writeBlock[i] = 0xF7FF;
                        writeBlock[i + 1] = 0x0000;
                    }
                    break;
                case '3':
                    for (int i = 0; i < 16384; i++) {
                        writeBlock[i] = 0x1000;
                    }
                    break;
                case '4':
                case '5':
                    for (int i = 0; i < 16384; i += 2) {
                        writeBlock[i] = POWER_PATTERNS[0];
                        writeBlock[i + 1] = POWER_PATTERNS[1];
                    }
                    break;
            }
            
            // Perform the test
            printf("\nWill perform WRITE test first, then the READ. Data read back will ");
            if (ch != '5') printf("not ");
            printf("be verified. Press SPACE to move on to read test once current write\n");
            printf("test has finished, N to skip on immediately, or S to skip it.\n");
            
            std::string displayStr = "Test ";
            displayStr += ch;
            
            int testMode = PAT_READ | PAT_WRITE | PAT_WRITE_CONTINUOUS;
            if (ch == '5') {
                testMode |= PAT_VERIFY;
            } else {
                testMode |= PAT_READ_CONTINUOUS;
            }
            
            long testErrors = PatternTest(writeBlock, readBlock, displayStr, testMode);
            if (testErrors > 0) {
                errors += testErrors;
            }
        }
    }
    
    if (errors == 0) {
        printf("No");
    } else {
        printf("%ld", errors);
    }
    printf(" errors were encountered.\n");
}