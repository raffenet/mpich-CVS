#include "mpi.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>

#define RGBtocolor_t(r,g,b) ((color_t)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define getR(r) ((int)((r) & 0xFF))
#define getG(g) ((int)((g>>8) & 0xFF))
#define getB(b) ((int)((b>>16) & 0xFF))

typedef int color_t;

int mpi_send_xyminmax(double xmin, double ymin, double xmax, double ymax);
int mpi_read_data(MPI_Comm comm, char *buffer, int length);

extern HWND g_hWnd;
extern HDC g_hDC;
extern int g_width, g_height;
extern HANDLE g_hMutex;
static int g_num_colors;
static color_t *colors;

MPI_Comm comm = MPI_COMM_NULL;

static color_t getColor(double fraction, double intensity)
{
    /* fraction is a part of the rainbow (0.0 - 1.0) = (Red-Yellow-Green-Cyan-Blue-Magenta-Red)
    intensity (0.0 - 1.0) 0 = black, 1 = full color, 2 = white
    */
    double red, green, blue;
    int r,g,b;
    double dtemp;

    fraction = fabs(modf(fraction, &dtemp));

    if (intensity > 2.0)
	intensity = 2.0;
    if (intensity < 0.0)
	intensity = 0.0;

    dtemp = 1.0/6.0;

    if (fraction < 1.0/6.0)
    {
	red = 1.0;
	green = fraction / dtemp;
	blue = 0.0;
    }
    else
    {
	if (fraction < 1.0/3.0)
	{
	    red = 1.0 - ((fraction - dtemp) / dtemp);
	    green = 1.0;
	    blue = 0.0;
	}
	else
	{
	    if (fraction < 0.5)
	    {
		red = 0.0;
		green = 1.0;
		blue = (fraction - (dtemp*2.0)) / dtemp;
	    }
	    else
	    {
		if (fraction < 2.0/3.0)
		{
		    red = 0.0;
		    green = 1.0 - ((fraction - (dtemp*3.0)) / dtemp);
		    blue = 1.0;
		}
		else
		{
		    if (fraction < 5.0/6.0)
		    {
			red = (fraction - (dtemp*4.0)) / dtemp;
			green = 0.0;
			blue = 1.0;
		    }
		    else
		    {
			red = 1.0;
			green = 0.0;
			blue = 1.0 - ((fraction - (dtemp*5.0)) / dtemp);
		    }
		}
	    }
	}
    }

    if (intensity > 1)
    {
	intensity = intensity - 1.0;
	red = red + ((1.0 - red) * intensity);
	green = green + ((1.0 - green) * intensity);
	blue = blue + ((1.0 - blue) * intensity);
    }
    else
    {
	red = red * intensity;
	green = green * intensity;
	blue = blue * intensity;
    }

    r = (int)(red * 255.0);
    g = (int)(green * 255.0);
    b = (int)(blue * 255.0);

    return RGBtocolor_t(r,g,b);
}

static int Make_color_array(int num_colors, color_t colors[])
{
    double fraction, intensity;
    int i;

    intensity = 1.0;
    for (i=0; i<num_colors; i++)
    {
	fraction = (double)i / (double)num_colors;
	colors[i] = getColor(fraction, intensity);
    }
    return 0;
}

static void getRGB(color_t color, int *r, int *g, int *b)
{
    *r = getR(color);
    *g = getG(color);
    *b = getB(color);
}

int mpi_connect_to_pmandel(const char *port, int &width, int &height)
{
    int result;
    /*char err[100];*/

    result = MPI_Comm_connect((char*)port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
    if (result != MPI_SUCCESS)
    {
	MessageBox(NULL, "MPI_Comm_connect failed", "Error", MB_OK);
	return -1;
    }

    mpi_read_data(comm, (char*)&width, sizeof(int));
    mpi_read_data(comm, (char*)&height, sizeof(int));
    mpi_read_data(comm, (char*)&g_num_colors, sizeof(int));
    g_width = width;
    g_height = height;
    colors = new color_t[g_num_colors+1];
    Make_color_array(g_num_colors, colors);
    colors[g_num_colors] = 0; /* add one on the top to avoid edge errors */
    /*
    sprintf(err, "num_colors = %d", g_num_colors);
    MessageBox(NULL, err, "Note", MB_OK);
    */
    return 0;
}

int mpi_send_xyminmax(double xmin, double ymin, double xmax, double ymax)
{
    double temp[4];
    int result, total;
    char err[100];
    RECT r;

    if (xmin != xmax && ymin != ymax && g_hDC)
    {
	r.left = 0;
	r.right = g_width;
	r.top = 0;
	r.bottom = g_height;
	FillRect(g_hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    total = 4 * sizeof(double);
    temp[0] = xmin;
    temp[1] = ymin;
    temp[2] = xmax;
    temp[3] = ymax;

    result = MPI_Send(temp, 4, MPI_DOUBLE, 0, 0, comm);
    if (result != MPI_SUCCESS)
    {
	sprintf(err, "send failed: error %d", result);
	MessageBox(NULL, err, "Error", MB_OK);
	return -1;
    }
    return 0;
}

int mpi_read_data(MPI_Comm comm, char *buffer, int length)
{
    int result;
    char err[100];
    MPI_Status status;

    result = MPI_Recv(buffer, length, MPI_CHAR, 0, 0, comm, &status);
    if (result != MPI_SUCCESS)
    {
	sprintf(err, "recv failed: error %d", result);
	MessageBox(NULL, err, "Error", MB_OK);
	return result;
    }
    return 0;
}

int mpi_get_pmandel_data()
{
    int temp[4];
    int *buffer;
    int size;
    int result;
    int i, j, k;
    RECT r;

    for (;;)
    {
	result = mpi_read_data(comm, (char*)temp, 4*sizeof(int));
	if (result)
	    return 0;
	if (temp[0] == 0 && temp[1] == 0 && temp[2] == 0 && temp[3] == 0)
	    return 0;
	size = (temp[1] + 1 - temp[0]) * (temp[3] + 1 - temp[2]);
	buffer = new int[size];
	result = mpi_read_data(comm, (char *)buffer, size * sizeof(int));
	if (result)
	{
	    delete buffer;
	    return 0;
	}
	WaitForSingleObject(g_hMutex, INFINITE);
	k = 0;
	for (j=temp[2]; j<=temp[3]; j++)
	{
	    for (i=temp[0]; i<=temp[1]; i++)
	    {
		//SetPixel(g_hDC, i, j, colors[buffer[k]]);
		SetPixelV(g_hDC, i, j, colors[buffer[k]]);
		k++;
	    }
	}
	ReleaseMutex(g_hMutex);
	r.left = 0;
	r.right = g_width;
	r.top = 0;
	r.bottom = g_height;
	InvalidateRect(g_hWnd, &r, FALSE);
	delete buffer;
    }
    return 1;
}
