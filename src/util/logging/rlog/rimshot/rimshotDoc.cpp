// rimshotDoc.cpp : implementation of the CRimshotDoc class
//

#include "stdafx.h"
#include "rimshot.h"

#include "rimshotDoc.h"
#include "RimshotView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRimshotDoc

IMPLEMENT_DYNCREATE(CRimshotDoc, CDocument)

BEGIN_MESSAGE_MAP(CRimshotDoc, CDocument)
	//{{AFX_MSG_MAP(CRimshotDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRimshotDoc construction/destruction

CRimshotDoc::CRimshotDoc()
{
    m_pInput = NULL;
    m_dLeft = 0.0;
    m_dRight = 0.0;
    m_dFirst = 0.0;
    m_dLast = 0.0;
    m_pStateList = NULL;
}

CRimshotDoc::~CRimshotDoc()
{
    if (m_pInput != NULL)
	RLOG_CloseInputStruct(&m_pInput);
    if (m_pStateList)
    {
	StateNode *pNode = m_pStateList;
	m_pStateList = m_pStateList->pNext;
	delete pNode;
    }
}

BOOL CRimshotDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CRimshotDoc serialization

void CRimshotDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRimshotDoc diagnostics

#ifdef _DEBUG
void CRimshotDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRimshotDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRimshotDoc commands

COLORREF StringToCOLORREF(const char *color_str)
{
    int r,g,b;
    CString str = color_str;

    str.TrimLeft();
    r = atoi(str);
    str.Delete(0, str.Find(" "));
    str.TrimLeft();
    g = atoi(str);
    str.Delete(0, str.Find(" "));
    str.TrimLeft();
    b = atoi(str);

    return RGB(r,g,b);
}

BOOL CRimshotDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
    int i;
    RLOG_EVENT event;
    RLOG_STATE state;

    if (!CDocument::OnOpenDocument(lpszPathName))
	return FALSE;
    
    if (m_pInput != NULL)
	RLOG_CloseInputStruct(&m_pInput);
    
    if (m_pStateList)
    {
	StateNode *pNode = m_pStateList;
	m_pStateList = m_pStateList->pNext;
	delete pNode;
    }
    m_dFirst = m_dLast = 0.0;
    m_dLeft = m_dRight = 0.0;

    m_pInput = RLOG_CreateInputStruct(lpszPathName);
    if (m_pInput == NULL)
    {
	MessageBox(NULL, "Failed to open input file", "Error", MB_OK);
    }
    else
    {
	m_dFirst = RLOG_MAX_DOUBLE;
	m_dLast = 0;
	for (i=0; i<m_pInput->nNumRanks; i++)
	{
	    if (m_pInput->pNumEventRecursions[i] > 0)
	    {
		RLOG_GetNextEvent(m_pInput, i, 0, &event);
		if (event.start_time < m_dFirst)
		    m_dFirst = event.start_time;
		RLOG_GetEvent(m_pInput, i, 0, m_pInput->ppNumEvents[i][0]-1, &event);
		if (event.end_time > m_dLast)
		    m_dLast = event.end_time;
	    }
	}
	m_dLeft = m_dFirst - ((m_dLast - m_dFirst) / 2.0);
	m_dRight = m_dFirst + ((m_dLast - m_dFirst) / 2.0);
    }

    while (RLOG_GetNextState(m_pInput, &state) == 0)
    {
	StateNode *pState = new StateNode;
	pState->state = state;
	pState->id = state.event;
	pState->color = StringToCOLORREF(state.color);
	pState->brush.CreateSolidBrush(pState->color);
	pState->pNext = m_pStateList;
	m_pStateList = pState;
    }

    return TRUE;
}

COLORREF CRimshotDoc::GetEventColor(int event)
{
    StateNode *pIter;

    pIter = m_pStateList;
    while (pIter)
    {
	if (pIter->id == event)
	    return pIter->color;
	pIter = pIter->pNext;
    }
    return RGB(0,0,255);
}

CBrush* CRimshotDoc::GetEventBrush(int event)
{
    StateNode *pIter;

    pIter = m_pStateList;
    while (pIter)
    {
	if (pIter->id == event)
	    return &pIter->brush;
	pIter = pIter->pNext;
    }
    return (CBrush*)GetStockObject(BLACK_BRUSH);
}

void CRimshotDoc::OnFileOpen() 
{
    CFileDialog f(TRUE, "rlog", NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, 
	"RLog Files (*.rlog)|*.rlog|All Files (*.*)|*.*||", NULL);

    if (f.DoModal() == IDOK)
    {
	if (OnOpenDocument(f.GetPathName()))
	{
	    CRimshotView *pView;
	    POSITION pos;
	    pos = GetFirstViewPosition();
	    pView = (CRimshotView*)GetNextView(pos);
	    pView->StopDrawing();
	    pView->m_Draw.pDoc = this;
	    pView->Invalidate(FALSE);
	    SetTitle(f.GetPathName());
	    pView->StartDrawing();
	}
    }
}

CString CRimshotDoc::GetEventDescription(int event)
{
    StateNode *pIter;

    pIter = m_pStateList;
    while (pIter)
    {
	if (pIter->id == event)
	{
	    return CString(pIter->state.description);
	}
	pIter = pIter->pNext;
    }
    return CString("unknown");
}
