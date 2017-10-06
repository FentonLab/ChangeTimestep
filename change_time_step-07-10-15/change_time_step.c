/*	change_time_step.c		*/

/* 08-08-2009
set the fread() and fwrite() parameters to read constant 1 and 4 byte data pieces, conforming to the TS format.
Previously these were machine-specific because they were set by sizeof() calls, which could generate errors on some machines */

#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<assert.h>
#include 	"libaoaux.h"
#include 	"librand.h"

#define DEB 		0	
#define DEB_PXL 	0	
#define MAX_SPSZ	256

#define	TS_1_BYTE	1
#define	TS_4_BYTES	4


void 	change_time_step (fp_i, fp_o, time_step_factor) 
    	FILE *fp_i, *fp_o;
	unsigned int time_step_factor;

{
	unsigned long  	
		step, l_dummy;
	unsigned int
		spk_time[MAX_SPSZ]; 

	extern unsigned int
		print_pixel_nmb, max_xsz, max_ysz,
		accept_all,
		scale_x, scale_y,
		one_zero_bad, anesth;

	unsigned int 
		i, i_spikes, numb_header_lines, pixel_nmb, different_pixel_nmb;

  	unsigned int
		x, y, spikes,
		x_sum, y_sum, spk_num_sum,
		bad_sample, nmb_unknown_pix;

	unsigned int **pixel;


	str256	s, keyword;

    	unsigned char
		xc, yc, spikesc, ch_dummy, found, spks;
	

    	static unsigned int
		ts_1_byte = TS_1_BYTE,
		ts_4_bytes = TS_4_BYTES;


	double 	*pixel_weight;
	float	value;
		
				

	void count_pixel_weights (unsigned int **pixel, unsigned int time_step_factor, double *pixel_weight, unsigned int *different_pixel_nmb);



//	printf ("\n time_step_factor = %d\n", time_step_factor);
	if (!accept_all) my_seed48();


	pixel = (unsigned int **) calloc (max_ysz * max_xsz, sizeof(unsigned int *)); assert (pixel);
	for (i=0; i< (max_ysz * max_xsz); i++) {
		pixel[i] = (unsigned int *) calloc (2, sizeof (unsigned int)); assert (pixel[i]);
	}

	pixel_weight = (double *) calloc (time_step_factor, sizeof(double)); assert (pixel_weight);


    
    	fscanf(fp_i,"%d", &numb_header_lines);
	fprintf(fp_o,"%d", numb_header_lines);

#if (DEB)
    printf("\nnumber of lines in the header : %d", numb_header_lines);
//    getchar();
#endif


	found = 0;
	for (i = 0; i < numb_header_lines ; i++){
		get_line(fp_i, s); 
                sscanf(s,"%s%f", &keyword, &value);
                if(!strcmp(keyword,"%SAMPLING_INTERVAL(samps/sec)")){
			fprintf(fp_o,"\t%s %0.2f\n", keyword, value /(float)time_step_factor); 
			found++;
		}else if(!strcmp(keyword, "%SCALE_Y(RatioTracktoMapPixels)")){
			fprintf(fp_o,"\t%s %0.2f\n", keyword, value * (float)scale_y); 
			found++;
		}else if(!strcmp(keyword, "%SCALE_X(RatioTracktoMapPixels)")){
			fprintf(fp_o,"\t%s %0.2f\n", keyword, value * (float)scale_x); 
			found++;
		}else{
			fprintf(fp_o,"%s\n", s); 
		}
        }
	if(found != 3){
                printf("String '%%SAMPLING_INTERVAL(samps/sec)' or \n");
                printf("String '%%%SCALE_Y(RatioTracktoMapPixels)' or\n");
                printf("String '%%%SCALE_X(RatioTracktoMapPixels)' not found. File is corrupt\n");
                exit(-1);
        }

	x_sum = y_sum = spk_num_sum = pixel_nmb = 0;
	nmb_unknown_pix = 0;
	i_spikes = 0;
	step = 1;

	while ( !feof(fp_i) ) {
		fread (&xc, ts_1_byte, 1, fp_i); x = (unsigned int) xc;
		fread (&yc, ts_1_byte, 1, fp_i); y = (unsigned int) yc;
		fread (&spikesc, ts_1_byte, 1, fp_i); spikes = (unsigned int) spikesc;
	

#if (DEB)
//	The first three characters:
		printf ("\n x = %d\t y = %d\t spikes = %d\n", x, y, spikes);
#endif
		assert ( spikes < MAX_SPSZ);

		if ((!x && !y) || (one_zero_bad && !x) || (one_zero_bad && !y))	
			bad_sample = 1;
		else
			bad_sample = 0;

		for (i=0; i<spikes; i++)  fread (&(spk_time[i + spk_num_sum]), ts_4_bytes, 1, fp_i);
	

//Collecting data for averaging along time_step_factor time steps		
		if(bad_sample)
			nmb_unknown_pix++;
		else{
			x_sum += (x / scale_x);
			y_sum += (y / scale_y);
		}

		if (print_pixel_nmb) {
			pixel [pixel_nmb][0] = y;
			pixel [pixel_nmb][1] = x;
			// pixel [pixel_nmb][0] = y / scale_y;
			// pixel [pixel_nmb][1] = x / scale_x;
			pixel_nmb++;
		} 

		spk_num_sum += spikes;
#if (DEB)
		printf ("\n x_sum = %d\t y_sum = %d\t spk_num_sum = %d\n", x_sum, y_sum, spk_num_sum);
		getchar();
#endif

		if ( step % time_step_factor == 0 )  {
			if (nmb_unknown_pix == time_step_factor) {
				xc = (unsigned char) 0;
				yc = (unsigned char) 0;
			} else {
				xc = (unsigned char) (x_sum / (time_step_factor - nmb_unknown_pix));
				yc = (unsigned char) (y_sum / (time_step_factor - nmb_unknown_pix));
			}
					 
//			printf ("\n****    x = %d\t y = %d spk_sum=%d\n", x, y, spk_num_sum);
//			getchar();
			assert(spk_num_sum < MAX_SPSZ); 

			if (accept_all || drand48() < 0.5) {
				fwrite((void *)&xc, ts_1_byte, 1, fp_o);
				fwrite((void *)&yc, ts_1_byte, 1, fp_o);
				spks = (unsigned char)spk_num_sum;
				fwrite((void *)&spks, ts_1_byte, 1, fp_o);
				for (i=0; i<spk_num_sum; i++) fwrite((void *) &(spk_time[i]), ts_4_bytes, 1, fp_o);
			}
			
			if (print_pixel_nmb) {
#if (DEB_PXL)
				for (i=0; i< pixel_nmb; i++ ) printf ("\n step = %d\t pixel [%d][0] = %d\t pixel [%d][1] = %d", step, i, pixel [i][0], i, pixel [i][1]);
				getchar();
#endif
				count_pixel_weights (pixel, time_step_factor, pixel_weight, &different_pixel_nmb);
				printf ("\n different_pixel_nmb:\t%d", different_pixel_nmb);	
				printf("\n highest pixel weight:\t%f", pixel_weight[0]);
			}

			x_sum = y_sum = spk_num_sum = pixel_nmb = 0;
			nmb_unknown_pix = 0;
		}
		step++;
//		end_while:
	}
	return;
}


void count_pixel_weights (pixel, time_step_factor, pixel_weight, different_pixel_nmb)
	unsigned int 	**pixel, time_step_factor, *different_pixel_nmb;
	double 		*pixel_weight;	
{
	unsigned int 		i, j, pixel_nmb, x0, y0, i_dummy;
	unsigned int 		unique_pixel [time_step_factor][2];
	double			unique_pixel_weight [time_step_factor];

	extern unsigned int 	max_xsz;

	void sort_d_0 (double *, unsigned int);

	
	for (i=0; i<time_step_factor; i++) unique_pixel_weight[i] = 0.0;

	pixel_nmb = 0; 

	for (i=0; i<time_step_factor ; i++) {
		if (pixel [i][0] == 0 && pixel [i][1] == 0) continue;
		
		unique_pixel [pixel_nmb][0] = y0 = pixel [i][0];
		unique_pixel [pixel_nmb][1] = x0 = pixel [i][1];
		unique_pixel_weight [pixel_nmb] = 1;
		pixel_nmb ++; 
 
		for (j=i+1; j<time_step_factor; j++) 
			if (pixel [j][0] == 0 && pixel [j][1] == 0) continue;
			else 
				if (pixel [j][0] == y0 && pixel [j][1] == x0 ) {
					pixel [j][0] = pixel [j][1] = 0;
					unique_pixel_weight [pixel_nmb - 1]++;
				}

	}



//	pixel_nmb   	- number of different pixels

//	NORMALIZING WEIGHTS

	i_dummy = 0;
	for (i=0; i<pixel_nmb; i++) {
		i_dummy += (unsigned int) unique_pixel_weight [i];
		unique_pixel_weight [i] = unique_pixel_weight [i] / time_step_factor;
	}

//	printf ("\ncontrol_sum = %d", i_dummy);


//	SORTING WEIGHTS

//	printf("\n");
//	for (i=0; i<pixel_nmb; i++) printf("\t unique_pixel_weight [%d] = %f", i, unique_pixel_weight[i]);

	sort_d_0 (unique_pixel_weight, pixel_nmb);

//			unique_pixel_weight [0] has the greatest weight;
//	printf("\n");
//	for (i=0; i<pixel_nmb; i++) printf("\t unique_pixel_weight [%d] = %f", i, unique_pixel_weight[i]);

	for (i=0; i<time_step_factor; i++) pixel_weight[i] = unique_pixel_weight[i];

	*different_pixel_nmb = pixel_nmb;

}


void sort_d_0 (a, n)
	double 		*a;
	unsigned int 	n;
{
	unsigned int 	i,j, flag;
	double 		d_dummy;



	for (i=0; i<n; i++) {
		flag = 1;
		for (j=0; j < (n - i -1); j++)
			if (a[j] < a[j+1]) { 
				d_dummy = a[j];
				a[j] = a[j+1];
				a[j+1] = d_dummy;
				flag = 0;
			}
		if (flag) break;

	}
}
