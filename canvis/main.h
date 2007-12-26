#if defined (__cplusplus) && (!(defined (__CPLUSLIBS)))
extern "C"
{
#endif

#ifndef CHNK
#define	CHNK(b)	(((b)-((int)b)))
#endif

#ifndef	ABS
#define	ABS(a)	(((a)<0)?(-(a)):((a)))
#endif

#ifndef FLINTSAME
#define	FLINTSAME(a)	(((((float)a-(int)a)>(float)0.0001)?(0):(1)))
#endif

//#define       ALLOC_TRACK     1


#define MIN_ZOOMFACTOR	(float)0.015625
#define MAX_ZOOMFACTOR	(float)256

#define	M_RED		1
#define	M_GREEN		2
#define	M_BLUE		4
#define	M_ALPHA		8
#define	M_BLACKMASK	16      // For Astro

/* From graphics.c */
    typedef struct MyImagesStructure
    {
        char *Buffer;
        int BufferSize;
        int X;
        int Y;
        int HotPosX;
        int HotPosY;
        int Width;
        int Height;
        float ZoomFactor;
        float FadeFactor;

        char *OriginalBuffer;
        int OriginalBufferSize;
        int OriginalX;
        int OriginalY;
        int OriginalWidth;
        int OriginalHeight;

        char *ModifiedBuffer;
        int ModifiedBufferSize;
        int ModifiedX;
        int ModifiedY;
        int ModifiedWidth;
        int ModifiedHeight;

        float ScaleX;
        float ScaleY;
        int CropTargetStartX;
        int CropTargetStartY;
        int ScaleTargetWidth;
        int ScaleTargetHeight;
        unsigned int NumOfComponent;
        int Mask;               // R/G/B mask to show
        char Name[64];
    } MyImagesStructure;

    typedef struct ColorStructure
    {
        unsigned char R;
        unsigned char G;
        unsigned char B;
        unsigned long Cnt;
        float MinDist;
        int MinIndex;
    } ColorStructure;

#define	KM_NONE		0
#define	KM_SHIFT	1
#define KM_CTRL		2
#define	KM_ALT		4
    enum
    { SK_F1 = 210, SK_F2, SK_F3, SK_F4, SK_F5, SK_F6, SK_F7, SK_F8, SK_F9, SK_F10, SK_F11, SK_F12,
        SK_UPAR, SK_DNAR, SK_LFAR, SK_RIAR, SK_HOME, SK_PGUP, SK_PGDN, SK_END
    };

#define	BKG_R	((unsigned char)0x00)
#define	BKG_G	((unsigned char)0x00)
#define	BKG_B	((unsigned char)0x00)


#ifndef MAIN_C
    extern char CommandLineBuffer[256];
#endif

#ifndef GRAPHICS_C

    extern int CreateMainWindow(void);
    extern void MainLoop(void);
    extern char *CreatePicture(unsigned int *ret_width, unsigned int *ret_height,
                               int *SrcBufferSize);
    extern int UpdateImageToModified(int a);
    extern int MoveImageModifiedToOriginal(int a);
    extern int UpdateImageToOriginal(int a);
    extern int InitializeNextImageStructure(void);
    extern void GR_DrawRectangle(int gx1, int gy1, int gx2, int gy2, int mode, unsigned char GRv_FwdR, unsigned char GRv_FwdG, unsigned char GRv_FwdB); // mode : 0-put 1-antialias 2-negate
    extern void GR_DrawFillRectangle(int gx1, int gy1, int gx2, int gy2, int mode,
                                     unsigned char GRv_FwdR, unsigned char GRv_FwdG,
                                     unsigned char GRv_FwdB);
    extern void GR_DrawLine(int gx1, int gy1, int gx2, int gy2, int mode, unsigned char GRv_FwdR, unsigned char GRv_FwdG, unsigned char GRv_FwdB);      // mode : 0-put 1-antialias 2-negate
    extern void GR_PutPixel(int x, int y, unsigned char GRv_BckR, unsigned char GRv_BckG,
                            unsigned char GRv_BckB);
    extern void GR_GetPixel(int x, int y, unsigned char *GRv_BckR, unsigned char *GRv_BckG,
                            unsigned char *GRv_BckB);
    extern void GR_Refresh(void);

    extern MyImagesStructure *MyImages;
    extern int NumberOfImages;
    extern unsigned char KeyModifers;
    extern int ActiveButton;
    extern int SelectedImage, PreviousSelectedImage;
    extern int DrawTarget;
    extern int WorkWidth, WorkHeight;
    extern int DisplayWidth, DisplayHeight;
    extern char TmpBuffer[256];

#endif

    extern int SavePNGFile(char *FileName, char *SrcBuffer, int Width, int Height,
                           int NumOfComponent);

#ifndef FUNCTIONS_C

/* From functions.c */
    extern void SavePicture(void *Data);
    extern void PNGButton(void *Data);

    extern int ComputeScaling(int a, int target_x, int target_y, int method);   // if target_xy is <0, using scaleXY
    extern int ResizePictureKey(int modi, char key);

    extern void MiscellaneousButton(void *Data);
    extern int MiscellaneousKey(int modi, char key);

    extern void RGB_to_HSV(unsigned char R, unsigned char G, unsigned char B, float *H, float *S,
                           float *V);
    extern void HSV_to_RGB(float H, float S, float V, unsigned char *R, unsigned char *G,
                           unsigned char *B);
#endif

#if defined (__cplusplus) && (!(defined (__CPLUSLIBS)))
}
#endif
