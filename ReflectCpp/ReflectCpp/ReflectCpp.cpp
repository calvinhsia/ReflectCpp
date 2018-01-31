// ReflectCpp.cpp : Defines the entry point for the application.
//

// Team Explorer->Connections->New Repository "ReflectCPP"
// DblClick the newly created Repo to get to the Team Explorer Home pane.
// Solutions->New->C++ Win32Project. Select all the defaults in the Win32 App Wizard
// add natvis: https://msdn.microsoft.com/en-us/library/jj620914.aspx?f=255&MSPPError=-2147217396

#include "stdafx.h"
#include "ReflectCpp.h"
#include "windowsx.h"
#include "functional"
#include "memory"
#include "vector"
#include "string"
#include "atlbase.h"
#define _USE_MATH_DEFINES
#include "math.h"

//#include "commctrl.h"
//#pragma comment(lib,"comctl32.lib")
//#pragma comment(linker,"\"/manifestdependency:type                  = 'win32' \
//                                              name                  = 'Microsoft.Windows.Common-Controls' \
//                                              version               = '6.0.0.0' \
//                                              processorArchitecture = '*' \
//                                              publicKeyToken        = '6595b64144ccf1df' \
//                                              language              = '*'\"")
using namespace std;
using namespace ATL;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
class BounceFrame;
BounceFrame *g_pBounceFrame;

HWND g_hWnd;



class MyBase
{
public:
	static int g_nInstances;
	static LONGLONG g_nTotalAllocated;
	virtual int DoSomething() = 0;
	void *_p;
	int _size;
	MyBase()
	{
		g_nInstances++;
	}
	virtual ~MyBase()
	{
		g_nInstances--;
	};
	void DoAllocation(int reqSize)
	{
		_ASSERT_EXPR(reqSize > 4, L"too small");
		// put the Free's in the derived classes to demo virtual dtor
		_p = malloc(reqSize);
		_size = reqSize;
		g_nTotalAllocated += reqSize;
		_ASSERT_EXPR(_p != nullptr, L"Not enough memory");
		*(int *)_p = reqSize;
	}
	void CheckSize()
	{
		if (_p != nullptr)
		{
			auto size = *((int *)_p);
			_ASSERT_EXPR(_size == size, L"sizes don't match");
		}
	}
};

int MyBase::g_nInstances = 0;
LONGLONG MyBase::g_nTotalAllocated = 0;

class MyDerivedA :
	public MyBase
{
public:
	MyDerivedA()
	{
		DoAllocation(10000);
	}
	MyDerivedA(int reqSize)
	{
		DoAllocation(reqSize);
	}
	~MyDerivedA()
	{
		_ASSERT_EXPR(_p != nullptr, L"_p should be non-null");
		CheckSize();
		free(_p);
		g_nTotalAllocated -= _size;
		_p = nullptr;
	}
	int DoSomething()
	{
		CheckSize();
		return 1;
	}
};

class MyDerivedB :
	public	MyBase
{
public:
	MyDerivedB()
	{
		auto x = 2;
		DoAllocation(10000);
	}
	MyDerivedB(int reqSize)
	{
		DoAllocation(reqSize);
	}
	~MyDerivedB()
	{
		_ASSERT_EXPR(_p != nullptr, L"_p should be non-null");
		free(_p);
		g_nTotalAllocated -= _size;
		_p = nullptr;
	}
	int DoSomething()
	{
		return 2;
	}
};


void DoTest()
{
	int numIter = 10;
	vector<unique_ptr<MyBase >> vecUniquePtr;
	for (int i = 0; i < numIter; i++)
	{
		vecUniquePtr.emplace_back(new MyDerivedA());
		vecUniquePtr.emplace_back(new MyDerivedB());
		vecUniquePtr.emplace_back(new MyDerivedA(123));
		vecUniquePtr.emplace_back(new MyDerivedB(456));
		//_ASSERT_EXPR(MyBase::g_nInstances == 4, L"should have 4 instances");
		for (auto &x : vecUniquePtr)
		{
			x->DoSomething();
		}
		vecUniquePtr.clear();
		_ASSERT_EXPR(MyBase::g_nInstances == 0, L"should have no instances");
		_ASSERT_EXPR(MyBase::g_nTotalAllocated == 0, L"should have none allocated");
	}

	vector<shared_ptr<MyBase >> vecSharedPtr;
	for (int i = 0; i < numIter; i++)
	{
		vecSharedPtr.emplace_back(new MyDerivedA());
		vecSharedPtr.emplace_back(new MyDerivedB(111));
		vecSharedPtr.emplace_back(new MyDerivedA());
		vecSharedPtr.emplace_back(new MyDerivedB(222));
		_ASSERT_EXPR(MyBase::g_nInstances == 4, L"should have 4 instances");
		for (auto x : vecSharedPtr)
		{
			x->DoSomething();
			auto y = x;
			auto xx = dynamic_pointer_cast<MyDerivedA>(x);
			if (xx != nullptr)
			{
				auto axx = "got da";
			}
		}
		vecSharedPtr.clear();
		_ASSERT_EXPR(MyBase::g_nInstances == 0, L"should have no instances");
		_ASSERT_EXPR(MyBase::g_nTotalAllocated == 0, L"should have none allocated");
	}
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REFLECTCPP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_REFLECTCPP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	g_hWnd = hWnd;
	UpdateWindow(hWnd);
	return TRUE;
}

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

typedef double MyPrecision;
const double epsilon = .00001;
const int SpeedMult = 1000;
class MyPoint
{
public:
	MyPoint()
	{
		this->X = 0;
		this->Y = 0;
	}
	MyPoint(MyPrecision x, MyPrecision y)
	{
		this->X = x;
		this->Y = y;
	}
	MyPoint(POINTS pt)
	{
		this->X = pt.x;
		this->Y = pt.y;
	}
	// assignment
	MyPoint & operator=(const MyPoint other)
	{
		this->X = other.X;
		this->Y = other.Y;
		return *this;
	}
	// equality
	bool operator ==(const MyPoint other)
	{
		if (abs(this->X - other.X) < epsilon && abs(this->Y - other.Y) < epsilon)
		{
			return true;
		}
		return false;
	}
	bool operator !=(const MyPoint other)
	{
		return !(*this == other);
	}
	bool IsNull()
	{
		return X == 0 && Y == 0;
	}
	void Clear()
	{
		X = Y = 0;
	}
	double DistanceFromPoint(MyPoint other)
	{
		auto deltax = this->X - other.X;
		auto deltay = this->Y - other.Y;
		auto d = sqrt(deltax * deltax + deltay * deltay);
		return d;
	}
	MyPrecision X;
	MyPrecision Y;
};

typedef MyPoint Vector; // as in physics vector with speed and direction

interface IMirror
{
public:
	virtual MyPoint IntersectingPoint(MyPoint ptcLight, Vector vecLight) = 0;
	virtual Vector Reflect(MyPoint ptLight, Vector vecLight, MyPoint ptIntersect) = 0;
	virtual void Draw(HDC hDC) = 0;
	virtual bool IsNull() = 0;
	virtual ~IMirror()
	{

	}
};

class CEllipse :
	public IMirror
{
	void *p;
	int foo;
public:
	CEllipse()
	{
		p = malloc(10000);
		foo = 0x1234;
	}
	CEllipse(MyPoint ptopLeft, MyPoint pbottomRight, MyPoint ptStartArc, MyPoint ptEndArc)
	{
		p = malloc(10000);
	}
	MyPoint IntersectingPoint(MyPoint ptcLight, Vector vecLight)
	{
		return ptcLight;

	}
	Vector Reflect(MyPoint ptLight, Vector vecLight, MyPoint ptIntersect)
	{
		return vecLight;
	}
	void Draw(HDC hDC)
	{

	}
	~CEllipse()
	{
		auto r = 2;
		free(p);
	}
};


class CLine :
	public IMirror
{
public:
	CLine(MyPoint &pt0, MyPoint &pt1, bool IsLimitedInLength = true)
	{
		this->pt0 = pt0;
		this->pt1 = pt1;
		this->IsLimitedInLength = IsLimitedInLength;
	}
	MyPrecision LineLength()
	{
		return sqrt(LineLengthSquared());
	}
	MyPrecision LineLengthSquared()
	{
		return deltaX()*deltaX() + deltaY()*deltaY();
	}
	MyPrecision deltaX()
	{
		return pt1.X - pt0.X;
	}
	MyPrecision deltaY()
	{
		return pt1.Y - pt0.Y;
	}

	MyPoint IntersectingPoint(MyPoint ptLight, Vector vecLight)
	{
		MyPoint ptIntersect;
		CLine lnIncident(ptLight, MyPoint(ptLight.X + vecLight.X, ptLight.Y + vecLight.Y));
		auto ptIntersectTest = IntersectingPoint(lnIncident);
		// the incident line intersects the mirror. Our mirrors have finite width
		// let's see if the intersection point is within the mirror's edges
		if (!ptIntersectTest.IsNull())
		{
			if (this->IsLimitedInLength)
			{
				if (pt0.DistanceFromPoint(ptIntersectTest) +
					ptIntersectTest.DistanceFromPoint(pt1) - LineLength() < epsilon)
				{
					if (abs(vecLight.X) < epsilon) // vert
					{
						auto ss = sgn(vecLight.Y);
						auto s2 = sgn(ptIntersectTest.Y - ptLight.Y);
						if (ss * s2 == 1) // in our direction?
						{
							ptIntersect = ptIntersectTest;
						}
					}
					else  // non-vertical
					{
						auto ss = sgn(vecLight.X);
						auto s2 = sgn(ptIntersectTest.X - ptLight.X);
						if (ss * s2 == 1) // in our direction?
						{
							ptIntersect = ptIntersectTest;
						}
					}
				}
			}
			else
			{
				ptIntersect = ptIntersectTest;
			}
		}
		return ptIntersect;
	}

	MyPoint IntersectingPoint(CLine otherLine)
	{
		MyPoint result;
		auto denom = (this->pt0.X - this->pt1.X) * (otherLine.pt0.Y - otherLine.pt1.Y) - (this->pt0.Y - this->pt1.Y) * (otherLine.pt0.X - otherLine.pt1.X);
		if (denom != 0)
		{
			result.X = ((this->pt0.X * this->pt1.Y - this->pt0.Y * this->pt1.X) * (otherLine.pt0.X - otherLine.pt1.X) - (this->pt0.X - this->pt1.X) * (otherLine.pt0.X * otherLine.pt1.Y - otherLine.pt0.Y * otherLine.pt1.X)) / denom;
			result.Y = ((this->pt0.X * this->pt1.Y - this->pt0.Y * this->pt1.X) * (otherLine.pt0.Y - otherLine.pt1.Y) - (this->pt0.Y - this->pt1.Y) * (otherLine.pt0.X * otherLine.pt1.Y - otherLine.pt0.Y * otherLine.pt1.X)) / denom;
		}
		return result;
	}

	Vector Reflect(MyPoint ptLight, Vector vecLight, MyPoint ptIntersect)
	{
		if (abs(pt0.X - pt1.X) < epsilon) // vertical line
		{
			vecLight.X = -vecLight.X;
		}
		else if (abs(pt0.Y - pt1.Y) < epsilon)// horiz line
		{
			vecLight.Y = -vecLight.Y;
		}
		else
		{
			//// create incident line endpoint to intersection with correct seg length
			CLine lnIncident(ptLight, ptIntersect);
			//var angBetween = lnIncidentTest.angleBetween(lnMirror);
			//var angClosest = Math.Atan(lnMirror.slope) / _piOver180;
			//var angIncident = Math.Atan(lnIncidentTest.slope) / _piOver180;
			//var angReflect = 2 * angClosest - angIncident;
			auto newSlope = tan(2 * atan(slope()) - atan(lnIncident.slope()));
			// now we have the slope of the desired reflection line: 
			// now we need to determine the reflection direction (x & y) along the slope
			// The incident line came from one side (half plane) of the mirror. We need to leave on the same side.
			// to do so, we assume we're going in a particular direction
			// then we create a test point using the new slope
			// we see which half plane the test point is in relation to the mirror.
			// and which half plane the light source is. If they're different, we reverse the vector

			// first set the new vector to the new slope in a guessed direction. 
			vecLight.X = SpeedMult;
			vecLight.Y = SpeedMult * newSlope;
			// create a test point along the line of reflection
			MyPoint ptTest(ptIntersect.X + vecLight.X, ptIntersect.Y + vecLight.Y);
			auto halfplaneLight = LeftHalf(ptLight);
			auto halfplaneTestPoint = LeftHalf(ptTest);
			if (halfplaneLight ^ halfplaneTestPoint) // xor
			{
				vecLight.X = -vecLight.X;
				vecLight.Y = -vecLight.Y;
			}
		}
		return vecLight;
	}

	MyPrecision slope()
	{
		return deltaY() / deltaX();
	}

	bool LeftHalf(MyPoint c)
	{
		auto a = pt0;
		auto b = pt1;
		auto res = (b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X);
		return res > 0;
	}
	void Draw(HDC hDC)
	{
		MoveToEx(hDC, (int)pt0.X, (int)pt0.Y, nullptr);
		LineTo(hDC, (int)pt1.X, (int)pt1.Y);
	}

	bool IsNull()
	{
		return pt0.IsNull() || pt1.IsNull();
	}
	MyPoint pt0;
	MyPoint pt1;
	bool IsLimitedInLength;
	~CLine()
	{

	}
};

struct Laser
{
	Laser(MyPoint pt, Vector vec)
	{
		_ptLight = pt;
		_vecLight = vec;
	}
	MyPoint _ptLight;
	Vector _vecLight;
};

class BounceFrame
{
	CComAutoCriticalSection csLstMirrors;
#define LOCKMirrors CComCritSecLock<CComAutoCriticalSection> lock(csLstMirrors)
	vector<shared_ptr<IMirror>> _lstMirrors; // as in a List<MyLine>
#define  margin  5
#define xScale 1
#define yScale 1

	int _nLasers = 10;
	vector<shared_ptr<Laser>> _vecLasers;
	MyPoint _frameSize; // size of drawing area
	HPEN _clrLine;
	HPEN _clrFillReflection;
	int _colorReflection;
	int _nPenWidth;
	HBRUSH _hBrushBackGround;
	MyPoint _ptOldMouseDown;
	MyPoint _ptCurrentMouseDown;
	bool _fPenModeDrag = false;
	bool _fPenDown = false;
	bool _fIsRunning = false;
	bool _fCancelRequest = false;
	int _nDelayMsecs = 0;
	int _nBounces = 0;
	DWORD _timeStartmSecs;
	Vector _vecLightInit;
	MyPoint _ptLightInit;
	int _nOutofBounds;
public:
	BounceFrame()
	{
		_nPenWidth = 0;
		_hBrushBackGround = CreateSolidBrush(0xffffff);
		_clrLine = CreatePen(0, _nPenWidth, 0x0);
		_colorReflection = 0xff;
		_clrFillReflection = CreatePen(0, _nPenWidth, _colorReflection);
		//INITCOMMONCONTROLSEX INITCOMMONCONTROLSEX_ = { sizeof(INITCOMMONCONTROLSEX),0 };
		//if (!InitCommonControlsEx(&INITCOMMONCONTROLSEX_))
		//{
		//    _ASSERTE(("initcommonctrls", false));
		//}
	}
	~BounceFrame()
	{
		DeleteObject(_hBrushBackGround);
		DeleteObject(_clrLine);
		DeleteObject(_clrFillReflection);
	}
private:
	void ShowStatusMsg(LPCWSTR pText, ...)
	{
		va_list args;
		va_start(args, pText);
		wstring strtemp(1000, '\0');
		_vsnwprintf_s(&strtemp[0], 1000, _TRUNCATE, pText, args);
		va_end(args);
		auto len = wcslen(strtemp.c_str());
		strtemp.resize(len);
		HDC hDC = GetDC(g_hWnd);
		HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
		HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
		TextOut(hDC, 0, (int)_frameSize.Y + 1, strtemp.c_str(), (int)strtemp.size());
		SelectObject(hDC, hOldFont);
		ReleaseDC(g_hWnd, hDC);
	}
	void DrawMirrors()
	{
		if (g_hWnd != nullptr)
		{
			HDC hDC = GetDC(g_hWnd);
			LOCKMirrors;
			SelectObject(hDC, _clrLine);
			for (auto line : _lstMirrors)
			{
				line->Draw(hDC);
			}
			//            Arc(hDC, 20, 20, 800, 400, 800, 200, 0, 200);
			ReleaseDC(g_hWnd, hDC);
		}
	}
	void SetColor(int color)
	{
		_colorReflection = color;
		DeleteObject(_clrFillReflection);
		_clrFillReflection = CreatePen(/*nPenStyle:*/ 0, _nPenWidth, _colorReflection);

	}
	void ChooseRandomStartingRay()
	{
		_ptLightInit.X = (MyPrecision)(margin + (_frameSize.X - 2 * margin)* ((double)rand()) / RAND_MAX);
		_ptLightInit.Y = (MyPrecision)(margin + (_frameSize.Y - 2 * margin)* ((double)rand()) / RAND_MAX);
		_vecLightInit.X = 1;
		_vecLightInit.Y = (MyPrecision)(((double)rand()) / RAND_MAX);
	}

	void Clear(bool fKeepUserMirrors)
	{
		MyPoint topLeft = { margin, margin };
		MyPoint topRight = { _frameSize.X - margin, margin };
		MyPoint botLeft = { margin, _frameSize.Y - margin };
		MyPoint botRight = { _frameSize.X - margin,_frameSize.Y - margin };
		_ptCurrentMouseDown.Clear();
		_ptOldMouseDown.Clear();
		_fPenDown = false;
		_fPenModeDrag = false;
		_ptLightInit.X = 140;
		_ptLightInit.Y = 140;
		_vecLightInit.X = 11;
		_vecLightInit.Y = 10;
		_nBounces = 0;
		_nOutofBounds = 0;
		_colorReflection = 0;
		if (!fKeepUserMirrors)
		{
			LOCKMirrors;
			_lstMirrors.clear();
			_lstMirrors.emplace_back(new CLine(topLeft, topRight));
			_lstMirrors.emplace_back(new CLine(topRight, botRight));
			_lstMirrors.emplace_back(new CLine(botRight, botLeft));
			_lstMirrors.emplace_back(new CLine(botLeft, topLeft));
		}
		if (g_hWnd != nullptr)
		{
			HDC hDC = GetDC(g_hWnd);
			RECT rect;
			GetClientRect(g_hWnd, &rect);
			FillRect(hDC, &rect, _hBrushBackGround);
			ReleaseDC(g_hWnd, hDC);
			DrawMirrors();
		}
	}

	int DoReflecting()
	{
		int nLastBounceWhenStagnant = 0;
		_timeStartmSecs = GetTickCount();
		_nOutofBounds = 0;
		_vecLasers.clear();
		for (int i = 0; i < _nLasers; i++)
		{
			auto vec = Vector(_vecLightInit);
			if (_nLasers > 1)
			{
				auto deltangle = 2 * M_PI / _nLasers;
				auto angle = deltangle * i;
				vec.X = SpeedMult * cos(angle);
				vec.Y = SpeedMult * sin(angle);
			}
			_vecLasers.push_back(make_shared<Laser>(_ptLightInit, vec));
		}

		HDC hDC = GetDC(g_hWnd);
		while (_fIsRunning && !_fCancelRequest)
		{
			for (auto laser : _vecLasers)
			{
				auto ptLight = laser->_ptLight;
				auto vecLight = laser->_vecLight;

				//MyPoint ptEndIncident(_ptLight.x + _vecLight.x, _ptLight.y + _vecLight.y);
				//auto lnIncident = MyLine(_ptLight, ptEndIncident);
				auto lnIncident = CLine(ptLight, MyPoint(ptLight.X + vecLight.X, ptLight.Y + vecLight.Y));
				auto minDist = 1000000.0;
				shared_ptr<IMirror> mirrorClosest;
				MyPoint ptIntersect = { 0,0 };
				{
					LOCKMirrors;
					for (auto mirror : _lstMirrors)
					{
						auto ptIntersectTest = mirror->IntersectingPoint(ptLight, vecLight);
						if (!ptIntersectTest.IsNull())
						{
							auto dist = ptLight.DistanceFromPoint(ptIntersectTest);

							if (dist > epsilon && dist < minDist)
							{
								minDist = dist;
								mirrorClosest = mirror;
								ptIntersect = ptIntersectTest;
							}
						}
					}
				}
				if (mirrorClosest == nullptr)
				{
					if (nLastBounceWhenStagnant == _nBounces)
					{// both the last bounce and this bounce were stagnant
						nLastBounceWhenStagnant = _nBounces;
						_nOutofBounds++;
						ChooseRandomStartingRay();
					}
					else
					{
						vecLight.X = -vecLight.X;
						nLastBounceWhenStagnant = _nBounces;
					}
					continue;
				}
				// now draw incident line from orig pt to intersection
				SelectObject(hDC, _clrFillReflection);
				MoveToEx(hDC, (int)(xScale * ptLight.X), (int)(yScale * ptLight.Y), nullptr);
				LineTo(hDC, (int)(xScale * ptIntersect.X), (int)(yScale * ptIntersect.Y));

				// now reflect vector
				vecLight = mirrorClosest->Reflect(ptLight, vecLight, ptIntersect);
				// now set new pt 
				ptLight = ptIntersect;
				laser->_vecLight = vecLight;
				laser->_ptLight = ptLight;
				SetColor((int)_colorReflection + 1 & 0xffffff);
				if (_nDelayMsecs > 0)
				{
					Sleep(_nDelayMsecs);
				}
				if (_nBounces % 1000 == 0)
				{
					ShowStatus();
				}
				_nBounces++;
			}

		}
		ReleaseDC(g_hWnd, hDC);
		_fCancelRequest = false;
		return 0;
	}
	static DWORD WINAPI ThreadRoutine(void *parm)
	{
		BounceFrame *pBounceFrame = (BounceFrame *)parm;
		return pBounceFrame->DoReflecting();
	}

	void CancelRunning()
	{
		if (_fIsRunning)
		{
			_fCancelRequest = true;
			_fIsRunning = false;
			while (_fCancelRequest)
			{
				Sleep(_nDelayMsecs + 10);
			}
		}
	}

	void DoRunCommand()
	{
		_fIsRunning = !_fIsRunning;
		if (_fIsRunning)
		{
			auto hThread = CreateThread(
				nullptr, // sec attr
				0,  // stack size
				ThreadRoutine,
				this, // param
				0, // creationflags
				nullptr // threadid
			);
		}
		else
		{
			CancelRunning();
		}
	}

	void ShowStatus()
	{
		int nBouncesPerSecond = 0;
		DWORD nTicks = GetTickCount() - _timeStartmSecs;
		if (_fIsRunning)
		{
			nBouncesPerSecond = (int)(_nBounces / (nTicks / 1000.0));
		}
		ShowStatusMsg(L"Drag %d PenDown %d (%d,%d)-(%d,%d) Delay=%4d #M=%d OOB=%d #Bounces=%-10d # b/sec = %d      ",
			_fPenModeDrag,
			_fPenDown,
			(int)_ptOldMouseDown.X, (int)_ptOldMouseDown.Y,
			(int)_ptCurrentMouseDown.X, (int)_ptCurrentMouseDown.Y,
			_nDelayMsecs,
			_lstMirrors.size(),
			_nOutofBounds,
			_nBounces,
			nBouncesPerSecond
		);
	}
public:
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_SIZE:
		{
			_frameSize = MAKEPOINTS(lParam);
			//_frameSize.X = 1000;
			//_frameSize.Y = 814;
			_frameSize.Y -= 14; // room for status
			Clear(/*fKeepUserMirrors=*/false);
		}
		break;
		case WM_RBUTTONDOWN:
			_fPenModeDrag = !_fPenModeDrag;
			_ptOldMouseDown = MAKEPOINTS(lParam);
			ShowStatus();
			break;
		case WM_LBUTTONDOWN:
			if (_fPenModeDrag)
			{
				_ptOldMouseDown = MAKEPOINTS(lParam);
			}
			else
			{
				_ptCurrentMouseDown = MAKEPOINTS(lParam);
				if (!_ptOldMouseDown.IsNull() && _ptOldMouseDown != _ptCurrentMouseDown)
				{
					LOCKMirrors;
					_lstMirrors.emplace_back(new CLine(_ptOldMouseDown, _ptCurrentMouseDown));
					DrawMirrors();
				}
				_ptOldMouseDown = _ptCurrentMouseDown;
			}
			ShowStatus();
			break;
		case WM_MOUSEMOVE:
		{
			if (_fPenModeDrag)
			{
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				{
					if (!_ptOldMouseDown.IsNull())
					{
						_ptCurrentMouseDown = MAKEPOINTS(lParam);
						if (_ptCurrentMouseDown != _ptOldMouseDown)
						{
							LOCKMirrors;
							_lstMirrors.emplace_back(new CLine(_ptOldMouseDown, _ptCurrentMouseDown));
							_ptOldMouseDown = _ptCurrentMouseDown;
							DrawMirrors();
						}
					}
				}
				else
				{
					_ptOldMouseDown.Clear();
				}
			}
			else
			{
				if (_fPenDown)
				{
					if (!_ptOldMouseDown.IsNull())
					{
						_ptCurrentMouseDown = MAKEPOINTS(lParam);
					}
				}
			}
		}
		ShowStatus();
		break;
		case WM_LBUTTONUP:
			if (_fPenDown)
			{
				_ptCurrentMouseDown = MAKEPOINTS(lParam);
				if (_ptCurrentMouseDown != _ptOldMouseDown)
				{
					LOCKMirrors;
					_lstMirrors.emplace_back(new CLine(_ptOldMouseDown, _ptCurrentMouseDown));
					_ptOldMouseDown = _ptCurrentMouseDown;
					_fPenDown = false;
					DrawMirrors();
				}
			}
			ShowStatus();
			break;
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			case ID_FILE_RUN:
				DoRunCommand();
				break;
			case ID_CLEAR:
				Clear(/*fKeepUserMirrors=*/true);
				break;
			case ID_CLEARMIRRORS:
				Clear(/*fKeepUserMirrors=*/false);
				break;
			case ID_FILE_SLOWER:
			{
				if (_nDelayMsecs == 0)
				{
					_nDelayMsecs = 1;
				}
				else
				{
					_nDelayMsecs *= 8;
				}
				ShowStatus();
			}
			break;
			case ID_FILE_FASTER:
			{
				_nDelayMsecs /= 8;
				ShowStatus();
			}
			break;
			case ID_FILE_UNDOLASTLINE:
				// don't erase the 4 walls
				if (_lstMirrors.size() > 4)
				{
					LOCKMirrors;
					auto last = _lstMirrors[_lstMirrors.size() - 1];
					_lstMirrors.pop_back();
					//HDC hDC = GetDC(g_hWnd);
					//auto oldObj = (HGDIOBJ) SelectObject(hDC, _hBrushBackGround);
					//MoveToEx(hDC, last.pt0.X, last.pt0.Y, nullptr);
					//LineTo(hDC, last.pt1.X, last.pt1.Y);
					//SelectObject(hDC, oldObj);
					//ReleaseDC(g_hWnd, hDC);
					Clear(/*fKeepUserMirrors=*/true);
					auto xx = dynamic_pointer_cast<CLine>(last);
					if (xx != nullptr)
					{
						_ptOldMouseDown = xx->pt1;
					}

					ShowStatus();
				}
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			DrawMirrors();
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			CancelRunning();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return g_pBounceFrame->WndProc(hWnd, message, wParam, lParam);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	DoTest();

	//create an instance of BounceFrame on the stack
	// that lives until the app exits
	BounceFrame bounceFrame;
	g_pBounceFrame = &bounceFrame;

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_REFLECTCPP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, SW_MAXIMIZE))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REFLECTCPP));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
