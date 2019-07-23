#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dlfcn.h>
#include "inkview.h"

#define FONTSCALE 2	//ToDo: use the config api to make it customizable. Supported values: 1, 2, 3, 4.
#define FILESPERPAGE (64/FONTSCALE)	//Code is prepared for a variable, not a def, so other code is ready for this.

extern const ibitmap background;

ifont *listfont, *smallfont;

int cursor=0, files=2;	//Current cursor position and total file number in the current dir;
char curfile[2048]="/mnt/ext1";	//Selected file/dir name (currently under the cursor);
char curpath[2048]="";	//Current path, ended with "/" (empty string means "My Computer" meta-folder)
char temp[2048];	//For the full pathname concatenation

/*void Print(char *cha, int i)	//Debugging output
{
	char l[16384];
	sprintf (l, cha, i);
	ClearScreen();
	SetFont(smallfont, BLACK);
	DrawTextRect( 0,  779, 600, 10, l, ALIGN_CENTER);
	FullUpdate();
}*/

void ListFiles()	//Optimized for solid-state drives (no time penalty for actually re-reading directory from disk every time, but saving every single Kb of RAM we can, to avoid swapping in other applications whatever it costs; however, you'll hardly notice the difference unless it works on devices with tiny RAM).
{				//The second reason to write such an awful code is the e-ink refresh time. It's no matter how fast you get the files list, you'll redraw it slowly either way. The third, the system usually decides to free RAM or to cache the directory contents, depending on current RAM reserve. In second case, calling opendir/readdir on every cursor movement is not as awful as it looks. Fourth, this app works for few minutes, it's not a file manager, it's a book opener. However, think twice if you're going to make similar things.
	int i;

	ClearScreen();
	SetFont(listfont, BLACK);

	if (!curpath[0])	//"My Computer" meta-folder;
	{
		if (cursor<1)
		{
			DrawTextRect( 10,                            0, 590, FONTSCALE*12, "E-Book Inner Memory", ALIGN_LEFT);
			DrawTextRect(  0,  FONTSCALE*12, 590, FONTSCALE*12, "MicroSD Card", ALIGN_LEFT);
			strcpy (curfile,"/mnt/ext1");
		} else {
			DrawTextRect(  0,                              0, 590, FONTSCALE*12, "E-Book Inner Memory", ALIGN_LEFT);
			DrawTextRect( 10,  FONTSCALE*12, 590, FONTSCALE*12, "MicroSD Card", ALIGN_LEFT);
			strcpy (curfile,"/mnt/ext2");
		}
//		FullUpdate();
		SoftUpdate();
		files=2;
		return;
	}

	DIR *path = NULL;
	path=opendir(curpath);

	struct dirent *dir_entry;
	struct stat file_info;

	curfile[0]=0;
	for (i=0; (dir_entry=readdir(path))!=NULL; i++)
	{
		strcpy (temp, curpath);
		strcat (temp, dir_entry->d_name);
		stat(dir_entry->d_name,&file_info);
		if (		((file_info.st_mode & S_IFMT) != S_IFDIR && (file_info.st_mode & S_IFMT) != S_IFREG)		||
				(dir_entry->d_name[0]=='.' && !dir_entry->d_name[1])									)	{i--; continue;}

		if (i/FILESPERPAGE != cursor/FILESPERPAGE) continue;	//show only files from the same page

		if (i==cursor)
		{
//			 DrawTextRect( 10,  FONTSCALE*12*(i%FILESPERPAGE), 590, FONTSCALE*12, temp, ALIGN_LEFT);
			 DrawTextRect( 10,  FONTSCALE*12*(i%FILESPERPAGE), 590, FONTSCALE*12, dir_entry->d_name, ALIGN_LEFT);
			strcpy (curfile, dir_entry->d_name);
		}
//		else DrawTextRect(  0,  FONTSCALE*12*(i%FILESPERPAGE), 600, FONTSCALE*12, temp, ALIGN_LEFT);
		else DrawTextRect(  0,  FONTSCALE*12*(i%FILESPERPAGE), 600, FONTSCALE*12, dir_entry->d_name, ALIGN_LEFT);
	}

	files=i;
	closedir(path);

	SetFont(smallfont, BLACK);
	sprintf (temp, "Page %i of %i", cursor/FILESPERPAGE+1, i/FILESPERPAGE+1);
	DrawTextRect(  0,  785, 600, 12, temp, ALIGN_CENTER);

//	FullUpdate();
	SoftUpdate();
}

int main_handler(int type, int par1, int par2)
{
	struct stat file_info;

	if (type == EVT_INIT)		// occurs once at startup, only in main handler
	{
		smallfont = OpenFont("DroidSans", 12, 1);
		switch (FONTSCALE)	//Yep, we don't need the hard-coded font size. We should create a config file instead, and put it's content into the "My computer" meta-folder (it's a bit too empty).
		{
			case 1:
				listfont  = OpenFont("DroidSans", 12, 1);	//may be just " = smallfont"? Should it work?
			break;
			case 2:
				listfont  = OpenFont("DroidSans", 24, 1);
			break;
			case 3:
				listfont  = OpenFont("DroidSans", 36, 1);
			break;
			case 4:
				listfont  = OpenFont("DroidSans", 48, 1);
			break;
		}
	}

	if (type == EVT_SHOW)	// occurs when this event handler becomes active
	{
		StretchBitmap(0, 0, 600, 800, &background, 0);
		FullUpdate();
	}

	if (type == EVT_KEYPRESS)
	{
		switch (par1)
		{
			case 24:	//left on my 622
				if (cursor>0) cursor--; else cursor = files-1;	//we always have at least 1 file A. K. A. ".."
				ListFiles();
			break;

			case 25:	//right on my 622
				if (cursor<files-1) cursor++; else cursor = 0;
				ListFiles();
			break;

			case KEY_MENU:
				strcpy (temp, curpath);
				strcat (temp, curfile);
				stat(temp,&file_info);
//Print ("res=%i", stat(temp,&file_info) );
				if ((file_info.st_mode & S_IFMT) == S_IFDIR)
				{
					if (curfile[0]=='.' && curfile[1]=='.' && !curfile[2])	//Dir Up
					{
						if (strlen(curpath)>10)	//	"/mnt/ext?/" top allowed
						{
							strrchr(curpath,'/')[0] = 0;	//removing trailing slash;
							strrchr(curpath,'/')[1] = 0;	//removing last dir name;
						} else curpath[0]=0;	//	Not a dir, but the windows-like "My Computer" folder
					} else {
						strcpy (curpath, temp);
						strcat (curpath, "/");
						cursor=0;
					}
					ListFiles();
			break;
				}

//				OpenBook(temp, "", 0);
				OpenBook(temp, NULL, 0);
//strcat (temp, "%i");
//Print (temp,0);
//				CloseApp();
			break;

//			default: Print ("key=%i", par1);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
//	unlink ("/mnt/ext1/system/bin/bookshelf.app");	//A handy debugging measure. Even if you halt your system and lose the USB remote access to your book files, it'll be OK after restart because the Ouch!Screen deletes itself. On the active debugging stage, it's faster than rebooting it in the Safe Mode every time you do something stupid. You can also use "/mnt/ext2/system/bin/bookshelf.app" if you debug this tiny thing from MicroSD, not Internal Flash Memory.

//  chdir("/mnt/ext1/");
	InkViewMain(main_handler);

	return 0;
}
