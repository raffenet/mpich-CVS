// pman_visView.cpp : implementation of the Cpman_visView class
//
#include "stdafx.h"
#include "pman_vis.h"

#include "pman_visDoc.h"
#include "pman_visView.h"
#include ".\pman_visview.h"
#include "ConnectDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int connect_to_pmandel(const char *host, int port, int &width, int &height);
int send_xyminmax(double xmin, double ymin, double xmax, double ymax);
int get_pmandel_data();

int g_width, g_height;
HDC g_hDC;
HANDLE g_hMutex;
HWND g_hWnd;
bool g_bDrawing;
double g_xmin, g_xmax, g_ymin, g_ymax;

// Cpman_visView

IMPLEMENT_DYNCREATE(Cpman_visView, CView)

BEGIN_MESSAGE_MAP(Cpman_visView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// Cpman_visView construction/destruction

Cpman_visView::Cpman_visView()
{
    g_hMutex = CreateMutex(NULL, FALSE, NULL);
    g_bDrawing = false;
    m_hThread = NULL;
    g_xmin = -1;
    g_xmax = 1;
    g_ymin = -1;
    g_ymax = 1;
}

Cpman_visView::~Cpman_visView()
{
    CloseHandle(g_hMutex);
    if (m_hThread != NULL)
    {
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	send_xyminmax(0, 0, 0, 0);
    }
}

BOOL Cpman_visView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}

// Cpman_visView drawing

void Cpman_visView::OnDraw(CDC* pDC)
{
    RECT r, r2;
    Cpman_visDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
	return;

    GetClientRect(&r);
    if (g_hDC != NULL)
    {
	WaitForSingleObject(g_hMutex, INFINITE);
	BitBlt(pDC->m_hDC,
	    r.left, r.top,
	    min(g_width, r.right - r.left),
	    min(g_height, r.bottom - r.top),
	    g_hDC, 0, 0, SRCCOPY);
	ReleaseMutex(g_hMutex);
	if (r.right - r.left > g_width)
	{
	    r2 = r;
	    r2.left = r2.left + g_width;
	    pDC->FillSolidRect(&r2, 0);
	}
	if (r.bottom - r.top > g_height)
	{
	    r2 = r;
	    r2.top = r2.top + g_height;
	    pDC->FillSolidRect(&r2, 0);
	}
    }
    else
    {
	pDC->FillSolidRect(&r, 0);
    }
}


// Cpman_visView printing

BOOL Cpman_visView::OnPreparePrinting(CPrintInfo* pInfo)
{
    return DoPreparePrinting(pInfo);
}

void Cpman_visView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void Cpman_visView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}


// Cpman_visView diagnostics

#ifdef _DEBUG
void Cpman_visView::AssertValid() const
{
	CView::AssertValid();
}

void Cpman_visView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

Cpman_visDoc* Cpman_visView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Cpman_visDoc)));
	return (Cpman_visDoc*)m_pDocument;
}
#endif //_DEBUG


// Cpman_visView message handlers

int work_thread(void *p)
{
    send_xyminmax(g_xmin, g_ymin, g_xmax, g_ymax);
    get_pmandel_data();
    g_bDrawing = false;
    return 0;
}

void Cpman_visView::OnFileConnect()
{
    CConnectDialog dlg;
    char host[100];
    DWORD len;
    CBitmap *canvas;
    static bool bConnected = false;

    if (bConnected)
    {
	MessageBox("You may only connect once.", "Note");
	return;
    }

    len = 100;
    GetComputerName(host, &len);
    dlg.m_pszHost = host;
    dlg.m_nPort = 0;
    if (dlg.DoModal() == IDOK)
    {
	if (connect_to_pmandel(dlg.m_pszHost, dlg.m_nPort, g_width, g_height) != 0)
	{
	    return;
	}
	g_hWnd = m_hWnd;
	g_hDC = CreateCompatibleDC(NULL);
	canvas = new CBitmap();
	canvas->CreateBitmap(
		g_width, 
		g_height, 
		GetDeviceCaps(g_hDC, PLANES),
		GetDeviceCaps(g_hDC, BITSPIXEL),
		NULL);
	SelectObject(g_hDC, *canvas);
	g_bDrawing = true;
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)work_thread, NULL, 0, NULL);
	bConnected = true;
    }
}

void Cpman_visView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (!g_bDrawing)
    {
	SIZE size;
	CDC *pDC = GetDC();
	m_p1 = point;
	m_rLast.left = point.x;
	m_rLast.right = point.x;
	m_rLast.top = point.y;
	m_rLast.bottom = point.y;
	size.cx = 2;
	size.cy = 2;
	pDC->DrawDragRect(&m_rLast, size, NULL, size);
    }

    CView::OnLButtonDown(nFlags, point);
}

void Cpman_visView::OnMouseMove(UINT nFlags, CPoint point)
{
    CDC *pDC;
    RECT r;
    SIZE size;

    if (nFlags & MK_LBUTTON && !g_bDrawing)
    {
	pDC = GetDC();
	r.left = min(m_p1.x, point.x);
	r.right = max(m_p1.x, point.x);
	r.top = min(m_p1.y, point.y);
	r.bottom = max(m_p1.y, point.y);
	size.cx = 2;
	size.cy = 2;
	pDC->DrawDragRect(&r, size, &m_rLast, size);
	m_rLast = r;
    }
    CView::OnMouseMove(nFlags, point);
}

void Cpman_visView::OnLButtonUp(UINT nFlags, CPoint point)
{
    RECT r;
    double x1,y1,x2,y2;
    double width, height, pixel_width, pixel_height;
    CDC *pDC;
    SIZE size;

    if (!g_bDrawing)
    {
	pDC = GetDC();
	size.cx = 2;
	size.cy = 2;
	pDC->DrawDragRect(&m_rLast, size, NULL, size);
    }

    if (!g_bDrawing && m_hThread)
    {
	CloseHandle(m_hThread);
	if (point.x > g_width)
	    point.x = g_width;
	if (point.y > g_height)
	    point.y = g_height;
	if (m_p1.x > g_width)
	    m_p1.x = g_width;
	if (m_p1.y > g_height)
	    m_p1.y = g_height;
	m_p2 = point;
	r = m_rLast;
	width = g_xmax - g_xmin;
	height = g_ymax - g_ymin;
	pixel_width = g_width;
	pixel_height = g_height;
	x1 = g_xmin + ((double)r.left * width / pixel_width);
	x2 = g_xmin + ((double)r.right * width / pixel_width);
	y2 = g_ymin + ((double)(pixel_height - r.top) * height / pixel_height);
	y1 = g_ymin + ((double)(pixel_height - r.bottom) * height / pixel_height);
	g_xmin = x1;
	g_xmax = x2;
	g_ymin = y1;
	g_ymax = y2;
	g_bDrawing = true;
	/*
	GetClientRect(&r);
	pDC->FillSolidRect(&r, 0);
	*/
 	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)work_thread, NULL, 0, NULL);
    }
    CView::OnLButtonUp(nFlags, point);
}

BOOL Cpman_visView::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
    //return CView::OnEraseBkgnd(pDC);
}

void Cpman_visView::OnRButtonUp(UINT nFlags, CPoint point)
{
    if (!g_bDrawing && m_hThread)
    {
	CloseHandle(m_hThread);
	g_xmin = -1;
	g_xmax = 1;
	g_ymin = -1;
	g_ymax = 1;
	g_bDrawing = true;
 	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)work_thread, NULL, 0, NULL);
    }
    CView::OnRButtonUp(nFlags, point);
}
