#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include <arpa/inet.h>

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
void get_color_bilinear(unsigned char*, int, int, float, float, int*, int*, int*);
unsigned char* merge(unsigned char*, unsigned char*, BITMAPINFOHEADER*, BITMAPINFOHEADER*, float, float, float);

int main(int argc, char *argv[]){
    BITMAPFILEHEADER fileheader1;
    BITMAPINFOHEADER infoheader1;
    BITMAPFILEHEADER fileheader2;
    BITMAPINFOHEADER infoheader2;
    unsigned char *bitmapImage1;
    unsigned char *bitmapImage2;
    unsigned char *bitmapRes;
    float ratio;
    float scale_x;
    float scale_y;
    char *outputtype;
    char dot;
    char *ratioptr;
    dot = '.';

    if(argc != 5){
        printf("Incorrect number of parameters, expected:\n[programname] [imagefile1] [imagefile2] [ratio] [outputfile]\n");
        return 1;
    }
    
    ratio = strtod(argv[3], &ratioptr);
    if(strcmp(ratioptr, "") != 0){
        printf("Ratio must be a float between 1 and 0.\n");
        return 1;
    }
    if(ratio > 1 || ratio < 0){
        printf("Ratio must be a float between 1 and 0.\n");
        return 1;
    }

    outputtype = strrchr(argv[4], dot);
    if(strcmp(outputtype, ".bmp") != 0){
        printf("Invalid output file type. Must be .bmp\n");
        return 1;
    }

    bitmapImage1 = readimage(argv[1], &fileheader1, &infoheader1);
    bitmapImage2 = readimage(argv[2], &fileheader2, &infoheader2);

    /*printf("width1: %d, height1: %d\n", infoheader1.biWidth, infoheader1.biHeight);
    printf("width2: %d, height2: %d\n", infoheader2.biWidth, infoheader2.biHeight);*/

    if(infoheader1.biWidth == infoheader2.biWidth){
        scale_x = 1;
        if(infoheader1.biHeight > infoheader2.biHeight){
            scale_y = (float)abs(infoheader2.biHeight) / abs(infoheader1.biHeight);
            bitmapRes = merge(bitmapImage1, bitmapImage2, &infoheader1, &infoheader2, ratio, scale_x, scale_y);
            writeimage(argv[4], &fileheader1, &infoheader1, bitmapRes);
        }
        else{
            scale_y = (float)abs(infoheader1.biHeight) / abs(infoheader2.biHeight);
            bitmapRes = merge(bitmapImage2, bitmapImage1, &infoheader2, &infoheader1, 1 - ratio, scale_x, scale_y);
            writeimage(argv[4], &fileheader2, &infoheader2, bitmapRes);
        }
    }
    else if(infoheader1.biWidth < infoheader2.biWidth){
        scale_x = (float)infoheader1.biWidth / infoheader2.biWidth;
        scale_y = (float)abs(infoheader1.biHeight) / abs(infoheader2.biHeight);
        bitmapRes = merge(bitmapImage2, bitmapImage1, &infoheader2, &infoheader1, 1 - ratio, scale_x, scale_y);
        writeimage(argv[4], &fileheader2, &infoheader2, bitmapRes);
    }
    else{
        scale_x = (float)infoheader2.biWidth / infoheader1.biWidth;
        scale_y = (float)abs(infoheader2.biHeight) / abs(infoheader1.biHeight);
        bitmapRes = merge(bitmapImage1, bitmapImage2, &infoheader1, &infoheader2, ratio, scale_x, scale_y);
        writeimage(argv[4], &fileheader1, &infoheader1, bitmapRes);
    }

    free(bitmapImage1);
    free(bitmapImage2);
    free(bitmapRes);
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

void get_color_bilinear(unsigned char* bitmapimage, int width, int height, float x, float y, int* blue, int* green, int* red){
    int x1;
    int x2;
    float dx;
    int y1;
    int y2;
    float dy;
    int red_left_upper, red_right_upper, red_left_lower, red_right_lower;
    int green_left_upper, green_right_upper, green_left_lower, green_right_lower;
    int blue_left_upper, blue_right_upper, blue_left_lower, blue_right_lower;
    int red_left, red_right;
    int blue_left, blue_right;
    int green_left, green_right;

    x1 = (int)x;
    x2 = (int)x;
    if(x - x2 > 0){
        x2++;
    }
    y2 = (int)y;
    y1 = (int)y;
    if(y - y1 > 0){
        y1++;
    }
    dx = x - x1;
    dy = y - y2;

    if(x2>=width){
        x2 = x1;
    }

    if(y1>=height){
        y1 = y2;
    }

    get_color(bitmapimage, width, height, x1, y2, &blue_left_upper, &green_left_upper, &red_left_upper);
    get_color(bitmapimage, width, height, x2, y2, &blue_right_upper, &green_right_upper, &red_right_upper);
    get_color(bitmapimage, width, height, x1, y1, &blue_left_lower, &green_left_lower, &red_left_lower);
    get_color(bitmapimage, width, height, x2, y1, &blue_right_lower, &green_right_lower, &red_right_lower);

    red_left = red_left_upper * (1-dy) + red_left_lower * dy;
    red_right = red_right_upper * (1-dy) + red_right_lower * dy;
    *red = red_left * ( 1- dx) + red_right * dx;

    green_left = green_left_upper * (1-dy) + green_left_lower * dy;
    green_right = green_right_upper * (1-dy) + green_right_lower * dy;
    *green = green_left * ( 1- dx) + green_right * dx;

    blue_left = blue_left_upper * (1-dy) + blue_left_lower * dy;
    blue_right = blue_right_upper * (1-dy) + blue_right_lower * dy;
    *blue = blue_left * ( 1- dx) + blue_right * dx;
}

unsigned char* merge(unsigned char* bitmap1, unsigned char* bitmap2, BITMAPINFOHEADER* infoheader1, BITMAPINFOHEADER* infoheader2, float ratio, float scale_x, float scale_y){
    int x;
    int y;
    float scaled_x;
    float scaled_y;
    int blue;
    int blue1;
    int blue2;
    int red;
    int red1;
    int red2;
    int green;
    int green1;
    int green2;
    int index;
    int padding;
    unsigned char* bitmapimage;

    padding = (4 - ((infoheader1->biWidth * 3) % 4)) % 4;

    bitmapimage = (unsigned char*)calloc(1, ((infoheader1->biWidth * 3) + padding) * infoheader1->biHeight*2);

    y = 0;
    while(y < infoheader1->biHeight){
        x = 0;
        scaled_y = y * scale_y;
        while(x < infoheader1->biWidth){
            scaled_x = x * scale_x;
            get_color(bitmap1, infoheader1->biWidth, infoheader1->biHeight, x, y, &blue, &green, &red);
            blue1 = blue;
            green1 = green;
            red1 = red;

            get_color_bilinear(bitmap2, infoheader2->biWidth, infoheader2->biHeight, scaled_x, scaled_y, &blue, &green, &red);
            blue2 = blue;
            green2 = green;
            red2 = red;

            red = red1 * ratio + red2 * (1-ratio);
            green = green1 * ratio + green2 * (1-ratio);
            blue = blue1 * ratio + blue2 * (1-ratio);
            
            index = y * (infoheader1->biWidth * 3 + padding) + x * 3;

            bitmapimage[index] = blue;          /* Append colors onto image data */
            bitmapimage[index+1] = green;
            bitmapimage[index+2] = red;

            x += 1;            
        }
        y += 1;
    }
    return bitmapimage;
}