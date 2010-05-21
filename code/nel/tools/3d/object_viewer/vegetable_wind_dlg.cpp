// vegetable_wind_dlg.cpp : implementation file
//

#include "std_afx.h"
#include "object_viewer.h"
#include "vegetable_wind_dlg.h"
#include "object_viewer.h"


#define	NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE		1000
#define	NL_VEGETABLE_EDIT_WIND_MAX_POWER		10.f
#define	NL_VEGETABLE_EDIT_WIND_MAX_FREQUENCY	10.f
#define	NL_VEGETABLE_EDIT_WIND_MAX_BENDSTART	1.f

/////////////////////////////////////////////////////////////////////////////
// CVegetableWindDlg dialog


CVegetableWindDlg::CVegetableWindDlg(CObjectViewer *objViewer, CWnd* pParent /*=NULL*/)
	: CDialog(CVegetableWindDlg::IDD, pParent), _ObjViewer(objViewer)
{
	//{{AFX_DATA_INIT(CVegetableWindDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CVegetableWindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVegetableWindDlg)
	DDX_Control(pDX, IDC_STATIC_VEGETABLE_WIND_POWER, StaticPower);
	DDX_Control(pDX, IDC_STATIC_VEGETABLE_WIND_FREQUENCY, StaticFrequency);
	DDX_Control(pDX, IDC_STATIC_VEGETABLE_WIND_BENDSTART, StaticBendStart);
	DDX_Control(pDX, IDC_SLIDER_VEGETABLE_WIND_POWER, SliderPower);
	DDX_Control(pDX, IDC_SLIDER_VEGETABLE_WIND_FREQUENCY, SliderFrequency);
	DDX_Control(pDX, IDC_SLIDER_VEGETABLE_WIND_BENDSTART, SliderBendStart);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVegetableWindDlg, CDialog)
	//{{AFX_MSG_MAP(CVegetableWindDlg)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_VEGETABLE_WIND_BENDSTART, OnReleasedcaptureSliderVegetableWindBendstart)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_VEGETABLE_WIND_FREQUENCY, OnReleasedcaptureSliderVegetableWindFrequency)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_VEGETABLE_WIND_POWER, OnReleasedcaptureSliderVegetableWindPower)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, OnButtonClose)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// ***************************************************************************
void	CVegetableWindDlg::updateView()
{
	float	a;
	char	str[256];

	// update Power.
	a= _ObjViewer->getVegetableWindPower();
	sprintf(str, "%.2f", a);
	StaticPower.SetWindowText(str);
	NLMISC::clamp(a, 0, NL_VEGETABLE_EDIT_WIND_MAX_POWER);
	SliderPower.SetPos((sint)(a*NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE / NL_VEGETABLE_EDIT_WIND_MAX_POWER));

	// update BendStart.
	a= _ObjViewer->getVegetableWindBendStart();
	sprintf(str, "%.2f", a);
	StaticBendStart.SetWindowText(str);
	NLMISC::clamp(a, 0, NL_VEGETABLE_EDIT_WIND_MAX_BENDSTART);
	SliderBendStart.SetPos((sint)(a*NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE / NL_VEGETABLE_EDIT_WIND_MAX_BENDSTART));

	// update Frequency.
	a= _ObjViewer->getVegetableWindFrequency();
	sprintf(str, "%.2f", a);
	StaticFrequency.SetWindowText(str);
	NLMISC::clamp(a, 0, NL_VEGETABLE_EDIT_WIND_MAX_FREQUENCY);
	SliderFrequency.SetPos((sint)(a*NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE / NL_VEGETABLE_EDIT_WIND_MAX_FREQUENCY));

}

// ***************************************************************************
// ***************************************************************************
// CVegetableWindDlg message handlers
// ***************************************************************************
// ***************************************************************************


// ***************************************************************************
BOOL CVegetableWindDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SliderPower.SetRange(0, NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE);
	SliderBendStart.SetRange(0, NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE);
	SliderFrequency.SetRange(0, NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE);
	
	// Init them and static.
	updateView();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



// ***************************************************************************
void CVegetableWindDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// test if one of my sliders.
	CSliderCtrl	*sliderCtrl= (CSliderCtrl*)pScrollBar;
	if( (sliderCtrl==&SliderPower || sliderCtrl==&SliderBendStart || sliderCtrl==&SliderFrequency)
		&& nSBCode==SB_THUMBPOSITION || nSBCode==SB_THUMBTRACK)
	{
		float	a;
		char	str[256];
		if(sliderCtrl == &SliderPower)
		{
			a= (float)nPos * NL_VEGETABLE_EDIT_WIND_MAX_POWER / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
			_ObjViewer->setVegetableWindPower(a);
			sprintf(str, "%.2f", a);
			StaticPower.SetWindowText(str);
		}
		else if(sliderCtrl == &SliderBendStart)
		{
			a= (float)nPos * NL_VEGETABLE_EDIT_WIND_MAX_BENDSTART / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
			_ObjViewer->setVegetableWindBendStart(a);
			sprintf(str, "%.2f", a);
			StaticBendStart.SetWindowText(str);
		}
		else if(sliderCtrl == &SliderFrequency)
		{
		
			a= (float)nPos * NL_VEGETABLE_EDIT_WIND_MAX_FREQUENCY / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
			_ObjViewer->setVegetableWindFrequency(a);
			sprintf(str, "%.2f", a);
			StaticFrequency.SetWindowText(str);
		}
	}
	else
	{
		CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	}
}

// ***************************************************************************
void CVegetableWindDlg::OnReleasedcaptureSliderVegetableWindPower(NMHDR* pNMHDR, LRESULT* pResult) 
{
	float	a;
	a= (float)SliderPower.GetPos() * NL_VEGETABLE_EDIT_WIND_MAX_POWER / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
	_ObjViewer->setVegetableWindPower(a);

	// refersh.
	updateView();
	
	*pResult = 0;
}

// ***************************************************************************
void CVegetableWindDlg::OnReleasedcaptureSliderVegetableWindBendstart(NMHDR* pNMHDR, LRESULT* pResult) 
{
	float	a;
	a= (float)SliderBendStart.GetPos() * NL_VEGETABLE_EDIT_WIND_MAX_BENDSTART / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
	_ObjViewer->setVegetableWindBendStart(a);

	// refersh.
	updateView();

	*pResult = 0;
}

// ***************************************************************************
void CVegetableWindDlg::OnReleasedcaptureSliderVegetableWindFrequency(NMHDR* pNMHDR, LRESULT* pResult) 
{
	float	a;
	a= (float)SliderFrequency.GetPos() * NL_VEGETABLE_EDIT_WIND_MAX_FREQUENCY / NL_VEGETABLE_EDIT_WIND_SLIDER_RANGE;
	_ObjViewer->setVegetableWindFrequency(a);

	// refersh.
	updateView();
	
	*pResult = 0;
}



// ***************************************************************************
void CVegetableWindDlg::OnButtonClose() 
{
	// hide the window
	ShowWindow(false);
}

