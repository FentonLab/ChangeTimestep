#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include 	<assert.h>
#include	"libaoaux.h"

#define DEB	0
#define MAX_XSZ		256
#define MAX_YSZ		256
#define SCALE_Y		4
#define SCALE_X		4
#define SCALE_DIR	1
#define	MAX_DIRECTIONSZ	360

// typedef char str256[256];
void instruct();

// void	change_time_step (FILE *, FILE *, unsigned int);
// void	change_time_step_tsd (FILE *, FILE *, unsigned int, float); 

unsigned int
		print_pixel_nmb = 0,
		max_xsz		= MAX_XSZ,
		max_ysz		= MAX_YSZ,
		scale_y		= SCALE_Y,
		scale_x		= SCALE_X,
		accept_all	= 1,
		anesth		= 0,
		one_zero_bad	= 0,
		max_directionsz = MAX_DIRECTIONSZ;
		

int main(int argc, char **argv)
{
	FILE	
	    *fp_i, *fp_o;
	

	extern unsigned int optind;
	extern char *optarg;
//        extern  char    *strcpy();

	unsigned int time_step_factor = 0, do_directions = 0;

	char 	ch_dummy;
	float	scale_dir = SCALE_DIR;

	str256 	in_file, out_file, out_dir, in_dir;
 
	
	if (argc < 2) instruct();

        while ((ch_dummy = getopt(argc,argv,"Dd:CPM:X:x:Y:y:Z")) != EOF)
                switch(ch_dummy){
                        // case 'A': anesth = 0; break;
                        case 'D': do_directions = 1; break;
                        case 'd': scale_dir = atoi(optarg); break;
                        case 'X': max_xsz = atoi(optarg); break;
                        case 'Y': max_ysz = atoi(optarg); break;
                        case 'y': scale_y = atoi(optarg); break;
                        case 'x': scale_x = atoi(optarg); break;
                        case 'M': time_step_factor = atof(optarg); break;
                        case 'P': print_pixel_nmb = 1; break;
                        case 'C': accept_all = 0; break;
                        case 'Z': one_zero_bad = 1; break;
               }

	if(time_step_factor == 0){
		printf("Must specify the factor by which to collapse the timestep in the TS file with option M\n.");
		instruct();
	}

	if(do_directions){
		check_env ("TSD_DIR");  strcpy(in_dir, getenv("TSD_DIR"));
		check_env ("TSD_INFO_DIR");  strcpy(out_dir, getenv("TSD_INFO_DIR"));
	}else{
		check_env ("TS_DIR");  strcpy(in_dir, getenv("TS_DIR"));
		check_env ("TS_INFO_DIR");  strcpy(out_dir, getenv("TS_INFO_DIR"));
	}

	sprintf(in_file, "%s/%s",in_dir, argv[optind]);
	printf ("using INPUT file = %s\n", in_file);

	sprintf (out_file, "%s/%s%s%d", out_dir, argv[optind], "_", time_step_factor);
	printf ("using OUTPUT file = %s\n\n", out_file);

        fp_i = fopen(in_file,"r"); assert (fp_i != NULL);
        fp_o = fopen(out_file,"w"); assert (fp_o != NULL);
	
	if(do_directions)
		change_time_step_tsd (fp_i, fp_o, time_step_factor, scale_dir);
	else
		change_time_step (fp_i, fp_o, time_step_factor);

	fclose (fp_i); 
	fclose (fp_o); 

	return(0);
}


void instruct()
{
	(void)fprintf(stderr,"\n Rewrites a file in TS format for a greater time step");
	(void)fprintf(stderr,"\n Call with a file in TS format and option M\n");
	(void)fprintf(stderr,"\n Necessary environment:");
	(void)fprintf(stderr,"\n TS_DIR directory must be specified for the input data");
	(void)fprintf(stderr,"\n TS_INFO_DIR directory must be specified for the output data filename is appended with the timestep factor");
	(void)fprintf(stderr,"\n Option:");
	(void)fprintf(stderr,"\n A - pixel (0,0) is not to be considered; by default it is");
	(void)fprintf(stderr,"\n D - Compute Directional Information. The input file is a tsd format file in TSD directory.");
	(void)fprintf(stderr,"\n d - Scale directions (will divide by this factor). Default is %d\n", SCALE_DIR);
	(void)fprintf(stderr,"\n M - (OBLIGATORY) number of initial time_steps that must be considered as one");
	(void)fprintf(stderr,"\n X - maximal number of pixels in X dimension; default is 256");
	(void)fprintf(stderr,"\n Y - maximal number of pixels in Y dimension; default is 256");
	(void)fprintf(stderr,"\n y - Scale y values (for P option to print a pixel number in a proper scale); default is %d", SCALE_Y);
	(void)fprintf(stderr,"\n x - Scale x values (for P option to print a pixel number in a proper scale); default is %d", SCALE_X);
	(void)fprintf(stderr,"\n P - print number of pixels passed during M time steps; default is do not print");
	(void)fprintf(stderr,"\n C - accept randomly chosen half of pixels; by default accept all");
	(void)fprintf(stderr,"\n Z - Consider a sample to be a no detect if either the x or the y coordinate is 0. Default: bad samples are only (0,0)\n\n");
	exit(2);
}
