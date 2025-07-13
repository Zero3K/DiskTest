# DiskTest Windows Port - Usage Examples

This document provides examples of how to use the Windows port of DiskTest.

## Basic Performance Test

Run a basic performance test with default settings (4MB file, 256 seeks):

```cmd
disktest.exe
```

## Custom File Size

Test with a specific file size:

```cmd
disktest.exe size=16M
```

## More Intensive Testing

Test with more random seeks for better statistical accuracy:

```cmd
disktest.exe size=8M maxseeks
```

## Media Integrity Testing

Test for media errors with pattern testing:

```cmd
disktest.exe mediatest
```

## Signal Quality Testing

Interactive signal quality testing for hardware development:

```cmd
disktest.exe signaltest
```

## Read-Only Testing

Test an existing test file without creating a new one:

```cmd
disktest.exe readonly
```

## Quiet Operation

Suppress progress indicators during testing:

```cmd
disktest.exe noprogress
```

## Combined Options

You can combine multiple options:

```cmd
disktest.exe size=32M highseeks noprogress
```

## Help

Get help on all available options:

```cmd
disktest.exe /?
```

## Output Example

```
DiskTest, by James Pearce & Foone Turing. Windows Version 2.5 (Windows)

Preparing drive...

Configuration: 4096 KB test file, 256 IOs in random tests.

Write Speed         : 45234.67 KB/s
Read Speed          : 89123.45 KB/s
8K random, 70% read : 234.5 IOPS
Sector random read  : 189.2 IOPS

Average access time (includes latency and file system overhead), is 5 ms.

Deleting TEST$$$.FIL.
```