# DiskTest
MS-DOS Disk performance tester, by James Pearce

For info on how to use it, see https://www.lo-tech.co.uk/wiki/DOS_Disk_Tester

This just adds a simple missing feature: it now deletes the test file at the end of testing.

## Windows Port

This repository now includes a Windows port of the original DOS program, compatible with Visual Studio 2019.

### Building for Windows

To build the Windows version with Visual Studio 2019:

1. Open `DiskTest.sln` in Visual Studio 2019
2. Select your desired configuration (Debug/Release) and platform (x86/x64)
3. Build the solution (Build -> Build Solution or Ctrl+Shift+B)

The executable will be created in the appropriate Debug/Release folder.

### Building for DOS (Original)

To build the original DOS version, you need Turbo Pascal Compiler:

```
make
```

### Usage

The Windows version maintains the same command-line interface as the original DOS version:

- Basic performance test: `disktest.exe`
- Media integrity test: `disktest.exe mediatest`
- Signal quality test: `disktest.exe signaltest`
- Custom size: `disktest.exe size=8M`
- More seeks: `disktest.exe maxseeks`

See the help for complete options: `disktest.exe /?`
