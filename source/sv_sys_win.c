/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//#include <sys/types.h>
//#include <sys/timeb.h>
#include "qwsvdef.h"
#include <winsock.h>
#include <conio.h>
#include <limits.h>
#include <direct.h>		// _mkdir
#include <time.h>
#include <process.h>

#include "sv_windows.h"

cvar_t	sys_sleep = {"sys_sleep", "8"};
cvar_t	sys_nostdout = {"sys_nostdout","0"};

qboolean WinNT;

static char title[16];

/*
================
Sys_FileTime
================
*/
int	Sys_FileTime (char *path)
{
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

/*
================
Sys_mkdir
================
*/
void Sys_mkdir (char *path)
{
	_mkdir(path);
}

/*
================
Sys_remove
================
*/
int Sys_remove (char *path)
{
	return remove(path);
}

/*
================
Sys_listdir
================
*/

dir_t Sys_listdir (char *path, char *ext)
{
	static file_t	list[MAX_DIRFILES];
	dir_t	dir;
	HANDLE	h;
	WIN32_FIND_DATA fd;
	int		i, pos, size;
	char	name[MAX_DEMO_NAME];
	qboolean all;

	memset(list, 0, sizeof(list));
	memset(&dir, 0, sizeof(dir));

	dir.files = list;
	all = !strcmp(ext, ".*");

	h = FindFirstFile (va("%s/*.*", path), &fd);
	if (h == INVALID_HANDLE_VALUE) {
		return dir;
	}
	
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			dir.numdirs++;
			continue;
		}

		size = fd.nFileSizeLow;
		Q_strncpyz (name, fd.cFileName, MAX_DEMO_NAME);
		dir.size += size;

		if (!all && !strstr(fd.cFileName, ext))
			continue;

		/*for (s = fd.cFileName + strlen(fd.cFileName); s > fd.cFileName; s--) {
			if (*s == '.')
				break;
		}

		if (strcmp(s, ext))
			continue;
		*/

		// inclusion sort
		/*
		for (i=0 ; i<numfiles ; i++)
		{
			if (strcmp (name, list[i].name) < 0)
				break;
		}
		*/

		i = dir.numfiles;
		pos = i;
		dir.numfiles++;
		for (i=dir.numfiles-1 ; i>pos ; i--)
			list[i] = list[i-1];

		strcpy (list[i].name, name);
		list[i].size = size;
		if (dir.numfiles == MAX_DIRFILES - 1)
			break;
	} while ( FindNextFile(h, &fd) );
	FindClose (h);

	return dir;
}

/*
================
Sys_Exit
================
*/
void Sys_Exit(int code)
{
#ifdef _CONSOLE
	exit(code);
#else
	RemoveNotifyIcon();
	exit(code);
#endif
}

/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,error);
	vsprintf (text, error,argptr);
	va_end (argptr);
#ifdef _CONSOLE
//    MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	printf ("ERROR: %s\n", text);
#else
	if (!COM_CheckParm("-noerrormsgbox"))
		MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	else
		Sys_Printf("ERROR: %s\n", text);

#endif
	if (sv_errorlogfile) {
		fprintf(sv_errorlogfile, "ERROR: %s\n", text);
		fclose(sv_errorlogfile);
	}

	Sys_Exit (1);
}

void Sys_TimeOfDay(date_t *date)
{
	struct tm *newtime;
	time_t long_time;

	time( &long_time );
	newtime = localtime( &long_time );

	date->day = newtime->tm_mday;
	date->mon = newtime->tm_mon;
	date->year = newtime->tm_year + 1900;
	date->hour = newtime->tm_hour;
	date->min = newtime->tm_min;
	date->sec = newtime->tm_sec;
	strftime( date->str, 128,
         "%a %b %d, %H:%M %Y", newtime);
}



#if 1
double Sys_DoubleTime (void)
{
	static DWORD starttime;
	static qboolean first = true;
	DWORD now;

	now = timeGetTime();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}
	
	if (now < starttime) // wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
}

#else

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );
 
	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;
	
	return t;
}
#endif


/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
#ifdef _CONSOLE
	static char	text[256];
	static int		len;
	int		c;

	// read a line out
	while (_kbhit())
	{
		c = _getch();

		if (c == 224) {
			if (_kbhit()) {
				// assume escape sequence (arrows etc), skip
				_getch();
				continue;
			}
			// assume character
		}

		if (c < 32 && c != '\r' && c != 8)
			continue;

		putch (c);
		if (c == '\r')
		{
			text[len] = 0;
			putch ('\n');
			len = 0;
			return text;
		}
		if (c == 8)
		{
			if (len)
			{
				putch (' ');
				putch (c);
				len--;
				text[len] = 0;
			}
			continue;
		}

		text[len] = c;
		len++;
		text[len] = 0;
		if (len == sizeof(text))
			len = 0;
	}

	return NULL;
#else
	return NULL;
#endif
}


/*
================
Sys_Printf
================
*/
void Sys_Printf (char *fmt, ...)
{
#ifdef _CONSOLE
	va_list		argptr;
	
	if (sys_nostdout.value)
		return;
		
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
#else
	va_list argptr;
	char tmp[MAXCMDBUF];

	va_start (argptr,fmt);
	vsprintf(tmp, fmt, argptr);
	va_end (argptr);

	ConsoleAddText(tmp);
#endif
}

/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	Sys_Exit(0);
}


/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
	{
		Sys_Error ("QuakeWorld requires at least Win95 or NT 4.0");
	}

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		WinNT = true;
	else
		WinNT = false;

	Cvar_RegisterVariable (&sys_sleep);
	Cvar_RegisterVariable (&sys_nostdout);

	if (COM_CheckParm ("-nopriority"))
	{
		Cvar_Set (&sys_sleep, "0");
	}
	else
	{
		if ( ! SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS))
			Con_Printf ("SetPriorityClass() failed\n");
		else
			Con_Printf ("Process priority class set to HIGH\n");

		// sys_sleep > 0 seems to cause packet loss on WinNT (why?)
		if (WinNT)
			Cvar_Set (&sys_sleep, "0");
	}
}

int NET_Sleep(double sec)
{
    struct timeval timeout;
	fd_set	fdset;

	FD_ZERO(&fdset);
	FD_SET(net_serversocket, &fdset);

	timeout.tv_sec = (long) sec;
	timeout.tv_usec = (sec - floor(sec))*1000000L;
	//Sys_Printf("%lf, %ld %ld\n", sec, timeout.tv_sec, timeout.tv_usec);
	return select(net_serversocket+1, &fdset, NULL, NULL, &timeout);
}

void Sys_Sleep(unsigned long ms)
{
	Sleep(ms);
}

int Sys_Script(char *path, char *args)
{
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;
	char cmdline[1024], curdir[MAX_OSPATH];

	memset (&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMINNOACTIVE;

	GetCurrentDirectory(sizeof(curdir), curdir);
	
	
	sprintf(cmdline, "%s\\sh.exe %s.qws %s", curdir, path, args);
	strcat(curdir,va("\\%s", com_gamedir+2));

	return CreateProcess (NULL, cmdline, NULL, NULL,
		FALSE, 0/*DETACHED_PROCESS /*CREATE_NEW_CONSOLE*/ , NULL, curdir, &si, &pi);
}

#ifdef _CONSOLE
/*
==================
main

==================
*/
char	*newargv[256];

int main (int argc, char **argv)
{
	quakeparms_t	parms;
	double			newtime, time, oldtime;
	static	char	cwd[1024];
#ifndef NEWWAY
	struct timeval	timeout;
	fd_set			fdset;
#endif
	int				t;
	int				sleep_msec;

	GetConsoleTitle(title, sizeof(title));
	COM_InitArgv (argc, argv);
	
	parms.argc = com_argc;
	parms.argv = com_argv;

	parms.memsize = 16*1024*1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024 * 1024;

	parms.membase = Q_Malloc (parms.memsize);

	parms.basedir = ".";

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame (0.1);		

//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;
#ifndef NEWWAY 
	while (1)
	{
		sleep_msec = sys_sleep.value;
		if (sleep_msec > 0)
		{
			if (sleep_msec > 13)
				sleep_msec = 13;
			Sleep (sleep_msec);
		}

	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
		FD_ZERO(&fdset);
		FD_SET(net_serversocket, &fdset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
		if (select (net_serversocket+1, &fdset, NULL, NULL, &timeout) == -1)
			continue;

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);				
	}
#else

	/* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		Sleep (1);

		do
		{
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;
		} while (time < 0.001);

		//_controlfp( _PC_24, _MCW_PC );

		SV_Frame (time);
		oldtime = newtime;
	}
#endif

	return true;
}

#else  // _CONSOLE

char		*argv[MAX_NUM_ARGVS];

int APIENTRY WinMain(   HINSTANCE   hInstance,
                        HINSTANCE   hPrevInstance,
                        LPSTR       lpCmdLine,
                        int         nCmdShow)
{

	MSG msg;
	quakeparms_t	parms;
	double			newtime, time, oldtime;
	static	char	cwd[1024];
	struct timeval	timeout;
	fd_set			fdset;
	int				sleep_msec;
	int t;
	extern int sv_port;
	
	// get the command line parameters

	argv[0] = "";
	parms.argc = 1;

	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms.argc] = lpCmdLine;
			parms.argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

	parms.argv = argv;

	COM_InitArgv (parms.argc, parms.argv);
	PR_CleanLogText_Init();

	// create main window
	if (!CreateMainWindow(hInstance, nCmdShow))
		return 1;

	parms.memsize = 16*1024*1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024 * 1024;

	parms.membase = Q_Malloc (parms.memsize);

	parms.basedir = ".";

	SV_Init (&parms);

	// if stared miminize update notify icon message (with correct port)
	if (minimized)
		UpdateNotifyIconMessage(va("mvdsv:%d", sv_port));

// run one frame immediately for first heartbeat
	SV_Frame (0.1);		

//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;

	while(1)
	{
		// get messeges sent to windows
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
        {
            if( !GetMessage( &msg, NULL, 0, 0 ) )
            {
                break;
            }

            if(!IsDialogMessage(DlgHwnd, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }

		CheckIdle();

		// server frame

		sleep_msec = sys_sleep.value;
		if (sleep_msec > 0)
		{
			if (sleep_msec > 13)
				sleep_msec = 13;
			Sleep (sleep_msec);
		}

	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.

		FD_ZERO(&fdset);
		FD_SET(net_serversocket, &fdset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
		if (select (net_serversocket+1, &fdset, NULL, NULL, &timeout) == -1)
			continue;

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);				
	}


	Sys_Exit(msg.wParam);

	return msg.wParam;
}

#endif // _CONSOLE

