#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <X11/Xlib.h>
#include <time.h>

#define FUNCTIONS_C

#ifdef __cplusplus
//extern "C" {
#endif /* __cplusplus */

#include "main.h"

extern	Window		WorkWindow;
extern	Display		*WorkDisplay;
extern	unsigned int	InternalKeyModifiers;

#ifndef ABS
#define	ABS(x)	(((x)<0)?(-(x)):((x)))
#endif

extern void PutMyStringB(char* TB,int xsz, int ysz, char *Text,int x, int y, int usebignums, int zoom);

int	MessageID=0;

char	*FileBuffer=NULL;
unsigned int	BufferLength=0;

unsigned char	DataBytes[8],DataBytesMask[8];
char		SearchTerm[16];
int		SearchPos,SpeedPos=0,DataOffset=0x3FFF,DoubleBytes=0,ShowSpeed=1,ShowKeys=1,ShowTimes=1,Show0=1,UseSigned=0,Use12bitSigned=0,NoSignumDraw=0,LineType=0;
int		ShowValueUnderPointerX=-1,ShowValueUnderPointerY=-1;
unsigned int	SampleDuration=0,NumberOfSamples=0;
float		ZoomVal=1.0f,SZoomVal=1.0f,DZoomVal=1.0f,DShiftVal=0.0f;
int		MinDataValues[8],MaxDataValues[8];

typedef struct MessageStructure
{
	unsigned int	ID;
	unsigned int	Occurance;
	unsigned char	BL;
}MessageStructure;

MessageStructure	*Messages=NULL;

typedef struct CStructure
{
	unsigned char	R;
	unsigned char	G;
	unsigned char	B;
}CStructure;

CStructure DataBytesColor[8] = {
{255,0,0},
{0,255,0},
{0,0,255},
{255,255,0},
{255,0,255},
{0,255,255},
{255,127,0},
{255,0,127}};

char	*TmpBufPtr=NULL;
int	TmpBufSize=-1;
int	RX,RY,GX,GY,BX,BY,RS,GS,BS;
int	DRed=0,DGreen=0,DBlue=0;
float	DLuma=255.0,DH=0.0f,DS=0.0f,DV=0.0f;

#define	RED_LUMA	0.212671f
#define	GREEN_LUMA	0.715160f
#define	BLUE_LUMA	0.072169f

void SavePicture(void* Data)
{
char*	FileName=NULL;
int	b;

	FileName="saved.png";
	GR_DrawFillRectangle((int)((DisplayWidth>>1)-200), (WorkHeight-32), (int)((DisplayWidth>>1)+200), (WorkHeight-2), 0, BKG_R, BKG_G, BKG_B);
	sprintf(TmpBuffer,"Saving '%s' as PNG... Size : %dx%d",FileName,MyImages[SelectedImage].Width,MyImages[SelectedImage].Height);
	b=GetMyStringLength(TmpBuffer,0,1);
	PutMyString(TmpBuffer,((int)(DisplayWidth-b)>>1),(WorkHeight-30),0,1);
	CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
	if((b=SavePNGFile(FileName,MyImages[SelectedImage].Buffer,MyImages[SelectedImage].Width,MyImages[SelectedImage].Height,MyImages[SelectedImage].NumOfComponent))==0)
		PutMyString("### Error ###",((int)(DisplayWidth>>1)),(WorkHeight-15),0,1);
	else
	{
		sprintf(TmpBuffer,"Ok. %d bytes.",b);
		b=GetMyStringLength(TmpBuffer,0,1);
		PutMyString(TmpBuffer,((int)(DisplayWidth-b)>>1),(WorkHeight-15),0,1);
	}
	ActiveButton=-1;
	DrawButtons();
	CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
}

struct ImageBufferStructure
{
	unsigned char	R,G,B;
};

extern struct ImageBufferStructure *ImageBuffer;


ColorStructure	*MyClrStr=NULL;
unsigned int	MyClrStrSize=0;

int CollectColors(char *BPTR,int TTX,int TTY,int NUMCO)
{
register int		x,y,tx=TTX,ty=TTY,nuco=NUMCO;
register unsigned char	*SrcBuf=(unsigned char*)BPTR;
register unsigned char	vR,vG,vB;
register unsigned int	a,b,c,NumClrs=0;
register unsigned int	*ClrSpUpBuf=NULL;

// First try using memory vs. CPU, this can speed up by 300x

	if((ClrSpUpBuf=(unsigned int*)malloc((size_t)((unsigned int)16777216*sizeof(unsigned int))))!=NULL)	// Wow, use memory!!!
	{
		for(a=0;a<16777216;a++) ClrSpUpBuf[a]=0;
		for(y=0;y<ty;y++)
		{
			for(x=0;x<tx;x++)
			{
				a=(unsigned int)SrcBuf[x*nuco+y*tx*nuco];		// R
				b=(unsigned int)SrcBuf[x*nuco+y*tx*nuco+1];
				c=(unsigned int)SrcBuf[x*nuco+y*tx*nuco+2];
				++ClrSpUpBuf[(a<<16)+(b<<8)+c];
			}
		}
		c=0;
		for(a=0;a<16777216;a++) if(ClrSpUpBuf[a]!=0) ++NumClrs;
		MyClrStrSize=NumClrs;
		if((MyClrStr=(ColorStructure *)realloc((void*)MyClrStr,(size_t)MyClrStrSize*sizeof(ColorStructure)))==NULL)
		{
			printf("\n\nCan not allocate ColorStructure for %d colors...",MyClrStrSize);fflush(stdout);
			return;
		}
		b=0;
		for(a=0;a<16777216;a++)
		{
			if(ClrSpUpBuf[a]!=0)
			{
				MyClrStr[b].R=(unsigned char)((a>>16)&0xFF);
				MyClrStr[b].G=(unsigned char)((a>>8)&0xFF);
				MyClrStr[b].B=(unsigned char)(a&0xFF);
				MyClrStr[b++].Cnt=ClrSpUpBuf[a];
			}
		}

		free((void*)ClrSpUpBuf);
		return(NumClrs);
	}

	if(MyClrStrSize==0)
	{
		MyClrStrSize=1000;
		if((MyClrStr=(ColorStructure *)realloc((void*)MyClrStr,(size_t)MyClrStrSize*sizeof(ColorStructure)))==NULL)
		{
			printf("\n\nCan not allocate ColorStructure for %d colors...",MyClrStrSize);fflush(stdout);
			return;
		}
		for(a=0;a<1000;a++)
		{
			MyClrStr[a].R=0;MyClrStr[a].G=0;MyClrStr[a].B=0;MyClrStr[a].Cnt=0;
		}
	}

	for(y=0;y<ty;y++)
	{
		for(x=0;x<tx;x++)
		{
			vR=SrcBuf[x*nuco+y*tx*nuco];
			vG=SrcBuf[x*nuco+y*tx*nuco+1];
			vB=SrcBuf[x*nuco+y*tx*nuco+2];
			a=0;
			while(a<NumClrs)
			{
				if((MyClrStr[a].R==vR)&&(MyClrStr[a].G==vG)&&(MyClrStr[a].B==vB))
				{
					++MyClrStr[a].Cnt;
					a=NumClrs+5;
				}
				++a;
			}
			if(a!=(NumClrs+6))
			{
				if((++NumClrs)>MyClrStrSize)		// realloc needed
				{
					b=(NumClrs-1);
					MyClrStrSize+=1000;
					if((MyClrStr=(ColorStructure *)realloc((void*)MyClrStr,(size_t)MyClrStrSize*sizeof(ColorStructure)))==NULL)
					{
						printf("\n\nCan not allocate ColorStructure for %d colors...",MyClrStrSize);fflush(stdout);
						return;
					}
					for(a=(b+1);a<MyClrStrSize;a++)
					{
						MyClrStr[a].R=0;MyClrStr[a].G=0;MyClrStr[a].B=0;MyClrStr[a].Cnt=0;
					}
				}
				else
					b=(NumClrs-1);
				MyClrStr[b].R=vR;
				MyClrStr[b].G=vG;
				MyClrStr[b].B=vB;
				MyClrStr[b].Cnt=1;
			}
		}
	}
return(NumClrs);
}


int GetNextEntry(void)
{
register int a;

	while(SearchPos<BufferLength)
	{
		if(FileBuffer[SearchPos]==SearchTerm[0])
		{
			a=0;
			while(FileBuffer[SearchPos+a]==SearchTerm[a]) ++a;
			if(SearchTerm[a]=='\0') return(1);
		}
		++SearchPos;
	}
return(0);
}

unsigned int GetMID(int a)
{
register unsigned int val=0,b;

	for(b=0;b<3;b++)
	{
		val<<=4;
		if(FileBuffer[a+b]>='A')
			val+=(unsigned int)((FileBuffer[a+b]-'A')+10);
		else
			val+=(unsigned int)(FileBuffer[a+b]-'0');
	}
return(val);
}

int GetData(int a)
{
register unsigned int val=0,b;
int	Bl;

	if((FileBuffer[a]>'9')||(FileBuffer[a]<'0')) return(-1);
	Bl=(int)(FileBuffer[a]-'0');

	for(b=0;b<8;b++) DataBytes[b]=0;
	++a;
//printf("\n");fflush(stdout);
	for(b=0;b<Bl;b++)
	{
		if(!((FileBuffer[a+(b<<1)]>='0')&&(FileBuffer[a+(b<<1)]<='9') || (FileBuffer[a+(b<<1)]>='A')&&(FileBuffer[a+(b<<1)]<='F')))	 return(-1);// Invalid...
		if(!((FileBuffer[a+((b<<1)+1)]>='0')&&(FileBuffer[a+((b<<1)+1)]<='9') || (FileBuffer[a+((b<<1)+1)]>='A')&&(FileBuffer[a+((b<<1)+1)]<='F')))	 return(-1);// Invalid...
		if(FileBuffer[a+(b<<1)]>='A')
			DataBytes[b]+=(unsigned char)((FileBuffer[a+(b<<1)]-'A')+10);
		else
			DataBytes[b]+=(unsigned char)(FileBuffer[a+(b<<1)]-'0');
		DataBytes[b]<<=4;
		if(FileBuffer[a+(b<<1)+1]>='A')
			DataBytes[b]+=(unsigned char)((FileBuffer[a+(b<<1)+1]-'A')+10);
		else
			DataBytes[b]+=(unsigned char)(FileBuffer[a+(b<<1)+1]-'0');
//printf(" %d : '%c%c' = %02x  ",b,FileBuffer[a+(b<<1)],FileBuffer[a+(b<<1)+1],DataBytes[b]);fflush(stdout);
	}
return(Bl);
}

void GetBasicStatistics(void)
{
unsigned int	a,b,c,d,DurationValid=1;
unsigned int	*TmpBfr=NULL;

	sprintf(SearchTerm,"t");SearchPos=0;
	if((TmpBfr=(unsigned int*)malloc(0xFFF*sizeof(int)))==NULL)	// For the IDs
	{

printf("\nError allocating tempbuffer...");fflush(stdout);

	}

	for(b=0;b<0xFFF;b++) TmpBfr[b]=0;NumberOfSamples=0;
	while(GetNextEntry())
	{
		b=GetMID(SearchPos+1);
		if((b>0)&&(b<0xFFF))
		{
			++TmpBfr[b];
			++NumberOfSamples;
		}
		SearchPos+=3;
	}

	a=0;
	for(b=0;b<0xFFF;b++) if(TmpBfr[b]>0) ++a;

	if((Messages=(struct MessageStructure*)malloc((a+1)*sizeof(struct MessageStructure)))==NULL)
	{

printf("\nError allocating ID buffer...");fflush(stdout);

	}

	for(b=0;b<=a;b++)
	{
		Messages[a].ID=0;
		Messages[a].Occurance=0;
		Messages[a].BL=0;
	}

	a=0;
	for(b=0;b<0xFFF;b++)
	{
		if(TmpBfr[b]>0)
		{
			Messages[a].ID=b;
			Messages[a].Occurance=TmpBfr[b];
//			if(b==0x3B) SpeedPos=a;
			if(b==0x3CA) SpeedPos=a;
//			if(b==0x3C8) SpeedPos=a;

			sprintf(SearchTerm,"t");SearchPos=0;c=1;
			while(c)
			{
				if(!GetNextEntry())
					c=0;
				else
				{
					d=GetMID(SearchPos+1);
					if((d>0)&&(d<0xFFF))
					{
						if(d==b)
						{
							if((d=GetData(SearchPos+4))>0)
							{
								Messages[a].BL=d;
							}
							c=0;
						}
					}
				}
				SearchPos+=3;
			}
			++a;
		}
	}


	printf("\nFound %d unique IDs in %d messages...",a,NumberOfSamples);fflush(stdout);
#if 0
	for(a=0;Messages[a].ID!=0;a++)
		printf("\n%3x = %d",Messages[a].ID,Messages[a].Occurance);
	fflush(stdout);
#endif
	free(TmpBfr);

	SampleDuration=0;
	sprintf(SearchTerm,"for");SearchPos=0;
	if(!GetNextEntry())
	{
//		printf("\nPanic, can not locate duration ( 'for' )");fflush(stdout);
		DurationValid=0;
	}
	else
	{
		SearchPos+=3;
		a=0;
		while((FileBuffer[SearchPos+a]>='0')&&(FileBuffer[SearchPos+a]<='9')) ++a;
		b=1;--a;
		for(c=0;c<a;c++) b*=10;
		for(c=0;c<=a;c++)
		{
			SampleDuration+=((unsigned int)(FileBuffer[SearchPos+c]-'0')*b);
			b/=10;
		}
	}
	for(c=0;c<8;c++) DataBytesMask[c]=1;

if(DurationValid) {printf(" Sample Duration is %d seconds.",SampleDuration);fflush(stdout);}
}


void DrawLine(unsigned char* TB,int sx,int sy,int ex,int ey,int tx,int ty,unsigned char R,unsigned char G,unsigned char B)
{
register int	axis,dir=1;
float		oaxis,stp;

	if(ey>ty) ey=ty-1;
	if(ey<0) ey=0;
	if(ex>tx) ex=tx-1;
	if(ex<0) ex=0;
	if(sy>ty) sy=ty-1;
	if(sy<0) sy=0;
	if(sx>tx) sx=tx-1;
	if(sx<0) sx=0;

	if(NoSignumDraw)
	{
		if(((ABS((sx-ex)))>140)||((ABS((sy-ey)))>140))
		{
				TB[ex*3+ey*tx*3]=R;
				TB[ex*3+ey*tx*3+1]=G;
				TB[ex*3+ey*tx*3+2]=B;
				return;
		}
	}

	if((ABS((sx-ex)))>(ABS((sy-ey))))	// stepping on x
	{
		stp=(float)(ey-sy)/(float)(ex-sx);
		oaxis=sy;
		axis=sx;
		if(sx>ex)
		{
			for(axis=sx;axis>=ex;axis--)
			{
				TB[axis*3+(int)oaxis*tx*3]=R;
				TB[axis*3+(int)oaxis*tx*3+1]=G;
				TB[axis*3+(int)oaxis*tx*3+2]=B;
				oaxis-=stp;
			}
		}
		else
		{
			for(axis=sx;axis<=ex;axis+=dir)
			{
				TB[axis*3+(int)oaxis*tx*3]=R;
				TB[axis*3+(int)oaxis*tx*3+1]=G;
				TB[axis*3+(int)oaxis*tx*3+2]=B;
				oaxis+=stp;
			}
		}
		return;
	}

	stp=(float)(ex-sx)/(float)(ey-sy);
	oaxis=sx;
	axis=sy;
	if(sy>ey)
	{
		for(axis=sy;axis>=ey;axis--)
		{
			TB[(int)oaxis*3+axis*tx*3]=R;
			TB[(int)oaxis*3+axis*tx*3+1]=G;
			TB[(int)oaxis*3+axis*tx*3+2]=B;
			oaxis-=stp;
		}
	}
	else
	{
		for(axis=sy;axis<=ey;axis++)
		{
			TB[(int)oaxis*3+axis*tx*3]=R;
			TB[(int)oaxis*3+axis*tx*3+1]=G;
			TB[(int)oaxis*3+axis*tx*3+2]=B;
			oaxis+=stp;
		}
	}
}


/*


case 0x3B  : EM Current // #0#1 Current

case 0x25  :	// Steering  8byte  #0 & #1 : Left = 0x1BC  Middle = 0x000  Righ = 0xE48
case 0x30  :	// Brakes 8byte  #0 : 0x84 = Not pressed, 0x04 = Pressed : #4 strength = 0 not pressed ~ 0x7F fully pressed
case 0x3CA :	// Speed 5byte #2 = speed in km/h
case 0x529 :	// Interlock ( brake transition ) 6byte  #0 : 27/A7 #3 : 85/8D 
case 0x540 :	// Shift Lever  4byte  #0 : 25 rest A5 transition ( bit! )   #1 : 80=P 10=D 40=R 20=N 00=B
case 0x5B6 :	// Doors  3byte  #0 : transition 64 / E4  #2 : 00 = Closed  80 = Driver  40 = Passenger  04 = all 3 rears
case 0x57F :	// Headlights  7byte  #0 transition : 68 / E8  #2 dimming instruments = 10 : no 18 = yes   #1 00 = off  10 = park  30 = on  38 = high beam
case 0x5C8 :	// Cruise 3byte  #0 transition ( 24/A4 )   #2 : 00 = Off  10 = On  


case 0x3A  :	// Gas pedal binary #4 : pressed / not
case 0x348 :    // Gas pedal value #2  ?

case 0x22 : related to steering, result on body sideway info ?
case 0x23 : accel result on body info
case 0xB1 : yaw/pitch/roll ?
case 0xB3 : -"-

case 0x38 : #0 ICE on/off ?  #1-RPM? #3-electric ?
case 0x39 : Dualbyte, ICE #0-1 temp?  #2-3 RPM ?
case 0x3A : engine ?

case 0x3CB : #3 SOC value
case 0x3CD : #3 current ?

case 0x244 : electric ?

case 0x348 : engine   #2 ICE on ?
case 0x3C8 : -"- ?
case 0x3C9 : -"- ?

case 0x528 : active when breaking...
case 0x52C : engine warmup ?

*/

int ProcessImage(void)
{
int		a=SelectedImage,b,c,tx,ty,x,y,vl,vl2;
unsigned char*	TB=NULL;
float		Divider=1.0f,tv;
unsigned char	SpeedBase=0,LastSpd=255;
register unsigned int	InQ=Messages[MessageID].ID;
int		PrSx=-1,PrSy=-1,PrSxT=-1,PrSyT=-1,PrDx[8]={-1,-1,-1,-1,-1,-1,-1,-1},PrDy[8]={-1,-1,-1,-1,-1,-1,-1,-1};
int		LastSpdX=-30;
unsigned int	Skippy=0,scnt;


	for(b=0;b<8;b++) {MinDataValues[b]=0xFFFF;MaxDataValues[b]=0;}

	tx=800;
	if(ShowSpeed)
	{
		if(Messages[MessageID].Occurance<Messages[SpeedPos].Occurance)
		{
			SpeedBase=1;
			Divider=(float)Messages[SpeedPos].Occurance/(float)tx;
		}
		else
		{
			Divider=(float)Messages[MessageID].Occurance/(float)tx;
		}
	}
	else
		Divider=(float)Messages[MessageID].Occurance/(float)tx;

	if(Divider==0.0f)
	{
		printf("\nSince occurance of data is %d, not drawing...",Messages[MessageID].Occurance);fflush(stdout);
		return;
	}

//printf("\nOcc : %d [%d] Div=%1.2f",Messages[MessageID].Occurance,Messages[SpeedPos].Occurance,Divider);fflush(stdout);

	Divider*=DZoomVal;

	ty=500;
	MyImages[a].ModifiedBufferSize=(tx*ty*MyImages[a].NumOfComponent);
	if((MyImages[a].ModifiedBuffer=(char*)realloc((void*)MyImages[a].ModifiedBuffer,(size_t)MyImages[a].ModifiedBufferSize))==NULL)
	{
		printf("\nPanic, not able to realloc (%2.4f x %2.4f scale) => %d byte",MyImages[a].ScaleX,MyImages[a].ScaleY,MyImages[a].ModifiedBufferSize);fflush(stdout);
		return(-1);
	}

	MyImages[a].ModifiedX=MyImages[a].X;
	MyImages[a].ModifiedY=MyImages[a].Y;
	MyImages[a].ModifiedWidth=tx;
	MyImages[a].ModifiedHeight=ty;
	TB=MyImages[a].ModifiedBuffer;

//printf("\nDZ=%1.2f DS=%1.2f",DZoomVal,DShiftVal);fflush(stdout);

	for(y=0;y<ty;y++)
	{
		for(x=0;x<tx;x++)
		{
			TB[x*3+y*tx*3]=10;
			TB[x*3+y*tx*3+1]=20;
			TB[x*3+y*tx*3+2]=17;
		}
	}

	sprintf(SearchTerm,"t");SearchPos=0;x=0;

	if(DShiftVal>0.0f)
	{
		Skippy=(unsigned int)((float)NumberOfSamples*(DShiftVal*DZoomVal*5.0f));

		if(Skippy>(NumberOfSamples-1)) Skippy=0;
//printf("\nSkipping first %d entries",Skippy);fflush(stdout);

		for(scnt=0;scnt<Skippy;scnt++) {++SearchPos;GetNextEntry();}
	}

//printf("\nSP=%d",SearchPos);fflush(stdout);

	while(GetNextEntry())
	{
		b=GetMID(SearchPos+1);
		if((x/Divider)>(tx-1)) x=((tx-1)*Divider);
		if((b>0)&&(b<=0xFFF))
		{
			switch(b)
			{
				case 0xFFF :	//"\ntFFF0:%02x\n\0",Kar
					if(ShowKeys)
					{
						if(PrSx!=-1)
						{
							DrawLine(TB,(int)((float)x/Divider),0,(int)((float)x/Divider),20,tx,ty,255,255,255);
							for(c=20;c<(ty-1);c+=20)
								DrawLine(TB,(int)((float)x/Divider),c,(int)((float)x/Divider),(c+3),tx,ty,40,40,60);
						}
						if(FileBuffer[SearchPos+6]>='a')
							c=(FileBuffer[SearchPos+6]-'a')+10;
						else
							c=(FileBuffer[SearchPos+6]-'0');
						c<<=4;
						if(FileBuffer[SearchPos+7]>='A')
							c+=(FileBuffer[SearchPos+7]-'a')+10;
						else
							c+=(FileBuffer[SearchPos+7]-'0');
						if(c<0x20) c='.';
						else
							if(c==0x20) c='^';
						sprintf(TmpBuffer,"%c",c);
						PutMyStringB(TB,tx,ty,TmpBuffer,(int)((float)x/Divider)-2,23,0,1);
//printf("\n%d : '%s'",c,TmpBuffer);fflush(stdout);
//printf("\nKar : %c%c",FileBuffer[SearchPos+6],FileBuffer[SearchPos+7]);fflush(stdout);
					}
				break;
				case 0xFFE :	// "\ntFFE0:%06d\n\0",curtime);
					if(ShowTimes)
					{
						if(PrSx!=-1) DrawLine(TB,(int)((float)x/Divider),0,(int)((float)x/Divider),5,tx,ty,80,80,80);
					}
				break;

#if 1
				case 0x3CA :
					if(ShowSpeed)
					{
						if((b=GetData(SearchPos+4))>0)
						{
							vl2=(ty-1)-(int)((float)DataBytes[2]*ZoomVal*SZoomVal);
							if(vl2<0) vl2=0;
							vl=(int)((float)x/Divider);
							if(PrSx!=-1) DrawLine(TB,PrSx,PrSy,vl,vl2,tx,ty,250,255,200);

							if((LastSpd!=DataBytes[2])&&((LastSpdX+20)<vl))
							{
								sprintf(TmpBuffer,"%2d",(unsigned char)((float)DataBytes[2]*0.625f));
								PutMyStringB(TB,tx,ty,TmpBuffer,vl,vl2,0,1);
								DrawLine(TB,vl-2,vl2,vl+2,vl2,tx,ty,250,255,0);
								LastSpd=DataBytes[2];
								LastSpdX=vl;
							}

							PrSx=vl;PrSy=vl2;
							if(SpeedBase) ++x;
						}
					}
				break;
#endif

#if 0
				case 0x3C8 :
					if(ShowSpeed)
					{
						if((b=GetData(SearchPos+4))>0)
						{
							vl2=(ty-1)-(int)((float)(((int)(DataBytes[2])<<8)+(int)(DataBytes[3])&0xFFFF)*ZoomVal*SZoomVal);
							if(vl2<0) vl2=0;
							vl=(int)((float)x/Divider);
							if(PrSx!=-1) DrawLine(TB,PrSx,PrSy,vl,vl2,tx,ty,250,255,200);
							PrSx=vl;PrSy=vl2;
							if(SpeedBase) ++x;
						}
					}
				break;
#endif

#if 0
				case 0x3B :
					if(ShowSpeed)
					{
						if((b=GetData(SearchPos+4))>0)
						{
							if((char)DataBytes[0]<0)
							{
								vl=(((int)((int)((char)DataBytes[0]))*256)+(int)(DataBytes[1]));
								vl|=0xF000;
							}
							else
							{
								vl=(((int)(DataBytes[0])<<8)+(int)(DataBytes[1])&0xFFFF);
								if((unsigned char)DataBytes[0]>0x7)
									vl|=0xFFFFF000;
							}

//							vl=(((int)(DataBytes[0])<<8)+(int)(DataBytes[1])&0xFFFF);
							tv=(float)vl;
							tv*=30.0f;
							tv/=100.0f;
							vl2=(ty-60)-((int)tv);
							if(vl2<0) vl2=0;
							vl=(int)((float)x/1.5f);
							if(PrSxT!=-1) DrawLine(TB,PrSxT,PrSyT,vl,vl2,tx,ty,250,255,20);
							PrSxT=vl;PrSyT=vl2;
							if(SpeedBase) ++x;
						}
					}
#endif

				default :
					if(b==InQ)
					{
						if((b=GetData(SearchPos+4))>0)
						{
							if(DoubleBytes)
							{
								b>>=1;
								for(a=0;a<b;a++)
								{
									if(DataBytesMask[a])
									{
										if(UseSigned)
										{
											if((char)DataBytes[(a*2)]<0)
											{
												vl=(((int)((int)((char)DataBytes[(a*2)]))*256)+(int)(DataBytes[(a*2)+1]));
												if(Use12bitSigned) vl|=0xF000;
											}
											else
											{
												vl=(((int)(DataBytes[(a*2)])<<8)+(int)(DataBytes[(a*2)+1])&0xFFFF);
												if(Use12bitSigned)
												{
													if((unsigned char)DataBytes[(a*2)]>0x7)
														vl|=0xFFFFF000;
												}
											}
											if(MinDataValues[a]>vl) MinDataValues[a]=vl;
											if(MaxDataValues[a]<vl) MaxDataValues[a]=vl;
											vl+=DataOffset;
										}
										else
										{
											vl=(((int)(DataBytes[(a*2)])<<8)+(int)(DataBytes[(a*2)+1])&0xFFFF);
											if(MinDataValues[a]>vl) MinDataValues[a]=vl;
											if(MaxDataValues[a]<vl) MaxDataValues[a]=vl;
										}
										tv=(float)vl;
										tv*=ZoomVal;
//printf("\nZoomVal = %f",ZoomVal);fflush(stdout);
										tv/=100.0f;
										vl2=(ty-1)-((int)tv);
										if(vl2<0) vl2=0;
										vl=(int)((float)x/Divider);
//printf("\nDivider = %f",Divider);fflush(stdout);
										if(ShowValueUnderPointerX>0)
										{
											if(ShowValueUnderPointerX<=PrDx[a])
											{
												printf("\nData = %02x%02x",DataBytes[(a*2)],DataBytes[(a*2+1)]);fflush(stdout);
												ShowValueUnderPointerX=-1;
											}
										}
										if(PrDx[a]!=-1)
										{
											switch(LineType)
											{
												case 0 :
													DrawLine(TB,PrDx[a],PrDy[a],vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
												case 1 :
													DrawLine(TB,PrDx[a],PrDy[a],vl,PrDy[a],tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
													DrawLine(TB,vl,PrDy[a],vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
												case 2 :
													DrawLine(TB,PrDx[a],PrDy[a],PrDx[a]+1,PrDy[a],tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
													DrawLine(TB,vl+1,vl2,vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
											}
										}
										PrDx[a]=vl;PrDy[a]=vl2;
									}
								}
							}
							else
							{
								for(a=0;a<b;a++)
								{
									if(DataBytesMask[a])
									{
										vl=(unsigned int)DataBytes[a];
										if(MinDataValues[a]>vl) MinDataValues[a]=vl;
										if(MaxDataValues[a]<vl) MaxDataValues[a]=vl;
										if(UseSigned)
											vl2=(ty-1)-(int)((float)((char)DataBytes[a]+127)*ZoomVal);
										else
											vl2=(ty-1)-(int)((float)(DataBytes[a])*ZoomVal);								
										if(vl2<0) vl2=0;
										vl=(int)((float)x/Divider);
										if(ShowValueUnderPointerX>0)
										{
											if(ShowValueUnderPointerX<=PrDx[a])
											{
												printf("\nData = %02x",DataBytes[a]);fflush(stdout);
												ShowValueUnderPointerX=-1;
											}
										}
										if(PrDx[a]!=-1)
										{
											switch(LineType)
											{
												case 0 :
													DrawLine(TB,PrDx[a],PrDy[a],vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
												case 1 :
													DrawLine(TB,PrDx[a],PrDy[a],vl,PrDy[a],tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
													DrawLine(TB,vl,PrDy[a],vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
												case 2 :
													DrawLine(TB,PrDx[a],PrDy[a],PrDx[a]+1,PrDy[a],tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
													DrawLine(TB,vl+1,vl2,vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
												break;
											}
										}
//										if(PrDx[a]!=-1) DrawLine(TB,PrDx[a],PrDy[a],vl,vl2,tx,ty,DataBytesColor[a].R,DataBytesColor[a].G,DataBytesColor[a].B);
										PrDx[a]=vl;PrDy[a]=vl2;
									}
								}
							}
							if(!SpeedBase) ++x;
						}
					}
				break;
			}
		}
		SearchPos+=3;
	}
	if(Show0)
	{
		vl=0;
		if(UseSigned)
			vl+=DataOffset;
		tv=(float)vl;
		tv*=ZoomVal;
		tv/=100.0f;
		vl2=(ty-1)-((int)tv);
		if(vl2<0) vl2=0;
		DrawLine(TB,0,vl2,(tx-1),vl2,tx,ty,10,40,200);
	}
	if(ShowValueUnderPointerY>=0)
	{
		if(DoubleBytes)
		{
			for(tx=0;tx<0xFFFF;tx++)
			{
				vl=tx;
				if(UseSigned)
				{
					if(((char)((vl>>8)&0xFF))<0)
					{
						if(Use12bitSigned) vl|=0xF000;
					}
					else
					{
						if(Use12bitSigned)
						{
							if((unsigned char)((vl>>8)&0xFF)>0x7)
								vl|=0xFFFFF000;
						}
					}
					vl+=DataOffset;
				}
				tv=(float)vl;
				tv*=ZoomVal;
				tv/=100.0f;
				vl2=(ty-1)-((int)tv);
				if(vl2<0) vl2=0;
				if(vl2==ShowValueUnderPointerY)
				{
					printf("\nPointer at value %04x",tx);fflush(stdout);
					tx=0xFFFF;
				}
//printf("%04x:%d:%d ",vl,vl2,ShowValueUnderPointerY);fflush(stdout);
			}
		}
	}
}

int ReadFile(void)
{
FILE	*fp=NULL;
long	FileL,n;

	if((fp=fopen(CommandLineBuffer,"r"))!=NULL)
	{
		fseek(fp,0L,SEEK_END);
		FileL=ftell(fp);
		fseek(fp,0L,SEEK_SET);
		if((FileBuffer=(char*)malloc((size_t)(FileL+2)))!=NULL)
		{
			if(fread(FileBuffer,(size_t)sizeof(char),(size_t)FileL,fp)!=FileL)
			{
				fseek(fp,0L,SEEK_SET);
				n=0;
				while(n<FileL) {FileBuffer[n]=(char)fgetc(fp);++n;}
			}
			fclose(fp);
			FileBuffer[FileL]='\0';
		}
		else
		{
			fclose(fp);
			fp=NULL;
			return(1);
		}
	}
	else
	{
		fp=NULL;
		return(1);
	}
BufferLength=(unsigned int)FileL;

printf(" -> Read %d bytes...",FileL);fflush(stdout);

	SearchPos=0;
	GetBasicStatistics();
return(0);
}

void PrintKeyUsage(void)
{
	printf("\n");
	printf("*** Key Assignments ***\n\n");
	printf("  Left,Right : Chg Message ID\n");
	printf("  Up and Down :\n");
	printf("\t           : Data Value Zoom */= 1.3\n");
	printf("\tShift      : Speed Value Zoom */= 1.3\n");
	printf("\tCtrl       : Data Offset -+= 0x100\n");
	printf("\tCtrl+Shift : Data Offset -+=0x1000\n");
	printf("  Home  : Reset zoom and offset values\n");
	printf("    <   : Scroll to left\n");
	printf("    >   : Scroll to right\n");
	printf("    ,   : Zoom out\n");
	printf("    .   : Zoom in\n");
	printf("    `   : SignumDraw\n");
	printf("    d   : DoubleByte\n");
	printf("    s   : ShowSpeed\n");
	printf("    -   : Signed\n");
	printf("    =   : 12bit signed\n");
	printf("    t   : Line type\n");
	printf("    k   : Show keys\n");
	printf("    0   : Show 0\n");
	printf("    p   : Show value under pointer\n");
	printf("    1-8 : Turn On/Off daya bytes\n");
	printf("    h   : Print this key assignment\n");
	printf("  enter : refresh\n");
	printf("  space : refresh");


	fflush(stdout);


}


int MiscellaneousKey(int modi,unsigned char key)
{
int	b;

	if(key==27)	// esc
	{
		ActiveButton=-1;
		DrawButtons();
		CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
		MyImages[SelectedImage].ScaleTargetWidth=-1;
		MyImages[SelectedImage].ScaleTargetHeight=-1;
		MyImages[SelectedImage].CropTargetStartX=-1;
		MyImages[SelectedImage].CropTargetStartY=-1;
		MyImages[SelectedImage].OriginalX=MyImages[SelectedImage].X;
		MyImages[SelectedImage].OriginalY=MyImages[SelectedImage].Y;
		MyImages[SelectedImage].ScaleX=(float)1.0;
		MyImages[SelectedImage].ScaleY=(float)1.0;
		UpdateImageToOriginal(SelectedImage);
		ClearWorkBuffer();
		CopyImagesToWorkBuffer();
		CopyWorkBufferToScreen();
		if(FileBuffer!=NULL) free(FileBuffer);FileBuffer=NULL;
		if(Messages!=NULL) free(Messages);Messages=NULL;
		return(1);
	}
	else
	{
		b=0;
		switch(key)
		{
			case SK_LFAR :
				if(MessageID>0) --MessageID;
				b=1;
			break;
			case SK_RIAR :
				++MessageID;
				if(Messages[MessageID].ID==0) --MessageID;
				b=1;
			break;
			case SK_DNAR :
				switch(modi)
				{
					case KM_SHIFT :
						SZoomVal/=1.3f;
					break;
					case KM_NONE :
						ZoomVal/=1.3f;
					break;
					case KM_CTRL :
						DataOffset-=0x100;
					break;
					case KM_CTRL+KM_SHIFT :
						DataOffset-=0x1000;
					break;
				}
				b=1;
			break;
			case SK_UPAR :
				switch(modi)
				{
					case KM_SHIFT :
						SZoomVal*=1.3f;
					break;
					case KM_NONE :
						ZoomVal*=1.3f;
					break;
					case KM_CTRL :
						DataOffset+=0x100;
					break;
					case KM_CTRL+KM_SHIFT :
						DataOffset+=0x1000;
					break;
				}
				b=1;
			break;
			case SK_HOME :
				ZoomVal=1.0f;
				SZoomVal=1.0f;
				DZoomVal=1.0f;
				DShiftVal=0.0f;
				b=1;
			break;
			
			case '<' :
				if((DShiftVal-=0.05f)<0.001f) DShiftVal=0.0f;
				b=1;
			break;
			
			case '>' :
				if((DShiftVal+=0.05f)>0.95f) DShiftVal=0.95f;
				b=1;
			break;
			
			case ',' :
				if((DZoomVal*=1.5f)>2.0f) DZoomVal=2.0f;
				b=1;
			break;
			
			case '.' :
				if((DZoomVal/=1.5f)<0.005f) DZoomVal=0.005f;
				b=1;
			break;
			
			case 'h' :
				PrintKeyUsage();
			break;

			case '?' :
				for(b=0;Messages[b].ID!=0;b++)
				printf("\n%3x = %d",Messages[b].ID,Messages[b].Occurance);
				printf("\n\n%03x ID data ranges : ",MessageID);
				for(b=0;b<8;b++)
				{
					printf("\n\t#%d. : %x - %x",b,MinDataValues[b],MaxDataValues[b]);
				}
				fflush(stdout);
				b=1;
			break;
			case '`' :
				if(++NoSignumDraw>1) NoSignumDraw=0;
				b=1;
			break;
			case 'd' :
				if(++DoubleBytes>1) DoubleBytes=0;
				b=1;
			break;
			case 's' :
				if(++ShowSpeed>1) ShowSpeed=0;
				b=1;
			break;
			case '-' :
				if(++UseSigned>1) UseSigned=0;
				b=1;
			break;
			case '=' :
				if(++Use12bitSigned>1) Use12bitSigned=0;
				b=1;
			break;
			case 't' :
				if(++LineType>2) LineType=0;
				b=1;
			break;
			case 'k' :
				if(++ShowKeys>1) ShowKeys=0;
				b=1;
			break;
			case '0' :
				if(++Show0>1) Show0=0;
				b=1;
			break;
			case 'p' :
				ShowValueUnderPointerX=MyImages[SelectedImage].HotPosX;
				ShowValueUnderPointerY=MyImages[SelectedImage].HotPosY;
				b=1;
			break;
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
				if(++DataBytesMask[key-'1']>1) DataBytesMask[key-'1']=0;
				b=1;
			break;
			
			case 32 :	// SPACE to see
			case 13 :	// return to accept and terminate
					ProcessImage();
					MoveImageModifiedToOriginal(SelectedImage);
					UpdateImageToOriginal(SelectedImage);
					ClearWorkBuffer();
					CopyImagesToWorkBuffer();
					b=1;
			break;
		}
	
		if(b)
		{
			GR_DrawFillRectangle((int)((DisplayWidth>>1)-140), (WorkHeight+10), (int)((DisplayWidth>>1)+140), (WorkHeight+26), 0, BKG_R, BKG_G, BKG_B);
			sprintf(TmpBuffer,"PresentID : %03x [%d] ( %d )  Z:%1.1f D:%01d %01d%01d%01d%01d%01d%01d%01d%01d",Messages[MessageID].ID,Messages[MessageID].BL,Messages[MessageID].Occurance,ZoomVal,DoubleBytes,DataBytesMask[0],DataBytesMask[1],DataBytesMask[2],DataBytesMask[3],DataBytesMask[4],DataBytesMask[5],DataBytesMask[6],DataBytesMask[7]);
			b=GetMyStringLength(TmpBuffer,0,1);
			PutMyString(TmpBuffer,((int)(DisplayWidth-b)>>1),(WorkHeight+14),0,1);
			CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
		}
	}
return(b);
}



void MiscellaneousButton(void* Data)
{
int	b;

	DrawTarget=0;
	DrawButtons();
	ClearWorkBuffer();
	CopyImagesToWorkBuffer();
	GR_DrawFillRectangle((int)((DisplayWidth>>1)-120), (WorkHeight-22), (int)((DisplayWidth>>1)+120), (WorkHeight-6), 0, BKG_R, BKG_G, BKG_B);
	sprintf(TmpBuffer,"Now analysing...");
	b=GetMyStringLength(TmpBuffer,0,1);
	PutMyString(TmpBuffer,((int)(DisplayWidth-b)>>1),(WorkHeight-18),0,1);
	CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
//	MessageID=0;

	printf("\n Working with file '%s'.",CommandLineBuffer);fflush(stdout);
	if(ReadFile())
	{
		printf("Error Reading...");fflush(stdout);
		ActiveButton=-1;
		DrawButtons();
	}
	else
	{
		PrintKeyUsage();
		MyImages[SelectedImage].X=10;
		MyImages[SelectedImage].Y=10;
		MyImages[SelectedImage].ModifiedX=MyImages[SelectedImage].X;
		MyImages[SelectedImage].ModifiedY=MyImages[SelectedImage].Y;
		ProcessImage();
		UpdateImageToModified(SelectedImage);
		ClearWorkBuffer();
		CopyImagesToWorkBuffer();
		CopyWorkBufferToScreen();
	}

	GR_DrawFillRectangle((int)((DisplayWidth>>1)-140), (WorkHeight+10), (int)((DisplayWidth>>1)+140), (WorkHeight+26), 0, BKG_R, BKG_G, BKG_B);
	sprintf(TmpBuffer,"PresentID : %03x [%d] ( %d )  Z:%1.1f D:%01d %01d%01d%01d%01d%01d%01d%01d%01d",Messages[MessageID].ID,Messages[MessageID].BL,Messages[MessageID].Occurance,ZoomVal,DoubleBytes,DataBytesMask[0],DataBytesMask[1],DataBytesMask[2],DataBytesMask[3],DataBytesMask[4],DataBytesMask[5],DataBytesMask[6],DataBytesMask[7]);
	b=GetMyStringLength(TmpBuffer,0,1);
	PutMyString(TmpBuffer,((int)(DisplayWidth-b)>>1),(WorkHeight+14),0,1);
	CopyDisplayBufferToScreen(0,0,DisplayWidth,DisplayHeight);
}
