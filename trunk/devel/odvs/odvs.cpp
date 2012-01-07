/*
 * ODVS - Omnidirectional Vision Sensor
 * -------------------------------------------
 * MIGUEL TORRES TORRITI (c) 2007 v. 1.0
 * -------------------------------------------
 *
 * To run type:
 *  odvs [input image base_filename=l1] [-e extension=jpg] [options] [output image base_filename=l1_o]
 *
 * Description: 
 *  Dewarps an omnidirectionla hyperbolic image by mapping it onto a cylinder,
 *  in a Mercator-like projection, but in such a way that "Earth" parallels (latitude)
 *  are positioned in the new image at a distance from the horizontal axis proportional
 *  to the angle of incidence/reflection on the mirror of the corresponding ray that pass
 *  passes through the focal point of the camera lens.
 *  
 * By default odvs looks for input file l1.jpg and generates an output file l1_o.jpg, unless the 
 * base filename and extensions are changed using the optional parameters.
 *
 * Options:
 *  -h  Prints this help.
 *  -nv Turns verbose off.
 *  -np Do not wait for user to press ESC when done.
 *  -nd Do not display images.
 *  -no Do not save the output.
 *  -sq initial_value final_value  Applies odvs to a sequence of images
 *                                 starting at filename<initial_value>.ext ending in filename<initial_value>.ext
 *  -zp num  Pad filename suffix with num zeros.
 *
 * Author: Miguel Torres Torriti (c) 2007.12.15, v. 1.
 *
 */

#include "opencv/cv.h"      // use " " instead of < > to get list of class/struct members
#include "opencv/highgui.h" // use " " instead of < > to get list of class/struct members
#include <stdio.h>
#include <math.h>
#include <time.h>
// #include <conio.h>
#include <gsl/gsl_machine.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>

// Define global variables
#define pi 3.1415926535897932384626433832795

// Emulate conio's getch
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
 
int getch( ) 
{
	struct termios oldt, newt;
	int ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}

struct camera_param { // Camera Parameters
  double pixelSize; // CCD pixel size [mm]
  double f;         // Focal length [mm]
};

struct mirror_param { // Mirror Parameters
  // y^2/b^2-x^2/a^2=1   % Hyperbola equation
  // hf=sqrt(a^2+b^2);`        % Hyperbola focal distance
  // e=sqrt(1+b^/2/a^2); % Hyperbola excentricity
  double a;       // Hyperbola minor axis [mm]
  double b;       // Hyperbola major axis [mm]
  double hf;      // Hyperbola focal distance [mm]
  //double e=sqrt(1+mirror.b^2/mirror.a^2); % Hyperbola excentricity
  double hr_max;  // Hyperbola largest radius [mm]
  double voffset; // Vertical offset of the mirror-coordinates system's origin with respect to 
                  // the origin of the general-coordinate system
                  // (located at the camera's focal point) [mm]
  double innerpixels; // Radius (in pixels) of the inner disc that must be discarded [mm]
  // Auxiliary values
  double a2; // a2=mirror.a^2;
  double a4; // a4=a2^2;
  double f2; // f2=mirror.hf^2;
  double b2; // b2=mirror.b^2;
};


void init_param(camera_param *camera, mirror_param *mirror)
{
	// Camera Parameters
	(*camera).pixelSize=0.00577;	// CCD pixel size [mm] 
	(*camera).f=6.5; 	        // Focal length [mm]

	// Mirror Parameters
	(*mirror).a=23.27;    // Hyperbola minor axis [mm]
	(*mirror).b=39.68;    // Hyperbola major axis [mm]
	(*mirror).hf=46;      // Hyperbola focal distance [mm]
	(*mirror).hr_max=32;  // Hyperbola largest radius [mm]
	(*mirror).voffset=40; // Vertical offset of the mirror-coordinates system's origin with respect to 
                              // the origin of the general-coordinate system
                              // (located at the camera's focal point) [mm]
	(*mirror).innerpixels=60;  // Radius (in pixels) of the inner disc that must be discarded [mm]
	// Auxiliary values
	(*mirror).a2=pow((*mirror).a,2.0);
	(*mirror).a4=pow((*mirror).a2,2.0);
	(*mirror).f2=pow((*mirror).hf,2.0);
	(*mirror).b2=pow((*mirror).b,2.0);
}

void show_param(camera_param *camera, mirror_param *mirror)
{
	printf("Camera (pixelSize, f) [mm]: (%.2f, %.2f)\n", (*camera).pixelSize, (*camera).f);
	printf("Mirror (a, b, f) [mm]: (%.2f, %.2f, %.2f)\n", (*mirror).a, (*mirror).b, (*mirror).hf);
	printf("Mirror (hr_max, voffset) [mm]: (%.2f, %.2f)\n", (*mirror).hr_max, (*mirror).voffset);
}

struct data { // Optimization Data Structure
	size_t n;
	//double * y;
	//double * sigma;
	mirror_param* mirror;
	double phi;
};
     
int nls_f (const gsl_vector * x, void *data, gsl_vector * f)
{
	size_t n = f->size;
	//size_t n = ((struct data *)data)->n;
	//double *y = ((struct data *)data)->y;
	//double *sigma = ((struct data *) data)->sigma;
	//mirror_param *mirror = ((struct data *) data)->mirror;
	double b = (((struct data *) data)->mirror)->b;
	double a2 = (((struct data *) data)->mirror)->a2;
	double a4 = (((struct data *) data)->mirror)->a4;
	double f2 = (((struct data *) data)->mirror)->f2;
	double b2 = (((struct data *) data)->mirror)->b2;
	double voffset = (((struct data *) data)->mirror)->voffset;
	double phi = ((struct data *) data)->phi;

	double alpha = gsl_vector_get(x,0);
	double l = gsl_vector_get(x,1);
	
	double l2 = pow(l, 2.0);
	double sa2 = pow(sin(alpha),2.0);

	//size_t i;
	//for (i = 0; i < n; i++)
	//{
	//	// Model fi = (xi-xi0) 
	//	double fi = gsl_vector_get(x, i)-5.0*(double) i-1.0+f2;
	//	gsl_vector_set(f, i, fi);
	//}

	// f = [-mirror.b*l*sa2+cos(alpha)*sqrt(a4+a2*l2*sa2)-cos(phi)*sqrt(a4+f2*l2*sa2);
	//      l^2*(f2*cos(alpha)^2-b2)-2*a2*mirror.voffset*cos(alpha)*l+a2*mirror.voffset^2-a2*b2];
	gsl_vector_set(f, 0, -b*l*sa2+cos(alpha)*sqrt(a4+a2*l2*sa2)-cos(phi)*sqrt(a4+f2*l2*sa2));
	gsl_vector_set(f, 1, l2*(f2*pow(cos(alpha),2.0)-b2)-2.0*a2*voffset*cos(alpha)*l+a2*pow(voffset,2.0)-a2*b2);

	return GSL_SUCCESS;
}
     
int nls_df (const gsl_vector * x, void *data, gsl_matrix * J)
{
	size_t n = ((struct data *)data)->n;
	//double *sigma = ((struct data *) data)->sigma;
	gsl_vector* xd = gsl_vector_alloc(x->size);
	gsl_vector* f  = gsl_vector_alloc(n); 
	gsl_vector* fd = gsl_vector_alloc(n);
    
	nls_f(x, data, f);
	// Jacobian matrix J(i,j) = dfi / dxj ~= (f_i(x+delta_j)-f_i(x))/delta = fd_i-f_i/delta
	//                 J     ~= [ (f(x+delta_1)-f(x))/d (f(x+delta_2)-f(x))/d ... (f(x+delta_p)-f(x))/d ] 
	// where delta_k = [0 0 0 ... 0 delta 0 ... 0 0]^T
	// 
	for (unsigned int j=0; j<x->size; j++)
	{
		gsl_vector_memcpy(xd, x);
		gsl_vector_set(xd, j, gsl_vector_get(x,j)+1e-4); //GSL_DBL_EPSILON); // GSL_DBL_EPSILON defined in gsl_machine.h
		nls_f(xd, data, fd);
		for(unsigned int i=0; i<n; i++)
		{
			gsl_matrix_set(J, i, j, (gsl_vector_get(fd,i)-gsl_vector_get(f,i))/1e-4); //GSL_DBL_EPSILON);
		}
	}
 
	//gsl_matrix_set (J, 0, 0, 1);
	//gsl_matrix_set (J, 0, 1, 0);
	//gsl_matrix_set (J, 1, 0, 0);
	//gsl_matrix_set (J, 1, 1, 1);

	gsl_vector_free(xd);
	gsl_vector_free(f);
	gsl_vector_free(fd);
	return GSL_SUCCESS;
}
     
int nls_fdf (const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J)
{
	nls_f (x, data, f);
	nls_df (x, data, J);
     
	return GSL_SUCCESS;
}

void print_state (size_t iter, gsl_multifit_fdfsolver * s)
{
	printf ("iter: %3u --- x = % 15.8f % 15.8f --- |f(x)| = %g\n", iter,
				gsl_vector_get (s->x, 0), 
				gsl_vector_get (s->x, 1),
				gsl_blas_dnrm2 (s->f));
}


void hmr(mirror_param *mirror, double phi, gsl_vector* x0, gsl_vector * xsol)
{
	for (unsigned int i=0; i < x0->size; i++)
	{
		printf("x0[%d]=%.2f\n", i, gsl_vector_get(x0,(size_t) i));
	}

	unsigned int MAX_ITER = 500;
	const gsl_multifit_fdfsolver_type *T;
	gsl_multifit_fdfsolver *s;
	int status;
	unsigned int iter = 0;
	const size_t n = 2; // number of equations, s.t. f = [f_1, f_2, ..., f_n]
	const size_t p = 2; // number of independent variables, s.t. f: x -> f(x), x\in x^p
	//double y[n], sigma[n];
	//struct data d = { n, y, sigma };
	struct data d = { n, mirror, phi };

	double x_init[p] = { gsl_vector_get(x0, 0), gsl_vector_get(x0, 1) };
	gsl_vector_view x = gsl_vector_view_array (x_init, p);

	gsl_multifit_function_fdf f;
	f.f   = &nls_f;
	f.df  = &nls_df;
	f.fdf = &nls_fdf;
	f.n   = n;
	f.p   = p;
	f.params = &d; //mirror;
	gsl_vector* gF = gsl_vector_alloc(n);	

	T = gsl_multifit_fdfsolver_lmsder;
	s = gsl_multifit_fdfsolver_alloc(T, n, p);
	gsl_multifit_fdfsolver_set(s, &f, &x.vector);

	print_state (iter, s);
     
	do
	{
		iter++;
		status = gsl_multifit_fdfsolver_iterate(s);

		//printf("status = %s\n", gsl_strerror(status));
		printf("status = %s\n", gsl_strerror(status));

		print_state(iter, s);

		if (status)
			break;

		status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);		
		//gsl_multifit_gradient(s->J, s->f, gF);
		//status = gsl_multifit_test_gradient(gF, 1e-4);
	}
	while(status == GSL_CONTINUE && iter < MAX_ITER);
	
	printf("status = %s\n", gsl_strerror(status));

	gsl_vector_memcpy(xsol,s->x);

	gsl_multifit_fdfsolver_free(s);
	gsl_vector_free(gF);
}

IplImage *odvs_lut(camera_param *camera, mirror_param *mirror, IplImage *imgW, CvMat *u_xy_out, CvMat *v_xy_out)
{
	// This function checks if a Look-Up Table (LUT) for dewarping the input image
	// exists on file and has the adequate dimensions.  If so, it loads the existing LUT,
	// otherwise, the function uses the camera, mirror and image parameters to create 
	// a new LUT.
	// This function also allocates a memory space for the dewarped image.
	
	double rs_x, rs_y;
	int Nu, Nv, Nx, Ny;
	clock_t t_start, t_finish;
	double t_elapsed;

	// ---- Compute Dimensions of Output Image ----
	//Nx=input('Enter output image X size [Auto]: ');
	//Ny=input('Enter output image Y size [Auto]: ');
	//if isempty(Nx) | isempty(Ny),

	rs_x=1.0; // Horizontal scaling
	rs_y=1.0; // Vertical scaling

	Nu=imgW->width;
	Nv=imgW->height;
	Nx=(int) (ceil(rs_x*2.0*pi*((double) Nu/2.0))+1.0); // Theta (Azimuth Angle)
	//Ny=round(rs_y*(pi-theta_min)/thetaStepMax)+1; // Phi (Indicence Angle)
	Ny=(int) (ceil((rs_y*(double) Nv/2.0))+1.0); //pixelmax; //round(Nv)+1; // Phi (Indicence Angle)
	
	printf("Input image dimensions: Nu = %d, Nv = %d\n", Nu, Nv);


	printf("Calculating output dimensions... Nx = %d, Ny = %d\n", Nx, Ny);
	//imgD.create(cvSize(Nx,Ny), IPL_DEPTH_8U, 3); // only if imgD is the actual variable (IplImage) and not a pointer (IplImage*)
	//imgD->create(cvSize(Nx,Ny), IPL_DEPTH_8U, 3);
	//img_out = cvCreateImage(cvGetSize(img), img->depth, img->nChannels);
	IplImage *imgD = cvCreateImage(cvSize(Nx,Ny), IPL_DEPTH_8U, 3);
	//printf("[1] imgD size: %d/%d\n", imgD->width, imgD->height); 

	// ---- Inverse Mapping Look-up Table ----
	// Table of input image coordinates (u, v) as a function of the output image coordinates (x, y), 
	// i.e. Inverse Mapping u(x,y), v(x,y).
	CvMat* u_xy;
	CvMat* v_xy;
	

	// ---- Load/Create LUT ----
	bool LUT_FERROR=0;		// LUT file error flag.  
					// LUT_FERROR = 1 -> Compute LUT only if there is no previously computed LUT or 
					//                   the LUT dimensions are not as expected.
	t_start = clock();
	printf("Checking for LUT Tables...\n");
	if ((u_xy = (CvMat *) cvLoad("u_xy.xml", NULL, "u_xy", NULL ))!=0 && 
	    (v_xy = (CvMat *) cvLoad("v_xy.xml", NULL, "v_xy", NULL ))!=0
	)  // Load LUT from file if it exists.
	{
		printf("LUT files opened successfully!\n");

		// Check if LUT dimension is OK
		if((u_xy->width!=Nx)||(u_xy->height!=Ny)||(v_xy->width!=Nx)||(v_xy->height!=Ny))
		{
			printf("LUT dimension inconsistency!\n A new LUT will be computed.\n");
			LUT_FERROR = 1;
			cvReleaseMat(&u_xy);
			cvReleaseMat(&v_xy);
		}
	}
	else
	{
		printf("LUT files u_xy.xml and v_xy.xml not found!\n A new LUT will be computed.\n");
		LUT_FERROR = 1;
	}
	t_finish = clock();
	t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
	printf("Checking/loading LUT completed in %.1f miliseconds\n", 1000.0*t_elapsed );

	if (LUT_FERROR==1) // Compute LUT
	{
		printf("Computing LUT...");
		t_start = clock();

		u_xy = cvCreateMat(Ny, Nx, CV_32F);
		v_xy = cvCreateMat(Ny, Nx, CV_32F);
		cvSetZero(u_xy);
		cvSetZero(v_xy);

		double ymax, alphamax, lmin, lmax, phimin, phimax;	
		double alphamin;
		double aux1, aux2, aux3;
		//int pixelmax;

		// --- Calculate geometry ---
		// --- Maximum camera FOV angle ---
		// Assuming CCD array is much larger than mirror area, the mirror constrains the ROI.
		ymax=(*mirror).b*sqrt(1+pow((*mirror).hr_max,2.0)/pow((*mirror).a,2.0))+(*mirror).voffset;
		alphamax=atan((*mirror).hr_max/ymax); // Maximum angle of vision
		//pixelmax=(int) floor(( (*camera).f*((*mirror).hr_max/ymax) )/(*camera).pixelSize); // = f*tan(alphamax) = f*(hr_max/ymax);
		aux1=pow((*mirror).hf,2.0)*pow(cos(alphamax),2)-pow((*mirror).b,2.0);
		aux2=-2*(*mirror).voffset*pow((*mirror).a,2.0)*cos(alphamax);
		aux3=pow((*mirror).a,2.0)*pow((*mirror).voffset,2.0)-pow((*mirror).a,2.0)*pow((*mirror).b,2.0);
		lmax=(-aux2+sqrt(pow(aux2,2.0)-4*aux1*aux3))/(2*aux1);
		phimax=acos((-(*mirror).b*lmax*pow(sin(alphamax),2.0)+cos(alphamax)*(sqrt(pow((*mirror).a,4.0)+pow((*mirror).a,2.0)*pow(lmax,2.0)*pow(sin(alphamax),2.0))))/sqrt(pow((*mirror).a,4.0)+pow((*mirror).hf,2.0)*pow(lmax,2.0)*pow(sin(alphamax),2.0)));
		//printf("phimax: %.2f\n",phimax);

		alphamin=atan((*mirror).innerpixels*(*camera).pixelSize/(*camera).f);
		aux1=(pow((*mirror).hf,2.0)*pow(cos(alphamin),2.0)-pow((*mirror).b,2.0));
		aux2=-2.0*(*mirror).voffset*pow((*mirror).a,2.0)*cos(alphamin);
		aux3=pow((*mirror).a,2.0)*pow((*mirror).voffset,2.0)-pow((*mirror).a,2.0)*pow((*mirror).b,2.0);
		lmin=(-aux2+sqrt(pow(aux2,2.0)-4.0*aux1*aux3))/(2.0*aux1);
		phimin=acos((-(*mirror).b*lmin*pow(sin(alphamin),2.0)+cos(alphamin)*(sqrt(pow((*mirror).a,4.0)+pow((*mirror).a,2.0)*pow(lmin,2.0)*pow(sin(alphamin),2.0))))/sqrt(pow((*mirror).a,4.0)+pow((*mirror).hf,2.0)*pow(lmin,2.0)*pow(sin(alphamin),2.0)));

		/*** Parameters for spherical mirror ***
		R=40; 		%Mirror Radius [mm]
		alpha_max=atan(((Nu*pixelSize)/2.0)/f);		// Maximum angle of vision	
		theta_min=pi/2+alpha_max;					// Smallest angle from centre of the spheric mirror
		d=-R/cos(theta_min);						// Distance between optical centre (f point) and sphere centre
		thetaStepMax=acos(-(1/round(Nv/2)))-(pi/2);	// Largest theta step
		*/

		// -------------- Compute LUT --------------

		int co_u=0, co_v=0;	//Image center offset
		bool LUT_mode=1;	// 1: Dewarping based on the angle of incidence Phi
		                        // otherwise: Linear dewarping.
		double phi, alpha, r, beta;

		// LUTTime=cputime;
		// x0=[0;mirror.b+mirror.voffset]; % use with eq. (1) to scan image from center to edge
                // x_0=[alphamax; lmax];           % use with eq. (2) to scan image from edge to center
		gsl_vector* x0 = gsl_vector_alloc (2);
		gsl_vector_set(x0, 0, alphamax); 
		gsl_vector_set(x0, 1, lmax);
		gsl_vector* xsol = gsl_vector_calloc (2);
		hmr(mirror, pi/6.0, x0, xsol);
		printf("xsol[1]=%.2f, xsol[2]=%.2f\n", gsl_vector_get(xsol, 0), gsl_vector_get(xsol, 1) );
		gsl_vector_memcpy(x0,xsol);
		//gsl_vector_free(x0);
		//gsl_vector_free(xsol);

		for(int i=0; i<Ny; i++)
		{
			// phi=phimin+(phimax-phimin)*(y-1)/(Ny-1); % use with eq. (1) to scan image from center to edge
			phi=phimax-(phimax-phimin)*(double) i/(double) (Ny-1); // use with eq. (2) to scan image from edge to center
			// [yh,rn,r]=hmr1(mirror,phi,x0);
			// x_0=y_h;
			// alpha=y_h.val[0];
			hmr(mirror, phi, x0, xsol);
			printf("xsol[1]=%.2f, xsol[2]=%.2f\n", gsl_vector_get(xsol, 0), gsl_vector_get(xsol, 1) );
			gsl_vector_memcpy(x0,xsol);
			alpha=gsl_vector_get(xsol,0);
			////theta=theta_min+(pi-theta_min)*(y/Ny);
			////alpha=atan(sin(theta)/((d/R)+cos(theta)));
			if (LUT_mode==1)
			{
				r=((*camera).f/(*camera).pixelSize)*tan(alpha); // REMEMBER to adjust camera parameters two get the 
                                                                // whole image right.
			}
			else
			{
				//phi=2*((pi-theta)+alpha);
				r=((double) Nv/2.0-(*mirror).innerpixels)*(phi-phimin)/(phimax-phimin)+(*mirror).innerpixels;
			}
			
			for(int j=0; j<Nx; j++)
			{			
				//beta=-2.0*pi*(x/Nx)+3.0*pi/2.0;
				beta=2.0*pi*(double) j/(double) (Nx-1)-pi/2.0;
				cvmSet(u_xy, i, j, ( r*cos(beta)+(Nu/2.0)+co_u) ); // u_xy(y,x)=( r*cos(beta)+(Nu/2.0)+co_u);
				cvmSet(v_xy, i, j, (-r*sin(beta)+(Nv/2.0)+co_v) ); // v_xy(y,x)=(-r*sin(beta)+(Nv/2.0)+co_v);
			} // end for col j
		} // end for row i
		
		t_finish = clock();
		t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
		printf("LUT computation completed in %.1f miliseconds\n", 1000.0*t_elapsed );
		// LUTTime=cputime-LUTTime;

		gsl_vector_free(x0);
		gsl_vector_free(xsol);

		cvSave( "u_xy.xml", u_xy, "u_xy", NULL, cvAttrList(0,0) );// cannot use non-alpha characters in object descriptor, e.g. '(', ')'.
		cvSave( "v_xy.xml", v_xy, "v_xy", NULL, cvAttrList(0,0) );// cannot use non-alpha characters in object descriptor, e.g. '(', ')'.

	} // end if (LUT_FERROR==1)

	cvInitMatHeader( u_xy_out, u_xy->rows, u_xy->cols, CV_32F, NULL, CV_AUTOSTEP ); // initialize the matrix header
	cvCreateData( u_xy_out ); // allocate matrix data
	cvInitMatHeader( v_xy_out, v_xy->rows, v_xy->cols, CV_32F, NULL, CV_AUTOSTEP ); // initialize the matrix header
	cvCreateData( v_xy_out ); // allocate matrix data
	cvCopy(u_xy, u_xy_out, NULL);
	cvCopy(v_xy, v_xy_out, NULL);

	cvReleaseMat(&u_xy);
	cvReleaseMat(&v_xy);

	return imgD;
}

int odvs_dewarp(IplImage *imgW, IplImage *imgD, CvMat *u_xy, CvMat *v_xy)
{
	// This function dewarps the input image using the LUT created/loaded with the
	// function odvs_lut().  The LUT must be passed in the matrices u_xy, v_xy.

	//printf("[2] imgD size: %d/%d\n", imgD->width, imgD->height);
	int Nx=imgD->width, Ny=imgD->height;
	clock_t t_start, t_finish;
	double t_elapsed;

	// -------------- Dewarping --------------

	bool DEWARP_MODE=1;	// (1->Fast dewarping, else -> Dewarping with Interpolation).
	printf("Dewarping...\n");
	//WARPTime=cputime;
	t_start = clock();

	if (DEWARP_MODE==1)
	{
		for(int y=0; y<Ny; y++)
		{
			for(int x=0;x<Nx; x++)
			{
				// -- ImgXY(y,x)=ImgUV(v_xy(y,x),u_xy(y,x)); --
				//int pv= (int) cvmGet(v_xy,0,0);
				//int pu= (int) cvmGet(u_xy,0,0);
				// Obtain ImgUV(v_xy(y,x)),u_xy(y,x)
				// - use single-indexed referencing
				// -  or double-indexed referencing
				// Note that cvGet*D must receive const CvArr* Arr,
				// therefore must pass the contents of 'imgW' with imgW[0] or *imgW,
				// not the address imgW or &imgW where the contents reside.
				//CvScalar Iuv = cvGet1D(imgW[0], pu+pv*imgW[0].width());  // - use single-indexed referencing
				//CvScalar Iuv = cvGet2D(imgW[0], pv, pu);                // -  or double-indexed referencing
				//CvScalar Iuv = cvGet2D(*imgW, pv, pu);
				//cvSet2D(*imgD,0,0, Iuv);
				if(y < u_xy->rows)
					cvSet2D(imgD, y, x, cvGet2D(imgW, (int) cvmGet(v_xy, y, x), (int) cvmGet(u_xy, y, x)));
			}
		}
		//printf("Size: %d/%d\n",Ny,Nx);
	}
	else
	{
		for(int y=1;y<Ny-1;y++)
		{
			for(int x=1;x<Nx-1;x++)
			{
				/**** Extract pixels with subpixel accuracy using bilinear interpolation ****
				// This code was replaced by the OpenCV functions below.
				int u=(int) cvmGet(u_xy, y, x);
				int v=(int) cvmGet(v_xy, y, x);
				double mant_u=cvmGet(u_xy, y, x)-(double) u;	//Mantissa
				double mant_v=cvmGet(v_xy, y, x)-(double) v;
				if (u<0)
					u=0;
				if (v<0)
				    v=0;
		
				//aux_h1=(1.0-mant_u)*ImgUV(v,u)+mant_u*ImgUV(v,(u+1)); // Horizontal interpolation storage (1st and 2nd scanlines)
				//aux_h2=(1.0-mant_u)*ImgUV((v+1),u)+mant_u*ImgUV((v+1),(u+1));
				//aux_v=(1.0-mant_v)*aux_h1+mant_v*aux_h2;				// Vertical interpolation of aux_hi's storage
				//ImgXY(y,x)=fix(aux_v);

				//CvScalar aux_h1;
				//CvScalar aux_h2;
				//CvScalar aux_v;
				CvMat* aux_h1=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* aux_h2=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* aux_v =cvCreateMat(1, 1, CV_32FC3 );
				CvMat* p1=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* p2=cvCreateMat(1, 1, CV_32FC3 );
				//CvScalar p1=cvGet2D(*imgW, v, u);
				//CvScalar p2=cvGet2D(*imgW, v, u+1);
				cvSet1D( p1, 0, cvGet2D(*imgW, v, u));
				cvSet1D( p2, 0, cvGet2D(*imgW, v, u+1));
				CvScalar dd = cvGet1D(p1,0);
				cvAddWeighted(p1, (1.0-mant_u), p2, mant_u, 0, aux_h1);
				//p1=cvGet2D(*imgW, v+1, u);
				//p2=cvGet2D(*imgW, v+1, u+1);
				cvSet1D( p1, 0, cvGet2D(*imgW, v+1, u));
				cvSet1D( p2, 0, cvGet2D(*imgW, v+1, u+1));
				cvAddWeighted(p1, (1.0-mant_u), p2, mant_u, 0, aux_h2);				
				cvAddWeighted(aux_h1, (1.0-mant_v), aux_h2, mant_u, 0, aux_v);
				cvSet2D(*imgD, y, x, cvGet1D(aux_v,0));
				cvReleaseMat(&aux_h1);
				cvReleaseMat(&aux_h2);
				cvReleaseMat(&aux_v);
				cvReleaseMat(&p1);
				cvReleaseMat(&p2);
				*/

				// Extract 1x1 pixel block (i.e. single pixel) with subpixel accuracy using bilinear interpolation		
				CvMat* dst_rect=cvCreateMat(1, 1, CV_8UC3 );
// 				printf("Debug: get(%d,%d), size(%d,%d) and (%d,%d)\n",
// 					y, x,
// 					u_xy->rows,
// 					u_xy->cols,
// 					v_xy->rows,
// 					v_xy->cols);
				if(y < u_xy->rows)
				{
					cvGetRectSubPix( imgW, dst_rect,  cvPoint2D32f(cvmGet(u_xy, y, x), cvmGet(v_xy, y, x)) );
					cvSet2D(imgD, y, x, cvGet1D(dst_rect,0));
					cvReleaseMat(&dst_rect);
				}
				else
				{
					//printf("Y out of range (%d >= %d)\n", y, u_xy->rows);
				}
				// 
			} // end for x
		} // end for y
	} // end if (DEWARP_MODE==1)
	//WARPTime=cputime-WARPTime;
	t_finish = clock();
	t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
	printf("Dewarping completed in %.1f miliseconds\n", 1000.0*t_elapsed );

	return 0;
}

void odvs_lut_and_dewarp(camera_param *camera, mirror_param *mirror, IplImage *imgW, IplImage *imgD)
{
	double rs_x, rs_y;
	int Nu, Nv, Nx, Ny;
	clock_t t_start, t_finish;
	double t_elapsed;

	// ---- Compute Dimensions of Output Image ----
	//Nx=input('Enter output image X size [Auto]: ');
	//Ny=input('Enter output image Y size [Auto]: ');
	//if isempty(Nx) | isempty(Ny),

	rs_x=1.0; // Horizontal scaling
	rs_y=1.0; // Vertical scaling

	Nu=imgW->width; 
	Nv=imgW->height;
	Nx=(int) (ceil(rs_x*2.0*pi*((double) Nu/2.0))+1.0); // Theta (Azimuth Angle)
	//Ny=round(rs_y*(pi-theta_min)/thetaStepMax)+1; // Phi (Indicence Angle)
	Ny=(int) (ceil((rs_y*(double) Nv/2.0))+1.0); //pixelmax; //round(Nv)+1; // Phi (Indicence Angle)
	
	printf("Input image dimensions: Nu = %d, Nv = %d\n", Nu, Nv);

	printf("Calculating output dimensions... Nx = %d, Ny = %d\n", Nx, Ny);
	//imgD.create(cvSize(Nx,Ny), IPL_DEPTH_8U, 3); // only if imgD is the actual variable (IplImage) and not a pointer (IplImage*)
	//imgD->create(cvSize(Nx,Ny), IPL_DEPTH_8U, 3);
	imgD = cvCreateImage(cvSize(Nx,Ny), IPL_DEPTH_8U, 3);
	//printf("[3] imgD size: %d/%d\n", imgD->width, imgD->height);

	// ---- Inverse Mapping Look-up Table ----
	// Table of input image coordinates (u, v) as a function of the output image coordinates (x, y), 
	// i.e. Inverse Mapping u(x,y), v(x,y).
	CvMat* u_xy;
	CvMat* v_xy;
	
	// ---- Load/Create LUT ----
	bool LUT_FERROR=0;		// LUT file error flag.  
							// LUT_FERROR = 1 -> Compute LUT only if there is no previously computed LUT or 
							//                   the LUT dimensions are not as expected.
	t_start = clock();
	printf("Checking for LUT Tables...\n");
	if ((u_xy = (CvMat *) cvLoad("u_xy.xml", NULL, "u_xy", NULL ))!=0 && 
	    (v_xy = (CvMat *) cvLoad("v_xy.xml", NULL, "v_xy", NULL ))!=0
	   )  // Load LUT from file if it exists.
	{
		printf("LUT files opened successfully!\n");

		// Check if LUT dimension is OK
		if((u_xy->width!=Nx)||(u_xy->height!=Ny)||(v_xy->width!=Nx)||(v_xy->height!=Ny))
		{
			printf("LUT dimension inconsistency!\n A new LUT will be computed.\n");
			LUT_FERROR = 1;
			cvReleaseMat(&u_xy);
			cvReleaseMat(&v_xy);
		}
	}
	else
	{
		printf("LUT files u_xy.xml and v_xy.xml not found!\n A new LUT will be computed.\n");
		LUT_FERROR = 1;
	}
	t_finish = clock();
	t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
	printf("Checking/loading LUT completed in %.1f miliseconds\n", 1000.0*t_elapsed );

	if (LUT_FERROR==1) // Compute LUT
	{
		printf("Computing LUT...");
		t_start = clock();

		u_xy = cvCreateMat(Ny, Nx, CV_32F);
		v_xy = cvCreateMat(Ny, Nx, CV_32F);
		cvSetZero(u_xy);
		cvSetZero(v_xy);

		double ymax, alphamax, lmin, lmax, phimin, phimax;	
		double alphamin;
		double aux1, aux2, aux3;
		//int pixelmax;

		// --- Calculate geometry ---
		// --- Maximum camera FOV angle ---
		// Assuming CCD array is much larger than mirror area, the mirror constrains the ROI.
		ymax=(*mirror).b*sqrt(1+pow((*mirror).hr_max,2.0)/pow((*mirror).a,2.0))+(*mirror).voffset;
		alphamax=atan((*mirror).hr_max/ymax); // Maximum angle of vision
		//pixelmax=(int) floor(( (*camera).f*((*mirror).hr_max/ymax) )/(*camera).pixelSize); // = f*tan(alphamax) = f*(hr_max/ymax);
		aux1=pow((*mirror).hf,2.0)*pow(cos(alphamax),2)-pow((*mirror).b,2.0);
		aux2=-2*(*mirror).voffset*pow((*mirror).a,2.0)*cos(alphamax);
		aux3=pow((*mirror).a,2.0)*pow((*mirror).voffset,2.0)-pow((*mirror).a,2.0)*pow((*mirror).b,2.0);
		lmax=(-aux2+sqrt(pow(aux2,2.0)-4*aux1*aux3))/(2*aux1);
		phimax=acos((-(*mirror).b*lmax*pow(sin(alphamax),2.0)+cos(alphamax)*(sqrt(pow((*mirror).a,4.0)+pow((*mirror).a,2.0)*pow(lmax,2.0)*pow(sin(alphamax),2.0))))/sqrt(pow((*mirror).a,4.0)+pow((*mirror).hf,2.0)*pow(lmax,2.0)*pow(sin(alphamax),2.0)));
		//printf("phimax: %.2f\n",phimax);

		alphamin=atan((*mirror).innerpixels*(*camera).pixelSize/(*camera).f);
		aux1=(pow((*mirror).hf,2.0)*pow(cos(alphamin),2.0)-pow((*mirror).b,2.0));
		aux2=-2.0*(*mirror).voffset*pow((*mirror).a,2.0)*cos(alphamin);
		aux3=pow((*mirror).a,2.0)*pow((*mirror).voffset,2.0)-pow((*mirror).a,2.0)*pow((*mirror).b,2.0);
		lmin=(-aux2+sqrt(pow(aux2,2.0)-4.0*aux1*aux3))/(2.0*aux1);
		phimin=acos((-(*mirror).b*lmin*pow(sin(alphamin),2.0)+cos(alphamin)*(sqrt(pow((*mirror).a,4.0)+pow((*mirror).a,2.0)*pow(lmin,2.0)*pow(sin(alphamin),2.0))))/sqrt(pow((*mirror).a,4.0)+pow((*mirror).hf,2.0)*pow(lmin,2.0)*pow(sin(alphamin),2.0)));

		/*** Parameters for spherical mirror ***
		R=40; 		%Mirror Radius [mm]
		alpha_max=atan(((Nu*pixelSize)/2.0)/f);		// Maximum angle of vision	
		theta_min=pi/2+alpha_max;					// Smallest angle from centre of the spheric mirror
		d=-R/cos(theta_min);						// Distance between optical centre (f point) and sphere centre
		thetaStepMax=acos(-(1/round(Nv/2)))-(pi/2);	// Largest theta step
		*/

		// -------------- Compute LUT --------------

		int co_u=0, co_v=0;	//Image center offset
		bool LUT_mode=1;	// 1: Dewarping based on the angle of incidence Phi
		                    // otherwise: Linear dewarping.
		double phi, alpha, r, beta;

		// LUTTime=cputime;
		// x0=[0;mirror.b+mirror.voffset]; %use with eq.  (1) to scan image from center to edge
        // x_0=[alphamax; lmax];           % use with eq. (2) to scan image from edge to center
		gsl_vector* x0 = gsl_vector_alloc (2);
		gsl_vector_set(x0, 0, alphamax); 
		gsl_vector_set(x0, 1, lmax);
		gsl_vector* xsol = gsl_vector_calloc (2);
		hmr(mirror, pi/6.0, x0, xsol);
		printf("xsol[1]=%.2f, xsol[2]=%.2f\n", gsl_vector_get(xsol, 0), gsl_vector_get(xsol, 1) );
		gsl_vector_memcpy(x0,xsol);
		//gsl_vector_free(x0);
		//gsl_vector_free(xsol);

		for(int i=0; i<Ny; i++)
		{
			// phi=phimin+(phimax-phimin)*(y-1)/(Ny-1); % use with eq. (1) to scan image from center to edge
			phi=phimax-(phimax-phimin)*(double) i/(double) (Ny-1); // use with eq. (2) to scan image from edge to center
			// [yh,rn,r]=hmr1(mirror,phi,x0);
			// x_0=y_h;
			// alpha=y_h.val[0];
			hmr(mirror, phi, x0, xsol);
			printf("xsol[1]=%.2f, xsol[2]=%.2f\n", gsl_vector_get(xsol, 0), gsl_vector_get(xsol, 1) );
			gsl_vector_memcpy(x0,xsol);
			alpha=gsl_vector_get(xsol,0);
			////theta=theta_min+(pi-theta_min)*(y/Ny);
			////alpha=atan(sin(theta)/((d/R)+cos(theta)));
			if (LUT_mode==1)
			{
				r=((*camera).f/(*camera).pixelSize)*tan(alpha); // REMEMBER to adjust camera parameters two get the 
                                                                // whole image right.
			}
		    else
			{
				//phi=2*((pi-theta)+alpha);
				r=((double) Nv/2.0-(*mirror).innerpixels)*(phi-phimin)/(phimax-phimin)+(*mirror).innerpixels;
			}
			for(int j=0; j<Nx; j++)
			{			
				//beta=-2.0*pi*(x/Nx)+3.0*pi/2.0;
				beta=2.0*pi*(double) j/(double) (Nx-1)-pi/2.0;
				cvmSet(u_xy, i, j, ( r*cos(beta)+(Nu/2.0)+co_u) ); // u_xy(y,x)=( r*cos(beta)+(Nu/2.0)+co_u);
				cvmSet(v_xy, i, j, (-r*sin(beta)+(Nv/2.0)+co_v) ); // v_xy(y,x)=(-r*sin(beta)+(Nv/2.0)+co_v);
			} // end for col j
		} // end for row i
		t_finish = clock();
		t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
		printf("LUT computation completed in %.1f miliseconds\n", 1000.0*t_elapsed );
		// LUTTime=cputime-LUTTime;

		gsl_vector_free(x0);
		gsl_vector_free(xsol);

		cvSave( "u_xy.xml", u_xy, "u_xy", NULL, cvAttrList(0,0) );// cannot use non-alpha characters in object descriptor, e.g. '(', ')'.
		cvSave( "v_xy.xml", v_xy, "v_xy", NULL, cvAttrList(0,0) );// cannot use non-alpha characters in object descriptor, e.g. '(', ')'.

	} // end if (LUT_FERROR==1)

	// -------------- Dewarping --------------

	bool DEWARP_MODE=0;	// (1->Fast dewarping, else -> Dewarping with Interpolation).
	printf("Dewarping...\n");
	//WARPTime=cputime;
	t_start = clock();

	if (DEWARP_MODE==1)
	{
		for(int y=0; y<Ny; y++)
		{
			for(int x=0;x<Nx; x++)
			{
				// -- ImgXY(y,x)=ImgUV(v_xy(y,x),u_xy(y,x)); --
				//int pv= (int) cvmGet(v_xy,0,0);
				//int pu= (int) cvmGet(u_xy,0,0);
				// Obtain ImgUV(v_xy(y,x)),u_xy(y,x)
				// - use single-indexed referencing
				// -  or double-indexed referencing
				// Note that cvGet*D must receive const CvArr* Arr,
				// therefore must pass the contents of 'imgW' with imgW[0] or *imgW,
				// not the address imgW or &imgW where the contents reside.
				//CvScalar Iuv = cvGet1D(imgW[0], pu+pv*imgW[0].width());  // - use single-indexed referencing
				//CvScalar Iuv = cvGet2D(imgW[0], pv, pu);                // -  or double-indexed referencing
				//CvScalar Iuv = cvGet2D(*imgW, pv, pu);
				//cvSet2D(*imgD,0,0, Iuv);
				cvSet2D(imgD, y, x, cvGet2D(imgW, (int) cvmGet(v_xy, y, x), (int) cvmGet(u_xy, y, x)));
			}
		}
	}
	else
	{
		for(int y=1;y<Ny-1;y++)
		{
			for(int x=1;x<Nx-1;x++)
			{
				/**** Extract pixels with subpixel accuracy using bilinear interpolation ****
				// This code was replaced by the OpenCV functions below.
				int u=(int) cvmGet(u_xy, y, x);
				int v=(int) cvmGet(v_xy, y, x);
				double mant_u=cvmGet(u_xy, y, x)-(double) u;	//Mantissa
				double mant_v=cvmGet(v_xy, y, x)-(double) v;
				if (u<0)
					u=0;
				if (v<0)
				    v=0;
		
				//aux_h1=(1.0-mant_u)*ImgUV(v,u)+mant_u*ImgUV(v,(u+1)); // Horizontal interpolation storage (1st and 2nd scanlines)
				//aux_h2=(1.0-mant_u)*ImgUV((v+1),u)+mant_u*ImgUV((v+1),(u+1));
				//aux_v=(1.0-mant_v)*aux_h1+mant_v*aux_h2;				// Vertical interpolation of aux_hi's storage
				//ImgXY(y,x)=fix(aux_v);

				//CvScalar aux_h1;
				//CvScalar aux_h2;
				//CvScalar aux_v;
				CvMat* aux_h1=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* aux_h2=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* aux_v =cvCreateMat(1, 1, CV_32FC3 );
				CvMat* p1=cvCreateMat(1, 1, CV_32FC3 );
				CvMat* p2=cvCreateMat(1, 1, CV_32FC3 );
				//CvScalar p1=cvGet2D(*imgW, v, u);
				//CvScalar p2=cvGet2D(*imgW, v, u+1);
				cvSet1D( p1, 0, cvGet2D(*imgW, v, u));
				cvSet1D( p2, 0, cvGet2D(*imgW, v, u+1));
				CvScalar dd = cvGet1D(p1,0);
				cvAddWeighted(p1, (1.0-mant_u), p2, mant_u, 0, aux_h1);
				//p1=cvGet2D(*imgW, v+1, u);
				//p2=cvGet2D(*imgW, v+1, u+1);
				cvSet1D( p1, 0, cvGet2D(*imgW, v+1, u));
				cvSet1D( p2, 0, cvGet2D(*imgW, v+1, u+1));
				cvAddWeighted(p1, (1.0-mant_u), p2, mant_u, 0, aux_h2);				
				cvAddWeighted(aux_h1, (1.0-mant_v), aux_h2, mant_u, 0, aux_v);
				cvSet2D(*imgD, y, x, cvGet1D(aux_v,0));
				cvReleaseMat(&aux_h1);
				cvReleaseMat(&aux_h2);
				cvReleaseMat(&aux_v);
				cvReleaseMat(&p1);
				cvReleaseMat(&p2);
				*/

				// Extract 1x1 pixel block (i.e. single pixel) with subpixel accuracy using bilinear interpolation		
				CvMat* dst_rect=cvCreateMat(1, 1, CV_8UC3 );
				cvGetRectSubPix( imgW, dst_rect,  cvPoint2D32f(cvmGet(u_xy, y, x), cvmGet(v_xy, y, x)) );
				cvSet2D(imgD, y, x, cvGet1D(dst_rect,0));
				cvReleaseMat(&dst_rect);
				// 
			} // end for x
		} // end for y
	} // end if (DEWARP_MODE==1)
	//WARPTime=cputime-WARPTime;
	t_finish = clock();
	t_elapsed = (double)(t_finish - t_start) / CLOCKS_PER_SEC;
	printf("Dewarping completed in %.1f miliseconds\n", 1000.0*t_elapsed );

	cvReleaseMat(&u_xy);
	cvReleaseMat(&v_xy);
} // end odvs_lut_and_dewarp


int main( int argc, char** argv )
{
	char basefilename[256]="l1";
	char basefilename_out[530]="l1";
	char file_extension[256]="jpg";
	char filename[530]="l1.jpg";
	char filename_out[530]="l1_o.jpg";
	char filenameformat[200]="%s%d.%s"; // (filename)(number)(extension)
	char filenameformat_out[200]="%s_o_%d.%s"; // (filename)(number)(extension)
	int v0=0, v1=0;
	char *straux=NULL;
	bool output_flag = 1;
	bool display_flag = 1;
	bool verbose_flag = 1;
	bool pause_flag = 1;
	bool s_flag = 1; // process single image (s_flag = 0, process sequence of images) 

	camera_param camera;
	mirror_param mirror;
	IplImage *img_out;
	
	// Parse command line options
	for (int i=1; i<argc; i++)
	{	
		if (strcspn( argv[i], "-" )==0 && strstr(argv[i],"-nv")!=NULL)
		{
			verbose_flag = 0;
			break;
		}
	}
	if (verbose_flag) printf("Verbose is on.\n");

	for (int i=1; i<argc; i++)
	{	
		if (strcspn( argv[i], "-" )==0)
		{
			if (strstr(argv[i],"-nv")!=NULL)
			{
				verbose_flag = 0;
			}
			else if (strstr(argv[i],"-np")!=NULL)
			{
				pause_flag = 0;
			}
			else if (strstr(argv[i],"-nd")!=NULL)
			{
				if (verbose_flag) printf("No display.\n");
				display_flag = 0;
			}
			else if (strstr(argv[i],"-no")!=NULL)
			{
				if (verbose_flag) printf("No output.\n");
				output_flag = 0;
			}
			else if (strstr(argv[i],"-h")!=NULL)
			{
				printf("odvs [input image filename=l1.jpg] [options] [output image filename=l1_o.jpg]\n");
				printf("Description: Crops an image.\n");
				printf("Options:\n");
				printf("  -h  Prints this help.\n");
				printf("  -nv Turns verbose off.\n");
				printf("  -np Do not wait for user to press ESC when done.\n");
				printf("  -nd Do not display images.\n");
				printf("  -no Do not save the output.\n");
				printf("Author: Miguel Torres Torriti (c) 2007.12.15, v. 1.1.\n");
			}
			else if (strstr(argv[i],"-e")!=NULL)
			{
				sprintf(file_extension,"%s",argv[i+1]);
				i++;
			}
			else if (strstr(argv[i],"-sq")!=NULL)
			{
				v0=atoi(argv[i+1]);
				v1=atoi(argv[i+2]);
				i=i+2;
				s_flag = 0;
				printf("v0:%d, v1:%d\n",v0,v1);
			}
			else if (strstr(argv[i],"-zp")!=NULL)
			{
				sprintf(filenameformat,"%%s%%0%dd.%%s",atoi(argv[i+1]));  // Builds file suffix format string "%s%0<n>d"
				                                                          // where <n> is the number for zero padding.
				sprintf(filenameformat_out,"%%s_o_%%0%dd.%%s",atoi(argv[i+1]));  // Builds file suffix format string "%s%0<n>d"
				i++;                                                             // where <n> is the number for zero padding.
			}
			else
			{
				if (verbose_flag) printf("Invalid option detected: %s\n", argv[i]);
			}
		}
		else if (i==1)
		{
			sprintf(basefilename, "%s", argv[i]);
//			straux = strtok(filename, ".");
			sprintf(basefilename_out, "%s_o", argv[i]);
//			straux = strtok(NULL, ".");
//			sprintf(filename_out, "%s%s", filename_out, straux);
			// Generate the input filename
//			sprintf(filename, "%s", argv[i]);  // Previous tokenization moves the pointer, thus must re-print
			if (verbose_flag) printf("Input filename : %s\n", basefilename);
			if (verbose_flag) printf("Output filename: %s\n", basefilename_out);
		}
		else
		{
			sprintf(basefilename_out, "%s", argv[i]);
			if (verbose_flag) printf("New output filename: %s\n", basefilename_out);
		}
	}

	// Initialize camera parameters
	init_param(&camera, &mirror);
	if (verbose_flag) show_param(&camera, &mirror);

	IplImage *img; //IplImage img(filename, 0, CV_LOAD_IMAGE_COLOR);
	CvMat *u_xy=cvCreateMatHeader(1, 1, CV_32F);  // allocate matrix headers
	CvMat *v_xy=cvCreateMatHeader(1, 1, CV_32F);
	if (display_flag) cvNamedWindow( "Input Image", CV_WINDOW_AUTOSIZE );
	if (display_flag) cvNamedWindow( "Dewarped Image", CV_WINDOW_AUTOSIZE ); 
	for(int i=v0; i<=v1; i++) // for-{sequence of images}
	{	
		if (verbose_flag) printf("Dewarping image %d/%d\n", i-v0+1, v1-v0+1);

		// Build input/output image filenames
		if (s_flag)
		{
			sprintf(filename, "%s.%s", basefilename, file_extension);
			sprintf(filename_out, "%s_o.%s", basefilename_out, file_extension);
			printf("Input: %s\n", filename);
			printf("Output: %s\n", filename_out);
		}
		else
		{
			sprintf(filename,filenameformat,basefilename,i,file_extension);
			sprintf(filename_out,filenameformat_out,basefilename_out,i,file_extension);
			printf("Input: %s\n", filename);
			printf("Output: %s\n", filename_out);
		}


		// Load input image
		//IplImage img(filename, 0, CV_LOAD_IMAGE_COLOR); // img cannot be declared multiple times (moved out of the for-loop)
		//img.load(filename, 0, CV_LOAD_IMAGE_COLOR);
		img = cvLoadImage(filename);

	    	if( !img ) // Check if the image has been loaded properly
		{
			fprintf( stderr, "ERROR: NULL image loaded, press any key to skip. \n" );
			getchar();
			return -1;
		}

		//img_out = cvCreateImage(cvGetSize(img), img->depth, img->nChannels);

		// Create a window and display the input image
		if (display_flag)
		{
			//cvNamedWindow( "Input Image", CV_WINDOW_AUTOSIZE ); // the same window "Input Image" cannot be declared multiple times (moved out of the for-loop)
			cvShowImage( "Input Image", img ); // .show method is the conveninient form of cvShowImage
			//if (pause_flag) printf("Press a key to continue...\n");
			//if (pause_flag) cvWaitKey();
		}

		// Process input image
		//odvs_lut_and_dewarp(&camera, &mirror, &img, &img_out);  // This function is equivalent to running:
									//       odvs_lut + odvs_dewarp
									// By separating the LUT calculation from
									// the dewarping process, it is now possible
									// to process a sequence of images without 
									// having to load the LUT each time.
	
		if (i==v0)
		{
			// moved outside the for loop
			//CvMat *u_xy=cvCreateMatHeader(1, 1, CV_32F);  // allocate matrix headers
			//CvMat *v_xy=cvCreateMatHeader(1, 1, CV_32F);
			img_out = odvs_lut(&camera, &mirror, img, u_xy, v_xy);
		}
		
		//printf("[4] imgD size: %d/%d\n", img_out->width, img_out->height);
		
		odvs_dewarp(img, img_out, u_xy, v_xy);
		
		if (i==v1)
		{
			cvReleaseMat(&u_xy);
			cvReleaseMat(&v_xy);
		}

		// Create a window and display the output image
		if (display_flag)
		{
			//cvNamedWindow( "Dewarped Image", CV_WINDOW_AUTOSIZE ); // the same window "Dewarped Image" cannot be declared multiple times (moved out of the for-loop)
			//img_out.show( "Dewarped Image" ); // .show method is the conveninient form of cvShowImage
			cvShowImage("Dewarped Image", img_out);
			if (pause_flag) printf("Press a key to continue...\n");
			if (pause_flag) cvWaitKey();
		}

		// Save output
		if (output_flag)
		{
			cvSaveImage(filename_out, img_out);
		}
	
		cvReleaseImage(&img);
		cvReleaseImage(&img_out);
	} // end for-{sequence of images}

	if (verbose_flag)
	{
		printf("Done!\n");
// 		if (pause_flag)
// 		{
// 			printf("Press ESC to quit.\n");
// 			while( getch()!= 0x1B); // Wait till ESC is pressed
// 		}
	}

	return 0;
    // all the images will be released automatically
}
