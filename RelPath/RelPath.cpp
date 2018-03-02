#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include "..\..\..\common\plugin.hpp"

static struct PluginStartupInfo Info;

enum {
  MTitle,
  MClipboardNA,
  MClipboardError,
  MOkButton,
};

const char *GetMsg(int MsgId);
void CopyToClipboard(char *data);
void ConvertPathToRelative(char *path, const char* relativeTo);

const char *GetMsg(int MsgId)
{
  return(Info.GetMsg(Info.ModuleNumber,MsgId));
}

void _export WINAPI SetStartupInfo(const struct PluginStartupInfo *psi)
{
  Info = *psi;
}

void _export WINAPI GetPluginInfo(struct PluginInfo *pi)
{
  static const char *PluginMenuStrings[1];

  pi->StructSize = sizeof(struct PluginInfo);
  pi->Flags = PF_EDITOR;

  PluginMenuStrings[0] = GetMsg(MTitle);
  pi->PluginMenuStrings = PluginMenuStrings;
  pi->PluginMenuStringsNumber = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
}


void ConvertPathToRelative(char *path, const char* relativeTo)
{
	const int MAX_SIZE = 1024;
	char pAbs[MAX_SIZE];
	char pRelTo[MAX_SIZE];
	
	strncpy(pAbs, path, MAX_SIZE);
	strncpy(pRelTo, relativeTo, MAX_SIZE);
	
	_strlwr(pAbs);
	_strlwr(pRelTo);
	
	if ((pAbs[0] == pRelTo[0]) && (pAbs[1] == pRelTo[1])) //same drive
	{
		char *pa = pAbs;
		char *pr = pRelTo;
		char *pos1 = NULL;
		char *pos2 = NULL;
		
		while ((pos1 = strchr(pa, '\\')) && (pos2 = strchr(pr, '\\')))
		{
			*pos1 = *pos2 = 0;
			if (strcmp(pa, pr) != 0) //stop on first mismatch
			{
				*pos1 = *pos2 = '\\';
				break;
			}
			pa = ++pos1;
			pr = ++pos2;
		}
		
		char rel[MAX_SIZE] = {0};
		
		while (pos2 = strchr(pr, '\\'))
		{
			strncat(rel, "..\\", MAX_SIZE);
			pr = ++pos2;
		}
		
		INT_PTR index = pa - pAbs;
		strncat(rel, path + index, MAX_SIZE);
		
		strncpy(path, rel, MAX_SIZE);
	}
}

void ShowErrorMessage(int message)
{
	const char *Msg[3];
	
	Msg[0] = GetMsg(MTitle);
	Msg[1] = GetMsg(message);
	Msg[2] = GetMsg(MOkButton);
	Info.Message(Info.ModuleNumber, FMSG_WARNING | FMSG_LEFTALIGN, "Contents", Msg, 3, 1);
}

void CopyToClipboard(char *data)
{
	if (!GetOpenClipboardWindow())
	{
		HWND hFar = (HWND) Info.AdvControl(Info.ModuleNumber, ACTL_GETFARHWND, 0);
		OpenClipboard(hFar);
		EmptyClipboard();
		
		LPSTR lpstrCopy;
		size_t len = strlen(data) + 1;
		HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, len + 2);
		lpstrCopy = (LPSTR) GlobalLock(hData);
		memcpy(lpstrCopy, data, len);
		lpstrCopy[len] = 0;
		GlobalUnlock(hData);
		if (!SetClipboardData(CF_TEXT, hData))
		{
			ShowErrorMessage(MClipboardError);
			int err = GetLastError();
		}
		CloseClipboard();
	}
	else{
		ShowErrorMessage(MClipboardNA);
	}
}

void CorrectPath(char *path)
{
	size_t len = strlen(path);
	if (path[len - 1] != '\\')
	{
		path[len++] = '\\';
		path[len] = 0;
	}
}

HANDLE _export WINAPI OpenPlugin(int OpenFrom,INT_PTR item)
{
	PanelInfo aPanel = {0};
	PanelInfo iPanel = {0};
	const int MAX_SIZE = 1024;
	char aDir[MAX_SIZE];
	char iDir[MAX_SIZE];
	char relPath[MAX_SIZE];
	
	Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &aPanel);
	Info.Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &iPanel);
	
	if ((aPanel.PanelType == PTYPE_FILEPANEL) && (iPanel.PanelType == PTYPE_FILEPANEL)) //only perform action if both panels are file panels
	{
		strncpy(aDir, aPanel.CurDir, MAX_SIZE);
		strncpy(iDir, iPanel.CurDir, MAX_SIZE);
		
		
		strcpy(relPath, aDir);
		CorrectPath(relPath);
		CorrectPath(iDir);
		
		for (int i = 0; i < aPanel.SelectedItemsNumber; i++)
		{
			if (strlen(aPanel.SelectedItems[i].FindData.cFileName) > 0)
			{
				strncat(relPath, aPanel.SelectedItems[i].FindData.cFileName, MAX_SIZE);
				break;
			}
		}
		
		ConvertPathToRelative(relPath, iDir);
		CopyToClipboard(relPath);
	}

  return INVALID_HANDLE_VALUE;
}
