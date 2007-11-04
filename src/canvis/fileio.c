/*
    Copyright (C) 2004 Attila Vass

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <png.h>


typedef struct ColorStructure
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned long Cnt;
    float MinDist;
    int MinIndex;
} ColorStructure;
extern ColorStructure *MyClrStr;

#ifdef __cplusplus
//extern "C" {
#endif /* __cplusplus */


int
SavePNGFilePalette(char *FileName, char *SrcBuffer, int Width, int Height, int NumOfComponent,
                   int NumClrs)
{
    FILE *fp = NULL;
    register int dbp, tmpdbp, x, y, a;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height, bpl;
    int depth, colortype;
    char *lbf, *tmplbptr;
    char *lbfptrs[1];
    unsigned char rv, gv, bv;
    png_color pngcolorpalette[256];

    if ((FileName == NULL) || (SrcBuffer == NULL) || (NumOfComponent < 1) || (Width < 1)
        || (Height < 1))
        return (0);

    if ((fp = fopen(FileName, "w")) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_PALETTE : Can not open '%s' for writing...", FileName);
        fflush(stdout);
#endif
        return (0);
    }

    fseek(fp, 0L, SEEK_SET);

    if ((png_ptr =
         png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, NULL, NULL)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_PALETTE : Can not allocate Write_Struct...");
        fflush(stdout);
#endif
        fclose(fp);
        return (0);
    }

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_PALETTE : Can not allocate Info_Struct...");
        fflush(stdout);
#endif
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        fclose(fp);
        return (0);
    }

    png_init_io(png_ptr, fp);
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_PAETH);    // PNG_FILTER_NONE, PNG_FILTER_PAETH
    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    width = (png_uint_32) Width;
    height = (png_uint_32) Height;
    depth = 8;
    colortype = PNG_COLOR_TYPE_PALETTE;
    bpl = width;                // 1 byte entry for all colors ( palette )
    png_set_IHDR(png_ptr, info_ptr, width, height, depth, colortype, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    info_ptr->valid |= PNG_INFO_PLTE;
    info_ptr->palette = pngcolorpalette;
    info_ptr->num_palette = 256;
    for (a = 0; a < info_ptr->num_palette; a++) {
        pngcolorpalette[a].red = 0;
        pngcolorpalette[a].green = 0;
        pngcolorpalette[a].blue = 0;
    }

    for (a = 0; a < NumClrs; a++) {
        pngcolorpalette[a].red = MyClrStr[a].R;
        pngcolorpalette[a].green = MyClrStr[a].G;
        pngcolorpalette[a].blue = MyClrStr[a].B;
    }

    png_write_info(png_ptr, info_ptr);
    if ((lbf = (char *) malloc((size_t) bpl)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_PALETTE : Can not allocate linebuffer... (%d bytes)", (int) bpl);
        fflush(stdout);
#endif
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return (0);
    }
    for (y = 0, dbp = 0; y < height; y++, dbp += (width * 3)) {
        lbfptrs[0] = lbf;
        tmplbptr = lbf;
        tmpdbp = dbp;
        for (x = 0; x < width; x++) {
            rv = ((unsigned char) SrcBuffer[x * NumOfComponent + y * Width * NumOfComponent + 0]);
            gv = ((unsigned char) SrcBuffer[x * NumOfComponent + y * Width * NumOfComponent + 1]);
            bv = ((unsigned char) SrcBuffer[x * NumOfComponent + y * Width * NumOfComponent + 2]);
            for (a = 0; a < NumClrs; a++) {
                if (rv == MyClrStr[a].R) {
                    if (gv == MyClrStr[a].G) {
                        if (bv == MyClrStr[a].B) {
                            *(tmplbptr++) = (unsigned char) a;
                            a = NumClrs + 10;
                        }
                    }
                }
            }
            if (a < (NumClrs + 10))
                *(tmplbptr++) = (unsigned char) a;
        }
        png_write_rows(png_ptr, (png_bytepp) lbfptrs, 1);
    }
    png_write_flush(png_ptr);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fflush(fp);
    fclose(fp);

    if ((fp = fopen(FileName, "r")) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE : Can not open for read-back '%s'.", FileName);
        fflush(stdout);
#endif
        return (1);
    }

    fseek(fp, 0L, SEEK_END);
    a = (int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return (a);
}

int
SavePNGFileRGB(char *FileName, char *SrcBuffer, int Width, int Height, int NumOfComponent)
{
    FILE *fp = NULL;
    register int dbp, tmpdbp, x, y, a;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height, bpl;
    int depth, colortype;
    char *lbf, *tmplbptr;
    char *lbfptrs[1];

    if ((FileName == NULL) || (SrcBuffer == NULL) || (NumOfComponent < 1) || (Width < 1)
        || (Height < 1))
        return (0);

    if ((fp = fopen(FileName, "w")) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_RGB : Can not open '%s' for writing...", FileName);
        fflush(stdout);
#endif
        return (0);
    }

    fseek(fp, 0L, SEEK_SET);

    if ((png_ptr =
         png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, NULL, NULL)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_RGB : Can not allocate Write_Struct...");
        fflush(stdout);
#endif
        fclose(fp);
        return (0);
    }

    if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_RGB : Can not allocate Info_Struct...");
        fflush(stdout);
#endif
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        fclose(fp);
        return (0);
    }

    png_init_io(png_ptr, fp);
    png_set_filter(png_ptr, 0, PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_PAETH);    // PNG_FILTER_NONE, PNG_FILTER_PAETH
    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    width = (png_uint_32) Width;
    height = (png_uint_32) Height;
    depth = 8;
    colortype = PNG_COLOR_TYPE_RGB;
    bpl = 3 * width;
    if (NumOfComponent == 4) {
        colortype = PNG_COLOR_TYPE_RGB_ALPHA;
        bpl = 4 * width;
    }
    png_set_IHDR(png_ptr, info_ptr, width, height, depth, colortype, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
#ifdef BYTE_LITTLE_ENDIAN
    if (depth > 8)
        png_set_packswap(png_ptr);
#endif // BYTE_LITTLE_ENDIAN
    if ((lbf = (char *) malloc((size_t) bpl)) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_RGB : Can not allocate linebuffer... (%d bytes)", (int) bpl);
        fflush(stdout);
#endif
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return (0);
    }
    for (y = 0, dbp = 0; y < height; y++, dbp += (width * 3)) {
        lbfptrs[0] = lbf;
        tmplbptr = lbf;
        tmpdbp = dbp;
        for (x = 0; x < width; x++) {
            *(tmplbptr++) = SrcBuffer[tmpdbp++];
            *(tmplbptr++) = SrcBuffer[tmpdbp++];
            *(tmplbptr++) = SrcBuffer[tmpdbp++];
            if (colortype == PNG_COLOR_TYPE_RGB_ALPHA)
                *(tmplbptr++) = SrcBuffer[tmpdbp++];
        }
        png_write_rows(png_ptr, (png_bytepp) lbfptrs, 1);
    }
    png_write_flush(png_ptr);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fflush(fp);
    fclose(fp);

    if ((fp = fopen(FileName, "r")) == NULL) {
#if VERBOSE>=1
        printf("\nPNG WRITE_RGB : Can not open for read-back '%s'.", FileName);
        fflush(stdout);
#endif
        return (1);
    }

    fseek(fp, 0L, SEEK_END);
    a = (int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return (a);
}


int
SavePNGFile(char *FileName, char *SrcBuffer, int Width, int Height, int NumOfComponent)
{
    int NumClrs = 0;

    NumClrs = CollectColors(SrcBuffer, Width, Height, NumOfComponent);
    if (NumClrs <= 256)
        return (SavePNGFilePalette(FileName, SrcBuffer, Width, Height, NumOfComponent, NumClrs));
    else
        return (SavePNGFileRGB(FileName, SrcBuffer, Width, Height, NumOfComponent));

}
