#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include "Constants.h"
#include <math.h>

// Superblock entry
struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_size;
};

// Time and date entry
struct __attribute__((__packed__)) dir_entry_timedate_t {
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
};
// Directory entry 
struct __attribute__((__packed__)) dir_entry_t {
  uint8_t                       status;
  uint32_t                      starting_block;
  uint32_t                      block_count;
  uint32_t                      size;
  struct   dir_entry_timedate_t create_time;
  struct   dir_entry_timedate_t modify_time;
  uint8_t                       filename[31];
  uint8_t                       unused[6];
};
//part 1 function
void diskinfo(int argc, char* argv[]){
    FILE * pFile;
    pFile = fopen (argv[1], "rb");
    
    if(!pFile){
        printf("File not found.\n");
        exit(0);
    }
    
    fseek(pFile, 0, SEEK_SET);//move to the start of the file
    
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, pFile);//read superblock since it is the first block
    
    //print superblock information
    printf("Super block information:\n");
    printf("Block size: %d\n", ntohs(sb.block_size));
    
    printf("Block count: %d\n", htonl(sb.file_system_block_count));
    
    printf("FAT starts: %d\n", htonl(sb.fat_start_block));
    printf("FAT blocks: %d\n", htonl(sb.fat_block_count));
    printf("Root directory start: %d\n", htonl(sb.root_dir_start_block));
    printf("Root directory blocks: %d\n", htonl(sb.root_dir_block_size));
    
    //get the position of FAT table
    int fat_startblock = htonl(sb.fat_start_block);
    int fat_num_blocks = htonl(sb.fat_block_count);
    
    //move to the start of FAT table
    fseek(pFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
    int num_entries = DEFAULT_BLOCK_SIZE / FAT_ENTRY_SIZE * fat_num_blocks;
    
    uint32_t number;//used to store the number of FAT entries
    int num_freeblocks = 0;//used to count number of free blocks
    int num_reservedblocks = 0;//used to count number of reserved blocks
    for(int i = 0; i < num_entries; i++){
        fread(&number, sizeof(uint32_t), 1, pFile);

        if(number == FAT_FREE)
            num_freeblocks++;
        else if(htonl(number) == FAT_RESERVED)
            num_reservedblocks++;
    }
    //print FAT table information
    printf("\nFAT information: \n");
    printf("Free Blocks: %d\n", num_freeblocks);
    printf("Reserved Blocks: %d\n", num_reservedblocks);
    printf("Allocated Blocks: %d\n", num_entries - num_freeblocks - num_reservedblocks);

    fclose(pFile);
}
//part 2 function
void disklist(int argc, char* argv[]){
    FILE * pFile;
    pFile = fopen (argv[1], "rb");

    if(!pFile){
        printf("File not found.\n");
        exit(0);
    }
    fseek(pFile, 0, SEEK_SET);
    
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, pFile);//read the super block
    
    //get root directory information
    int root_dir_start_block = htonl(sb.root_dir_start_block);
    int root_dir_block_size = htonl(sb.root_dir_block_size);
    int num_entries = root_dir_block_size * 8;//compute the number of entries in root directory blocks
    fseek(pFile, root_dir_start_block * DEFAULT_BLOCK_SIZE, SEEK_SET);//move to the start of root directory block
    
    struct dir_entry_t de;//used to store the directory entry information
    for(int i = 0; i < num_entries; i++){
        fread(&de, sizeof(struct dir_entry_t), 1, pFile);//read directory entry 

        //if the entry is a file entry
        if( (de.status & DIRECTORY_ENTRY_FILE) != 0){

            struct   dir_entry_timedate_t modify_time = de.modify_time;//used to store modify time information
            //print file information
            printf("F %10d %30s %04d/%02d/%02d %02d:%02d:%02d\n", htonl(de.size), (char *)(de.filename),
                  ntohs(modify_time.year), modify_time.month, modify_time.day,
                  modify_time.hour, modify_time.minute, modify_time.second);
        }
        //if the entry is a directory entry
        else if( (de.status & DIRECTORY_ENTRY_DIRECTORY) != 0){
            struct   dir_entry_timedate_t modify_time = de.modify_time;//used to store modify time information
            //print file information
            printf("D %10d %30s %04d/%02d/%02d %02d:%02d:%02d\n", htonl(de.size), (char *)(de.filename),
                  ntohs(modify_time.year), modify_time.month, modify_time.day,
                  modify_time.hour, modify_time.minute, modify_time.second);
        }
    }
    fclose(pFile);
}
//part 3 function
void diskget(int argc, char* argv[]){
    FILE * pFile;
    pFile = fopen (argv[1], "rb");
    
    if(!pFile){
        printf("File not found.\n");
        exit(0);
    }
    
    FILE *oFile;
    oFile  = fopen (argv[2], "w+");
    
    fseek(pFile, 0, SEEK_SET);
    
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, pFile);//read superblock
    
    int fat_startblock = htonl(sb.fat_start_block);
    
    int root_dir_start_block = htonl(sb.root_dir_start_block);
    int root_dir_block_size = htonl(sb.root_dir_block_size);
    int num_entries = root_dir_block_size * 8;
    
    //find the staring block and number of blocks of the file
    fseek(pFile, root_dir_start_block * DEFAULT_BLOCK_SIZE, SEEK_SET);//move to the root directory block
    
    struct dir_entry_t de;
    int fileFound = 0;
    int starting_block;
    int block_count;
    for(int i = 0; i < num_entries; i++){
        fread(&de, sizeof(struct dir_entry_t), 1, pFile);//read the entry into de

        //check if the entry is a file entry
        if( (de.status & DIRECTORY_ENTRY_FILE) != 0){
            
            char filename[strlen(argv[2])];
            strncpy(filename, (char *)(de.filename), strlen(argv[2])+1);
      
            //check if the file names are same
            if(strncmp(filename, argv[2], strlen(argv[2])) == 0){
                fileFound = 1;
                starting_block = ntohl(de.starting_block);
                block_count = ntohl(de.block_count);
                int filesize = htonl(de.size);
                if(filesize == 0)
                    exit(0);
                break;
            }
        }
    }
    //if the file is not found
    if(fileFound == 0){
        printf("File not found.\n");
        exit(0);
    }
    int file_blocks[block_count];//used to store the blocks number of the file
    file_blocks[0] = starting_block;
    uint32_t number;
    
    //get the file blocks number and store them into array file_blocks
    for(int i = 1; i < block_count; i++){
        fseek(pFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
        fseek(pFile, file_blocks[i-1] * 4, SEEK_CUR);
        fread(&number, sizeof(uint32_t), 1, pFile);
        if(number != -1){
            file_blocks[i] = ntohl(number);//store the block number into array file_blocks
        }
    }
    //copy the file in the file system to the current directory
    int flag = 1;
    for(int i = 0; i < block_count && flag == 1; i++){//copy all the blocks
        
        int block_number = file_blocks[i];
        
        fseek(pFile, block_number * DEFAULT_BLOCK_SIZE, SEEK_SET);//go to next block
        uint8_t ch;
        
        //copy each block
        for(int j = 0; j < DEFAULT_BLOCK_SIZE; j++){
            fread(&ch, sizeof(uint8_t), 1, pFile);
            if(ch == '\0'){//break if it is the end of the file
                flag = 0;
                break;
            }
            char chwrite = (char)(ch);
            
            fwrite(&chwrite, sizeof(char), 1, oFile);//write the char into the output file
            
        }
    }
    fclose(pFile);
    fclose(oFile);
}
//part 4 function
void diskput(int argc, char* argv[]){
    FILE * pFile;
    pFile = fopen (argv[2], "r");
    if(!pFile){
        printf("File not found.\n");
        exit(0);
    }
    fseek(pFile, 0, SEEK_END); // seek to end of file
    int filesize = ftell(pFile);//get the file size

    //get the number of blocks needed for the file
    int num_blocks = filesize / DEFAULT_BLOCK_SIZE;
    if (num_blocks * DEFAULT_BLOCK_SIZE < filesize) ++num_blocks;
    
    if(num_blocks == 0)//if file size is 0, assign 1 block
        num_blocks = 1;
    
    FILE *oFile;
    oFile  = fopen (argv[1], "r+");
    if(!oFile){
        printf("File not found.\n");
        exit(0);
    }
    fseek(oFile, 0, SEEK_SET);
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, oFile);
    
    int fat_startblock = htonl(sb.fat_start_block);
    int fat_num_blocks = htonl(sb.fat_block_count);
    int root_dir_start_block = htonl(sb.root_dir_start_block);
    int root_dir_block_size = htonl(sb.root_dir_block_size);
    int num_dir_entries = root_dir_block_size * 8;
    
    //check if the file alreadly exist
    fseek(oFile, root_dir_start_block * DEFAULT_BLOCK_SIZE, SEEK_SET);
    
    int old_starting_block;
    int ifExist = 0;
    struct dir_entry_t de;
    for(int i = 0; i < num_dir_entries; i++){
        fread(&de, sizeof(struct dir_entry_t), 1, oFile);
        if( (de.status & DIRECTORY_ENTRY_FILE) != 0){
            
            char filename[strlen(argv[2])];
            strncpy(filename, (char *)(de.filename), strlen(argv[2])+1);
            
            if(strncmp(filename, argv[2], strlen(argv[2])) == 0){
                ifExist = 1;
                //get the starting block of the old file to delete the old file and then write the new file
                old_starting_block = ntohl(de.starting_block);

                //update the root directory entry as free
                fseek(oFile, -sizeof(struct dir_entry_t), SEEK_CUR);
                uint8_t dir_status = DIRECTORY_ENTRY_FREE;
                fwrite(&dir_status, sizeof(uint8_t), 1, oFile);
                break;
            }
        }
    }
    //if the file already exist, delete 
    if(ifExist != 0){
    //delete old file entries in FAT table
    int next_block = old_starting_block;
    
    uint32_t delete_number;
    uint32_t fat_status = FAT_FREE;
    do{
        //move to the entry and get the next block number into delete_number
        fseek(oFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
        fseek(oFile, next_block * 4, SEEK_CUR);
        fread(&delete_number, sizeof(uint32_t), 1, oFile);

        //move back to the entry and update it as FAT_FREE
        fseek(oFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
        fseek(oFile, next_block * 4, SEEK_CUR);
        fwrite(&fat_status, sizeof(uint32_t), 1, oFile);
        
        //get the next block number
        next_block = ntohl(delete_number);
    }
    while(next_block > 0);
    }
    
    //find the free blocks in FAT
    fseek(oFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
    int num_fat_entries = DEFAULT_BLOCK_SIZE / FAT_ENTRY_SIZE * fat_num_blocks;
    uint32_t number;
    
    int blocks[num_blocks];
    int index = 0;
    for(int i = 0; i < num_fat_entries; i++){
        fread(&number, sizeof(uint32_t), 1, oFile);

        if(number == FAT_FREE){//get the free block number and store it in blocks
            blocks[index] = i;
            index++;
        if(index == num_blocks)
            break;
        }
    }
    //for(int i = 0; i < num_blocks; i++){printf("block number %d: %d\n", i, blocks[i]);}
    //update FAT table
    for(int i = 0; i < num_blocks-1; i++){
        fseek(oFile, fat_startblock * DEFAULT_BLOCK_SIZE, SEEK_SET);
        fseek(oFile, blocks[i] * 4, SEEK_CUR);
        uint32_t write_number = ntohl(blocks[i+1]);
        fwrite(&write_number, sizeof(uint32_t), 1, oFile);
    }
    uint32_t eof = FAT_EOF;
    fwrite(&eof, sizeof(uint32_t), 1, oFile);

    // update root dir entry
    fseek(oFile, root_dir_start_block * DEFAULT_BLOCK_SIZE, SEEK_SET);
    
    struct dir_entry_t de_update;

    for(int i = 0; i < num_dir_entries; i++){
        fread(&de_update, sizeof(struct dir_entry_t), 1, oFile);
        if(de_update.status == DIRECTORY_ENTRY_FREE){
            //move back of sizeof a dir_entry_t
            fseek(oFile, -sizeof(struct dir_entry_t), SEEK_CUR);
            
            //assign the dir entry for the file
            struct dir_entry_t file_entry;
            file_entry.status = DIRECTORY_ENTRY_FILE;
            file_entry.starting_block = htonl(blocks[0]);
            file_entry.block_count = htonl(num_blocks);
            file_entry.size = htonl(filesize);
            
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            
            file_entry.create_time.year = ntohs(tm.tm_year + 1900);
            file_entry.create_time.month = tm.tm_mon + 1;
            file_entry.create_time.day = tm.tm_mday;
            file_entry.create_time.hour = tm.tm_hour;
            file_entry.create_time.minute = tm.tm_min;
            file_entry.create_time.second = tm.tm_sec;
            
            file_entry.modify_time = file_entry.create_time;
            
            for(int i = 0; i < strlen(argv[2]); i++){
                file_entry.filename[i] = (uint8_t)(argv[2][i]);
            }
            for(int i = 0; i < sizeof(file_entry.unused); i++){
                file_entry.unused[i] = -1;
            }
            //write the new root directory entry
            fwrite(&file_entry, sizeof(file_entry), 1, oFile);
            break;
        }
    }
    //write file content to the img
    fseek(pFile, 0, SEEK_SET); 
    int count_bytes = 0;//count the number of bytes written
    int flag = 1;
    for(int i = 0; i < num_blocks && flag == 1; i++){
        int block_number = blocks[i];
        //move to next block
        fseek(oFile, block_number * DEFAULT_BLOCK_SIZE, SEEK_SET);
        
        int num_bytes_block = 0;
        char char_read;
        while(num_bytes_block < DEFAULT_BLOCK_SIZE){
            //write bytes in a block
            fread(&char_read, sizeof(char), 1, pFile);
        
            uint8_t char_write = (uint8_t)(char_read);//char will be written in the file
            fwrite(&char_write, sizeof(uint8_t), 1, oFile);//write the char into the file
            num_bytes_block++;
            count_bytes++;
            if(count_bytes >= filesize){
                char_read = '\0';
                char_write = (uint8_t)(char_read);
                fwrite(&char_write, sizeof(uint8_t), 1, oFile);
                fclose(oFile);
                flag = 0;
                break;
            }
                
        }
    }
    fclose(pFile);
    
}
int main(int argc, char* argv[]) {
#if defined(PART1)
 diskinfo(argc, argv);
#elif defined(PART2)
 disklist(argc, argv);
#elif defined(PART3)
 diskget(argc, argv);
#elif defined(PART4)
 diskput(argc, argv);
#else
# error "PART[1234] must be defined"
#endif


    return 0;
}