// 3D Pinball (for Windows) Bitmap Viewer
// Adrien Geets

#include <stdio.h>
#include <Windows.h>
#include "resource.h"

//#define RBSWAP(i) ( ((i)&0xFF00) | (((i)>>16)&0xFF) | (((i)&0xFF)<<16) )
#define MIR(i) MAKEINTRESOURCE(i)

typedef struct
{
	unsigned char type;
	union {
		unsigned int size;
		unsigned short value;
	};
	unsigned char *data;
} dentry;

typedef struct
{
	int n;
	dentry *e;
} dgroup;

typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
} bi256;

char *title = "3D Pinball Bitmap Viewer", *clsname = "AG_PbwBmpVwWinClass";
HINSTANCE hInst; HWND hwnd; HMENU hmenu;
unsigned char *fdat; int fsiz; int ngrp;
dgroup *grp;
//char tbuf[256];
int pal[256];
int acgrp, acent, acent_name, acent_p2;
bi256 acbi = {{40, 0, 0, 1, 8, BI_RGB, 0, 0, 0, 0, 0}};
char datname[256], bmpname[256];
OPENFILENAME ofndat = {sizeof(OPENFILENAME), 0, 0, "3D Pinball DAT resource file\0*.DAT\0", NULL, 0, 1, datname, 256, NULL, 0, NULL, "Open a 3D Pinball DAT resource file", OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR, 0, 0, "DAT"};
OPENFILENAME ofnbmp = {sizeof(OPENFILENAME), 0, 0, "Windows bitmap file\0*.BMP\0", NULL, 0, 1, bmpname, 256, NULL, 0, NULL, "Import/export a bitmap", OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR, 0, 0, "BMP"};
int acbmptype = 0;

void fErr(char *text, int n)
{
	MessageBox(0, text, title, 48);
	exit(n);
}

void ChangeGroup(int g)
{
	acgrp = g;
	//if(acgrp < 0) acgrp = ngrp-1;
	//if(acgrp >= ngrp) acgrp = 0;
	if(ngrp) {acgrp = acgrp % ngrp; if(acgrp < 0) acgrp += ngrp;}
	acent = FindType(acgrp, 1);
	acent_name = FindType(acgrp, 3);
	acent_p2 = FindType(acgrp, 12);
}

void ldcerr(int c, int n)
{
	char tbuf[128];
	if(!c) return;
	_snprintf(tbuf, 127, "Failed to load data!\nLoadData error code: %i", n);
	MessageBox(hwnd, tbuf, title, 16);
	exit(n);
}

void LoadData(char *fn)
{
	FILE *ff; unsigned char *p; int g, en, n, i;
	ff = fopen(fn, "rb");
	ldcerr(!ff, -1);
	fseek(ff, 0, SEEK_END);
	fsiz = ftell(ff);
	if(fdat) free(fdat);
	fdat = (unsigned char*)malloc(fsiz);
	ldcerr(!fdat, -2);
	fseek(ff, 0, SEEK_SET);
	fread(fdat, fsiz, 1, ff);
	fclose(ff);
	if(grp) {for(i = 0; i < ngrp; i++) if(grp[i].e) free(grp[i].e); free(grp);}
	ngrp = *((short*)(fdat+0xAF));
	grp = malloc(ngrp*sizeof(dgroup));
	ldcerr(!grp, -3);

	p = fdat+0xB7;
	for(g = 0; g < ngrp; g++)
	{
		n = *(p++);
		grp[g].n = n;
		grp[g].e = malloc(n*sizeof(dentry));
		ldcerr(!grp[g].e, -4);
		for(en = 0; en < n; en++)
		{
			grp[g].e[en].type = *(p++);
			if(grp[g].e[en].type)
			{
				grp[g].e[en].size = *((int*)p); p += 4;
				grp[g].e[en].data = p;
				p += grp[g].e[en].size;
			}
			else	{grp[g].e[en].size = *((short*)p); p += 2;}
		}
	}

	for(g = 0; g < ngrp; g++)
		{i = FindType(g, 5); if(i != -1) break;}
	if(i != -1)
	{
		memcpy(pal, grp[g].e[i].data, 1024);
		//for(i = 0; i < 256; i++)
		//	pal[i] = RBSWAP(pal[i]);
	} else {
		for(i = 0; i < 256; i++)
			pal[i] = i|(i<<8)|(i<<16);
	}
	memcpy(acbi.bmiColors, pal, 1024);

	ChangeGroup(0);
}

int SaveData(char *fn)
{
	FILE *ff;
	if(!fdat) return -2;
	ff = fopen(fn, "wb");
	if(!ff) return -1;
	fwrite(fdat, fsiz, 1, ff);
	fclose(ff);
	return 0;
}

int FindType(int g, int t)
{
	int e;
	for(e = 0; e < grp[g].n; e++)
		if(grp[g].e[e].type == t) return e;
	return -1;
}

void PaintPalette(HDC hdc)
{
	int i; HGDIOBJ hgob, hgop, ohgob, ohgop;
	hgob = GetStockObject(DC_BRUSH);
	ohgob = SelectObject(hdc, hgob);
	hgop = GetStockObject(DC_PEN);
	ohgop = SelectObject(hdc, hgop);
	for(i = 0; i < 256; i++)
	{
		SetDCBrushColor(hdc, pal[i]);
		SetDCPenColor(hdc, pal[i]);
		Rectangle(hdc, (i&15)*16, (i>>4)*16, (i&15)*16+16, (i>>4)*16+16);
	}
	SelectObject(hdc, ohgob); SelectObject(hdc, ohgop);
}

void DrawPic(HDC hdc, unsigned char *data)
{
	int pw, ph, pp, x, y; BITMAPINFO *bi;
	bi = malloc(sizeof(BITMAPINFOHEADER)+1024);
	if(!bi) return;
	pw = *((short*)(data+1));
	ph = *((short*)(data+3));
	pp = pw+((4-(pw&3))&3);
	//for(y = 0; y < ph; y++)
	//for(x = 0; x < pw; x++)
	//	SetPixel(hdc, x, y+16, pal[*(data+14+(ph-1-y)*pp+x)]);
	bi->bmiHeader.biSize = 40;
	bi->bmiHeader.biWidth = pw; bi->bmiHeader.biHeight = ph;
	bi->bmiHeader.biPlanes = 1; bi->bmiHeader.biBitCount = 8;
	bi->bmiHeader.biCompression = BI_RGB; bi->bmiHeader.biSizeImage = *((unsigned int*)(data+9));
	bi->bmiHeader.biXPelsPerMeter = bi->bmiHeader.biYPelsPerMeter = 0;
	bi->bmiHeader.biClrUsed = bi->bmiHeader.biClrImportant = 0;
	memcpy(bi->bmiColors, pal, 1024);
	StretchDIBits(hdc, 0, 16, pw, ph, 0, 0, pw, ph, data+14, bi, DIB_RGB_COLORS, SRCCOPY);
	free(bi);
}

void DrawAcPic(HDC hdc)
{
	int pw, ph, pp, x, y; char *data;
	if(acent == -1) return;
	data = grp[acgrp].e[acent].data;
	pw = *((short*)(data+1));
	ph = *((short*)(data+3));
	pp = pw+((4-(pw&3))&3);

	acbi.bmiHeader.biWidth = pw; acbi.bmiHeader.biHeight = ph;
	acbi.bmiHeader.biBitCount = 8;
	acbi.bmiHeader.biSizeImage = *((unsigned int*)(data+9));

	StretchDIBits(hdc, 0, 32, pw, ph, 0, 0, pw, ph, data+14, (BITMAPINFO*)&acbi, DIB_RGB_COLORS, SRCCOPY);
}

void DrawAcPic16(HDC hdc)
{
	int pw, ph, pp, x, y; char *data;
	if(acent_p2 == -1) return;
	data = grp[acgrp].e[acent_p2].data;
	pw = *((short*)(data+4));
	ph = *((short*)(data+2));

	acbi.bmiHeader.biWidth = pw; acbi.bmiHeader.biHeight = ph;
	acbi.bmiHeader.biBitCount = 16;
	acbi.bmiHeader.biSizeImage = pw * ph * 2;

	StretchDIBits(hdc, 0, 32, pw, ph, 0, 0, pw, ph, data+14, (BITMAPINFO*)&acbi, DIB_RGB_COLORS, SRCCOPY);
}

void fput16(short n, FILE *file) {fwrite(&n, 2, 1, file);}
void fput32(int n, FILE *file) {fwrite(&n, 4, 1, file);}
unsigned short fget16(FILE *file) {short n; fread(&n, 2, 1, file); return n;}
unsigned int fget32(FILE *file) {int n; fread(&n, 4, 1, file); return n;}

int ExportPic(int g, int e, char *fn)
{
	int i; FILE *ff; char *data; unsigned int bmpsiz;
	if(grp[g].e[e].type != 1) return -2;
	ff = fopen(fn, "wb");
	if(!ff) return -1;
	data = grp[g].e[e].data;
	bmpsiz = *((unsigned int*)(data+9));

	fput16('MB', ff);
	fput32(14+40+1024+bmpsiz, ff);	// Size of the file
	fput32(0, ff);			// 2 16-bits fields that are reserved.
	fput32(14+40+1024, ff);		// Location of the bitmap data

	fput32(40, ff);					// size of BITMAPINFOHEADER
	fput32(*((unsigned short*)(data+1)), ff);	// width
	fput32(*((unsigned short*)(data+3)), ff);	// height
	fput16(1, ff);					// num. of planes
	fput16(8, ff);					// bits/pixel
	fput32(0, ff);					// compression: BI_RGB (not compressed)
	fput32(bmpsiz, ff);				// size of bitmap (0 valid for BI_RGB)
	fput32(0, ff);					// x pixels/meter (0 valid)
	fput32(0, ff);					// y pixels/meter (0 valid)
	fput32(0, ff);					// num. of colors used (0 for max.)
	fput32(0, ff);					// num. of colors that are important (0 for max.)

	// Palette
	//for(i = 0; i < 256; i++) fput32(RBSWAP(pal[i]), ff);
	fwrite(pal, 1024, 1, ff);
	// Bitmap
	fwrite(data+14, bmpsiz, 1, ff);

	fclose(ff); return 0;
}

int ImportPic(int g, int e, char *fn)
{
	int i; FILE *ff; char *data; BITMAPINFOHEADER bh;
	if(grp[g].e[e].type != 1) return -2;
	data = grp[g].e[e].data;
	ff = fopen(fn, "rb");
	if(!ff) return -1;
	if(fget16(ff) != 'MB') {fclose(ff); return -3;}
	fseek(ff, 14, SEEK_SET);
	i = fget32(ff); if((unsigned int)i > sizeof(BITMAPINFOHEADER) || (unsigned int)i < 4) {fclose(ff); return -4;}
	*((unsigned int*)&bh) = i;
	fread(((char*)&bh)+4, i-4, 1, ff);
	if(bh.biWidth == *((unsigned short*)(data+1)))
	if(bh.biHeight == *((unsigned short*)(data+3)))
	if(bh.biPlanes == 1 && bh.biBitCount == 8)
	if(bh.biCompression == BI_RGB)
	{
		i = bh.biClrUsed;
		fseek(ff, i?(i*4):1024, SEEK_CUR);
		fread(data+14, grp[g].e[e].size, 1, ff);
		fclose(ff); return 0;
	}
	fclose(ff); return -5;
}

#define MChgGrp(n) ChangeGroup(n); InvalidateRect(hWnd, NULL, TRUE); break;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc; int i; PAINTSTRUCT ps; char tbuf[128];
	switch(uMsg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			SetTextColor(hdc, 0xFF0000);
			//PaintPalette(hdc);
			_snprintf(tbuf, 127, "Group %i", acgrp);
			TextOut(hdc, 0, 0, tbuf, strlen(tbuf));
			SetTextColor(hdc, 0);
			if(acent_name != -1)
				TextOut(hdc, 0, 16, grp[acgrp].e[acent_name].data, grp[acgrp].e[acent_name].size - 1);
			else
				TextOut(hdc, 0, 16, "No name", 7);
			if(!acbmptype && (acent != -1))
				DrawAcPic(hdc);
				//DrawPic(hdc, grp[acgrp].e[acent].data);
			else if(acbmptype && (acent_p2 != -1))
				DrawAcPic16(hdc);
			else
				{SetTextColor(hdc, 0xFF);
				TextOut(hdc, 0, 32, "No bitmap!", 10);}
			EndPaint(hWnd, &ps);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDM_OPEN:
					if(!GetOpenFileName(&ofndat)) break;
					LoadData(datname);
					InvalidateRect(hWnd, NULL, TRUE);
					break;
				case IDM_SAVEAS:
					if(!GetSaveFileName(&ofndat)) break;
				case IDM_SAVE:
					SaveData(datname); break;
				case IDM_BMPEXPORT:
					if(!GetSaveFileName(&ofnbmp)) break;
					ExportPic(acgrp, acent, bmpname); break;
				case IDM_BMPIMPORT:
					if(!GetOpenFileName(&ofnbmp)) break;
					i = ImportPic(acgrp, acent, bmpname);
					if(i >= 0)
						InvalidateRect(hWnd, NULL, TRUE);
					else
						{_snprintf(tbuf, 127, "Failed to import the bitmap file.\nError code: %i", i);
						MessageBox(hWnd, tbuf, title, 16);}
					break;
				case IDM_ABOUT:
					MessageBox(hWnd, "3D Pinball Bitmap Viewer\nAdrien Geets\nIN DEVELOPMENT!", title, 64);
					break;
				case IDM_QUIT:
					DestroyWindow(hWnd); break;
			} break;
		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_LEFT:		
				case VK_UP:		MChgGrp(acgrp-1);
				case VK_DOWN:
				case VK_RIGHT:		MChgGrp(acgrp+1);
				case VK_PRIOR:		MChgGrp(acgrp-10);
				case VK_NEXT:		MChgGrp(acgrp+10);
				case VK_HOME:		MChgGrp(0);
				case VK_END:		MChgGrp(ngrp-1);
				case VK_F6:
					ExportPic(acgrp, acent, "pexport.bmp"); break;
				case VK_TAB:
					acbmptype = !acbmptype;
					InvalidateRect(hWnd, NULL, TRUE);
					break;
			} break;
		case WM_DESTROY:
			PostQuitMessage(0);
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void AppExit(void)
{
	int i;
	if(grp)
	{
		for(i = 0; i < ngrp; i++)
			if(grp[i].e)
				free(grp[i].e);
		free(grp);
	}
	if(fdat) free(fdat);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *cmdArgs, int showMode)
{
	HWND hWnd; MSG msg; BOOL bRet; char *argfn, *s, *ad;
	WNDCLASS wndclass = {CS_OWNDC, WndProc, 0, 0, hInstance,
			NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1), NULL, clsname};

	hInst = hInstance; atexit(AppExit);
	ofndat.hInstance = ofnbmp.hInstance = hInst;

	// Get first argument and open it directly if available.
	argfn = strdup(cmdArgs);
	s = argfn;
	while(isspace(*s)) s++;
	if(*s)
	{
		if(*s == '\"')
		{
			ad = ++s;
			while(*s != '\"' && *s != 0) s++;
			*s = 0;
			LoadData(ad);
		}
		else	LoadData(s);
	}
	else LoadData("C:\\Users\\Adrien\\Downloads\\pbhack\\PINBALLa.DAT");

	wndclass.hIcon = LoadIcon(hInst, MIR(IDI_APPICON));
	if(!RegisterClass(&wndclass)) fErr("Class registration failed.", -1);
	ofndat.hwndOwner = ofnbmp.hwndOwner = hwnd = hWnd = CreateWindow(clsname, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, NULL, hmenu = LoadMenu(hInst, MIR(IDR_APPMENU)), hInstance, NULL);
	if(!hWnd) fErr("Window creation failed.", -2);
	ShowWindow(hWnd, showMode);

	while(bRet = GetMessage(&msg, NULL, 0, 0))
	if(bRet != -1)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}