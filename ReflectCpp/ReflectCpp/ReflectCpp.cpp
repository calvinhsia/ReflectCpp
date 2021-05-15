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
#include <atlbase.h>
#include <atlwin.h>

using namespace std;
using namespace ATL;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE g_hInstance;                                // current instance
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
int g_nLasers = 10;
int g_distBetweenEllipses = 85;
bool g_ShowEllipsePts = true;


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
	g_hInstance = hInstance; // Store instance handle in our global variable

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

MyPrecision square(MyPrecision n)
{
	return n * n;
}


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
	MyPoint Add(MyPoint other)
	{
		this->X += other.X;
		this->Y += other.Y;
		return *this;
	}

	MyPrecision X;
	MyPrecision Y;
};

void DrawPoint(MyPoint pt)
{
	auto hDC = GetDC(g_hWnd);
	POINT ptPrev;
	auto pen = CreatePen(0, 2, 0x0);

	MoveToEx(hDC, (int)(pt.X), (int)(pt.Y), &ptPrev);
	auto old = SelectObject(hDC, pen);
	int s = 2;
	Ellipse(hDC, (int)((pt.X - s)), (int)((pt.Y - s)), (int)((pt.X + s)), (int)((pt.Y + s)));
	// restore
	SelectObject(hDC, old);
	MoveToEx(hDC, ptPrev.x, ptPrev.y, &ptPrev);
	ReleaseDC(g_hWnd, hDC);
	DeleteObject(pen);
}


typedef MyPoint Vector; // as in physics vector with speed (magnitude) and direction

interface IMirror
{
public:
	virtual MyPoint IntersectingPoint(MyPoint ptcLight, Vector vecLight) = 0;
	virtual Vector Reflect(MyPoint ptLight, Vector vecLight, MyPoint ptIntersect) = 0;
	virtual void Draw(HDC hDC) = 0;
	virtual bool IsNull() = 0;
	virtual ~IMirror()
	{
		// https://blogs.msdn.microsoft.com/calvin_hsia/2018/01/31/store-different-derived-classes-in-collections-in-c-and-c-covariance-shared_ptr-unique_ptr/
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
		auto pt = MyPoint(ptLight.X + vecLight.X, ptLight.Y + vecLight.Y);
		CLine lnIncident(ptLight, pt);
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

	MyPrecision YIntercept()
	{
		auto yint1 = pt0.Y - slope() * pt0.X;
		return yint1;
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
};

void DrawLine(CLine line)
{
	auto hPen = CreatePen(0, 10, 0xff);
	auto hDC = GetDC(g_hWnd);
	POINT ptPrev;
	MoveToEx(hDC, (int)(line.pt0.X), (int)(line.pt0.Y), &ptPrev);
	auto old = SelectObject(hDC, hPen);
	auto res = LineTo(hDC, (int)(line.pt1.X), (int)(line.pt1.Y));
	// restore
	SelectObject(hDC, old);
	MoveToEx(hDC, ptPrev.x, ptPrev.y, &ptPrev);
	ReleaseDC(g_hWnd, hDC);
	DeleteObject(hPen);
}

bool IsVectorInSameDirection(MyPoint ptTest, MyPoint ptLight, Vector vecLight)
{
	auto fIsSameDirection = false;
	if (abs(vecLight.X) < epsilon) // vertical line
	{
		if (sgn(ptTest.Y - ptLight.Y) == sgn(vecLight.Y))
		{
			fIsSameDirection = true;
		}
	}
	else if (abs(vecLight.Y) < epsilon) // horiz
	{
		if (sgn(ptTest.X - ptLight.X) == sgn(vecLight.X))
		{
			fIsSameDirection = true;
		}
	}
	else
	{ // non-vertical. Construct a normal through the ptLight (the ray can extend beyond the test pt)
		Vector vecNormal(-vecLight.Y, vecLight.X);
		auto pt = MyPoint(ptLight.X + vecNormal.X, ptLight.Y + vecNormal.Y);
		CLine lnNormal(ptLight, pt);
		//BounceFrame._instance.DrawLine(lnNormal);
		auto lefthalfTestPt = lnNormal.LeftHalf(ptTest);
		auto ptvecTip = ptLight.Add(MyPoint(vecLight.X, vecLight.Y));
		auto lefthalfVectorTip = lnNormal.LeftHalf(ptvecTip);
		if (!(lefthalfTestPt ^ lefthalfVectorTip)) // xor
		{
			fIsSameDirection = true;
		}
	}
	return fIsSameDirection;
}


class CEllipse :
	public IMirror
{
	MyPoint _ptTopLeft;
	MyPoint _ptBotRight;
	MyPoint _ptStartArc;
	MyPoint _ptEndArc;
	MyPoint _ptonArc;
public:
	CEllipse(MyPoint pttopLeft, MyPoint ptbottomRight, MyPoint ptStartArc, MyPoint ptEndArc)
	{
		this->_ptTopLeft = pttopLeft;
		this->_ptBotRight = ptbottomRight;
		this->_ptStartArc = ptStartArc;
		this->_ptEndArc = ptEndArc;
		_ptonArc.X = 0;
		_ptonArc.Y = 0;
	}
	MyPoint Center()
	{
		return MyPoint(_ptTopLeft.X + Width() / 2, _ptTopLeft.Y + Height() / 2); ;
	}
	double a()
	{
		return Width() / 2;
	}
	double b()
	{
		return Height() / 2;
	}
	double f()
	{
		return sqrt(abs(a() * a() - b() * b()));
	}
	MyPoint Focus1()
	{
		if (a() > b())
		{
			return MyPoint(Center().X - f(), Center().Y);
		}
		return MyPoint(Center().X, Center().Y - f());
	}
	MyPoint Focus2()
	{
		if (a() > b())
		{
			return MyPoint(Center().X + f(), Center().Y);
		}
		return MyPoint(Center().X, Center().Y + f());
	}
	MyPrecision Width()
	{
		return _ptBotRight.X - _ptTopLeft.X;
	}
	MyPrecision Height()
	{
		return _ptBotRight.Y - _ptTopLeft.X;
	}
	bool IsPointInside(MyPoint pt)
	{
		auto IsInside = false;
		auto vx = pt.X - Center().X;
		auto vy = pt.Y - Center().Y;
		auto val = vx * vx / (a() * a()) + vy * vy / (b() * b());
		if (val <= 1 + 100 * epsilon)
		{
			IsInside = true;
		}
		return IsInside;
	}

	MyPoint IntersectingPoint(MyPoint ptLight, Vector vecLight)
	{
		//BounceFrame._instance.DrawPoint(ptLight);
		MyPoint  ptIntersectResult;
		MyPoint  ptIntersect0;
		MyPoint  ptIntersect1;
		auto pt = MyPoint(ptLight.X + vecLight.X, ptLight.Y + vecLight.Y);
		CLine lnIncident(ptLight, pt);
		//BounceFrame._instance.DrawLine(lnIncident);
		double A = 0, B = 0, C = 0, m = 0, c = 0;
		auto Isvertical = abs(vecLight.X) < 10 * epsilon;
		if (!Isvertical)
		{
			m = lnIncident.slope();
			c = lnIncident.YIntercept();
			A = b() * b() + a() * a() * m * m;
			B = 2 * a() * a()* m * (c - Center().Y) - 2 * b() * b() * Center().X;
			C = b() * b() * Center().X * Center().X + a() * a() * ((c - Center().Y) * (c - Center().Y) - b() * b());
		}
		else
		{
			A = a() * a();
			B = -2 * Center().Y * a() * a();
			C = -a() * a() * b() * b() + b() * b() * (lnIncident.pt0.X - Center().X) * (lnIncident.pt0.X - Center().X);
		}
		// quadratic formula (-b +- sqrt(b*b-4ac)/2a
		auto disc = B * B - 4 * A * C;
		if (disc > epsilon) // else no intersection (==0 means both points are the same)
		{
			auto sqt = sqrt(disc);
			auto x = (-B + sqt) / (2 * A);
			// we have >0 intersections.
			if (!Isvertical)
			{
				auto y = m * x + c;
				ptIntersect0 = MyPoint(x, y);
				x = (-B - sqt) / (2 * A);
				y = m * x + c;
				ptIntersect1 = MyPoint(x, y);
			}
			else
			{
				//ptIntersect0 = new Point(lnIncident.pt0.X, x);
				//x = (-B - sqt) / (2 * A);
				//ptIntersect1 = new Point(lnIncident.pt0.X, x);

				auto y = (b() / a()) * sqrt(a()*a() - square(ptLight.X - Center().X)) + Center().Y;
				auto y2 = -(b() / a()) * sqrt(a()*a() - square(ptLight.X - Center().X)) + Center().Y;
				ptIntersect0 = MyPoint(ptLight.X, y);
				ptIntersect1 = MyPoint(ptLight.X, y2);
			}
			// we have 2 pts: choose which one
			//BounceFrame._instance.DrawLine(lnIncident);
			//BounceFrame._instance.DrawPoint(ptIntersect0.Value);
			//BounceFrame._instance.DrawPoint(ptIntersect1.Value);
			if (!IsCompleteEllipse())
			{
				if (!IsPointOnArc(ptIntersect0))
				{
					ptIntersect0 = MyPoint(0, 0);
				}
				if (!IsPointOnArc(ptIntersect1))
				{
					ptIntersect1 = MyPoint(0, 0);
				}
			}
			////is one of the 2 intersections where the light came from?
			if (!ptIntersect0.IsNull() && ptIntersect0.DistanceFromPoint(ptLight) < 1)
			{
				ptIntersect0 = MyPoint(0, 0);
			}
			if (!ptIntersect1.IsNull() && ptIntersect1.DistanceFromPoint(ptLight) < 1)
			{
				ptIntersect1 = MyPoint(0, 0);
			}
			// now determine which point is in the right direction 
			//(could be both if point started outside ellipse)
			if (!ptIntersect0.IsNull() &&
				!IsVectorInSameDirection(ptIntersect0, ptLight, vecLight))
			{
				ptIntersect0 = MyPoint(0, 0);
			}
			if (!ptIntersect1.IsNull() &&
				!IsVectorInSameDirection(ptIntersect1, ptLight, vecLight))
			{
				ptIntersect1 = MyPoint(0, 0);
			}
			if (!ptIntersect0.IsNull())
			{
				if (!ptIntersect1.IsNull())// 2 pts still: choose closer
				{
					auto dist0 = ptLight.DistanceFromPoint(ptIntersect0);
					auto dist1 = ptLight.DistanceFromPoint(ptIntersect1);
					if (dist0 < dist1)
					{
						ptIntersectResult = ptIntersect0;
					}
					else
					{
						ptIntersectResult = ptIntersect1;
					}

				}
				else
				{
					ptIntersectResult = ptIntersect0;
				}
			}
			else
			{
				ptIntersectResult = ptIntersect1;
			}
		}
		if (ptIntersectResult.IsNull())
		{
//			_ASSERTE(false);
			//"no intersection on ellipse?".ToString();
		}
		if (!ptIntersectResult.IsNull() && !IsPointInside(ptIntersectResult)) // we said that it intersects at this point, but the point is not in the ellipse. Let's recalc to force it onto ellipse
		{
			_ASSERT_EXPR(false, "");
			//BounceFrame._instance.EraseRect();
			//BounceFrame._instance.DrawMirrors();
			//BounceFrame._instance.DrawPoint(ptIntersectResult.Value);
			//"PtIntersect not on ellipse".ToString();
//			ptIntersectResult = GetPointOnEllipseClosestToPoint(ptIntersectResult.Value);
		}
		return ptIntersectResult;
	}

	bool IsCompleteEllipse()
	{
		return _ptStartArc == _ptEndArc;
	}

	bool IsPointOnArc(MyPoint pt)
	{
		auto result = false;
		if (IsCompleteEllipse())
		{
			result = true;
		}
		else
		{
			//BounceFrame._instance.DrawPoint(pt);
			// imagine a straight line between the arc start and arc end pts on the ellipse
			// the intpoint is either in the same half plane or not as the arcpt
			CLine lnArcStartEnd(_ptStartArc, _ptEndArc);
			auto isLeft = lnArcStartEnd.LeftHalf(pt);
			auto ptArc = GetPointOnArc();
			auto isPtOnArcLeft = lnArcStartEnd.LeftHalf(ptArc);
			if (isLeft == isPtOnArcLeft)
			{
				result = true;
			}
		}
		return result;
	}
	/// <summary>
	/// To determine if a point is included in the arc of an ellipse
	/// imagine a line from the start and end arc pts. 
	/// Get a point in the half plane that includes the arc
	/// </summary>
	MyPoint GetPointOnArc()
	{
		if (_ptonArc.IsNull())
		{
			// default direction of arc is counter clockwise.
			if (_ptStartArc.X > _ptEndArc.X)
			{
				_ptonArc.X = _ptStartArc.X - 1;
				_ptonArc.Y = _ptStartArc.Y - 1;
			}
			else
			{
				if (_ptStartArc.X == _ptEndArc.X)
				{
					if (_ptStartArc.Y > _ptEndArc.Y)
					{
						_ptonArc.X = _ptStartArc.X + 1;
						_ptonArc.Y = _ptStartArc.Y - 1;
					}
					else
					{
						_ptonArc.X = _ptStartArc.X - 1;
						_ptonArc.Y = _ptStartArc.Y - 1;
					}
				}
				else
				{
					_ptonArc.X = _ptStartArc.X + 1;
					_ptonArc.Y = _ptStartArc.Y + 1;
				}
			}
		}
		return _ptonArc;
	}

	CLine GetTangentLineAtPoint(MyPoint ptIntersect)
	{
		double m = 0;
		if (abs(ptIntersect.Y - Center().Y) < epsilon) // vertical 
		{
			//" the tangent to a vertical line is horizontal (slope = 0)".ToString();
		}
		else
		{
			// calculate the slope of the tangent line at that point by differentiation
			m = -b() * b() * (ptIntersect.X - Center().X) / (a()* a()* (ptIntersect.Y - Center().Y));
		}
		auto pt = MyPoint(ptIntersect.X + SpeedMult,
			ptIntersect.Y + SpeedMult * m);
		CLine lnTangent(
			ptIntersect,
			pt,
			/*IsLimitedInLength:*/ false // a tangent has infinite length
		);
		return lnTangent;
	}

	Vector Reflect(MyPoint ptLight, Vector vecLight, MyPoint ptIntersect)
	{
		// now reflect the light off that tangent line
		auto lnTangent = GetTangentLineAtPoint(ptIntersect);
		//BounceFrame._instance.DrawLine(lnTangent);
		vecLight = lnTangent.Reflect(ptLight, vecLight, ptIntersect);
		return vecLight;
	}
	void Draw(HDC hDC)
	{
		Arc(hDC,
			(int)_ptTopLeft.X, (int)_ptTopLeft.Y,
			(int)_ptBotRight.X, (int)_ptBotRight.Y,
			(int)_ptStartArc.X, (int)_ptStartArc.Y,
			(int)_ptEndArc.X, (int)_ptEndArc.Y
		);
		if (g_ShowEllipsePts)
		{
			DrawPoint(Center());
			DrawPoint(Focus1());
			DrawPoint(Focus2());
		}
	}
	bool IsNull()
	{
		return false;
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



class CBounceFrameOptionsDialog : public CDialogImpl<CBounceFrameOptionsDialog>
{
public:
	enum { IDD = IDD_OptionsDialog };

	BEGIN_MSG_MAP(CBounceFrameOptionsDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK, OnOK)
	END_MSG_MAP()
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		auto hwnd = GetDlgItem(IDC_EDITNLasers);
		char buff[1000];
		sprintf_s(buff, "%d", g_nLasers);
		SetWindowTextA(hwnd, buff);

		return 0;
	}
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		auto hwnd = GetDlgItem(IDC_EDITNLasers);
		char buff[1000];
		int nLen = GetWindowTextA(hwnd, buff, sizeof(buff));
		int num = atoi(buff);
		if (num > 0)
		{
			g_nLasers = num;
		}
		EndDialog(wID);
		return 0;
	}
};

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

class BounceFrame
{
	CComAutoCriticalSection csLstMirrors;
#define LOCKMirrors CComCritSecLock<CComAutoCriticalSection> lock(csLstMirrors)
	vector<shared_ptr<IMirror>> _lstMirrors; // as in a List<MyLine>

	vector<shared_ptr<Laser>> _vecLasers;

#define  margin  5
#define xScale 1
#define yScale 1
	bool _AddEllipse = true;
	bool _AddMushrooms = true;
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
	ULONGLONG _timeStartmSecs;
	Vector _vecLightInit;
	MyPoint _ptLightInit;
	int _nOutofBounds;
	HMENU _hMenu = nullptr;
	HMENU _hCtxMenu = nullptr;
public:
	BounceFrame()
	{
		_nPenWidth = 1;
		_hBrushBackGround = CreateSolidBrush(0xffffff);
		_clrLine = CreatePen(0, _nPenWidth, 0x0);
		_colorReflection = 0xff;
		_clrFillReflection = CreatePen(0, _nPenWidth, _colorReflection);
		_ptLightInit.X = 140;
		_ptLightInit.Y = 140;
		_nOutofBounds = 0;
		_timeStartmSecs = 0;
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
		_ptLightInit.X = (MyPrecision)(margin + (_frameSize.X - 2.0 * margin)* ((double)rand()) / RAND_MAX);
		_ptLightInit.Y = (MyPrecision)(margin + (_frameSize.Y - 2.0 * margin)* ((double)rand()) / RAND_MAX);
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
		_vecLightInit.X = 11;
		_vecLightInit.Y = 10;
		_nBounces = 0;
		_nOutofBounds = 0;
		_colorReflection = 0;

		if (!fKeepUserMirrors)
		{
			LOCKMirrors;
			_lstMirrors.clear();
			_lstMirrors.push_back(make_shared<CLine>(topLeft, topRight));
			_lstMirrors.push_back(make_shared<CLine>(topRight, botRight));
			_lstMirrors.push_back(make_shared<CLine>(botRight, botLeft));
			_lstMirrors.push_back(make_shared<CLine>(botLeft, topLeft));
			if (_AddEllipse || !_AddMushrooms)
			{
                auto ellipse = make_shared<CEllipse>(
                    topLeft,
                    botRight,
                    MyPoint(0, 0),
                    MyPoint(0, 0));
				_lstMirrors.push_back(ellipse);
                _ptLightInit = ellipse->Center();
			}
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
		_timeStartmSecs = GetTickCount64();
		_nOutofBounds = 0;
		{
			_vecLasers.clear();
			for (int i = 0; i < g_nLasers; i++)
			{
				auto vec = Vector(_vecLightInit);
				if (g_nLasers > 1)
				{
					auto deltangle = 2 * M_PI / g_nLasers;
					auto angle = deltangle * i;
					vec.X = SpeedMult * cos(angle);
					vec.Y = SpeedMult * sin(angle);
				}
				_vecLasers.push_back(make_shared<Laser>(_ptLightInit, vec));
			}
		}
		HDC hDC = GetDC(g_hWnd);
		while (_fIsRunning && !_fCancelRequest)
		{
			for (auto laser : _vecLasers)
			{
				if (_fCancelRequest)
				{
					break;
				}
				auto ptLight = laser->_ptLight;
				auto vecLight = laser->_vecLight;

				//MyPoint ptEndIncident(_ptLight.x + _vecLight.x, _ptLight.y + _vecLight.y);
				//auto lnIncident = MyLine(_ptLight, ptEndIncident);
				auto pt = MyPoint(ptLight.X + vecLight.X, ptLight.Y + vecLight.Y);
				auto lnIncident = CLine(ptLight, pt);
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
		auto nTicks = GetTickCount64() - _timeStartmSecs;
		if (_fIsRunning)
		{
			nBouncesPerSecond = (int)(_nBounces / (nTicks / 1000.0));
		}
		ShowStatusMsg(L"#Lasers=%d Drag %d PenDown %d (%d,%d)-(%d,%d) Delay=%4d #M=%d OOB=%d #Bounces=%-10d # b/sec = %d      ",
			g_nLasers,
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
			_frameSize.Y -= 14; // room for status
			Clear(/*fKeepUserMirrors=*/false);
		}
		break;
		case WM_CONTEXTMENU:
		{
			CancelRunning();
			auto pos = MAKEPOINTS(lParam);
			if (_hMenu == nullptr)
			{
				_hMenu = LoadMenu(g_hInstance, L"MyContextMenu");
				_hCtxMenu = GetSubMenu(_hMenu, 0); //skip menu bar to first popup
			}
			auto res = TrackPopupMenuEx(_hCtxMenu,
				TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
				pos.x, pos.y,
				g_hWnd, nullptr);
			switch (res)
			{
			case ID_CONTEXT_SETINITIALPOINT:
			{
				POINT pt = { pos.x, pos.y };
				ScreenToClient(hWnd, &pt);
				_ptLightInit.X = pt.x;
				_ptLightInit.Y = pt.y;
				_ptCurrentMouseDown.X = pt.x;
				_ptCurrentMouseDown.Y = pt.y;
				ShowStatus();
			}
			break;
			case ID_CONTEXT_TOGGLEDRAGMODE:
				_fPenModeDrag = !_fPenModeDrag;
				_ptOldMouseDown = MAKEPOINTS(lParam);
				ShowStatus();
				break;
			case ID_FILE_OPTIONS:
				SendMessage(hWnd, WM_COMMAND, res, lParam);
				break;
			case ID_CONTEXT_ADDELLIPSE:
				_AddEllipse = !_AddEllipse;
				Clear(/*fKeepUserMirrors=*/false);
				break;
			case ID_CONTEXT_ADDMUSHROOMS:
				_AddMushrooms = !_AddMushrooms;
				Clear(/*fKeepUserMirrors=*/false);
				break;
			case ID_CONTEXT_SHOWELLIPSEPOINTS:
				g_ShowEllipsePts = !g_ShowEllipsePts;
				Clear(/*fKeepUserMirrors=*/false);
				break;
			default:
				break;
			}
		}
		break;
		case WM_CLOSE:
			if (_hMenu != nullptr)
			{
				DestroyMenu(_hCtxMenu);
				DestroyMenu(_hMenu);
			}
			goto _default;
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
					_lstMirrors.push_back(make_shared<CLine>(_ptOldMouseDown, _ptCurrentMouseDown));
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
							_lstMirrors.push_back(make_shared<CLine>(_ptOldMouseDown, _ptCurrentMouseDown));
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
					_lstMirrors.push_back(make_shared<CLine>(_ptOldMouseDown, _ptCurrentMouseDown));
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
					_nDelayMsecs *= 4;
				}
				ShowStatus();
			}
			break;
			case ID_FILE_FASTER:
			{
				_nDelayMsecs /= 4;
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
			case ID_FILE_OPTIONS:
			{
				CancelRunning();
				CBounceFrameOptionsDialog dialog;
				dialog.DoModal(hWnd);
			}
			break;
			case IDM_ABOUT:
				DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
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
		_default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return g_pBounceFrame->WndProc(hWnd, message, wParam, lParam);
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

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
