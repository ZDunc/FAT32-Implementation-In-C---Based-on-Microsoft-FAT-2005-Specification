# FAT32 Implementation (Based on Microsoft FAT 2005 Specification)

## PROGRAM INCENTIVE
      Familiarization with file system design and implementation using C langauge, given the 2005 Microsoft 
      FAT Specification document. A few concepts of the FAT32 file system necessary for implementation 
      include endianness, FAT tables, cluser-based storage, sectors, and directory structure.
      
## PROGRAM DESCRIPTION
      Design and implement a shell-like, user-space utility capable of interpreting and manipulating an FAT32
      system imagefile. The program should interpret specified commands in order to manipulate the imagefile,
      should not corrupt the imagefile from its proper operations, and should be robust. You may NOT reuse 
      any kernel file system code or copy code from other file system utilities. 

      Your program will take the imagefile path name as an argument and will then read and write to it 
      according to command implementations. You can check the validity of the imagefile by mounting it 
      with the loop back option and using tools like Hexedit.  

      You will also be responsibe for handling various errors. When an error presents itself, print a 
      descriptive message detailing the error. However, the program must continue running. The state 
      of the system should remain unchanged as before the command was called.
      
## REPOSITORY CONTENTS
- fat.cpp
- Makefile
- Microsoft Specification Document PDF
