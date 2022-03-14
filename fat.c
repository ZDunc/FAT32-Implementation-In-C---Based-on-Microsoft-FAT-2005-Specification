#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//----------------------------STRUCT DECLARATIONS-------------------------------

// NOTE: We used uint8_t, uint16_t, and uint32_t -- But could have also used
// unsigned char (8 bits), unsigned short (16 bits), and unsigned int (32 bits)
// as suggested by class PPT

// BIOS PARAMETER BLOCK DATA STRUCTURE
// Refer to pages 7-12 of Microsoft FAT Specification document
typedef struct{
  // Boot Sector Fields
  uint8_t BS_jmpBoot[3]; // offset 0
  uint8_t BS_OEMName[8]; // offset 3

  // BIOS Parameter Block Fields
  uint16_t BPB_BytsPerSec; // offset 11
  uint8_t BPB_SecPerClus; // offset 13
  uint16_t BPB_RsvdSecCnt; // offset 14
  uint8_t BPB_NumFATs; // offset 16
  uint16_t BPB_RootEntCnt; // offset 17
  uint16_t BPB_TotSec16; // offset 19
  uint8_t BPB_Media; // offset 21
  uint16_t BPB_FATSz16; // offset 22
  uint16_t BPB_SecPerTrk; // offset 24
  uint16_t BPB_NumHeads; // offset 26
  uint32_t BPB_HiddSec; // offset 28
  uint32_t BPB_TotSec32; // offset 32

  //BPB and BS Fields Specific to FAT32 Volumes
  uint32_t BPB_FATSz32; // offset 36
  uint16_t BPB_ExtFlags; // offset 40
  uint16_t BPB_FSVer; // offset 42
  uint32_t BPB_RootClus; // offset 44
  uint16_t BPB_FSInfo; // offset 48
  uint16_t BPB_BkBootSec; // offset 50
  uint8_t BPB_Reserved[12]; // offset 52
  uint8_t BS_DrvNum; // offset 64
  uint8_t BS_Reserved1; // offset 65
  uint8_t BS_BootSig; // offset 66
  uint32_t BS_VollD; // offset 67
  uint8_t BS_VolLab[11]; // offset 71
  uint8_t BS_FilSysType[8]; // offset 82

} __attribute__((packed)) BPB; // NOTE: Struct is packed to avoid compiler
                               // adding any extra space in memory between
                               // struct fields, as seen in PPT

// DIRECTORY ENTRY STRUCTURE
// Refer to pages 23-24 of Microsoft FAT Specification document
// NOTE: Does not support long directory names
typedef struct {

  uint8_t DIR_Name[11]; // offset 0
  uint8_t DIR_Attr; // offset 11
  uint8_t DIR_NTRes; // offset 12
  uint8_t DIR_CrtTimeTenth; // offset 13
  uint16_t DIR_CrtTime; // offset 14
  uint16_t DIR_CrtDate; // offset 16
  uint16_t DIR_LstAccDate; // offset 18
  uint16_t DIR_FstClusHI; // offset 20
  uint16_t DIR_WrtTime; // offset 22
  uint16_t DIR_WrtDate; // offset 24
  uint16_t DIR_FstClusLO; // offset 26
  uint32_t DIR_FileSize; // offset 28

} __attribute__((packed)) DIR_ENTRY; // NOTE: Fixed Size of 32B

// LONG NAME DIRECTORY ENTRY STRUCTURE
// Refer to pages 30-31 of Microsoft FAT Specification document
// NOTE: Does not support long directory names
typedef struct {

  uint8_t LDIR_Ord; // offset 0
  uint16_t LDIR_Name1[5]; // offset 1
  uint8_t LDIR_Attr; // offset 11
  uint8_t LDIR_Type; // offset 12
  uint8_t LDIR_Chksum; // offset 13
  uint16_t LDIR_Name2[6]; // offset 14
  uint16_t LDIR_FstClusLO; // offset 26
  uint16_t LDIR_Name3[2]; // offset 28

} __attribute__((packed)) LDIR_ENTRY; // NOTE: Fixed Size of 32B

// OPENFILE STRUCTURE -- USED IN GLOBAL OPENFILE_LIST
typedef struct{

  char file[12]; // filename (plus /0 terminator)
  int first_cluster; // firs cluster no.
  char m[2]; // mode -- r, w, rw, or wr
  int offset; // offset (must be <= file size)
} OPENFILE;

//------------------------------GLOBAL VARIABLES--------------------------------

FILE* IMAGEFILE; // Given in argv[1]
BPB BOOT; // Reading in BPB struct, Boot Info, Size consistent at 90 bytes
int FIRST_CLUSTER; // Clusters 0 and 1 are reserved, data starts at 2

int MAX_STACK_SIZE = 50; // Maximum directories supported in stack
int CURRENT_STACK_SIZE = 0; // Current size valid of DIR_STACK entries
int DIR_STACK[51]; // DIR_STACK to keep track of traversing directories

OPENFILE OPENFILE_LIST[101]; // List of OPENFILEs for reading or writing
int OPENFILE_LIST_SIZE = 0; // No. of valid entries in OPENFILE_LIST

//---------------------------PARSER.C DECLARATIONS------------------------------
// PROVIDED CODE FOR PARSING

typedef struct {
int size;
char **items;
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

//--------------------------FUNCTION DECLARATIONS-------------------------------

// HELPER FUNCTIONS-----------------------------------------------

// TRAVERSING THE FAT
int ClusterNo_to_FATOffset(uint32_t cluster_no); // Return IMAGEFILE offset in
                                             // FAT refered to by cluster_no
uint32_t NextClusterNo(uint32_t cluster_no); // Retern next cluster as specified
                                             // in IMAGEFILE's FAT
uint32_t Get_Child_Cluster_No(DIR_ENTRY dir); // Return cluster_no of file/dir
                                              // within DIR_ENTRY struct
uint32_t Find_Free_Cluster(void); // Returns first free cluster no.

// TRAVERSING THE DATA REGION
int ClusterNo_To_DataOffset(uint32_t cluster_no); // Returns offset in data
                             // region of IMAGEFILE refered to by cluster_no
DIR_ENTRY Get_DIR_ENTRY(char* entry, uint32_t cluster_no); // Given filename
                             // & CWD cluster_no, return DIR_ENTRY struct
int Get_DIR_ENTRY_Offset (char* entry, uint32_t cluster_no); // Given filename
              // & CWD, return offset of DIR_ENTRY in data region of IMAGEFILE
int GetFreeEntryOffset(uint32_t cluster_no); // Get data offset in IMAGEFILE
                             // of first empty DIR_ENTRY in a cluster
int DirAlreadyExists(char* dir, uint32_t cluster_no); // Check if directory
                             // exists within CWD cluster of IMAGEFILE
int IsDirEmpty(char* dir, uint32_t cluster_no); // Check if a dir is empty
char* ReadToBuffer(char* buffer, int buffer_size, uint32_t cluster_no);
// Convert cluster no to its data region offset, read to buffer for size bytes


// IMAGEFILE MANIPULATION
void rm_DIR_ENTRY(char* file, uint32_t cluster_no); // Remove DIR_ENTRY in CWD
                                                    // cluster of imagefile
DIR_ENTRY create_newfile(char* file); // Create a new DIR_ENTRY of name file
void UpdateFileSize(char* file, uint32_t cluster_no, int new_size);
              // Update DIR_ENTRYs file size in CWD data region of IMAGEFILE
void UpdateClusterInFAT(uint32_t cluster_no, uint32_t next_cluster);
                             // Update the next cluster a cluster points to in
                                                 // the IMAGEFILE's FAT Region
void AllocateClusterToEmptyFile(DIR_ENTRY current, int offset,
                                  uint32_t cluster_no);
              // Given a free cluster, change a DIR_ENTRY in IMAGEFILE's data
              // region to point to said cluster, change cluster_no in IMAGEFILE
              // FAT Region to point to last cluster in a list (0xFFFFFFFF)
DIR_ENTRY UpdateTwoDotDirectory(DIR_ENTRY entry, uint32_t parent_cluster_no);
              // Update .. DIR_ENTRY to point to parent cluster no.

// OPENFILE_LIST FUNCS
int Get_OPENFILE_Entry(char* filename); // Return index of OPENFILE list entry
void AddToList(uint32_t first_cluster, char* filename, char* mode, int offset);
                                        // Add new entry to OEPNFILE_LIST
int RemoveFromList(char* filename); // If valid filename, remove from list
void PrintList(void); // Print func for debugging

// DIR_STACK HELPER FUNCS
void DIR_push(uint32_t cluster_no); // Push directory cluster_no onto stack
int DIR_pop(void);  // Pop cluster_no from stack
int DIR_peek(void); // Peek at second to last entry of DIR_STACK
                    // Looking at PREVIOUS dir, not current dir

// STRING UTILITIES
char* RemoveWhiteSpaces(char* name); // Remove white spaces in strings for
                                     // proper comparisons
int ModeCheck(char* mode); // Check for valid mode when opening file

// DEBUGGING
void Print_DIR(DIR_ENTRY current); // Print func for debugging
void Print_LDIR(LDIR_ENTRY long_entry); // Print func for debugging

// MAIN FUNCTIONS----------------------------------------------
void info(void); // Parse boot sector & print relevant info
void size(char* file, uint32_t cluster_no); // Print size in bytes of FILE file
void ls_CWD(uint32_t cluster_no); // List contents of CWD
void ls_dirname(char* dirname, uint32_t cluster_no); // List contents of DIRNAME
int cd(char* dir, uint32_t cluster_no); // Change CWD to DIRNAME, 0 return for
                                // failure, For success -- return new cluster_no
void creat(char* file, uint32_t cluster_no, DIR_ENTRY NewFile);
                                // Create FILE file in CWD, size 0 bytes
void mkdir(char* dir, uint32_t cluster_no); // Make directory DIRNAME in CWD
void mv(char* dir1, char* dir2, uint32_t cluster_no); // Move file or dir --
                                                      // mv FROM TO
void mv_rename(char* oldName, char* newName); // Rename file or dir
void open(char* file, char* mode, uint32_t cluster_no); // Opens FILE file in
               // CWD -- open in modes r (read-only), w (write-only), rw, or wr
void close(char* file, uint32_t cluster_no); // Close FILE file
void lseek(char* file, int offset, uint32_t cluster_no); // Set offset of FILE
                                                         // file in bytes
void read(char* file, int size, uint32_t cluster_no); // Read data from FILE
               // file starting at stored offset in open file list and for size
               // bytes and print to screen
void write(char* file, int size, char* string, uint32_t cluster_no);
               // Write "string" to FILE file in CWD at offset
void rm(char* file, uint32_t cluster_no); // Remove FILE file in CWD
void cp(char* file, char* dir, uint32_t cluster_no); // Copy FILE file to a
               // given directory or copy file contents to a new file

// EXTRA CREDIT FUNCTION
void rmdir(char* dir, uint32_t cluster_no); // Remove dir DIRNAME from CWD

//------------------------------------------------------------------------------
//------------------------------------MAIN--------------------------------------

int main(int argc, const char * argv[])
{
  // CHECK FOR VALID USAGE
  if (argc != 2)
  {
    printf("Usage: ./main.x imagename\n");
    return 1; // Program failure
  }

  // OPEN UP IMAGEFILE
  IMAGEFILE=fopen(argv[1],"r+");
  if (IMAGEFILE == NULL){
    printf("Can't Read. Invalid File\n");
    return 1;
  }

  // SET UP BOOT BLOCK
  fread(&BOOT, sizeof(BPB), 1, IMAGEFILE);

  // INFO FOR TRAVERSING THE FAT------------------------------
  int FirstFATSector = BOOT.BPB_RsvdSecCnt;
  int FATSize_in_Bytes = BOOT.BPB_NumFATs * BOOT.BPB_FATSz32 *
                         BOOT.BPB_BytsPerSec;
  int FAT_Offset = (FirstFATSector * BOOT.BPB_BytsPerSec +
                         BOOT.BPB_RootClus * 4);
  // Offset in bytes for cluster N:
  // Offset = FirstFATSector * BootInfo.BPB_BytsPerSec + N * 4;
  // (1) Get the offset of the FAT (in bytes)
  // (2) For a given cluster, find its entry
      // (a) Find the cluster's offset
      // (b) Read the entry (4 bytes) at the offset
      // (c) If the entry is 0x0FFFFFF8 to 0x0FFFFFFFE (or 0xFFFFFFFF)
      //    it means it is the last cluster, STOP
      // (d) If not, the read entry is the next cluster. Repeat
      //-------------------------------------------------------

  // INFO FOR TRAVERSING THE DATA REGION-----------------------
  // CALCULATE FIRST DATA SECTOR (Formula from p. 29 of FAT Spec Document)
  // NOTE: Count of sectors occupied by root dir is always 0 on FAT32 Volumes
  //       according to p. 14, leaving that out of following equation
  // NOTE: FATSz = BPB_FATSz32 on FAT32 Volumes, according to p. 14
  int FirstDataSector = BOOT.BPB_RsvdSecCnt +
                       (BOOT.BPB_NumFATs * BOOT.BPB_FATSz32);
  int Offset = FirstDataSector + (BOOT.BPB_RootClus - 2) *
                    BOOT.BPB_SecPerClus;
  int First_Byte_Offset = Offset * BOOT.BPB_BytsPerSec;

  // Offset for a given cluster N (within the data region)
  // Offset = FirstDataSector + (N - 2) * BootInfo.BPB_SecPerClus;
  //-----------------------------------------------------------

  // POSITION OURSELVES IN THE FIRST USABLE CLUSTER OF FAT
  // KEEP CWD_Cluster_No UPDATED TO KNOW WHERE WE ARE IN FAT & CORRESPONDING
  // DATA REGION
  FIRST_CLUSTER = BOOT.BPB_RootClus;
  uint32_t CWD_Cluster_No = FIRST_CLUSTER;

  // USER INPUT LOOP
  while(1)
  {
    // READ AND PARSE USER INPUT
    printf("> ");
    /* input contains the whole command
       tokens contains substrings from input split by spaces */
    char *input = get_input();
    tokenlist *tokens = get_tokens(input);

    if (tokens->size==0){		// Makes sure no input won't crash program
		continue;
    }

    // COMMAND EXECUTION--------------------------------------------
    // SIMULATES BUILT-IN COMMANDS OF FAT32 MANIPULATION UTILITY

    if (strcmp(tokens->items[0], "exit") == 0)         // exit program
    {
      fclose(IMAGEFILE); // close imagefile

      free(input); // Free malloc'd input and tokens
      free_tokens(tokens);
      break; // break from loop
    }
    else if (strcmp(tokens->items[0], "info") == 0)    // Print boot info
    {
      info();
    }
    else if (strcmp(tokens->items[0], "size") == 0)    // Print file size
    {
      if (tokens->size == 2)
        size(tokens->items[1], CWD_Cluster_No);
      else  // Invaid usage
        printf("Usage: size [filename]\n");
    }
    else if (strcmp(tokens->items[0], "ls") == 0)      // list dir contents
    {
      if (tokens->size == 1)
        ls_CWD(CWD_Cluster_No); // list current working directory
      else if (tokens->size == 2)
        ls_dirname(tokens->items[1], CWD_Cluster_No); // list child/parent dir
      else  // Invalid usage
        printf("Usage: ls [dirname]\n");
    }
    else if (strcmp(tokens->items[0], "cd") == 0)      // change directory
    {
      if (tokens->size == 1) // cd
      {
        CWD_Cluster_No = FIRST_CLUSTER; // set CWD back to root
        CURRENT_STACK_SIZE = 0; // Update DIR_STACK size
      }
      else if (tokens->size == 2) // cd [dirname]
        CWD_Cluster_No = cd(tokens->items[1], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: cd [dirname]\n");
    }
    else if (strcmp(tokens->items[0], "creat") == 0)   // create file
    {
      if (tokens->size==2)
      {
        creat(tokens->items[1], CWD_Cluster_No,
          create_newfile(tokens->items[1]));
      }
      else // Invalid usage
        printf("Usage: creat [filename]\n");
    }
    else if (strcmp(tokens->items[0], "mkdir") == 0)   // mk new directory
    {
      if (tokens->size==2)
        mkdir(tokens->items[1], CWD_Cluster_No);
      else // Invalid usage
        printf("Usage: mkdir [dirname]\n");
    }
    else if (strcmp(tokens->items[0], "mv") == 0)      // move file/ rename
    {
      if (tokens->size == 3)
        mv(tokens->items[1], tokens->items[2], CWD_Cluster_No);
      else // Invalid usage
        printf("Usage: open [FROM] [TO]\n");
    }
    else if (strcmp(tokens->items[0], "open") == 0)    // open file
    {
      if (tokens->size == 3)
        open(tokens->items[1], tokens->items[2], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: open [filename] [mode]\n");
    }
    else if (strcmp(tokens->items[0], "close") == 0)   // close file
    {
      if (tokens->size == 2)
        close(tokens->items[1], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: close [filename]\n");
    }
    else if (strcmp(tokens->items[0], "lseek") == 0)   // change file offset
    {
      if (tokens->size == 3)
      {
        int i;
        sscanf(tokens->items[2], "%d", &i); // convert offset string to int
        lseek(tokens->items[1], i, CWD_Cluster_No);
      }
      else  // Invalid usage
        printf("Usage: lseek [filename] [offset]\n");
    }
    else if (strcmp(tokens->items[0], "read") == 0)    // read from file
    {
      if (tokens->size == 3)
      {
        int i;
        sscanf(tokens->items[2], "%d", &i); // convert size string to int
        read(tokens->items[1], i, CWD_Cluster_No);
      }
      else  // Invalid usage
        printf("Usage: read [filename] [size]\n");
    }
    else if (strcmp(tokens->items[0], "write") == 0)   // write to file
    {
      if (tokens->size >= 4)
      {
        int i;
        sscanf(tokens->items[2], "%d", &i); // convert size string to int

        // CONVERT ALL TOKENS AFTER tokens->items[3] TO ONE GIANT STRING
        //--------------------------------------------------------------
        int len = 0; // string length

        // Iterate through tokens to find desired string length
        for (int j = 3; j < tokens->size; j++)
        {
          // If > 4 tokens, we need to append a space to end of each string
          if ((tokens->size) > 4 && (j != (tokens->size - 1)))
            len += 1; // Need extra char to append space

          // Update total string length
          len += strlen(tokens->items[j]);
        }

        // Allocate char array of size len - 1
        len -= 1; // (removing 2 quote chars & adding 1 '/0' char to terminate)
        char string[len];
        //printf("Length of string: %i\n", len);

        int counter = 0;
        for (int k = 3; k < tokens->size; k++) // Iterate through tokens
        {
          for (int l = 0; l < strlen(tokens->items[k]); l++) // Iterate through
          {                                                  // characters
            if (k == 3 && l == 0) // DO NOT ADD FIRST QUOTE TO STRING
            {
              counter--;
              continue;
            }
            if (k == (tokens->size - 1) && l == (strlen(tokens->items[k]) - 1))
              continue; // DO NOT ADD LAST QUOTE TO STRING

            string[l + counter] = tokens->items[k][l];
          }

          counter += strlen(tokens->items[k]);
          if (((tokens->size) > 4) && (k != (tokens->size - 1)))
          {
            string[counter] = ' ';
            counter++;
          }
        }
        string[len - 1] = '\0';
        //---------------------------------------------------------------

        // Finally, call write function passing in this giant string
        write(tokens->items[1], i, string, CWD_Cluster_No);
      }
      else  // Invalid usage
        printf("Usage: read [filename] [size] [\"string\"]\n");
    }
    else if (strcmp(tokens->items[0], "rm") == 0)      // rm file
    {
      if (tokens->size == 2)
        rm(tokens->items[1], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: rm [filename]\n");
    }
    else if (strcmp(tokens->items[0], "cp") == 0)      // copy file
    {
      if (tokens->size == 3)
        cp(tokens->items[1], tokens->items[2], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: cp [filename] [to]\n");
    }
    else if (strcmp(tokens->items[0], "rmdir") == 0)      // rm directory
    {
      if (tokens->size == 2)
        rmdir(tokens->items[1], CWD_Cluster_No);
      else  // Invalid usage
        printf("Usage: rmdir [dir]\n");
      continue;
    }
    else                                               // Invalid entry
    {
      printf("%s: Command not found.\n", tokens->items[0]);
    }

    free(input); // Free malloc'd input and tokens
    free_tokens(tokens);
  } // END OF USER INPUT LOOP

  // EXIT TRIGGERED
  return 0;
}

//---------------------------FUNCTION DEFINITIONS-------------------------------

//---------------------------parser.c DEFINITIONS-------------------------------
// PROVIDED CODE FOR PARSING

tokenlist *new_tokenlist(void)
{
  tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
  tokens->size = 0;
  tokens->items = (char **) malloc(sizeof(char *));
  tokens->items[0] = NULL; /* make NULL terminated */
  return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
  int i = tokens->size;

  tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
  tokens->items[i] = (char *) malloc(strlen(item) + 1);
  tokens->items[i + 1] = NULL;
  strcpy(tokens->items[i], item);

  tokens->size += 1;
}

char *get_input(void)
{
  char *buffer = NULL;
  int bufsize = 0;

  char line[5];
  while (fgets(line, 5, stdin) != NULL) {
    int addby = 0;
    char *newln = strchr(line, '\n');
    if (newln != NULL)
      addby = newln - line;
    else
      addby = 5 - 1;
      buffer = (char *) realloc(buffer, bufsize + addby);
      memcpy(&buffer[bufsize], line, addby);
      bufsize += addby;

    if (newln != NULL)
      break;
}

  buffer = (char *) realloc(buffer, bufsize + 1);
  buffer[bufsize] = 0;

  return buffer;
}

tokenlist *get_tokens(char *input)
{
  char *buf = (char *) malloc(strlen(input) + 1);
  strcpy(buf, input);

  tokenlist *tokens = new_tokenlist();

  char *tok = strtok(buf, " ");
  while (tok != NULL) {
    add_token(tokens, tok);
    tok = strtok(NULL, " ");
  }

  free(buf);
  return tokens;
}

void free_tokens(tokenlist *tokens)
{
  for (int i = 0; i < tokens->size; i++)
    free(tokens->items[i]);
  free(tokens->items);
  free(tokens);
}

//------------------------------------------------------------------------------
//-----------------------------HELPER FUNCTIONS---------------------------------

//----------------------------TRAVERSING THE FAT--------------------------------

int ClusterNo_to_FATOffset(uint32_t cluster_no)
{
  // First usable sector
  int FirstFATSector = BOOT.BPB_RsvdSecCnt;

  // Calculate offest
  int FAT_Offset = (FirstFATSector * BOOT.BPB_BytsPerSec +
                   cluster_no * 4); // Offset in IMAGEFILE

  return FAT_Offset; // return byte offset
}

uint32_t NextClusterNo(uint32_t cluster_no) // Traverse the FAT to find the next
                                  // cluster, slides 11-12 BPB & commands PPT
{
  int offset = (BOOT.BPB_RsvdSecCnt * BOOT.BPB_BytsPerSec) +
               (cluster_no * 4); // offset in FAT region

  uint32_t entry = 0;
  fseek(IMAGEFILE, offset, SEEK_SET);
  fread(&entry, sizeof(entry), 1, IMAGEFILE); // Read in next cluster_no

  return entry;
}

uint32_t Get_Child_Cluster_No(DIR_ENTRY current)
{
  // Get cluster_no for child directory
  char low[5];
  char high[5];

  sprintf(low, "%04x", current.DIR_FstClusLO);  // Read in high and low
  sprintf(high, "%04x", current.DIR_FstClusHI); // bytes in hex format

  char child_cluster[11] = "0x"; // Concatenate bytes to string
  strcat(child_cluster, high);
  strcat(child_cluster, low);
  //printf("%s\n", child_cluster);

  uint32_t child_cluster_no = 0; // Convert hex string to int
  sscanf(child_cluster, "%x", &child_cluster_no);
  //printf("Located at cluster no. %i\n", child_cluster_no);

  return child_cluster_no;
}

uint32_t Find_Free_Cluster(void) // Returns first free cluster no.
{
  uint32_t cluster_no = BOOT.BPB_RootClus; // Root, first usable cluster no.

  // Size of one FAT
  int FirstFATSizeInBytes = BOOT.BPB_FATSz32 * BOOT.BPB_BytsPerSec;

  // Calculate FAT_Offset of Root
  int FAT_Offset = ClusterNo_to_FATOffset(BOOT.BPB_RootClus);

  //printf("FAT Size: %d\n", BOOT.BPB_FATSz32);
  //printf("Start iterating FAT at offset: %x\n", FAT_Offset);
  fseek(IMAGEFILE, FAT_Offset, SEEK_SET); // Position at offset
  uint32_t cluster; // Read in cluster_no to uint32_t cluster

  // Iterate through FAT until free cluster found
  for (int i = 0; i < FirstFATSizeInBytes; i++)
  {
    fread(&cluster, sizeof(cluster), 1, IMAGEFILE);
    if (cluster == 0x0) // If free cluster found, return cluster_no
    {
      // In case any prior data exists within this cluster, set cluster to 0
      int data_offset = ClusterNo_To_DataOffset(cluster_no);
      uint8_t empty_cluster[BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus];
      for (int i = 0; i < (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus); i++)
        empty_cluster[i] = 0x00;
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fwrite(&empty_cluster, sizeof(empty_cluster), 1, IMAGEFILE);

      return cluster_no;
    }

    cluster_no++;
  }

  printf("Out of memory. Sectors per FAT (BPB_FATSz32) set to: ");
  printf("%i (%i bytes).\n", BOOT.BPB_FATSz32, FirstFATSizeInBytes);
  return -1; // NO FREE CLUSTERS LEFT! Return -1
}

//-------------------------TRAVERSING THE DATA REGION---------------------------

int ClusterNo_To_DataOffset(uint32_t cluster_no)
// Returns offset in data region refered to by cluster_no
{
  int FirstDataSector = BOOT.BPB_RsvdSecCnt +
                       (BOOT.BPB_NumFATs * BOOT.BPB_FATSz32);
  int Offset = FirstDataSector + (cluster_no - 2) *
                    BOOT.BPB_SecPerClus;
  return(Offset * BOOT.BPB_BytsPerSec); // return byte offset
}

DIR_ENTRY Get_DIR_ENTRY(char* entry, uint32_t cluster_no)
{
  DIR_ENTRY current;
  LDIR_ENTRY long_entry;
  int iteration = 0; // How many CLUSTERS traversed, for debugging

  // ITERATE THROUGH DIRECTORY
  do {
    iteration++;

    // FIND OFFSET OF CLUSTER IN THE DATA REGION
    int data_offset = ClusterNo_To_DataOffset(cluster_no);
    int starting_offset = data_offset;

    // CHECK FOR . and .. ENTRIES
    if ((cluster_no != FIRST_CLUSTER) && (iteration == 1))
    {
      DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. have NOT longentry
                                // preceding them
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
      if (strcmp(entry, ".") == 0) // If match found
        return OneDot;
      data_offset += sizeof(OneDot);

      fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      if (strcmp(entry, "..") == 0) // If match found
        return TwoDots;
      data_offset += sizeof(TwoDots);
    }

    // READ IN DIR_ENTRY VALUES UNTIL ALL CLUSTER SPACE HAS BEEN READ IN
    // (Very likely empty space will exist for additional DIR_ENTRYs)
    while (data_offset < ((starting_offset + BOOT.BPB_BytsPerSec)
                                           * BOOT.BPB_SecPerClus)) {

      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);

      int additional_offset = sizeof(long_entry);

      if (long_entry.LDIR_Ord == 0x00) // The remainder of the cluster is empty
        break;

      // IGNORE LONG DIRECTORY NAME ENTRIES
      while (long_entry.LDIR_Ord != 65)
                             // If the last LDIR entry is the first,
                             // it should equal 0x1 & 0x40 = 0x41 (or int 65)
                             // If ignoring LDIR entries indicating long
                             // filename impexitlementation, we will skip ahead
                             // until this field == 65
      {
        fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
        additional_offset += sizeof(long_entry);

        if ((long_entry.LDIR_Ord == 65) && (long_entry.LDIR_Attr != 0xf))
        // NOTE: LDIR_Attr must be set to the following for LDIR entries:
        // ( ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID ) =
        // ( 0x01 | 0x02 | 0x04 | 0x08 ) = 0xf     (equivalent to 15)
                  // In the rare instance we come across an entry whose first
                  // stored value is 65 and who does NOT have an attr equal to
                  // 0xf, it is a actually a DIR_ENTRY starting w/ an "A"
                  // and not an LDIR entry at all
                  // --> "A" = 65 in ASCII code value
                  // This comparison works bc Attr values are stored in offset
                  // 11 on both DIR and LDIR entries; in this case, continue on
        {
          continue;
        }
      }

      fread(&current, sizeof(current), 1, IMAGEFILE); // Read in new DIR_ENTRY

      if (current.DIR_Name[0] == 0xE5) // If dir_entry is deallocated, skip
      {
        data_offset += (sizeof(current) + additional_offset); // update data_off
        continue;
      }

      char* temp = RemoveWhiteSpaces(current.DIR_Name);
      if (strcmp(temp, entry) == 0) // If match found
        return current;

        data_offset += (sizeof(current) + additional_offset); // update data_off
      } // END OF INSIDE WHILE LOOP -- cluster fully read

      cluster_no = NextClusterNo(cluster_no);
    } while(cluster_no < 0x0FFFFFF6); // End loop if final cluster

    // IF UNSUCCESSFUL
    current.DIR_Name[0] = 0x00;
    return current;
}

int Get_DIR_ENTRY_Offset(char* entry, uint32_t cluster_no)
{
  DIR_ENTRY current;
  LDIR_ENTRY long_entry;
  int iteration = 0; // How many CLUSTERS traversed, for debugging

  // ITERATE THROUGH DIRECTORY
  do {
    iteration++;

    // FIND OFFSET OF CLUSTER IN THE DATA REGION
    int data_offset = ClusterNo_To_DataOffset(cluster_no);
    int starting_offset = data_offset;

    // CHECK FOR . and .. ENTRIES
    if ((cluster_no != FIRST_CLUSTER) && (iteration == 1))
    {
      DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. have NOT longentry
                                // preceding them
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
      if (strcmp(entry, ".") == 0) // If match found
        return data_offset;
      data_offset += sizeof(OneDot);

      fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      if (strcmp(entry, "..") == 0) // If match found
        return data_offset;
      data_offset += sizeof(TwoDots);
    }

    // READ IN DIR_ENTRY VALUES UNTIL ALL CLUSTER SPACE HAS BEEN READ IN
    // (Very likely empty space will exist for additional DIR_ENTRYs)
    while (data_offset < ((starting_offset + BOOT.BPB_BytsPerSec)
                                           * BOOT.BPB_SecPerClus)) {

      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);

      int additional_offset = sizeof(long_entry);

      if (long_entry.LDIR_Ord == 0x00) // The remainder of the cluster is empty
        break;

      // IGNORE LONG DIRECTORY NAME ENTRIES
      while (long_entry.LDIR_Ord != 65)
                             // THIS LOOP IS EXPLAINED IN Get_DIR_ENTRY() func
      {
        fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
        additional_offset += sizeof(long_entry);

        if ((long_entry.LDIR_Ord == 65) && (long_entry.LDIR_Attr != 0xf))
          continue;
      }

      fread(&current, sizeof(current), 1, IMAGEFILE); // Read in new DIR_ENTRY

      if (current.DIR_Name[0] == 0xE5) // If dir_entry is deallocated, skip
      {
        data_offset += (sizeof(current) + additional_offset); // update data_off
        continue;
      }

      char* temp = RemoveWhiteSpaces(current.DIR_Name);
      if (strcmp(temp, entry) == 0) // If match found
        return (data_offset + additional_offset);

      data_offset += (sizeof(current) + additional_offset); // update data_off
      } // END OF INSIDE WHILE LOOP -- cluster fully read

      cluster_no = NextClusterNo(cluster_no);
    } while(cluster_no < 0x0FFFFFF6); // End loop if final cluster

    // IF UNSUCCESSFUL
    return 0x0;
}

int GetFreeEntryOffset(uint32_t cluster_no)
{
  //printf("Cluster no. inside GetFreeEntryOffset(): %i\n", cluster_no);

  DIR_ENTRY current;
  LDIR_ENTRY long_entry;
  int iteration = 0; // How many CLUSTERS traversed, for debugging

  // ITERATE THROUGH DIRECTORY
  do {
    iteration++;
    //printf("do/while iteration: %i\n", iteration);
    //printf("cluster_no in do/while: %i\n", cluster_no);

    // FIND OFFSET OF CLUSTER IN THE DATA REGION
    int data_offset = ClusterNo_To_DataOffset(cluster_no);
    int starting_offset = data_offset;

    // CHECK FOR . and .. ENTRIES
    if ((cluster_no != FIRST_CLUSTER) && (iteration == 1))
    {
      DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. have NOT longentry
                                // preceding them
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
      data_offset += sizeof(OneDot);

      fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      data_offset += sizeof(TwoDots);
    }

    // READ IN DIR_ENTRY VALUES UNTIL ALL CLUSTER SPACE HAS BEEN READ IN
    // (Very likely empty space will exist for additional DIR_ENTRYs)

    while (data_offset < ((starting_offset + BOOT.BPB_BytsPerSec)
                                           * BOOT.BPB_SecPerClus)) {

      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);

      int additional_offset = 0;
      int long_offset = sizeof(long_entry);

      if (long_entry.LDIR_Ord == 0x0){ // The remainder of the cluster is empty
        //printf("Cluster no is now %d and offset is ", cluster_no);
        //printf("%d\n", data_offset);
        return data_offset;
	    }

      // IGNORE LONG DIRECTORY NAME ENTRIES
      while (long_entry.LDIR_Ord != 65)
      // THIS LOOP IS EXPLAINED IN Get_DIR_ENTRY() func
      {
        fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
        additional_offset += sizeof(long_entry);

        if ((long_entry.LDIR_Ord == 65) && (long_entry.LDIR_Attr != 0xf))
          continue;
      }

      fread(&current, sizeof(current), 1, IMAGEFILE); // Read in new DIR_ENTRY

      if (current.DIR_Name[0] == 0xE5) { // If dir_entry is deallocated, return
        //printf("Cluster no is now %d and offset is ", cluster_no);
        //printf("%d\n", data_offset + additional_offset);
        return (data_offset + additional_offset);
      }

      // Update data_offset
      data_offset += (sizeof(current) + additional_offset + long_offset);

    } // END OF INSIDE WHILE LOOP -- cluster fully read

    cluster_no = NextClusterNo(cluster_no);
  } while(cluster_no < 0x0FFFFFF6); //


  return -999;	//didn't get a free cluster
}

int DirAlreadyExists(char* dir, uint32_t cluster_no)
{
  if (strcmp(dir, ".") == 0 || strcmp(dir, "..") == 0)
    return 2; // File exists, special case

  DIR_ENTRY current = Get_DIR_ENTRY(dir, cluster_no);
  if (current.DIR_Name[0] != 0x00) // File exists in CWD
    return 1;

  return 0; // File does not yet exist
}

int IsDirEmpty(char* dir, uint32_t cluster_no)
{
  DIR_ENTRY current;
  LDIR_ENTRY long_entry;

  // START TO ITERATE THROUGH DIRECTORY
  // FIND OFFSET OF CLUSTER IN THE DATA REGION
  int data_offset = ClusterNo_To_DataOffset(cluster_no);
  int starting_offset = data_offset;

  if (cluster_no != BOOT.BPB_RootClus)
  {
    // CHECK FOR . and .. ENTRIES
    DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. have NOT longentry
                                  // preceding them
    fseek(IMAGEFILE, data_offset, SEEK_SET);
    fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
    data_offset += sizeof(OneDot);

    fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
    data_offset += sizeof(TwoDots);
  }

  // READ NEXT ENTRY
  fseek(IMAGEFILE, data_offset, SEEK_SET);
  fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
  fseek(IMAGEFILE, data_offset, SEEK_SET);
  fread(&current, sizeof(current), 1, IMAGEFILE);

  if (long_entry.LDIR_Ord == 0x00) // The remainder of the cluster is empty
      return 0; // TRUE -- DIR IS EMPTY
  else
    return 1; // FALSE -- DIR IS NOT EMPTY
}

char* ReadToBuffer(char* buffer, int buffer_size, uint32_t cluster_no)
// Convert cluster no to its data region offset, read to buffer for size bytes
{
  // Position data_offset at file cluster in data region, read for new_size
  // bytes into buffer
  int data_offset = ClusterNo_To_DataOffset(cluster_no);
  fseek(IMAGEFILE, data_offset, SEEK_SET);

  int new_size = buffer_size;
  int offset = 0; // Offset for reading into buffer

  // ITERATE THROUGH CLUSTERS AND READ INTO BUFFER
  while (new_size > (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus))
  // WHILE MORE THAN ONE CLUSTER LEFT TO BE READ
  {
    // READ TO END OF CLUSTER AND PRINT
    int size_to_read = BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus;
    fread(&buffer[offset], size_to_read, 1, IMAGEFILE);

    //printf("\n\nBuffer on iteration %i: %s\n\n\n", iteration, buffer);

    // THEN ADVANCE CLUSTER
    cluster_no = NextClusterNo(cluster_no);
    data_offset = ClusterNo_To_DataOffset(cluster_no);
    fseek(IMAGEFILE, data_offset, SEEK_SET);

    // UPDATE NEW_SIZE and OFFSET
    new_size -= (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
    offset += size_to_read;
  }

  // READ AND PRINT REMAINING (Final cluster)
  fread(&buffer[offset], new_size, 1, IMAGEFILE);
  buffer[buffer_size] = '\0';

  //printf("\n\n\nBuffer before write function: %s\n\n\n", buffer);
}

//---------------------------IMAGEFILE MANIPULATION-----------------------------

void rm_DIR_ENTRY(char* file, uint32_t cluster_no)
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);

  // Delete the DIR_ENTRY in the current directory----------------------

  // Get data_offset of DIR_ENTRY
  int data_offset = Get_DIR_ENTRY_Offset(file, cluster_no);
  if (data_offset == 0x0)
  {
    printf("Error in rm_DIR_ENTRY function.\n");
    return;
  }

  // Postion IMAGEFILE at offset + size of current DIR_ENTRY
  fseek(IMAGEFILE, data_offset + sizeof(current), SEEK_SET);

  // Check if this DIR_ENTRY is the last entry in cluster
  LDIR_ENTRY next;
  fread(&next, sizeof(next), 1, IMAGEFILE);
  if(next.LDIR_Ord = 0x00) // If no more entries afterward
    current.DIR_Name[0] = 0x0; // Set entry to empty
  else
    current.DIR_Name[0] = 0xE5; // Otherwise mark as deallocated

  fseek(IMAGEFILE, data_offset, SEEK_SET);
  fwrite(&current, sizeof(current), 1, IMAGEFILE);
}

DIR_ENTRY create_newfile(char* file)
{
  // Set all necessary fields for program operation
  // As well as all fields that must be set to a specific value
  // According to FAT32 specs
	DIR_ENTRY NewFile;
	NewFile.DIR_FileSize=0;
	NewFile.DIR_FstClusHI=0;
	NewFile.DIR_FstClusLO=0;
  NewFile.DIR_Attr=0x20;
  NewFile.DIR_NTRes=0;
	strcpy(NewFile.DIR_Name,file);
  //printf("%c\n",NewFile.DIR_Name[0]);

  return NewFile;
}

void UpdateFileSize(char* file, uint32_t cluster_no, int new_size)
{
  // Get current DIR_ENTRY and its offset
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int update_offset = Get_DIR_ENTRY_Offset(file, cluster_no);

  // Change current file size and write back to data region of IMAGEFILE
  current.DIR_FileSize = new_size;
  fseek(IMAGEFILE, update_offset, SEEK_SET);
  fwrite(&current, sizeof(current), 1, IMAGEFILE);
}

void UpdateClusterInFAT(uint32_t cluster_no, uint32_t next_cluster)
{
  // Position IMAGEFILE to current cluster in FAT Region
  int FAT_Offset = ClusterNo_to_FATOffset(cluster_no);
  //printf("FAT Offset of current cluster: %x\n", FAT_Offset);
  fseek(IMAGEFILE, FAT_Offset, SEEK_SET);

  // Write next cluster in current cluster's FAT sector
  fwrite(&next_cluster, sizeof(next_cluster), 1, IMAGEFILE);
}

void AllocateClusterToEmptyFile(DIR_ENTRY current, int offset,
                                  uint32_t cluster_no)
// Given a free cluster, change a DIR_ENTRY in IMAGEFILE's data
// region to point to said cluster, change cluster_no in IMAGEFILE
// FAT Region to point to last cluster in a list (0xFFFFFFFF)
{
  // Allocate char arrays for 8 bit byte, high 4 bits and low 4 bits
  char byte[9];
  char low[5];
  char high[5];

  // write cluster number in 8 bit hex format into string byte, null terminate
  sprintf(byte, "%08x", cluster_no);
  byte[8] = '\0';
  //printf("%s\n", byte);

  // Now that bytes string is set, write 4 high bits to high string
  for (int i = 0; i < 4; i++)
    high[i] = byte[i];
  high[4] = '\0';
  // And write four low bits to low string
  for (int j = 4; j < 8; j++)
    low[j - 4] = byte[j];
  low[4] = '\0';
  //printf("%s %s\n", high, low);

  // Declare 16 bit integers for high and low
  uint16_t high_clus;
  uint16_t low_clus;

  // Convert high and low bit strings to ints, write into current DIR_ENTRY
  sscanf(high, "%x", &high_clus);
  sscanf(low, "%x", &low_clus);
  //printf("High Clus: %i\n", high_clus);
  //printf("Low Clus: %i\n", low_clus);
  current.DIR_FstClusHI = high_clus;
  current.DIR_FstClusLO = low_clus;

  // Re-write DIR_ENTRY over old version of DIR_ENTRY in IMAGEFILE data region
  fseek(IMAGEFILE, offset, SEEK_SET);
  fwrite(&current, sizeof(current), 1, IMAGEFILE);
  UpdateClusterInFAT(cluster_no, 0xFFFFFFFF); // Set new cluster to last in list
  //printf("Cluster No. %i set to 0xFFFFFFFF in FAT Region\n", cluster_no);
}

DIR_ENTRY UpdateTwoDotDirectory(DIR_ENTRY entry, uint32_t parent_cluster_no)
// Update .. DIR_ENTRY to point to parent cluster no.
{
  // Allocate char arrays for 8 bit byte, high 4 bits and low 4 bits
  char byte[9], low[5], high[5];
  uint16_t high_clus, low_clus;

  // write cluster number in 8 bit hex format into string byte, null terminate
  sprintf(byte, "%08x", parent_cluster_no);
  byte[8] = '\0';

  // Now that bytes string is set, write 4 high bits to high string
  for (int i = 0; i < 4; i++)
    high[i] = byte[i];
  high[4] = '\0';
  // And write four low bits to low string
  for (int j = 4; j < 8; j++)
    low[j - 4] = byte[j];
  low[4] = '\0';

  // Convert high and low bit strings to ints, write into current DIR_ENTRY
  sscanf(high, "%x", &high_clus);
  sscanf(low, "%x", &low_clus);
  entry.DIR_FstClusHI = high_clus;
  entry.DIR_FstClusLO = low_clus;
}

//-----------------------------OPENFILE_LIST FUNCS------------------------------

int Get_OPENFILE_Entry(char* filename) // Return array index of openfile
{
  // Iterate through list
  for (int i = 0; i < 100; i++)
  {
    if(strcmp(OPENFILE_LIST[i].file, filename) == 0)
      return i; // return entry index
  }

  return -1; // if no index, return -1
}

void AddToList(uint32_t first_cluster, char* filename, char* mode, int offset)
// Add new entry to OEPNFILE_LIST
{
  // UPDATE OPENFILE_LIST
  if (OPENFILE_LIST_SIZE == 100)
  {
    printf("Program set to allow 100 maximum open files. ");
    printf("Readjust program code to continue.\n");
    return;
  }

  // Iterate through list
  for (int i = 0; i < 100; i++)
  {
    if(OPENFILE_LIST[i].first_cluster == 0 &&
        ModeCheck(OPENFILE_LIST[i].m) == 1) // If not allocated
    {
      // Allocate and set struct fields w/ given function parameters
      OPENFILE_LIST[i].first_cluster = first_cluster;
      strcpy(OPENFILE_LIST[i].m, mode);
      OPENFILE_LIST[i].offset = offset;
      strcpy(OPENFILE_LIST[i].file, filename);
      OPENFILE_LIST_SIZE++;
      return;
    }
  }
}

int RemoveFromList(char* filename)
// If valid entry, remove from list
{
  // Iterate through list
  for (int i = 0; i < 100; i++)
  {
    if(strcmp(OPENFILE_LIST[i].file, filename) == 0)
    {
      // Deallocate list entry, return to default settings
      OPENFILE_LIST[i].first_cluster = 0;
      strcpy(OPENFILE_LIST[i].m, "");
      OPENFILE_LIST[i].offset = 0;
      strcpy(OPENFILE_LIST[i].file, "");
      OPENFILE_LIST_SIZE--;
      return 0;
    }
  }

  // If not found anywhere in list
  return 1;
}

void PrintList(void) // For debugging
{
  // Iterate through list
  for (int i = 0; i < 100; i++)
  {
    if(ModeCheck(OPENFILE_LIST[i].m) == 0) // If valid mode, a valid entry must
    {                                      // exist
      printf("Entry at index[%i]:\n", i);
      printf("Filename: %s\n", OPENFILE_LIST[i].file);
      printf("First cluster: %i\n", OPENFILE_LIST[i].first_cluster);
      printf("Mode: %s\n", OPENFILE_LIST[i].m);
      printf("Offset: %i\n\n", OPENFILE_LIST[i].offset);
    }
  }
}

//---------------------------DIRECTORY STACK FUNCS------------------------------

// Very simple stack implementation
void DIR_push(uint32_t cluster_no) // Push directory cluster_no onto stack
{
  if (CURRENT_STACK_SIZE == MAX_STACK_SIZE)
    printf("Error. Directory stack only supports up to 50 entries.\n");
  else
  {
    DIR_STACK[CURRENT_STACK_SIZE] = cluster_no; // push to stack
    CURRENT_STACK_SIZE++; // increment stack size
  }
}

int DIR_pop(void) // Pop cluster_no from stack, return the stored cluster_no
{
  if (CURRENT_STACK_SIZE == 0)
    return FIRST_CLUSTER;
  else if (CURRENT_STACK_SIZE == 1)
  {
    CURRENT_STACK_SIZE--; // decrement stack size
    return FIRST_CLUSTER;
  }
  else
  {
    CURRENT_STACK_SIZE--; // decrement stack size
    int cluster_no = DIR_STACK[CURRENT_STACK_SIZE - 1];
    return cluster_no; // Return PREVIOUS directory's cluster no
  }
}

int DIR_peek(void) // Peek at second to last entry of DIR_STACK
                   // Looking at PREVIOUS dir, not current dir
{
  if (CURRENT_STACK_SIZE == 0 || CURRENT_STACK_SIZE - 1 == 0) // Return root
    return FIRST_CLUSTER;
  else  // Return PREVIOUS directory's cluster no
    return DIR_STACK[CURRENT_STACK_SIZE - 2];
}

//-------------------------------STRING UTILITES--------------------------------

char* RemoveWhiteSpaces(char* name)
{
  // REMOVE TRAILING SPACES AFTER current.DIR_Name -- necessary for comparison
  // Derived from code at:
  // https://www.geeksforgeeks.org/remove-spaces-from-a-given-string/
  int counter = 0;
  char temp[11];
  strcpy(temp, name);

  for (int i = 0; i < 11; i++) {
    if (temp[i] != ' ')
        temp[counter++] = temp[i];
  }
  temp[counter] = '\0';
  name = temp;
  return name;
}

int ModeCheck(char* mode) // Check for valid mode when opening file
{
  if (strcmp(mode, "r") == 0 || strcmp(mode, "w") == 0 ||
      strcmp(mode, "rw") == 0 || strcmp(mode, "wr") == 0 )
    return 0; // valid modes
  else
    return 1; // invalid
}

//----------------------------------DEBUGGING-----------------------------------

void Print_DIR (DIR_ENTRY current) // Print func for debugging
{
  printf("\n");
  printf("%x\n", current.DIR_Attr); // Print all fields in DIR_ENTRY struct
  printf("%i\n", current.DIR_NTRes);
  printf("%i\n", current.DIR_CrtTimeTenth);
  printf("%i\n", current.DIR_CrtTime);
  printf("%i\n", current.DIR_LstAccDate);
  printf("%x\n", current.DIR_FstClusHI);
  printf("%i\n", current.DIR_WrtTime);
  printf("%i\n", current.DIR_WrtDate);
  printf("%x\n", current.DIR_FstClusLO);
  printf("%i\n", current.DIR_FileSize);
}

void Print_LDIR (LDIR_ENTRY long_entry) // Print func for debugging
{
  printf("\n");
  printf("%i\n", long_entry.LDIR_Ord); // Print all fields in LDIR_ENTRY struct
  printf("%c\n", long_entry.LDIR_Name1[0]);
  printf("%c\n", long_entry.LDIR_Name1[1]);
  printf("%c\n", long_entry.LDIR_Name1[2]);
  printf("%c\n", long_entry.LDIR_Name1[3]);
  printf("%c\n", long_entry.LDIR_Name1[4]);
  printf("%x\n", long_entry.LDIR_Attr);
  printf("%i\n", long_entry.LDIR_Type);
  printf("%i\n", long_entry.LDIR_Chksum);
  printf("%c\n", long_entry.LDIR_Name2[0]);
  printf("%c\n", long_entry.LDIR_Name2[1]);
  printf("%c\n", long_entry.LDIR_Name2[2]);
  printf("%c\n", long_entry.LDIR_Name2[3]);
  printf("%c\n", long_entry.LDIR_Name2[4]);
  printf("%c\n", long_entry.LDIR_Name2[5]);
  printf("%x\n", long_entry.LDIR_FstClusLO);
  printf("%s\n", long_entry.LDIR_Name3);
}

//------------------------------------------------------------------------------
//-------------------------------MAIN FUNCTIONS---------------------------------

void info(void) // Parse boot sector & print relevant info
{
  printf("Bytes Per Sector: %d\n", BOOT.BPB_BytsPerSec);
  printf("Sectors Per Cluster: %d\n", BOOT.BPB_SecPerClus);
  printf("Reserved Sector Count: %d\n", BOOT.BPB_RsvdSecCnt);
  printf("Number of FATs: %d\n", BOOT.BPB_NumFATs);
  printf("Total Sectors: %d\n", BOOT.BPB_TotSec32);
  printf("FATsize: %d sectors\n", BOOT.BPB_FATSz32);
  printf("Root Cluster: %d\n", BOOT.BPB_RootClus);
}

void size(char* file, uint32_t cluster_no)
// Print size in bytes of FILE file in CWD
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);

  if (current.DIR_Name[0] == 0x00) // Check if found
    printf("%s not found in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // Check if a directory
    printf("Error. %s is a Directory.\n", file);
  else  // VALID -- print
    printf("%i bytes\n", current.DIR_FileSize);
}

void ls_CWD(uint32_t cluster_no) // List contents of CWD
{
  DIR_ENTRY current;
  LDIR_ENTRY long_entry;
  int iteration = 0; // How many CLUSTERS traversed, for debugging

  // ITERATE THROUGH DIRECTORY
  do {
    iteration++;

    // FIND OFFSET OF CLUSTER IN THE DATA REGION
    int data_offset = ClusterNo_To_DataOffset(cluster_no);
    int starting_offset = data_offset;

    // CHECK FOR . and .. ENTRIES
    if ((cluster_no != FIRST_CLUSTER) && (iteration == 1))
    {
      DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. have NOT longentry
                                // preceding them
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
      printf("%s\n", OneDot.DIR_Name); // Should print .
      data_offset += sizeof(OneDot);

      fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      printf("%s\n", TwoDots.DIR_Name); // Should print ..
      data_offset += sizeof(TwoDots);
    }

    // READ IN DIR_ENTRY VALUES UNTIL ALL CLUSTER SPACE HAS BEEN READ IN
    // (Very likely empty space will exist for additional DIR_ENTRYs)
    while (data_offset < ((starting_offset + BOOT.BPB_BytsPerSec)
                                           * BOOT.BPB_SecPerClus)) {

      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);

      int additional_offset = sizeof(long_entry);

      if (long_entry.LDIR_Ord == 0x00) // The remainder of the cluster is empty
        break;

      // IGNORE LONG DIRECTORY NAME ENTRIES
      while (long_entry.LDIR_Ord != 65)
                             // THIS LOOP IS EXPLAINED IN Get_DIR_ENTRY() func
      {
        fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
        additional_offset += sizeof(long_entry);

        if ((long_entry.LDIR_Ord == 65) && (long_entry.LDIR_Attr != 0xf))
          continue;
      }

      fread(&current, sizeof(current), 1, IMAGEFILE); // Read in new DIR_ENTRY

      if (current.DIR_Name[0] == 0xE5) // If dir_entry is deallocated, skip
      {
        data_offset += (sizeof(current) + additional_offset); // update data_off
        continue;
      }


      if (strcmp((char *)current.DIR_Name, "") != 0) // If DIR_Name not empty,
        printf("%s\n", current.DIR_Name);            // print

      //if (current.DIR_Attr == 0x10)
        //printf("\tIs a directory.\n", current.DIR_Name);

      //Print_LDIR(long_entry);
      //Print_DIR(current);

      //printf("%x\t", current.DIR_FstClusHI);
      //printf("%x\n", current.DIR_FstClusLO);

      data_offset += (sizeof(current) + additional_offset); // update data_off
    } // END OF INSIDE WHILE LOOP -- cluster fully read

    cluster_no = NextClusterNo(cluster_no);
  } while(cluster_no < 0x0FFFFFF6); // End loop if final cluster
}

void ls_dirname(char* dirname, uint32_t cluster_no) // List contents of DIRNAME
{
  if (strlen(dirname) > 11) // long dir names unsupported
  {
    printf("Long directory names unsupported. 11 Character Maximum per ");
    printf("FAT Spec Doc.\n");
    return;
  }

  if (strcmp(dirname, ".") == 0) // if listing current dir
    ls_CWD(cluster_no);
  else if (strcmp(dirname, "..") == 0) // if listing parent dir
    ls_CWD( DIR_peek() );
  else
  {
    // Change directory in entered dirname
    uint32_t new_cluster_no = cd(dirname, cluster_no);
    if (new_cluster_no == cluster_no) // CD was unsuccessful (err printed)
      return;
    else                     // Traverse through new CWD and list contents
      ls_CWD(new_cluster_no);
  }
}

int cd(char* dir, uint32_t cluster_no) // Change CWD to DIRNAME,
                                       // return 0 for failure
{
  if (strcmp(dir, ".") == 0)  // Do not need to change into current directory
  {
    CURRENT_STACK_SIZE = 0;
    return cluster_no;
  }
  if (strcmp(dir, "..") == 0) // Change into parent directory using DIR_STACK
  {
    return DIR_pop();
  }

  DIR_ENTRY current;
  LDIR_ENTRY long_entry;
  int given_cluster = cluster_no; // Keep record of given cluster_no
  int iteration = 0; // How many CLUSTERS traversed, for debugging

  // ITERATE THROUGH DIRECTORY
  do {
    iteration++;

    // FIND OFFSET OF CLUSTER IN THE DATA REGION
    int data_offset = ClusterNo_To_DataOffset(cluster_no);
    int starting_offset = data_offset;

    // CHECK FOR . and .. ENTRIES
    if ((cluster_no != FIRST_CLUSTER) && (iteration == 1))
    {
      DIR_ENTRY OneDot, TwoDots; // dir entries for . and .. do not have
                                 // long_entry preceding them
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
      data_offset += sizeof(OneDot);

      fread(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      data_offset += sizeof(TwoDots);
    }

    // READ IN DIR_ENTRY VALUES UNTIL ALL CLUSTER SPACE HAS BEEN READ IN
    // (Very likely empty space will exist for additional DIR_ENTRYs)
    while (data_offset < ((starting_offset + BOOT.BPB_BytsPerSec)
                                           * BOOT.BPB_SecPerClus)) {

      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);

      int additional_offset = sizeof(long_entry);

      if (long_entry.LDIR_Ord == 0x00) // The remainder of the cluster is empty
        break;

      // IGNORE LONG DIRECTORY NAME ENTRIES
      while (long_entry.LDIR_Ord != 65)
                              // THIS LOOP IS EXPLAINED IN Get_DIR_ENTRY() func
     {
        fread(&long_entry, sizeof(long_entry), 1, IMAGEFILE);
        additional_offset += sizeof(long_entry);

        if ((long_entry.LDIR_Ord == 65) && (long_entry.LDIR_Attr != 0xf))
        // If statement explained in Get_DIR_ENTRY() func
          continue;
     }

      fread(&current, sizeof(current), 1, IMAGEFILE); // Read in new DIR_ENTRY

      if (current.DIR_Name[0] == 0xE5) // If dir_entry is deallocated, skip
      {
        data_offset += (sizeof(current) + additional_offset); // update data_off
        continue;
      }

      char* temp = RemoveWhiteSpaces(current.DIR_Name);

      //printf("%s\n", dir);
      //printf("%s\n", temp);
      //printf("%s\n", current.DIR_Name);

      // IF dir IS FOUND IN THE DIRECTORY-----------------------
      if (strcmp(dir, temp) == 0)
      {
        if (current.DIR_Attr != 0x10) // dir fnd, check if actually a directory
        {
          printf("%s is not a directory.\n", dir);
          return given_cluster; // Cannot change dir, so return initial cluster
        }

        //printf("High byte: %x\t", current.DIR_FstClusHI);
        //printf("Low byte: %x\n", current.DIR_FstClusLO);

        int new_cluster_no = Get_Child_Cluster_No(current);
        DIR_push(new_cluster_no);

        // Return the hex string value as an int
        return new_cluster_no;
      }

      data_offset += (sizeof(current) + additional_offset); // update data_off
    } // END OF INSIDE WHILE LOOP

    cluster_no = NextClusterNo(cluster_no);
  } while(cluster_no < 0x0FFFFFF6);

  // IF WE FINISH LOOP ITERATION -- dir NOT FOUND
  printf("%s not found in current working directory.\n", dir);
  return given_cluster; // Cannot change dir, so return initial cluster
}

void creat(char* file, uint32_t cluster_no, DIR_ENTRY NewFile)
// Create file FILENAME in CWD, size 0 bytes
{
  // Check for valid entry
  if (DirAlreadyExists(file, cluster_no) != 0) {
    printf("Filename already exists.\n");
    return;
  }

  if (strlen(file) > 8) {
		printf("Filename too long. Maximum 8 chararcters supported.\n");
		return;
	}

  LDIR_ENTRY NewFileLongEntry;
  NewFileLongEntry.LDIR_Ord = 65;
  NewFileLongEntry.LDIR_Attr = 0xf;
  NewFileLongEntry.LDIR_FstClusLO = 0;

  // Get data_offset of first free space in CWD cluster

  int data_offset = GetFreeEntryOffset(cluster_no);

  while (NextClusterNo(cluster_no) < 0x0FFFFFF6)
    cluster_no = NextClusterNo(cluster_no);

  //printf("cluster_no: %i\n", cluster_no);

  // If cluster is full allocate new cluster, update data_offset
  if (data_offset == -999)
  {
    // Update FAT
    uint32_t new_cluster = Find_Free_Cluster();
    if (new_cluster == -1) // NO MORE MEMORY
      return;
    UpdateClusterInFAT(cluster_no, new_cluster);
    UpdateClusterInFAT(new_cluster, 0xFFFFFFFF);

    data_offset = ClusterNo_To_DataOffset(new_cluster);
  }

  // Write LDIR and DIR Entries to data_offset
  fseek(IMAGEFILE, data_offset, SEEK_SET);
  fwrite(&NewFileLongEntry, sizeof(NewFileLongEntry), 1, IMAGEFILE);
  fwrite(&NewFile, sizeof(NewFile), 1, IMAGEFILE);
}

void mkdir(char* dir, uint32_t cluster_no) // Make directory DIRNAME in CWD
{
  // All checks for valid entry covered within creat() function call

  // Create new_directory DIR_ENTRY and set its attributes to DIR
  DIR_ENTRY new_directory = create_newfile(dir);
  new_directory.DIR_Attr = 0x10;

  // Find first free cluster for new_directory in FAT
  uint32_t new_cluster = Find_Free_Cluster();
  if (new_cluster == -1) // NO MORE MEMORY
    return;

  // Add new_directory DIR_ENTRY to CWD
  creat(dir, cluster_no, new_directory);

  // Allocate cluster to new_directory (update FrstClusHI & FrstClusLO)
  // (And set new cluster's FAT offset to 0xFFFFFFFF)
  int data_offset = Get_DIR_ENTRY_Offset(dir, cluster_no);
  //printf("Offset of current dir: %i\n", data_offset);
  AllocateClusterToEmptyFile(new_directory, data_offset, new_cluster);

  // Write . and .. entries at new_cluster's data_offset
  data_offset = ClusterNo_To_DataOffset(new_cluster);
  //printf("Offset of newly allocated dir: %i\n", data_offset);
  DIR_ENTRY OneDot = create_newfile(".");
  DIR_ENTRY TwoDots = create_newfile("..");

  if (cluster_no != BOOT.BPB_RootClus) // If not starting in root
  // SET DIR_FstClusHI and DIR_FstClusLO for .. to current dir's first cluster
  // (which is actually cluster_no)
    TwoDots = UpdateTwoDotDirectory(TwoDots, cluster_no);

  fseek(IMAGEFILE, data_offset, SEEK_SET);
  fwrite(&OneDot, sizeof(OneDot), 1, IMAGEFILE);
  fwrite(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
}

void mv(char* dir1, char* dir2, uint32_t cluster_no)
// Move file or dir -- mv FROM TO
{
  // Check if dir1 is . or ..
  if (strcmp(dir1, ".") == 0 || strcmp(dir1, "..") == 0)
  {
    printf("%s cannot be moved into another directory.\n", dir1);
    return;
  }
  // Check if dir 2 is . or ..
  if (strcmp(dir2, ".") == 0)
    return;
  if (strcmp(dir2, "..") == 0)
  {
    printf("Feature unsupported.\n");
    return;
  }

  // If file is open, close before moving
  int entry_index = Get_OPENFILE_Entry(dir1);
  if (entry_index != -1) // check if file is open
    close(dir1, cluster_no);

  // Check if dir1 exists in CWD
  DIR_ENTRY to_move = Get_DIR_ENTRY(dir1, cluster_no);
  if (to_move.DIR_Name[0] == 0x00) // If it does not, there is no entry to move
  {
    printf("%s not found in CWD.\n", dir1);
    return;
  }

  // Check if dir2 exists in CWD
  DIR_ENTRY destination = Get_DIR_ENTRY(dir2, cluster_no);

  if (destination.DIR_Name[0] != 0x00 && destination.DIR_Attr == 0x10)
  // If dir2 found in CWD and it is a directory
  // Then move dir1 (a file OR another dir) to dir2
  {
    uint32_t new_cluster_no = cd(dir2, cluster_no);
    DIR_ENTRY check = Get_DIR_ENTRY(dir1, new_cluster_no);

    // Check if dir2 exists in child directory
    if (check.DIR_Name[0] != 0x00)
    {
        cluster_no = cd("..", new_cluster_no);
        printf("The name is already being used by another file.\n", dir2);
        return;
    }

    rm_DIR_ENTRY(dir1, cluster_no);
    creat(dir1, new_cluster_no, to_move);

    if (to_move.DIR_Attr == 0x10)
    // If moving a directory, need to update its .. entry to refelct new parent
    {
      uint32_t child_cluster = cd(dir1, new_cluster_no);
      DIR_ENTRY TwoDots = Get_DIR_ENTRY("..", child_cluster);
      TwoDots = UpdateTwoDotDirectory(TwoDots, new_cluster_no);
      int data_offset = Get_DIR_ENTRY_Offset("..", child_cluster);
      fseek(IMAGEFILE, data_offset, SEEK_SET);
      fwrite(&TwoDots, sizeof(TwoDots), 1, IMAGEFILE);
      cluster_no = cd("..", child_cluster);
    }
    cluster_no = cd("..", new_cluster_no);
  }
  else if (destination.DIR_Name[0] != 0x00)
  // If dir2 found in CWD and it is a file that already exists
    printf("The name is already being used by another file.\n", dir2);
  else
  // If dir2 NOT FOUND in CWD, rename file
  {
    strcpy(to_move.DIR_Name, dir2);
    int data_offset = Get_DIR_ENTRY_Offset(dir1, cluster_no);
    fseek(IMAGEFILE, data_offset, SEEK_SET);
    fwrite(&to_move, sizeof(to_move), 1, IMAGEFILE);
  }
}

void open(char* file, char* mode, uint32_t cluster_no) // Opens FILE file in CWD,
                    // and adds it to the OPENFILE_LIST,
                    // open in modes r (read-only), w (write-only), rw, or wr
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s not found in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else if (ModeCheck(mode) == 1) // check for valid mode
    printf("Invalid mode entered. Acceptable modes: r, w, rw, or wr.\n");
  else if ((current.DIR_Attr == 0x01) && (mode != "r")) // check if read-only
  // If file permission set to read only
    printf("File permissions set to read-only. Retry opening w/ mode r.\n");
  else if (entry_index != -1) // files not in list will return an index of -1
    printf("File is already open.\n");
  else
  {
    // Get file cluster number specifed in dir entry
    int file_cluster_no = Get_Child_Cluster_No(current);
    AddToList(file_cluster_no, file, mode, 0);
  }

}

void close(char* file, uint32_t cluster_no) // Close FILE file
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s not found in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else // VALID -- remove from OPENFILE_LIST
  {
    // Get file cluster number specifed in dir entry
    int file_cluster_no = Get_Child_Cluster_No(current);

    if (RemoveFromList(file) != 0)
      printf("%s is not open.\n", file);
  }
}

void lseek(char* file, int offset, uint32_t cluster_no) // Set offset (in bytes)
                                       // of FILENAME given CWD cluster_no
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s not found in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else if (offset > current.DIR_FileSize) // check if offset is too large
    printf("Error. Offset entered is greater than file size.\n");
  else if (entry_index == -1) // check if file is open
    printf("Error. File is not open.\n");
  else if ( strcmp(OPENFILE_LIST[entry_index].m, "w") == 0) // check mode
    printf("Error. File not open for reading.\n");
  else // VALID -- update offset
    OPENFILE_LIST[entry_index].offset = offset;
}

void read(char* file, int size, uint32_t cluster_no)
    // Read data from FILE file starting at stored offset in open file list
    // for size bytes, print to screen
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s does not exist in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else if (entry_index == -1) // check if file is open
    printf("Error. File is not open.\n");
  else if ( strcmp(OPENFILE_LIST[entry_index].m, "w") == 0) // check mode
    printf("Error. File not open for reading.\n");
  else if (OPENFILE_LIST[entry_index].offset == current.DIR_FileSize)
    printf("Offset set to end of file. Nothing left to read.\n");
  else // VALID -- read file for size bytes starting at offset
  {
    // Get 1st cluster_no of file
    uint32_t first_cluster = OPENFILE_LIST[entry_index].first_cluster;
    uint32_t cluster = first_cluster;

    int offset = OPENFILE_LIST[entry_index].offset; // get file offset

    // Check if size entered is larger than what can be read, adjust if needed
    int maximum_read = current.DIR_FileSize - offset;
    if (size > maximum_read)
      size = maximum_read;

    int given_size = size; // SAVE SIZE TO UPDATE OPENFILE_LIST ENTRY LATER

    // While offset is larger than bytes per cluster, advance clusters
    while (offset > (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus))
    {
      cluster = NextClusterNo(cluster);
      offset -= (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
    }

    // Then, get data_offset from cluster
    int data_offset = ClusterNo_To_DataOffset(cluster);
    data_offset += offset;
    fseek(IMAGEFILE, data_offset, SEEK_SET); // set IMAGEFILE to data offset

    char buffer[size + 1]; // Allocate buffer, read IMAGEFILE into buffer

    while ((offset + size) > (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus))
    // WHILE MORE THAN ONE CLUSTER LEFT TO BE READ
    {
      // READ TO END OF CLUSTER AND PRINT
      int size_to_read = BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus;
      size_to_read -= offset;
      fread(&buffer, size_to_read, 1, IMAGEFILE);

      buffer[size_to_read] = '\0';
      for (int i = 0; i < size_to_read; i++)
        printf("%c", buffer[i]);

      // THEN ADVANCE CLUSTER
      cluster = NextClusterNo(cluster);
      data_offset = ClusterNo_To_DataOffset(cluster);
      fseek(IMAGEFILE, data_offset, SEEK_SET);

      // UPDATE SIZE, OFFSET WILL BE 0 AFTER 1ST ITERATION
      size -= ((BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus) - offset);
      offset = 0;
    }

    // READ AND PRINT REMAINING
    fread(&buffer, size, 1, IMAGEFILE);
    buffer[size] = '\0';
    for (int i = 0; i < size; i++)
      printf("%c", buffer[i]);

    printf("\n");

    // FINALLY, UPDATE OFFSET IN OPENFILE_LIST ENTRY
    OPENFILE_LIST[entry_index].offset += given_size;
  }
}

void write(char* file, int size, char* string, uint32_t cluster_no)
// Write to file FILENAME in CWD
// ASSUMES STRING ALWAYS ENTERED IN QUOTES
{
  //printf("String: %s\n", string);

  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s does not exist in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else if (entry_index == -1) // check if file is open
    printf("Error. File is not open.\n");
  else if ( strcmp(OPENFILE_LIST[entry_index].m, "r") == 0) // check mode
    printf("Error. File not open for writing.\n");
  else // VALID -- read file for size bytes starting at offset
  {
    // Adjust string to given size-----------------------------------
    char to_write[size + 1];

    for (int i = 0; i < size + 1; i++)
    {
      if (i <= strlen(string)) // Copy string to to_write[]
      {
        to_write[i] = string[i];
        continue;
      }
      to_write[i] = '\0'; // If string is fully copied, fill rest w/ null chars
    }
    to_write[size] = '\0';
    //printf("to_write string: %s\n", to_write);

    // Get 1st cluster_no of file------------------------------------
    uint32_t first_cluster = OPENFILE_LIST[entry_index].first_cluster;
    uint32_t current_cluster = first_cluster;
    int offset = OPENFILE_LIST[entry_index].offset;
    int initial_offset = offset;
    int initial_size = size;
    int size_written = 0; // no. of bytes already written from string

    if (first_cluster == 0) // If cluster not yet allcated, must allocate now
    {
      // Find first available cluster
      first_cluster = Find_Free_Cluster();
      current_cluster = first_cluster;
      if (first_cluster == -1) // NO MORE MEMORY
        return;
      OPENFILE_LIST[entry_index].first_cluster = first_cluster;
      int data_offset = Get_DIR_ENTRY_Offset(file, cluster_no);
      AllocateClusterToEmptyFile(current, data_offset, first_cluster);
    }

    int data_offset = ClusterNo_To_DataOffset(current_cluster);

    // While offset is larger than bytes per cluster, advance clusters
    while (offset > (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus))
    {
      current_cluster = NextClusterNo(current_cluster);
      data_offset = ClusterNo_To_DataOffset(current_cluster);
      offset -= (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
    } // OFFSET SHOULD NOW BE < 512 bytes

    // Determine if the file size is to be increased
    if ((initial_offset + size) > current.DIR_FileSize)
    {
      // Determine how many new clusters need to be allocated
      int current_clusters = current.DIR_FileSize /
        (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
      int final_clusters = (initial_offset + size) /
        (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
      int new_clusters = final_clusters - current_clusters;

      if (new_clusters == 0) // If string fully written, end here
      {
        // Write size bytes
        fseek(IMAGEFILE, data_offset + offset, SEEK_SET);
        fwrite(&to_write, size, 1, IMAGEFILE);

        // Update variables
        UpdateFileSize(file, cluster_no, (initial_offset + initial_size));
        OPENFILE_LIST[entry_index].offset = (initial_offset + initial_size);
        return;
      }

      // Else, 1 or more new clusters to allocate
      // Write to end of current cluster
      fseek(IMAGEFILE, data_offset + offset, SEEK_SET);
      int bytes_to_write = BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus;
      bytes_to_write -= offset;
      if (bytes_to_write > size)
        bytes_to_write = size;
      fwrite(&to_write, bytes_to_write, 1, IMAGEFILE);
      size_written+= bytes_to_write;
      //printf("%s\n", to_write);

      // UPDATE SIZE, OFFSET WILL BE 0 FROM HERE ON
      size -= ((BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus) - offset);
      //printf("Size written: %i\n", size_written);
      //printf("Size left to write: %i\n", size);
      offset = 0;

      uint32_t next_cluster = 0;
      while (new_clusters > 0)
      {
        // Find first available cluster
        next_cluster = Find_Free_Cluster();
        //printf("Next cluster: %i\n", next_cluster);
        if (next_cluster == -1) // NO MORE MEMORY
        {
          OPENFILE_LIST[entry_index].offset = (initial_offset + initial_size
                                                                     - size);
          UpdateFileSize(file, cluster_no, (initial_offset + initial_size
                                                                     - size));
          return;
        }

        // Link current_cluster to next_cluster in FAT
        UpdateClusterInFAT(current_cluster, next_cluster);
        UpdateClusterInFAT(next_cluster, 0xFFFFFFFF);

        // Advance current cluster, Postion IMAGEFILE to new cluster in Data Reg
        current_cluster = NextClusterNo(current_cluster);
        //printf("Advanced cluster no.: %i\n", current_cluster);
        data_offset = ClusterNo_To_DataOffset(current_cluster);
        fseek(IMAGEFILE, data_offset, SEEK_SET);

        if (new_clusters > 1)
        {
          fwrite(&to_write[size_written - 1],
            (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus), 1, IMAGEFILE);
          size_written += (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
          size -= (BOOT.BPB_BytsPerSec * BOOT.BPB_SecPerClus);
          //printf("Size written: %i\n", size_written);
          //printf("Size left to write: %i\n", size);
        }
        if (new_clusters == 1)
        {
          fwrite(&to_write[size_written - 1], size, 1, IMAGEFILE);
          size_written += size;
          size -= size;
          //printf("Size written: %i\n", size_written);
          //printf("Size left to write: %i\n", size);
        }

        // Adjust new_cluster variables
        new_clusters--;
      } // END OF WHILE LOOP

      // Update file size
      UpdateFileSize(file, cluster_no, (initial_offset + initial_size));
    } // END OF IF STATEMENT
    else // Simple case -- no new clusters to allocate & size < file size
    {
      // Write size bytes
      fseek(IMAGEFILE, data_offset + offset, SEEK_SET);
      fwrite(&to_write, size, 1, IMAGEFILE);
      //printf("%s\n", to_write);
    }

    // Update Offset
    OPENFILE_LIST[entry_index].offset = (initial_offset + initial_size);
  }
}

void rm(char* file, uint32_t cluster_no)
// Remove FILE file given CWD cluster_no
{
  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
  {
    printf("%s does not exist in current working directory.\n", file);
    return;
  }
  else if (current.DIR_Attr == 0x10) // check if file is a dir
  {
    printf("Error. %s is a Directory.\n", file);
    return;
  }
  else if (entry_index != -1) // check if file is open, if it is, CLOSE IT
    close(file, cluster_no);


  // Get 1st cluster_no of file
  uint32_t first_cluster = Get_Child_Cluster_No(current);
  uint32_t current_cluster = first_cluster;

  // Traverse FAT and deallocate all CLUSTERS---------------------------

  uint32_t entry = 0x0; // Empty entry used to deallocate cluster
  int next_cluster = current_cluster;

  do {
    next_cluster = NextClusterNo(current_cluster);

    if (next_cluster >= 0x0FFFFFF6)
      break;

    // Calculate FAT offset (in bytes):
    int FAT_Offset = ClusterNo_to_FATOffset(current_cluster);

    fseek(IMAGEFILE, FAT_Offset, SEEK_SET);
    fwrite(&entry, sizeof(entry), 1, IMAGEFILE); // Write to IMAGEFILE

    current_cluster = next_cluster; // store next cluster

  } while(1); // End loop if final cluster

  // Deallocate final cluster
  int FAT_Offset = ClusterNo_to_FATOffset(current_cluster);
  fseek(IMAGEFILE, FAT_Offset, SEEK_SET);
  fwrite(&entry, sizeof(entry), 1, IMAGEFILE); // Write to IMAGEFILE

  // Remove the DIR_ENTRY from the CWD
  rm_DIR_ENTRY(file, cluster_no);
}

void cp(char* file, char* dir, uint32_t cluster_no)
// Copy file FILENAME to specified directory
{
  // Special case
  if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
  {
    printf(". and .. cannot be copied\n", file);
    return;
  }

  DIR_ENTRY current = Get_DIR_ENTRY(file, cluster_no);
  DIR_ENTRY destination = Get_DIR_ENTRY(dir, cluster_no);
  int entry_index = Get_OPENFILE_Entry(file);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s does not exist in current working directory.\n", file);
  else if (current.DIR_Attr == 0x10) // check if file is a dir
    printf("Error. %s is a Directory.\n", file);
  else if (destination.DIR_Name[0] != 0x00 && destination.DIR_Attr != 0x10)
  // If destination exists and is a file, NOT a dir
    printf("Error. Cannot copy to a file that already exists.\n", dir);
  else if (entry_index != -1) // check if file is open, if it is, CLOSE IT
  {
    close(file, cluster_no);
    printf("Notice: file [%s] was closed before copying.\n", file);
  }
  else if (destination.DIR_Name[0] != 0x00) // VALID CASE
  // If destination exists in CWD and it IS a directory
  // Copy file to destination
  {
    // Get first cluster of original file
    uint32_t parent_cluster = Get_Child_Cluster_No(current);

    // Change dir, write new DIR_ENTRY in child directory of name file
    uint32_t new_cluster_no = cd(dir, cluster_no);
    DIR_ENTRY check = Get_DIR_ENTRY(file, new_cluster_no);
    if (check.DIR_Name[0] != 0x00)
    {
      printf("File %s already exists in dir %s\n", file, dir);
      return;
    }

    creat(file, new_cluster_no, create_newfile(file));

    // Create buffer of appropriate file size and read to it
    int new_size = current.DIR_FileSize;
    //printf("File size is: %i\n", new_size);
    char buffer[new_size + 1];
    ReadToBuffer(buffer, new_size, parent_cluster);

    // Write buffer into copied file
    open(file, "w", new_cluster_no);
    write(file, current.DIR_FileSize, buffer, new_cluster_no);
    close(file, new_cluster_no);

    // Return to previous directory
    cd("..", new_cluster_no);
  }
  else // VALID CASE
  // If destination does not exist, creat newfile and copy file contents to it
  {
    // Get first cluster of original file
    uint32_t parent_cluster = Get_Child_Cluster_No(current);

    creat(dir, cluster_no, create_newfile(dir));

    // Create buffer of appropriate file size and read to it
    int new_size = current.DIR_FileSize;
    //printf("File size is: %i\n", new_size);
    char buffer[new_size + 1];
    ReadToBuffer(buffer, new_size, parent_cluster);

    // Write buffer into copied file
    open(dir, "w", cluster_no);
    write(dir, current.DIR_FileSize, buffer, cluster_no);
    close(dir, cluster_no);
  }
}

//----------------------------EXTRA CREDIT FUNCTION-----------------------------

void rmdir(char* dir, uint32_t cluster_no) // Remove dir DIRNAME from CWD
{
  DIR_ENTRY current = Get_DIR_ENTRY(dir, cluster_no);
  uint32_t first_cluster = Get_Child_Cluster_No(current);

  if (current.DIR_Name[0] == 0x00) // check if file exists
    printf("%s does not exist in current working directory.\n", dir);
  else if (current.DIR_Attr != 0x10) // check if file is a dir
    printf("Error. %s is not a Directory.\n", dir);
  else if (IsDirEmpty(dir, first_cluster) == 1)
    printf("Directory is not empty.\n");
  else // VALID CASE
  {
    // Traverse FAT and deallocate all clusters for child dir
    uint32_t current_cluster = first_cluster;

    int data_offset = ClusterNo_to_FATOffset(cluster_no);

    while (NextClusterNo(current_cluster) < 0x0FFFFFF6)
    {
      int previous_cluster = current_cluster;
      current_cluster = NextClusterNo(current_cluster);
      UpdateClusterInFAT(previous_cluster, 0x00000000);
    }
    UpdateClusterInFAT(current_cluster, 0x00000000);

    // Then delete the DIRENTRY from the current directory
    rm_DIR_ENTRY(dir, cluster_no);
  }
}
