#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#define SECTORSIZE 512
#define CLUSTERSIZE 1024


char *disk_name;
int disk_fd;

uint8_t volumesector[SECTORSIZE];
uint32_t volumefat[SECTORSIZE / 4];
uint8_t volumecluster[CLUSTERSIZE];

struct BIOSParameterBlock
{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt; // relocated sector count from the SMART status probably reads if any of these blocks are occupied
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16; // if this value returns 0, then there are more than 65535 sectors
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16; // this applies to fat12/fat16 only. 0 otherwise
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads; // should apply for spinning hard disk drives only
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32; // this field is set if there are more than 65535 sectors

    // the rest applies for extended boot records
    uint32_t BPB_FATSz32; // size of fat in sectors
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;    // high byte s the major, low byte is the minor version
    uint32_t BPB_RootClus; // usually set to 2
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12]; // should all be set to zero when volume is formatted
    uint8_t BS_DrvNum;        // 0x00 for floppy disks, 0x80 for hard disks
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID; // hardware specific. used to tracking volumes between computers
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8]; // Always says "FAT32 ". Useless...
    uint8_t BPB_boot_code[420];
    uint16_t BPB_bootable_partition_signature; // used for sanity checks. Has to be 0xaa55 for the drive to be bootable.
} __attribute__((packed));

// The directory structure of the FAT filesystem. Create date and create time resides in the reserved block. Check microsoft's documentation.
struct DIRStructure
{
    uint8_t DIR_file_name[8];
    uint8_t DIR_extension[3];
    uint8_t DIR_attributes;
    uint8_t DIR_reserved[10];
    uint16_t DIR_modify_time;
    uint16_t DIR_modify_date;
    uint16_t DIR_starting_cluster;
    uint32_t DIR_file_size;
} __attribute__((packed));

// given in the project skeleton
int get_sector(uint8_t *buf, int snum)
{
    off_t offset;
    int n;
    offset = snum * SECTORSIZE;
    lseek(disk_fd, offset, SEEK_SET);
    n = read(disk_fd, buf, SECTORSIZE);
    if (n == SECTORSIZE)
        return (0);
    else
    {
        printf("sector number %d invalid or read error.\n", snum);
        exit(1);
    }
}

int readcluster(unsigned char *buf,
                unsigned int cnum, int data_start_sector, int sectors_per_cluster)
{
    off_t offset;
    int n;
    unsigned int snum; // sector number
    snum = data_start_sector +
           (cnum - 2) * sectors_per_cluster;
    offset = snum * SECTORSIZE;
    lseek(disk_fd, offset, SEEK_SET);
    n = read(disk_fd, buf, CLUSTERSIZE);
    if (n == CLUSTERSIZE)
        return (0); // success
    else
        return (-1);
}

// given in the project skeleton
void print_sector(int givenIndex, unsigned int counter)
{
    get_sector(volumesector, givenIndex);
    // unsigned int counter = 0;
    printf("%08x:  ", counter);

    for (int index = 0; index < 512; index++)
    {

        printf("%02x", volumesector[index]);
        printf(" ");

        if ((index + 1) % 16 == 0)
        {
            printf("      ");
            int index2 = index - 15;
            for (int k = index2; k < index + 1; k++)
            {
                if (isprint(volumesector[k]))
                    printf("%c", volumesector[k]);
                else
                    printf(".%c", 0);
            }
            counter = counter + 16;
            printf("\n");
            if (index != 511)
                printf("%08x:  ", counter);
        }
    }
}

void print_sector2(int givenIndex, unsigned int counter)
{
    get_sector(volumesector, givenIndex);
    // unsigned int counter = 0;
    // printf("%08x:  ", counter);

    for (int index = 0; index < 512; index++)
    {

        // printf("%02x", volumesector[index]);
        // printf(" ");

        if ((index + 1) % 16 == 0)
        {
            //    printf("      ");
            int index2 = index - 15;
            for (int k = index2; k < index + 1; k++)
            {
                if (isprint(volumesector[k]))
                    printf("%c", volumesector[k]);
                else
                    printf(".%c", 0);
            }
            counter = counter + 16;
            //  printf("\n");
            // if (index != 511)
            //     printf("%08x:  ", counter);
        }
    }
}

void read_path(uint8_t *volumeCluster, int clusterNo, int FirstDataSector, int sector_per_cluster, int oldCluster, struct DIRStructure *olddep, char *rootpath, int re)
{
    readcluster(volumeCluster, clusterNo, FirstDataSector, sector_per_cluster);
    unsigned char cluster[1024];
    struct DIRStructure *dep;
    dep = (struct DIRStructure *)volumecluster;

    dep++;
    dep++;

    for (int j = 0; j < (SECTORSIZE * sector_per_cluster) / sizeof(struct DIRStructure); j++)
    {
        // printf("re : %d j : %d rootpath : %s\n", re, j, rootpath);

        // printf("(d)oldout:  %s\n", out);

        if (dep->DIR_file_name[0] != 0x00 && dep->DIR_file_name[0] != 0xe5)
        {

            if (dep->DIR_extension[0] == ' ' && dep->DIR_starting_cluster != 0)
            {

                //  if (isprint(dep->DIR_file_name))
                printf("(d)  %.8s\n", dep->DIR_file_name);
                char *x = (char *)dep->DIR_file_name;
                char newRoot[100];
                strcat(newRoot, rootpath);
                strcat(newRoot, x);

                if (dep->DIR_starting_cluster != 0)
                {
                    oldCluster = clusterNo;

                    read_path(volumeCluster, dep->DIR_starting_cluster, FirstDataSector, sector_per_cluster, oldCluster, dep, newRoot, re + 1);
                }
            }
            else
            {
                // if (isprint(dep->DIR_file_name))
                printf("(f)  %.8s.%s\n", dep->DIR_file_name, dep->DIR_extension);
            }
        }
        dep++;
    }

    if (re != 1)
    {
        readcluster(volumeCluster, oldCluster, FirstDataSector, sector_per_cluster);
        dep = olddep;
    }

    // printf("done %d\n", re);
}

void print_path(char **x, uint8_t *volumeCluster, int clusterNo, int FirstDataSector, int sector_per_cluster, int oldCluster, struct DIRStructure *olddep, int re, int count)
{
    readcluster(volumeCluster, clusterNo, FirstDataSector, sector_per_cluster);
    unsigned char cluster[1024];
    struct DIRStructure *dep;
    dep = (struct DIRStructure *)volumecluster;

    dep++;
    dep++;

    for (int j = 0; j < (SECTORSIZE * sector_per_cluster) / sizeof(struct DIRStructure); j++)
    {
        // printf("re : %d j : %d rootpath : %s\n", re, j, rootpath);

        // printf("(d)oldout:  %s\n", out);

        if (dep->DIR_file_name[0] != 0x00 && dep->DIR_file_name[0] != 0xe5)
        {

            if (dep->DIR_extension[0] == ' ' && dep->DIR_starting_cluster != 0)
            {

                int totChar = 0;
                for (int i = 0; x[0][i] != '\0'; i++)
                {
                    if (x[0][i] != ' ') // this condition is used to avoid counting space
                    {
                        totChar++; // totChar=totChar+1
                    }
                }
                int arr[100];

                for (int i = 0; i < 100; i++)
                {
                    arr[i] = 0;
                }

                for (int i = 0; i < totChar; i++)
                {
                    if (dep->DIR_file_name[i] == x[0][i])
                        arr[i] = 1;
                }

                for (int i = 0; i < totChar; i++)
                {

                    //     printf("arr [%d] = %d **", i, arr[i]);
                    //     printf("\n");
                }

                int check = 1;

                for (int i = 0; i < totChar; i++)
                {
                    if (arr[i] != 1)
                        check = 0;
                }
                // printf("%d\n", check);

                if (check)
                {
                    if (count > 1)
                    {
                        oldCluster = clusterNo;
                        char *array[count - 1];

                        for (int i = 0; i < count - 1; i++)
                            array[i] = x[i + 1];

                        //  for (int i = 0; i < count - 1; i++)
                        //    printf("%s\n", array[i]);

                        print_path(array, volumeCluster, dep->DIR_starting_cluster, FirstDataSector, sector_per_cluster, oldCluster, dep, re + 1, count - 1);
                    }

                    else
                    {
                        printf("Res : %d \n", dep->DIR_starting_cluster);
                    }
                }
            }

            else
            {
                if (count == 1)
                {
                    char *tofind = strtok(x[0], ".");
                    // printf("tofind : %s\n", tofind);
                    int totChar = 0;
                    // printf(" find : %s\n", x[0]);
                    for (int i = 0; x[0][i] != '\0'; i++)
                    {
                        if (x[0][i] != ' ') // this condition is used to avoid counting space
                        {
                            totChar++; // totChar=totChar+1
                        }
                    }
                    int arrf[100];

                    for (int i = 0; i < 100; i++)
                    {
                        arrf[i] = 0;
                    }

                    for (int i = 0; i < totChar; i++)
                    {
                        if (dep->DIR_file_name[i] == x[0][i])
                            arrf[i] = 1;
                    }

                    for (int i = 0; i < totChar; i++)
                    {

                        //    printf("arrf [%d] = %d **", i, arrf[i]);
                        //  printf("\n");
                    }

                    int check = 1;

                    for (int i = 0; i < totChar; i++)
                    {
                        if (arrf[i] != 1)
                            check = 0;
                    }
                    // printf("%d\n", check);
                    if (check)
                    {
                        //  printf("Res : %d \n", dep->DIR_starting_cluster);
                        int clusterNo = dep->DIR_starting_cluster;
                        int givenIndex = FirstDataSector + 2 * (clusterNo - 2);
                        print_sector2(givenIndex, 0);
                        print_sector2(givenIndex + 1, 512);
                    }
                }
            }
        }
        dep++;
    }

    if (re != 1)
    {
        readcluster(volumeCluster, oldCluster, FirstDataSector, sector_per_cluster);
        dep = olddep;
    }

    // printf("done %d\n", re);
}

void print_path2(char **x, uint8_t *volumeCluster, int clusterNo, int FirstDataSector, int sector_per_cluster, int oldCluster, struct DIRStructure *olddep, int re, int count)
{
    readcluster(volumeCluster, clusterNo, FirstDataSector, sector_per_cluster);
    unsigned char cluster[1024];
    struct DIRStructure *dep;
    dep = (struct DIRStructure *)volumecluster;

    dep++;
    dep++;

    for (int j = 0; j < (SECTORSIZE * sector_per_cluster) / sizeof(struct DIRStructure); j++)
    {
        // printf("re : %d j : %d rootpath : %s\n", re, j, rootpath);

        // printf("(d)oldout:  %s\n", out);

        if (dep->DIR_file_name[0] != 0x00 && dep->DIR_file_name[0] != 0xe5)
        {

            if (dep->DIR_extension[0] == ' ' && dep->DIR_starting_cluster != 0)
            {

                int totChar = 0;
                for (int i = 0; x[0][i] != '\0'; i++)
                {
                    if (x[0][i] != ' ') // this condition is used to avoid counting space
                    {
                        totChar++; // totChar=totChar+1
                    }
                }
                int arr[100];

                for (int i = 0; i < 100; i++)
                {
                    arr[i] = 0;
                }

                for (int i = 0; i < totChar; i++)
                {
                    if (dep->DIR_file_name[i] == x[0][i])
                        arr[i] = 1;
                }

                for (int i = 0; i < totChar; i++)
                {

                    //     printf("arr [%d] = %d **", i, arr[i]);
                    //     printf("\n");
                }

                int check = 1;

                for (int i = 0; i < totChar; i++)
                {
                    if (arr[i] != 1)
                        check = 0;
                }
                // printf("%d\n", check);

                if (check)
                {
                    if (count > 1)
                    {
                        oldCluster = clusterNo;
                        char *array[count - 1];

                        for (int i = 0; i < count - 1; i++)
                            array[i] = x[i + 1];

                        //  for (int i = 0; i < count - 1; i++)
                        //    printf("%s\n", array[i]);

                        print_path2(array, volumeCluster, dep->DIR_starting_cluster, FirstDataSector, sector_per_cluster, oldCluster, dep, re + 1, count - 1);
                    }

                    else
                    {
                        printf("Res : %d \n", dep->DIR_starting_cluster);
                    }
                }
            }

            else
            {
                if (count == 1)
                {
                    char *tofind = strtok(x[0], ".");
                    // printf("tofind : %s\n", tofind);
                    int totChar = 0;
                    // printf(" find : %s\n", x[0]);
                    for (int i = 0; x[0][i] != '\0'; i++)
                    {
                        if (x[0][i] != ' ') // this condition is used to avoid counting space
                        {
                            totChar++; // totChar=totChar+1
                        }
                    }
                    int arrf[100];

                    for (int i = 0; i < 100; i++)
                    {
                        arrf[i] = 0;
                    }

                    for (int i = 0; i < totChar; i++)
                    {
                        if (dep->DIR_file_name[i] == x[0][i])
                            arrf[i] = 1;
                    }

                    for (int i = 0; i < totChar; i++)
                    {

                        //    printf("arrf [%d] = %d **", i, arrf[i]);
                        //  printf("\n");
                    }

                    int check = 1;

                    for (int i = 0; i < totChar; i++)
                    {
                        if (arrf[i] != 1)
                            check = 0;
                    }
                    // printf("%d\n", check);
                    if (check)
                    {
                        //  printf("Res : %d \n", dep->DIR_starting_cluster);
                        int clusterNo = dep->DIR_starting_cluster;
                        int givenIndex = FirstDataSector + 2 * (clusterNo - 2);
                        print_sector(givenIndex, 0);
                        print_sector(givenIndex + 1, 512);
                    }
                }
            }
        }
        dep++;
    }

    if (re != 1)
    {
        readcluster(volumeCluster, oldCluster, FirstDataSector, sector_per_cluster);
        dep = olddep;
    }

    // printf("done %d\n", re);
}

void print_info(uint8_t *volumeCluster, int clusterNo, int FirstDataSector, int sector_per_cluster, int oldCluster, struct DIRStructure *olddep, int re)
{
    readcluster(volumeCluster, clusterNo, FirstDataSector, sector_per_cluster);
    unsigned char cluster[1024];
    struct DIRStructure *dep;
    dep = (struct DIRStructure *)volumecluster;

    dep++;
    dep++;

    for (int j = 0; j < (SECTORSIZE * sector_per_cluster) / sizeof(struct DIRStructure); j++)
    {

        if (dep->DIR_file_name[0] != 0x00 && dep->DIR_file_name[0] != 0xe5)
        {

            if (dep->DIR_extension[0] == ' ' && dep->DIR_starting_cluster != 0)
            {
                printf("name= %s         fcn= %d       size(bytes)= %d      date =", dep->DIR_file_name, dep->DIR_starting_cluster, dep->DIR_file_size);
                printf("%d - %d - %d\n", dep->DIR_modify_date & 0x001f, (dep->DIR_modify_date & 0x01ff) >> 5, 1980 + ((dep->DIR_modify_date & 0xfd00) >> 9));
                print_info(volumeCluster, dep->DIR_starting_cluster, FirstDataSector, sector_per_cluster, oldCluster, dep, re + 1);
            }

            else
            {
                printf("name= %s         fcn= %d       size(bytes)= %d      date =", dep->DIR_file_name, dep->DIR_starting_cluster, dep->DIR_file_size);
                printf("date = %d - %d - %d\n", dep->DIR_modify_date & 0x001f, (dep->DIR_modify_date & 0x01ff) >> 5, 1980 + ((dep->DIR_modify_date & 0xfd00) >> 9));
            }
        }

        dep++;
    }

    if (re != 1)
    {
        readcluster(volumeCluster, oldCluster, FirstDataSector, sector_per_cluster);
        dep = olddep;
    }

    // printf("done %d\n", re);
}

void print_spec_info(char **x, uint8_t *volumeCluster, int clusterNo, int FirstDataSector, int sector_per_cluster, int oldCluster, struct DIRStructure *olddep, int re, int count)
{
    readcluster(volumeCluster, clusterNo, FirstDataSector, sector_per_cluster);
    unsigned char cluster[1024];
    struct DIRStructure *dep;
    dep = (struct DIRStructure *)volumecluster;

    dep++;
    dep++;

    for (int j = 0; j < (SECTORSIZE * sector_per_cluster) / sizeof(struct DIRStructure); j++)
    {

        if (dep->DIR_file_name[0] != 0x00 && dep->DIR_file_name[0] != 0xe5)
        {

            if (dep->DIR_extension[0] == ' ' && dep->DIR_starting_cluster != 0)
            {

                int totChar = 0;
                for (int i = 0; x[0][i] != '\0'; i++)
                {
                    if (x[0][i] != ' ') // this condition is used to avoid counting space
                    {
                        totChar++; // totChar=totChar+1
                    }
                }
                int arr[100];

                for (int i = 0; i < 100; i++)
                {
                    arr[i] = 0;
                }

                for (int i = 0; i < totChar; i++)
                {
                    if (dep->DIR_file_name[i] == x[0][i])
                        arr[i] = 1;
                }

                for (int i = 0; i < totChar; i++)
                {

                    //     printf("arr [%d] = %d **", i, arr[i]);
                    //     printf("\n");
                }

                int check = 1;

                for (int i = 0; i < totChar; i++)
                {
                    if (arr[i] != 1)
                        check = 0;
                }
                // printf("%d\n", check);

                if (check)
                {
                    if (count > 1)
                    {
                        oldCluster = clusterNo;
                        char *array[count - 1];

                        for (int i = 0; i < count - 1; i++)
                            array[i] = x[i + 1];

                        //  for (int i = 0; i < count - 1; i++)
                        //    printf("%s\n", array[i]);

                        print_spec_info(array, volumeCluster, dep->DIR_starting_cluster, FirstDataSector, sector_per_cluster, oldCluster, dep, re + 1, count - 1);
                    }

                    else
                    {
                        printf("name = %s\n", dep->DIR_file_name);
                        printf("type = DIRECTORY\n");
                        int clusterNo = dep->DIR_starting_cluster;
                        int givenIndex = FirstDataSector + 2 * (clusterNo - 2);
                        int size = dep->DIR_file_size == -1 ? 0 : dep->DIR_file_size;
                        printf("size = %d\n", dep->DIR_file_size == -1 ? 0 : dep->DIR_file_size);
                        printf("first cluster = %d\n", dep->DIR_starting_cluster);
                        printf("cluster count = %d\n", size / 1024);
                        printf("date = %d - %d - %d\n", dep->DIR_modify_date & 0x001f, (dep->DIR_modify_date & 0x01ff) >> 5, 1980 + ((dep->DIR_modify_date & 0xfd00) >> 9));
                        if (((dep->DIR_modify_time & 0x0800) >> 5) < 10)
                            printf("time =  %d:0%d\n", (dep->DIR_modify_time & 0xfd00) >> 11, (dep->DIR_modify_time & 0x0800) >> 5);
                        else
                            printf("time =  %d:%d\n", (dep->DIR_modify_time & 0xfd00) >> 11, (dep->DIR_modify_time & 0x0800) >> 5);
                    }
                }
            }
        }

        else
        {
            if (count == 1)
            {
                char *tofind = strtok(x[0], ".");
                // printf("tofind : %s\n", tofind);
                int totChar = 0;
                // printf(" find : %s\n", x[0]);
                for (int i = 0; x[0][i] != '\0'; i++)
                {
                    if (x[0][i] != ' ') // this condition is used to avoid counting space
                    {
                        totChar++; // totChar=totChar+1
                    }
                }
                int arrf[100];

                for (int i = 0; i < 100; i++)
                {
                    arrf[i] = 0;
                }

                for (int i = 0; i < totChar; i++)
                {
                    if (dep->DIR_file_name[i] == x[0][i])
                        arrf[i] = 1;
                }

                for (int i = 0; i < totChar; i++)
                {

                    printf("arrf [%d] = %d **", i, arrf[i]);
                    printf("\n");
                }

                int check = 1;

                for (int i = 0; i < totChar; i++)
                {
                    if (arrf[i] != 1)
                        check = 0;
                }
                // printf("%d\n", check);
                if (check)
                {
                    //  printf("Res : %d \n", dep->DIR_starting_cluster);
                    printf("name = %s\n", dep->DIR_file_name);
                    printf("type = FILE\n");
                    int clusterNo = dep->DIR_starting_cluster;
                    int givenIndex = FirstDataSector + 2 * (clusterNo - 2);
                    int size = dep->DIR_file_size == -1 ? 0 : dep->DIR_file_size;
                    printf("size = %d\n", dep->DIR_file_size == -1 ? 0 : dep->DIR_file_size);
                    printf("first cluster = %d\n", dep->DIR_starting_cluster);
                    printf("cluster count = %d\n", size / 1024);
                    printf("date = %d - %d - %d\n", dep->DIR_modify_date & 0x001f, (dep->DIR_modify_date & 0x01ff) >> 5, 1980 + ((dep->DIR_modify_date & 0xfd00) >> 9));
                    if (((dep->DIR_modify_time & 0x0800) >> 5) < 10)
                        printf("time =  %d:0%d\n", (dep->DIR_modify_time & 0xfd00) >> 11, (dep->DIR_modify_time & 0x0800) >> 5);
                    else
                        printf("time =  %d:%d\n", (dep->DIR_modify_time & 0xfd00) >> 11, (dep->DIR_modify_time & 0x0800) >> 5);
                }
            }
        }

        dep++;
    }

    if (re != 1)
    {
        readcluster(volumeCluster, oldCluster, FirstDataSector, sector_per_cluster);
        dep = olddep;
    }

    // printf("done %d\n", re);
}
int main(int argc, char *argv[])
{
    //*********************************************************************************************************************
    int option;
    int givenIndex;
    int clusterNo;
    char *filename;
    //  char buf[] = "/DIR2/F1.TXT";
    char *buf;
    char *buf2;
    int givenInput;
    if (strcmp("-v", argv[2]) == 0)
    {
        disk_name = argv[1];
        // printf("./fat disk1 -v");
        // printf("%s\n",disk_name);
        option = 1;
    }

    else if (strcmp("-s", argv[2]) == 0)
    {
        disk_name = argv[1];
        givenIndex = atoi(argv[3]);
        // printf("./fat disk1 -s 32");
        // printf("\n");
        option = 2;
        // argv[2] = 32;
    }

    else if (strcmp("-c", argv[2]) == 0)
    {
        disk_name = argv[1];
        clusterNo = atoi(argv[3]);
        // printf("./fat disk1 -c 2");
        // printf("\n");
        option = 3;
    }

    else if (strcmp("-t", argv[2]) == 0)
    {
        disk_name = argv[1];
        printf("./fat disk1 -t");
        printf("\n");
        option = 4;
    }

    else if (strcmp("-a", argv[2]) == 0)
    {
        disk_name = argv[1];
        buf = argv[3];
        //  printf("./fat disk1 -a /DIR2/F1.TXT ");
        //  printf("\n");
        option = 5;
        // argv[3] = 100;
        // argv[4] = 64;
    }
    else if (strcmp("-b", argv[2]) == 0)
    {
        disk_name = argv[1];
        buf2 = argv[3];
        //  printf("./fat disk1 -a /DIR2/F1.TXT ");
        option = 6;
        // printf("option : %d\n", option);
    }

    else if (strcmp("-l", argv[2]) == 0)
    {
        disk_name = argv[1];
        printf("./fat disk1 -l /DIR2");
        printf("\n");
        option = 7;
    }

    /* else if (strcmp(argv[0], "disk1") == 0 && strcmp("-n", argv[1]) == 0 && strcmp("/DIR1/AFILE1.BIN", argv[2]) == 0)
     {
         printf("./fat disk1 -n /DIR2/F1.TXT 100 64");
         printf("\n");
         option = 8;
     }*/

    else if (strcmp("-d", argv[2]) == 0)
    {
        disk_name = argv[1];
        filename = argv[3];
        //    printf("./fat disk1 -d /FILE2.BIN");
        //    printf("\n");
        option = 9;
        // argv[2] = 100;
    }

    else if (strcmp("-f", argv[2]) == 0)
    {
        disk_name = argv[1];
        givenInput = atoi(argv[3]);
        //  printf("./fat disk1 -f 15");
        //  printf("\n");
        option = 10;
    }

    /*  else if (strcmp(argv[0], "disk1") == 0 && strcmp("-l", argv[1]) == 0 && strcmp("/", argv[2]) == 0)
      {
          printf("./fat disk1 -l /");
          printf("\n");
          option = 11;
      }

      else if (strcmp(argv[0], "disk1") == 0 && strcmp("-l", argv[1]) == 0 && strcmp("/DIR2", argv[2]) == 0)
      {
          printf("./fat disk1 -l /DIR2");
          printf("\n");
          option = 12;
      }
  */
    if (strcmp("-h", argv[2]) == 0)
    {
        disk_name = argv[1];
        printf("./fat disk1 -v \n");
        printf("./fat disk1 -s 0 \n");
        printf("/fat disk1 -c 2\n");
        printf("/fat disk1 -t\n");
        printf(" ./fat disk1 -r /DIR2/F1.TXT 100 64\n");
        printf(" ./fat disk1 -b /DIR2/F1.TXT\n");
        printf("   ./fat disk1 -a /DIR2/F1.TXT");
        printf("./fat disk1 -n /DIR1/AFILE1.BIN\n");
        printf("./fat disk1 -m 100\n");
        printf(" ./fat disk1 -f 50\n");
        printf("./fat disk1 -d /DIR1/AFILE1.BIN\n");
        printf(" ./fat disk1 -l /\n");
        printf("./fat disk1 -l /DIR2\n");
        printf("./fat disk1 -h\n");
    }
    //*********************************************************************************************************************
    // int option = 5;

    if (argc < 2)
    {
        printf("wrong usage\n");
        exit(1);
    }

    // strcpy(disk_name, argv[1]);

    disk_fd = open(disk_name, O_RDWR);
    if (disk_fd < 0)
    {
        printf("could not open the disk image\n");
        exit(1);
    }

    struct BIOSParameterBlock bootEntry;
    struct DIRStructure dirEntry;
    unsigned char *buffer = malloc(1024);

    int count = read(disk_fd, buffer, 512);
    if (!count)
    {
        printf("Error (%d) - Boot Sector \n", count);
        exit(0);
    }

    get_sector(volumesector, 0);
    // printf("%s\n", volumesector);

    // get the first sector of the drive. It is always the MasterBootRecord.
    memcpy(&bootEntry, volumesector, sizeof(struct BIOSParameterBlock));
    unsigned int RootDirSectors = ((bootEntry.BPB_RootEntCnt * 32) + (bootEntry.BPB_BytsPerSec - 1)) / bootEntry.BPB_BytsPerSec;
    unsigned int FirstDataSector = bootEntry.BPB_RsvdSecCnt + (bootEntry.BPB_NumFATs * bootEntry.BPB_FATSz32) + RootDirSectors;
    // printf("%d\n", volumesector[90]);
    int data_sector_offset = bootEntry.BPB_RsvdSecCnt + 2 * bootEntry.BPB_FATSz32;
    int sector_per_cluster = bootEntry.BPB_SecPerClus;
    //  printf("%d\n", volumesector[91]);

    //************************************************1**********************************************************************
    // option = 1;
    if (option == 1)
    {
        printf("File system type:  %s\n", bootEntry.BS_FilSysType);
        printf("Volume Label:  %s\n", bootEntry.BS_VolLab);
        printf("Number of sectors in disk:  %d\n", bootEntry.BPB_TotSec32);
        printf("Sector size in bytes:   %d\n", bootEntry.BPB_BytsPerSec);
        printf("Number of reserved sectors:    %d\n", bootEntry.BPB_RsvdSecCnt);
        printf("Number of sectors per FAT table:    %d\n", bootEntry.BPB_FATSz32);
        printf("Number of FAT tables:    %d\n", bootEntry.BPB_NumFATs);

        printf("Number of sectors per cluster:   %d\n", bootEntry.BPB_SecPerClus);

        printf("Number of clusters:    %d\n", bootEntry.BPB_FATSz32);

        printf("Data region starts at sector:  %d\n", FirstDataSector);
        printf("Root directory starts at sector:  %d\n", FirstDataSector);
        printf("Root directory starts at cluster: 2  \n");
        printf("Disk size in bytes :  %d bytes\n", bootEntry.BPB_TotSec32 * SECTORSIZE);
        printf("Disk size in Megabytes :  %d MB\n", (bootEntry.BPB_TotSec32 * SECTORSIZE) / (1024 * 1024));
        printf("Number of used clusters  :  %d MB\n", (bootEntry.BPB_TotSec32 * SECTORSIZE) / (1024 * 1024));
    }
    //*************************************END OF 1 ********************************************************************************

    //*****************************************************2************************************************************************
    if (option == 2)
    {
        // int givenIndex = 0;
        print_sector(givenIndex, 0);
    }
    //*****************************************************END OF 2*****************************************************************

    //***************************************3**************************************************************************************
    if (option == 3)
    {
        // unsigned int FirstDataSector = bootEntry.BPB_RsvdSecCnt + (bootEntry.BPB_NumFATs * bootEntry.BPB_FATSz32) + RootDirSectors;

        int givenIndex = FirstDataSector + 2 * (clusterNo - 2);
        print_sector(givenIndex, 0);
        print_sector(givenIndex + 1, 512);
    }

    //*****************************************************END OF 3*****************************************************************

    //***************************************4**************************************************************************************
    if (option == 4)
    {
        char out[100] = "";

        char oldout[100];

        struct DIRStructure *dep;
        read_path(volumecluster, 2, FirstDataSector, sector_per_cluster, 1, dep, out, 1);
    }
    //*************************************END OF 4 ********************************************************************************
    //******************************************************5***********************************************************************
    if (option == 5)
    {

        int count = 0;
        for (int i = 0; buf[i]; i++)
        {
            if (buf[i] == '/')
            {
                count++;
            }
        }

        int i = 0;
        char *p = strtok(buf, "/");
        char *array[count];

        while (p != NULL)
        {
            array[i++] = p;
            p = strtok(NULL, "/");
        }

        for (i = 0; i < count; ++i)
            printf("%s \n", array[i]);

        struct DIRStructure *dep;
        print_path(array, volumecluster, 2, FirstDataSector, sector_per_cluster, 1, dep, 1, count);
    }
    //*************************************END OF 5 ********************************************************************************

    //******************************************************6***********************************************************************
    if (option == 6)
    {
        // char buf[] = "/DIR2/F1.TXT";

        int count = 0;
        for (int i = 0; buf2[i]; i++)
        {
            if (buf2[i] == '/')
            {
                count++;
            }
        }

        int i = 0;
        char *p = strtok(buf2, "/");
        char *array[count];

        while (p != NULL)
        {
            array[i++] = p;
            p = strtok(NULL, "/");
        }

        struct DIRStructure *dep;
        print_path2(array, volumecluster, 2, FirstDataSector, sector_per_cluster, 1, dep, 1, count);
    }
    //*************************************END OF 6********************************************************************************
    //***************************************** 7********************************************************************************
    //*************************************END OF 7********************************************************************************
    if (option == 7)
    {
        struct DIRStructure *dep;
        print_info(volumecluster, 2, FirstDataSector, sector_per_cluster, 1, dep, 1);
    }

    //***************************************9**************************************************************************************
    if (option == 9)
    {
        printf("JKBDANCNKÖ");
        int count = 0;
        for (int i = 0; filename[i]; i++)
        {
            if (filename[i] == '/')
            {
                count++;
            }
        }

        int i = 0;
        char *p = strtok(filename, "/");
        char *array[count];

        while (p != NULL)
        {
            array[i++] = p;
            p = strtok(NULL, "/");
        }

        // for (i = 0; i < count; ++i)
        //   printf("%s \n", array[i]);

        struct DIRStructure *dep;
        print_spec_info(array, volumecluster, 2, FirstDataSector, sector_per_cluster, 1, dep, 1, count);
    }
    //*************************************END OF 9 ********************************************************************************

    //*************************************10****************************************************************************************
    if (option == 10)
    {

        if (givenInput == -1)
            givenInput = 128;

        get_sector(volumesector, 32);
        for (int i = 0; i < givenInput; i++)
        {
            char *out;
            if (i < 10)
                out = "000000";
            if (i >= 10 && i < 100)
                out = "00000";
            if (i >= 100 && i < 1000)
                out = "0000";

            int j = 4 * i;
            long total = ((long)256 * (long)256 * (long)256 * (long)volumesector[j] + (long)256 * (long)256 * (long)volumesector[j + 1] + (long)256 * (long)volumesector[j + 2] + (long)volumesector[j + 3]);
            if (total > (long)0x0ffffff8)
                // printf("%d : %d, %d, %d, %d, %ld\n", i, volumesector[j], volumesector[j + 1], volumesector[j + 2], volumesector[j + 3], total);
                printf("%s%d: EOF \n", out, i);
            else
                printf("%s%d: %d \n", out, i, volumesector[j]);
        }
    }
    //*************************************END OF 10 ********************************************************************************

    close(disk_fd);

    return (0);
}