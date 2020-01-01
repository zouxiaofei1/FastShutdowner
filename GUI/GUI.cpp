#include "stdafx.h"
#include "GUI.h"
#include "WndShadow.h"
#include "TestFunctions.h"

#pragma warning(disable:4996)

//部分(重要)函数的前向声明
BOOL				InitInstance(HINSTANCE, int);//初始化
LRESULT	CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);//主窗口

//全局变量
HINSTANCE hInst;// 当前实例备份变量，CreateWindow&LoadIcon时需要
const wchar_t szWindowClass[] = L"FastShutdowner";

//和绘图有关的全局变量
HBRUSH DBlueBrush, LBlueBrush, WhiteBrush, BlueBrush, green, grey, yellow, Dgrey;//各色笔刷
HPEN YELLOW, RED, BLACK, White, GREEN, GREEN2, LGREY, BLUE, DBlue, LBlue;//笔
HDC hdc, rdc;//主窗口缓冲hdc + 贴图hdc
HBITMAP hBmp, lBmp;//主窗口hbmp
CWndShadow Cshadow;//主窗口阴影特效
BOOL Effect = TRUE;//特效开关
bool slient;

class CathyClass//控件主类
{
public:
	void InitClass(HINSTANCE HInstance)
	{
		hInstance = HInstance;//设置hinst
		CurButton = CurLine = CurText = 0;
		CurWnd = 1;//清"零"
		CurCover = -1;
		DefFont = CreateFontW((int)(16 * DPI), (int)(8 * DPI), 0, 0, FW_THIN, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("宋体"));
	}

	inline wchar_t* GetStr(LPCWSTR ID) { return str[Hash(ID)]; }//通过Hash + map 来快速索引字符串的数据结构
	//															ID(索引字符串) -> Hash -(map)> 字符串地址

	void SetStr(LPCWSTR Str, LPCWSTR ID)//通过ID设置字符串
	{
		delete[]str[Hash(ID)];//删除当前ID中原有的字符串
		str[Hash(ID)] = new wchar_t[wcslen(Str) + 1];
		wcscpy(str[Hash(ID)], Str);//复制新的
	}

	void CreateString(LPCWSTR Str, LPCWSTR ID)//创建新字符串
	{
		CurString++;
		if (Str != NULL)
		{
			string[CurString].str = new wchar_t[wcslen(Str) + 1];
			wcscpy(string[CurString].str, Str);
		}
		wcscpy_s(string[CurString].ID, ID);
		str[Hash(ID)] = string[CurString].str;
	}


	void CreateButtonEx(int Number, int Left, int Top, int Wid, int Hei, int Page, LPCWSTR name, HBRUSH Leave, \
		HBRUSH Hover, HBRUSH press, HPEN Leave2, HPEN Hover2, HPEN Press2, HFONT Font, BOOL Enabled, BOOL Visible, COLORREF FontRGB, LPCWSTR ID)
	{//创建按钮的复杂函数...
		Button[Number].Left = Left; Button[Number].Top = Top;
		Button[Number].Width = Wid; Button[Number].Height = Hei;
		Button[Number].Page = Page; Button[Number].Leave = Leave;
		Button[Number].Hover = Hover; Button[Number].Press = press;
		Button[Number].Leave2 = Leave2; Button[Number].Hover2 = Hover2;
		Button[Number].Press2 = Press2; Button[Number].Font = Font;
		Button[Number].Enabled = Enabled; Button[Number].Visible = Visible;
		Button[Number].FontRGB = FontRGB;
		wcscpy_s(Button[Number].Name, name);
		wcscpy_s(Button[Number].ID, ID);
		but[Hash(ID)] = Number;

		LOGBRUSH LogBrush;//从HBRUSH中提取出RGB颜色
		LOGPEN LogPen;//	(渐变色需要)
		GetObject(Leave, sizeof(LogBrush), &LogBrush);
		Button[Number].b1[0] = (byte)LogBrush.lbColor;
		Button[Number].b1[1] = (byte)(LogBrush.lbColor >> 8);
		Button[Number].b1[2] = (byte)(LogBrush.lbColor >> 16);
		GetObject(Hover, sizeof(LogBrush), &LogBrush);
		Button[Number].b2[0] = (byte)LogBrush.lbColor;
		Button[Number].b2[1] = (byte)(LogBrush.lbColor >> 8);
		Button[Number].b2[2] = (byte)(LogBrush.lbColor >> 16);

		GetObject(Leave2, sizeof(LogPen), &LogPen);
		Button[Number].p1[0] = (byte)LogPen.lopnColor;
		Button[Number].p1[1] = (byte)(LogPen.lopnColor >> 8);
		Button[Number].p1[2] = (byte)(LogPen.lopnColor >> 16);
		GetObject(Hover2, sizeof(LogPen), &LogPen);
		Button[Number].p2[0] = (byte)LogPen.lopnColor;
		Button[Number].p2[1] = (byte)(LogPen.lopnColor >> 8);
		Button[Number].p2[2] = (byte)(LogPen.lopnColor >> 16);
	}
	void CreateButton(int Left, int Top, int Wid, int Hei, int Page, LPCWSTR name, LPCWSTR ID)//创建按钮（简化版）
	{
		++CurButton;//这里的name Wid Hei 不用全名是因为警告"隐藏了全局声明"
		CreateButtonEx(CurButton, Left, Top, Wid, Hei, Page, name, WhiteBrush, DBlueBrush, LBlueBrush, BLACK, BLACK, BLACK, 0, TRUE, TRUE, RGB(0, 0, 0), ID);
	}


	void CreateText(int Left, int Top, int Page, LPCWSTR name, COLORREF rgb)//创建注释文字
	{
		++CurText;
		Text[CurText].Left = Left; Text[CurText].Top = Top;
		Text[CurText].Page = Page; Text[CurText].rgb = rgb;
		wcscpy_s(Text[CurText].Name, name);
	}

	void CreateLine(int StartX, int StartY, int EndX, int EndY, int Page, COLORREF rgb)//创建线段
	{
		++CurLine;
		Line[CurLine].StartX = StartX; Line[CurLine].StartY = StartY;
		Line[CurLine].EndX = EndX; Line[CurLine].EndY = EndY;
		Line[CurLine].Page = Page; Line[CurLine].Color = rgb;
	}

	BOOL InsideButton(int cur, POINT& point)//根据传入的point判断鼠标指针是否在按钮内
	{
		return (Button[cur].Left * DPI <= point.x && Button[cur].Top * DPI <= point.y && (long)((Button[cur].Left + Button[cur].Width) * DPI) >= point.x && (long)((Button[cur].Top + Button[cur].Height) * DPI) >= point.y);
	}


	void DrawButtons(int cur)//绘制按钮
	{
		int i;
		if (cur != 0) { i = cur; goto begin; }//如果使用ObjectRedraw则跳过其他Button
		for (i = 1; i <= CurButton; ++i)
		{
		begin:
			if (Button[i].Page == CurWnd || Button[i].Page == 0)
			{
				HPEN tmp = 0; HBRUSH tmb = 0;
				if (Button[i].Enabled == false)//禁用则显示灰色
				{
					SelectObject(hdc, Dgrey);
					SelectObject(hdc, Button[i].Leave2);
					SetTextColor(hdc, RGB(100, 100, 100));
					goto ok;//直接跳过渐变色
				}
				SetTextColor(hdc, Button[i].FontRGB);
				if (Button[i].Percent != 0 && Button[i].Percent != 100)//渐变色绘制
				{
					tmp = CreatePen(PS_SOLID, 1, RGB((Button[i].p2[0] - Button[i].p1[0]) * Button[i].Percent / 100 + Button[i].p1[0], \
						(Button[i].p2[1] - Button[i].p1[1]) * Button[i].Percent / 100 + Button[i].p1[1], (Button[i].p2[2] - Button[i].p1[2]) * Button[i].Percent / 100 + Button[i].p1[2]));

					SelectObject(hdc, tmp);
					tmb = CreateSolidBrush(RGB((Button[i].b2[0] - Button[i].b1[0]) * Button[i].Percent / 100 + Button[i].b1[0], \
						(Button[i].b2[1] - Button[i].b1[1]) * Button[i].Percent / 100 + Button[i].b1[1], (Button[i].b2[2] - Button[i].b1[2]) * Button[i].Percent / 100 + Button[i].b1[2]));
					SelectObject(hdc, tmb);
					goto ok;
				}
				if (CurCover == i)//没有禁用&渐变色 -> 默认颜色
					if (Press == 1) {
						SelectObject(hdc, Button[i].Press);//按下按钮
						SelectObject(hdc, Button[i].Press2);
					}
					else {
						SelectObject(hdc, Button[i].Hover);//悬浮
						SelectObject(hdc, Button[i].Hover2);
					}
				else
				{
					SelectObject(hdc, Button[i].Leave);//离开
					SelectObject(hdc, Button[i].Leave2);
				}
			ok:
				if (Button[i].Font == NULL)SelectObject(hdc, DefFont); else SelectObject(hdc, Button[i].Font);//字体

				Rectangle(hdc, (int)(Button[i].Left * DPI), (int)(Button[i].Top * DPI),
					(int)(Button[i].Left * DPI + Button[i].Width * DPI), (int)(Button[i].Top * DPI + Button[i].Height * DPI));//绘制方框

				RECT rc = GetRECT(i);

				SetBkMode(hdc, TRANSPARENT);//去掉文字背景
				DrawTextW(hdc, Button[i].Name, (int)wcslen(Button[i].Name), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

				if (tmp != NULL)DeleteObject(tmp);//回收句柄
				if (tmb != NULL)DeleteObject(tmb);
			}
			if (cur != 0)return;
		}
		SetTextColor(hdc, RGB(0, 0, 0));
	}

	void DrawLines()//绘制线段
	{//线段一般不需要重绘
		for (int i = 1; i <= CurLine; ++i)//因此没有加ObjectRedraw
			if (Line[i].Page == 0 || Line[i].Page == CurWnd)
			{
				SelectObject(hdc, CreatePen(0, 1, Line[i].Color));//直接用lineto
				MoveToEx(hdc, (int)(Line[i].StartX * DPI), (int)(Line[i].StartY * DPI), NULL);
				LineTo(hdc, (int)(Line[i].EndX * DPI), (int)(Line[i].EndY * DPI));
			}
	}
	void DrawTexts(int cur)//绘制文字
	{
		int i;
		if (cur != 0) { i = cur; goto begin; }//如果使用ObjectRedraw则跳过其他Texts
		for (i = 1; i <= CurText; ++i)
		{
		begin:
			if (Text[i].Page == 0 || Text[i].Page == CurWnd)
			{
				SetTextColor(hdc, Text[i].rgb);
				SelectObject(hdc, DefFont);//文字的字体缩放效果不太理想
				wchar_t* tmp = str[Hash(Text[i].Name)];
				TextOutW(hdc, (int)(Text[i].Left * DPI), (int)(Text[i].Top * DPI), tmp, (int)wcslen(tmp));
			}
			if (cur != 0)return;
		}
	}

	void RedrawObject(int type, int cur)
	{
		if (type == 2)DrawButtons(cur);
		if (type == 4)DrawTexts(cur);
	}
	//自动绘制所有控件的函数，效率低，不应经常使用
	void DrawEVERYTHING() { DrawButtons(0); DrawLines(); DrawTexts(0); }
	RECT GetRECT(int cur)//更新Buttons的rc
	{
		RECT rc = { (long)(Button[cur].Left * DPI), (long)(Button[cur].Top * DPI),
			(long)(Button[cur].Left * DPI + Button[cur].Width * DPI),(long)(Button[cur].Top * DPI + Button[cur].Height * DPI) };
		return rc;
	}
	void LeftButtonDown()//鼠标左键按下
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);//获取坐标
		if (CurCover != -1)//当按钮按下 & 停留在按钮上时
		{
			Press = 1;//重绘这个按钮
			RECT rc = GetRECT(CurCover);
			if (Obredraw)Readd(2, CurCover);
			Redraw(&rc);
		}
	}
	void ButtonGetNewInside(POINT& point)//检查point是否在check内
	{
		for (int i = 0; i <= CurButton; ++i)//历史原因，Button编号是从0开始的
			if ((Button[i].Page == CurWnd || Button[i].Page == 0) && Button[i].Enabled)
				if (InsideButton(i, point))//在按钮中
				{
					CurCover = i;//设置curcover
					if (ButtonEffect)//特效开启
					{
						Button[i].Percent += 40;//先给40%的颜色 （太淡了看不出来）
						if (Button[i].Percent > 100)Button[i].Percent = 100;
					}
					if (Obredraw)Readd(2, i);
					RECT rc = GetRECT(i);//重绘
					Redraw(&rc);
					return;
				}
	}

	void MouseMove()//鼠标移动
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);
		if (CurCover == -1)ButtonGetNewInside(point);//原来不在按钮内 -> 看看现在是否移进按钮
		else//原来在
		{
			if (!Button[CurCover].Enabled) { CurCover = -1; goto disabled; }//这个按钮被禁用了  直接跳到下面
			if ((Button[CurCover].Page != CurWnd && Button[CurCover].Page != 0) || !InsideButton(CurCover, point))
			{//现在不在
				if (Obredraw)Readd(2, CurCover);
				if (ButtonEffect)
				{//curcover设为-1 , 重绘
					Button[CurCover].Percent -= Delta;
					if (Button[CurCover].Percent < 0)Button[CurCover].Percent = 0;
				}
				RECT rc = GetRECT(CurCover);
				CurCover = -1;
				Redraw(&rc);
				ButtonGetNewInside(point);//有可能从一个按钮直接移进另一个按钮内
			}
		}
	disabled:

		if (Msv == 0)
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);//检测鼠标移进移出的函数
			tme.hwndTrack = hWnd;//在鼠标移出窗体时会触发一个WM_LEAVE消息，根据这个可以改变按钮颜色
			tme.dwFlags = TME_LEAVE;//缺点是当焦点直接被另一个窗口夺取时(比如按下windows键)
			TrackMouseEvent(&tme);//什么反应都没有
			Msv = 1;//移出
		}
		else Msv = 0;//移进
	}

	void InfoBox(LPCWSTR Str)//全自动的MessageBox
	{
		const bool f = (bool)GetStr(Str);
		if (!slient)//如果Str中是GUIstr的ID则打印str的内容，否则直接打印Str
			if (f)MessageBox(hWnd, GetStr(Str), GetStr(L"Info"), 0x40L); else MessageBox(hWnd, Str, GetStr(L"Info"), 0x40L);
		else if (f)printf("%ls\n", GetStr(Str)); else printf("%ls\n", Str);//打印到命令行中
	}
	LPWSTR GetCurInsideID()//获取当前鼠标处于的按钮的ID
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hWnd, &point);
		for (int i = 0; i <= CurButton; ++i)
			if ((Button[i].Page == CurWnd || Button[i].Page == 0) && Button[i].Enabled)
				if (InsideButton(i, point))
					return Button[i].ID;
		return Button[0].ID;
	}
	inline int GetNumbyID(LPCWSTR ID) { return but[Hash(ID)]; }//通过按钮的ID获取其编号

	void SetHDC(HDC HDc)//给要绘制的窗口设置一个新的hdc
	{
		hdc = HDc;
	}

	FORCEINLINE void Erase(RECT& rc) { es.push(rc); }//设置要擦除的区域
	void Redraw(const RECT* rc) { InvalidateRect(hWnd, rc, FALSE); UpdateWindow(hWnd); }//自动重绘 & 刷新指定区域
	void Readd(int type, int cur) { rs.push(std::make_pair(type, cur)); }//添加要刷新的控件

	//下面是Class的变量

	struct ButtonEx//按钮
	{
		long Left, Top, Width, Height, Page, Percent;
		bool Visible, Enabled, Border = true;//border:是否有边框
		HBRUSH Leave, Hover, Press;
		HPEN Leave2, Hover2, Press2;
		HFONT Font;
		wchar_t Name[31], ID[11];
		COLORREF FontRGB;
		BYTE b1[3], b2[3], p1[3], p2[3];
	}Button[MAX_BUTTON];
	struct LineEx//线段
	{
		int StartX, StartY, EndX, EndY, Page;//线段的起始坐标和终点坐标
		COLORREF Color;
	}Line[MAX_LINE];
	struct TextEx//文字
	{
		int Left, Top, Page;
		COLORREF rgb;
		wchar_t Name[11];//这里的"Name"其实是GUIString的ID
	}Text[MAX_TEXT];
	struct GUIString//GUI工程专用带ID标签的字符串
	{
		wchar_t* str, ID[11];
	}string[MAX_STRING];

	std::map<unsigned int, wchar_t*> str;//GUIstr的ID ->编号
	std::map<unsigned int, int>but;//button的ID -> 编号
	HFONT DefFont;//默认的字体
	int Msv;//鼠标移出检测变量
	int CurString, CurButton, CurLine, CurText;//各种控件的数量
	double DPI = 1;
	int CurCover;//当前被鼠标覆盖的东西
	bool Obredraw = false;//是否启用ObjectRedraw技术
	bool ButtonEffect = false;//是否开启渐变色
	int CurWnd;//当前的页面
	int Press;//鼠标左键是否按下
	std::stack<std::pair<int, int>>rs;//重绘列表
	std::stack<RECT>es;//清理列表
	HDC hdc;//缓存dc
	HDC tdc;//真实dc
	HBITMAP Bitmap;//缓存窗口bitmap
	int Width, Height;//窗口的宽和高
	HWND hWnd;//Class绘制的窗体的hwnd
	HINSTANCE hInstance;//程序的hInst
private://没有任何private变量或函数= =
}Main, CatchWnd, UpWnd;

void KillProcessFM(LPCWSTR ProcessName)//根据进程名结束进程.
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;//创建进程快照
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapShot, &pe))return;
	while (Process32Next(hSnapShot, &pe))
	{
		_wcslwr_s(pe.szExeFile);
		if (wcsstr(ProcessName, pe.szExeFile) != 0)
		{

			DWORD dwProcessID = pe.th32ProcessID;
			HANDLE hProcess = 0;
			hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessID);
			TerminateProcess(hProcess, 1);//否则使用普通的OpenProcess和TerminateProcess
			CloseHandle(hProcess);
		}
	}
}

ATOM MyRegisterClass(HINSTANCE h, WNDPROC proc, LPCWSTR ClassName)
{//封装过的注册Class函数
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = proc;
	wcex.hInstance = h;
	wcex.hIcon = LoadIcon(h, MAKEINTRESOURCE(IDI_GUI));//不能自定义窗体图标
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GUI);
	wcex.lpszClassName = ClassName;//自定义ClassName和WndProc
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_GUI));//小图标
	return RegisterClassExW(&wcex);
}
BOOL ReleaseRes(const wchar_t* strFileName, WORD wResID, const wchar_t* strFileType)
{//释放指定资源
	if (GetFileAttributes(strFileName) != INVALID_FILE_ATTRIBUTES) { return TRUE; }//资源已存在->退出

	DWORD   dwWrite = 0;// 资源大小  

	// 创建文件  
	HANDLE  hFile = CreateFile(strFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)return FALSE;//创建失败->退出

	// 查找资源文件中、加载资源到内存、得到资源大小  
	HRSRC   hrsc = FindResource(NULL, MAKEINTRESOURCE(wResID), strFileType);
	HGLOBAL hG = LoadResource(NULL, hrsc);
	DWORD   dwSize = SizeofResource(NULL, hrsc);

	// 写入文件  
	WriteFile(hFile, hG, dwSize, &dwWrite, NULL);
	CloseHandle(hFile);
	return TRUE;
}

BOOL RunWithAdmin(wchar_t* path)//以管理员身份运行一个程序.
{
	SHELLEXECUTEINFO info = { 0 };
	info.cbSize = sizeof(info);
	info.lpFile = path;
	info.lpVerb = L"runas";
	info.nShow = SW_SHOWNORMAL;
	return ShellExecuteEx(&info);
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,//程序入口点
	_In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	/*if (!IsUserAnAdmin())
	{
		wchar_t Path[301];
		GetModuleFileName(NULL, Path, 300);
		RunWithAdmin(Path);
		ExitProcess(0);
	}*/
	if (!InitInstance(hInstance, nCmdShow))return FALSE;
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GUI));

	MSG msg;
	// 主消息循环: 
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

bool SetAutoRun()
{
	TCHAR szFilePath[MAX_PATH] = L"C:\\SAtemp\\FastShutdownCore.exe";

	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(hKey, _T("FastShutdowner"), 0, REG_SZ, (LPBYTE)szFilePath, (wcslen(szFilePath) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return true;
		}
		else
		{
			RegCloseKey(hKey);
			return false;
		}
	}
	else {
		RegCloseKey(hKey); return false;
	}

}
bool DeleteAutoRun()
{
	TCHAR szFilePath[MAX_PATH] = L"C:\\SAtemp\\FastShutdownCore.exe";

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegDeleteValue(hKey, L"FastShutdowner") == ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return true;
		}
		else
		{
			RegCloseKey(hKey);
			return false;
		}
	}
	else
	{
		RegCloseKey(hKey);
		return false;
	}

}
HBITMAP hZXFsign;
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)//初始化
{
	DBlueBrush = CreateSolidBrush(RGB(210, 255, 255));
	LBlueBrush = CreateSolidBrush(RGB(230, 255, 255));
	WhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
	green = CreateSolidBrush(RGB(0x66, 0xCC, 0xFF));
	grey = CreateSolidBrush(RGB(248, 248, 248));
	Dgrey = CreateSolidBrush(RGB(230, 230, 230));
	DBlue = CreatePen(PS_SOLID, 1, RGB(210, 255, 255));
	LBlue = CreatePen(PS_SOLID, 1, RGB(230, 255, 255));
	BLACK = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	White = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	GREEN = CreatePen(PS_SOLID, 2, RGB(0x70, 0xCF, 0xFF));
	GREEN2 = CreatePen(PS_SOLID, 2, RGB(5, 195, 195));
	LGREY = CreatePen(PS_SOLID, 1, RGB(115, 115, 115));
	BLUE = CreatePen(PS_SOLID, 1, RGB(40, 130, 240));

	hInst = hInstance; // 将实例句柄存储在全局变量中
	Main.InitClass(hInst);//初始化主类
	if (!MyRegisterClass(hInst, WndProc, szWindowClass))return FALSE;//初始化Class

	hZXFsign = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);//加载签名
	Main.Obredraw = true;//默认使用ObjectRedraw
	Main.CreateString(L"快速关机 v1.0", L"Title");
	Main.hWnd = CreateWindowW(szWindowClass, Main.GetStr(L"Title"), NULL, \
		CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, NULL, nullptr, hInstance, nullptr);//创建主窗口
	if (!Main.hWnd)return FALSE;
	if (Effect)
	{
		SetWindowLong(Main.hWnd, GWL_EXSTYLE, GetWindowLong(Main.hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(Main.hWnd, NULL, 234, LWA_ALPHA);//半透明特效
	}
	SetWindowLong(Main.hWnd, GWL_STYLE, GetWindowLong(Main.hWnd, GWL_STYLE) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_SYSMENU & ~WS_GROUP & ~WS_TABSTOP);//无边框窗口


	
	Main.CreateString(L"    Win10的关机界面有时令人烦心。", L"t1");
	Main.CreateString(L"按下关机键后，如果有程序尝试阻止关机，", L"t2");
	Main.CreateString(L"你就必须劳神费力地点一下\"仍要关机\"按钮，", L"t3");
	Main.CreateString(L"并在这上面浪费十余秒的时间.", L"t4");
	Main.CreateString(L"现在，在需要关机时按下           ，", L"t5");
	Main.CreateString(L"Ctrl+Alt+S", L"t6");
	Main.CreateString(L"需要重启时按下           ,", L"t7");
	Main.CreateString(L"Ctrl+Alt+R", L"t8");
	Main.CreateString(L"再按下回车即可。", L"t9");
	Main.CreateString(L"注：只要在关机前保存好文档，", L"t10");
	Main.CreateString(L"一般就不会对电脑造成损害。", L"t11");

	Main.CreateLine(0, 50, 0, 600, 0, RGB(120,120,120));
	Main.CreateText(60, 17, 0, L"Title", RGB(250,250,250));
	Main.CreateText(20, 70, 0, L"t1", RGB(230, 92, 0));
	Main.CreateText(20, 105, 0, L"t2", RGB(70, 70, 70));
	Main.CreateText(20, 130, 0, L"t3", RGB(70, 70, 70));
	Main.CreateText(20, 155, 0, L"t4", RGB(70, 70, 70));
	Main.CreateText(20, 195, 0, L"t5", RGB(70, 70, 70));
	Main.CreateText(200, 195, 0, L"t6", RGB(255,0, 0));
	Main.CreateText(20, 220, 0, L"t7", RGB(70, 70, 70));
	Main.CreateText(135, 220, 0, L"t8", RGB(255, 0, 0));
	Main.CreateText(20, 245, 0, L"t9", RGB(70, 70, 70));
	Main.CreateText(20, 285, 0, L"t10", RGB(70, 70, 70));
	Main.CreateText(20, 310, 0, L"t11", RGB(70, 70, 70));
	Main.CreateButton(22, 345, 110, 50, 0, L"开启功能", L"Start");
	Main.CreateButton(150, 345, 110, 50, 0, L"关闭功能", L"Stop");

	Main.CreateButtonEx(3, 510, 10, 60, 30, 0, L"×", \
		CreateSolidBrush(RGB(255, 109, 109)), CreateSolidBrush(RGB(250, 100, 100)), CreateSolidBrush(RGB(232, 95, 95)), \
		CreatePen(PS_SOLID, 1, RGB(255, 109, 109)), CreatePen(PS_SOLID, 1, RGB(250, 100, 100)), CreatePen(PS_SOLID, 1, RGB(232, 95, 95)), \
		Main.DefFont, 1, 1, RGB(255, 255, 255), L"Close");
	Main.CurButton = 3;
	SetWindowPos(Main.hWnd, 0, 0, 0, 600, 425, SWP_NOMOVE);
	Main.Width = 600; Main.Height = 425;

	typedef DWORD(CALLBACK* SEtProcessDPIAware)(void);
	SEtProcessDPIAware SetProcessDPIAware;
	HMODULE huser;//让系统不对这个程序进行缩放
	huser = LoadLibrary(L"user32.dll");//在一些笔记本上有用
	SetProcessDPIAware = (SEtProcessDPIAware)GetProcAddress(huser, "SetProcessDPIAware");
	if (SetProcessDPIAware != NULL)SetProcessDPIAware();

	Main.Redraw(NULL);//第一次创建窗口时全部重绘
	ShowWindow(Main.hWnd, nCmdShow);

	return TRUE;
}

//响应函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)//主窗口响应函数
{
	switch (message)
	{
	case WM_CLOSE://关闭
		PostQuitMessage(0);
		break;
	case	WM_CREATE://创建窗口
		if (Effect)
		{//启动阴影特效
			Cshadow.Initialize(hInst);
			Cshadow.Create(hWnd);
		}
		rdc = GetDC(Main.hWnd);//创建bitmap
		hdc = CreateCompatibleDC(rdc);
		hBmp = CreateCompatibleBitmap(rdc, 1330, 1100);
		SelectObject(hdc, hBmp);
		ReleaseDC(Main.hWnd, rdc);
		DragAcceptFiles(hWnd, true);
		break;
	case WM_PAINT://绘图
	{
		HBRUSH BitmapBrush = NULL;
		RECT rc; bool f = false;
		GetUpdateRect(hWnd, &rc, false);
		if (rc.top != 0)f = true;
		if (Main.hdc == NULL)Main.SetHDC(hdc);
		PAINTSTRUCT ps;
		rdc = BeginPaint(hWnd, &ps);
		if (!Main.es.empty())//根据es来擦除区域
		{
			SelectObject(Main.hdc, White);
			SelectObject(Main.hdc, WhiteBrush);
			while (!Main.es.empty())
			{
				Rectangle(Main.hdc, Main.es.top().left, Main.es.top().top, Main.es.top().right, Main.es.top().bottom);
				Main.es.pop();
			}
		}
		if (!Main.rs.empty())
		{
			while (!Main.rs.empty())
			{
				Main.RedrawObject(Main.rs.top().first, Main.rs.top().second);
				Main.rs.pop();//根据rs用redrawobject绘制
			}
			goto finish;
		}
		SetBkMode(rdc, TRANSPARENT);
		SetBkMode(hdc, TRANSPARENT);

		SelectObject(hdc, WhiteBrush);//白色背景
		Rectangle(hdc, 0, 0, (int)(900 * Main.DPI), (int)(Main.Height * Main.DPI + 1));

		SelectObject(hdc, GREEN);//绿色顶部
		SelectObject(hdc, green);
		Rectangle(hdc, 0, 0, (int)(900 * Main.DPI), (int)(50 * Main.DPI));

		SetTextColor(hdc, RGB(0, 0, 0));
		SelectObject(hdc, BLACK);
		SelectObject(hdc, WhiteBrush);

		Main.DrawEVERYTHING();//重绘全部
		HICON hicon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_GUI));
		DrawIconEx(Main.hdc, (int)(18 * Main.DPI), (int)(10 * Main.DPI), hicon, (int)(32 * Main.DPI), (int)(32 * Main.DPI), 0, NULL, DI_NORMAL | DI_COMPAT);
		DeleteObject(hicon);
		BitmapBrush = CreatePatternBrush(hZXFsign);//绘制xiaofei签名
		SelectObject(Main.hdc, BitmapBrush);
		SelectObject(Main.hdc, BLACK);
		Rectangle(Main.hdc, 230 * 3, 250 * 3, 230 * 4, 250 * 4);
		BitBlt(Main.hdc, 350, 70, 230, 250, Main.hdc, 230 * 3, 250 * 3, SRCCOPY);
	finish://贴图
		BitBlt(rdc, rc.left, rc.top, max((long)(Main.Width * Main.DPI), rc.right - rc.left), max((long)(Main.Height * Main.DPI), rc.bottom - rc.top), hdc, rc.left, rc.top, SRCCOPY);
		EndPaint(hWnd, &ps);
	}
	break;

	case WM_LBUTTONDOWN://点下鼠标左键时
	{
		POINT point; GetCursorPos(&point); ScreenToClient(Main.hWnd, &point);
		Main.ButtonGetNewInside(point);
		if (Main.CurCover != -1)Main.LeftButtonDown();
		else PostMessage(Main.hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);//点在外面 -> 拖动窗口
		break;
	}
	case WM_LBUTTONUP://抬起鼠标左键时
		if (Main.CurCover != -1)//这时候就要做出相应的动作了
		{
			Main.Press = 0;
			RECT rc;
			rc = Main.GetRECT(Main.CurCover);
			InvalidateRect(Main.hWnd, &rc, FALSE);
		}
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(Main.hWnd, &point);

		unsigned int x;//通过hash来确定按钮编号
		x = Hash(Main.GetCurInsideID());//这样就可以不受编号干扰，随便在前后添加按钮了
		BUTTON_IN(x, L"Start")
		{

			if (SetAutoRun())Main.InfoBox(L"操作成功完成，\n请按Ctrl + Alt + S 测试效果!"); else Main.InfoBox(L"注册失败，权限不足，请重试");
			CreateDirectory(L"C:\\SAtemp", 0);
			ReleaseRes(L"C:\\SAtemp\\FastShutdownCore.exe", IDR_ZXF3, L"ZXF");
			WinExec("C:\\SAtemp\\FastShutdownCore.exe", SW_HIDE);
			break;
		}
		BUTTON_IN(x, L"Stop")
		{
			if (DeleteAutoRun())Main.InfoBox(L"操作成功完成!"); else Main.InfoBox(L"注册失败，权限不足，请重试");
			KillProcessFM(L"fastshutdowncore.exe");
			break;
		}
		BUTTON_IN(x, L"Close") { PostQuitMessage(0); }

		break;

	case WM_MOUSEMOVE: {Main.MouseMove(); break; }

	case WM_MOUSELEAVE://TrackMouseEvent带来的消息
		PostMessage(Main.hWnd, WM_MOUSEMOVE, NULL, 0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
