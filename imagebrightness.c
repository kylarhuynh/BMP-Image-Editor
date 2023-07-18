#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/mman.h>
#include<time.h>
#include<sys/types.h>
#include<sys/wait.h>

char *filename;

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;

typedef struct tagBITMAPFILEHEADER
{
    WORD bfType; /*specifies the file type*/
    DWORD bfSize; /*specifies the size in bytes of the bitmap file*/
    WORD bfReserved1; /*reserved; must be 0*/
    WORD bfReserved2; /*reserved; must be 0*/
    DWORD bfOffBits; /*specifies the offset in bytes from the bitmapfileheader to the bitmap bits*/
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD biSize; /*specifies the number of bytes required by the struct*/
    LONG biWidth; /*specifies width in pixels*/
    LONG biHeight; /*species height in pixels*/
    WORD biPlanes; /*specifies the number of color planes, must be 1*/
    WORD biBitCount; /*specifies the number of bit per pixel*/
    DWORD biCompression;/*spcifies the type of compression*/
    DWORD biSizeImage; /*size of image in bytes*/
    LONG biXPelsPerMeter; /*number of pixels per meter in x axis*/
    LONG biYPelsPerMeter; /*number of pixels per meter in y axis*/
    DWORD biClrUsed; /*number of colors used by th ebitmap*/
    DWORD biClrImportant; /*number of colors that are important*/
}BITMAPINFOHEADER;

 /* Function declarations */
unsigned char* readimage(char*, BITMAPFILEHEADER*, BITMAPINFOHEADER*);
void writeimage(char*, BITMAPFILEHEADER*, BITMAPINFOHEADER*, unsigned char*);
void get_color(unsigned char*, int, int, int, int, int*, int*, int*);
void edit(unsigned char*, BITMAPINFOHEADER*, float, int, int, unsigned char*);

int main(int argc, char *argv[]){
    BITMAPFILEHEADER fileheader;
    BITMAPINFOHEADER infoheader;
    clock_t start;
    clock_t end;
    unsigned char *bitmapImage;
    unsigned char *bitmapRes;
    float brightness;
    char *outputtype;
    char dot;
    char *brightptr;
    int midy;
    int pid;
    double timediff;
    dot = '.';

    if(argc != 5){
        printf("Incorrect number of parameters, expected:\n[programname] [IMAGEFILE] [BRIGHTNESS] [PARALLEL] [OUTPUTFILE]\n");
        return 1;
    }
    
    brightness = strtod(argv[2], &brightptr);
    if(strcmp(brightptr, "") != 0){
        printf("Brightness must be a float between 1 and 0.\n");
        return 1;
    }

    if(brightness > 1 || brightness < 0){
        printf("Brightness must be a float between 1 and 0.\n");
        return 1;
    }

    outputtype = strrchr(argv[4], dot);
    if(strcmp(outputtype, ".bmp") != 0){
        printf("Invalid output file type. Must be .bmp\n");
        return 1;
    }

    bitmapImage = readimage(argv[1], &fileheader, &infoheader);
    bitmapRes = mmap(NULL, ((infoheader.biWidth * 3) + (4 - ((infoheader.biWidth * 3) % 4)) % 4) * infoheader.biHeight*2, PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);

    midy = (int)(infoheader.biHeight / 2);
    
    start = clock();

    if(atoi(argv[3]) == 0){
        edit(bitmapImage, &infoheader, brightness, 0, infoheader.biHeight, bitmapRes);
    }
    else{
        pid = fork();
        if(pid == 0){
            edit(bitmapImage, &infoheader, brightness, 0, midy, bitmapRes);
            exit(0);
        }
        else{
            edit(bitmapImage, &infoheader, brightness, midy, infoheader.biHeight, bitmapRes);
            wait(NULL);
        }
    }
   
    writeimage(argv[4], &fileheader, &infoheader, bitmapRes);

    end = clock();
    timediff = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Result Time: %f seconds\n", timediff);
    free(bitmapImage);
    munmap(bitmapRes, ((infoheader.biWidth * 3) + (4 - ((infoheader.biWidth * 3) % 4)) % 4) * infoheader.biHeight*2);
    printf("Done\n");
    return 0;
}

/* Read Image */
unsigned char* readimage(char* filename, BITMAPFILEHEADER* fileheader, BITMAPINFOHEADER* infoheader){
    FILE *fileptr;      /* File pointer */
    unsigned char* bitmapImage;
    int rowSize;
    int padding;
    int readret;

    /* Open file in binary mode*/
    fileptr = fopen(filename, "rb");
    if (fileptr == NULL){
        printf("At least one inputfile is missing\n");
        exit(1);
    }
    
    readret = fread(&(fileheader->bfType), sizeof(WORD), 1, fileptr); /* read filetype */
    if(readret == 0){
        fclose(fileptr);
        printf("One or more input images are not of type .BMP\n");
        exit(1); 
    }
    if(fileheader->bfType != 0x4D42){      /* Check if bmp file */
        fclose(fileptr);
        printf("One or more input images are not of type .BMP\n");
        exit(1); 
    }   
    fread(&(fileheader->bfSize), sizeof(DWORD), 1, fileptr); 
    fread(&(fileheader->bfReserved1), sizeof(WORD), 1, fileptr); 
    fread(&(fileheader->bfReserved2), sizeof(WORD), 1, fileptr); 
    fread(&(fileheader->bfOffBits), sizeof(DWORD), 1, fileptr); 

    fread(&(infoheader->biSize), sizeof(DWORD), 1, fileptr); /* start of infoheader */
    fread(&(infoheader->biWidth), sizeof(LONG), 1, fileptr); 
    fread(&(infoheader->biHeight), sizeof(LONG), 1, fileptr); 
    fread(&(infoheader->biPlanes), sizeof(WORD), 1, fileptr); 
    fread(&(infoheader->biBitCount), sizeof(WORD), 1, fileptr); 
    fread(&(infoheader->biCompression), sizeof(DWORD), 1, fileptr); 
    fread(&(infoheader->biSizeImage), sizeof(DWORD), 1, fileptr); 
    fread(&(infoheader->biXPelsPerMeter), sizeof(LONG), 1, fileptr); 
    fread(&(infoheader->biYPelsPerMeter), sizeof(LONG), 1, fileptr); 
    fread(&(infoheader->biClrUsed), sizeof(DWORD), 1, fileptr); 
    fread(&(infoheader->biClrImportant), sizeof(DWORD), 1, fileptr); 

    rowSize = infoheader->biWidth * 3;
    padding = (4 - (rowSize % 4)) % 4;

    bitmapImage = (unsigned char*)calloc(1, (rowSize + padding) * infoheader->biHeight * 2);

    /*printf("%d\n", infoheader-> biWidth);*/
  
    if(!bitmapImage){                   /* verify calloc */
        free(bitmapImage);
        fclose(fileptr);
        printf("Failed to allocate memory\n");
        exit(1);
    }

    fread(bitmapImage, (rowSize + padding) * infoheader->biHeight, 1, fileptr);

    if(bitmapImage == NULL){
        fclose(fileptr);
        printf("Failed to read bitmap image data\n");
        exit(1);
    }

    fclose(fileptr);
    return bitmapImage;
}

void writeimage(char* outfile, BITMAPFILEHEADER* fileheader, BITMAPINFOHEADER* infoheader, unsigned char* bitmapimage){
    FILE *outptr;
    int rowSize;
    int padding;
    outptr = fopen(outfile, "wb");

    fwrite(&(fileheader->bfType), sizeof(WORD), 1, outptr); 
    if(fileheader->bfType != 0x4D42){      /* Check if bmp file */
        fclose(outptr);
        printf("Output file not of type .BMP\n");
        exit(1); 
    }

    fwrite(&(fileheader->bfSize), sizeof(DWORD), 1, outptr); 
    fwrite(&(fileheader->bfReserved1), sizeof(WORD), 1, outptr); 
    fwrite(&(fileheader->bfReserved2), sizeof(WORD), 1, outptr); 
    fwrite(&(fileheader->bfOffBits), sizeof(DWORD), 1, outptr); 

    fwrite(&(infoheader->biSize), sizeof(DWORD), 1, outptr); /* start of infoheader */
    fwrite(&(infoheader->biWidth), sizeof(LONG), 1, outptr); 
    fwrite(&(infoheader->biHeight), sizeof(LONG), 1, outptr); 
    fwrite(&(infoheader->biPlanes), sizeof(WORD), 1, outptr); 
    fwrite(&(infoheader->biBitCount), sizeof(WORD), 1, outptr); 
    fwrite(&(infoheader->biCompression), sizeof(DWORD), 1, outptr); 
    fwrite(&(infoheader->biSizeImage), sizeof(DWORD), 1, outptr); 
    fwrite(&(infoheader->biXPelsPerMeter), sizeof(LONG), 1, outptr); 
    fwrite(&(infoheader->biYPelsPerMeter), sizeof(LONG), 1, outptr); 
    fwrite(&(infoheader->biClrUsed), sizeof(DWORD), 1, outptr); 
    fwrite(&(infoheader->biClrImportant), sizeof(DWORD), 1, outptr); 

    rowSize = infoheader->biWidth * 3;
    padding = (4 - (rowSize % 4)) % 4;

    fwrite(bitmapimage, (rowSize + padding) * infoheader->biHeight, 1, outptr);

    fclose(outptr);
    return;
}

void get_color(unsigned char* bitmapimage, int width, int height, int x, int y, int* blue, int* green, int* red){

    int padding;
    int index;
    int rowSize;
    rowSize = width * 3;
    padding = (4-(rowSize%4))%4;

    if(x >= 0 && x < width && y >= 0 && y < height){
        index = y * (rowSize + padding) + x * 3;
        *blue = bitmapimage[index];
        *green = bitmapimage[index+1];
        *red = bitmapimage[index+2];
    }
    else{
        *blue = 0;
        *green = 0;
        *red = 0;
    }
}

void edit(unsigned char* bitmap, BITMAPINFOHEADER* infoheader, float brightness, int starty, int endy, unsigned char* bitmapimage){
    int x;
    int y;
    int blue;
    int newblue;
    int red;
    int newred;
    int green;
    int newgreen;
    int index;
    int padding;

    padding = (4 - ((infoheader->biWidth * 3) % 4)) % 4;

    y = starty;
    while(y < endy){      /* y < infoheader->biHeight */
        x = 0;
        while(x < infoheader->biWidth){    
            get_color(bitmap, infoheader->biWidth, infoheader->biHeight, x, y, &blue, &green, &red);

            newblue = blue + (brightness * 255);
            newgreen = green + (brightness * 255);
            newred = red + (brightness * 255);
            if(newblue > 255){
                newblue = 255;
            }
            if(newgreen > 255){
                newgreen = 255;
            }
            if(newred > 255){
                newred = 255;
            }
            
            index = y * (infoheader->biWidth * 3 + padding) + x * 3;

            bitmapimage[index] = newblue;          /* Append colors onto image data */
            bitmapimage[index+1] = newgreen;
            bitmapimage[index+2] = newred;

            x += 1;            
        }
        y += 1;
    }
    return;
}