/*	change_time_step_tsd.c		*/

/* 07-10-2015
enable change time step to operate on a tsd file */

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
#define NO_DIRECTION	-1

void 	change_time_step_tsd (fp_i, fp_o, time_step_factor, scale_dir) 
    	FILE *fp_i, *fp_o;
	unsigned int time_step_factor;
	float	scale_dir;

{
	unsigned long  	
		step, l_dummy;
	unsigned int
		spk_time[MAX_SPSZ]; 

	extern unsigned int
		print_direction_nmb, print_pixel_nmb, max_xsz, max_ysz, max_directionsz,
		accept_all,
		scale_x, scale_y,
		one_zero_bad, anesth;

	int	dir4;

	unsigned int 
		i, i_spikes, numb_header_lines, pixel_nmb, direction_nmb, different_pixel_nmb, different_direction_nmb;

  	unsigned int
		x, y, spikes,
		x_sum, y_sum, spk_num_sum,
		bad_pix_sample, bad_dir_sample, nmb_pix_unknown, nmb_dir_unknown;

	unsigned int **pixel;
	int	*direction;

	str256	s, keyword;

    	unsigned char
		xc, yc, spikesc, ch_dummy, found, spks;

    	static unsigned int
		ts_1_byte = TS_1_BYTE,
		ts_4_bytes = TS_4_BYTES;


	double 	*pixel_weight, *direction_weight;
	float	value, dir, dir_sum;
				
	void count_pixel_weights (unsigned int **pixel, unsigned int time_step_factor, double *pixel_weight, unsigned int *different_pixel_nmb);
	void count_dir_weights (int *direction, unsigned int time_step_factor, double *direction_weight, unsigned int *different_direction_nmb);

	if (!accept_all) my_seed48();

	pixel = (unsigned int **) calloc (max_ysz * max_xsz, sizeof(unsigned int *)); assert (pixel);
        for (i=0; i< (max_ysz * max_xsz); i++) {
                pixel[i] = (unsigned int *) calloc (2, sizeof (unsigned int)); assert (pixel[i]);
        }

        pixel_weight = (double *) calloc (time_step_factor, sizeof(double)); assert (pixel_weight);

	direction = (int *) calloc (max_directionsz, sizeof(unsigned int)); assert (direction);
	direction_weight = (double *) calloc (time_step_factor, sizeof(double)); assert (direction_weight);

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
	fflush(fp_o);
	if(found != 3){
                printf("String '%%SAMPLING_INTERVAL(samps/sec)' or \n");
                printf("String '%%%SCALE_Y(RatioTracktoMapPixels)' or\n");
                printf("String '%%%SCALE_X(RatioTracktoMapPixels)' not found. File is corrupt\n");
                exit(-1);
        }

	dir_sum = 0.0;
	x_sum = y_sum = spk_num_sum = pixel_nmb = direction_nmb = 0;
	nmb_pix_unknown = 0;
	nmb_dir_unknown = 0;
	i_spikes = 0;
	step = 1;

	while ( !feof(fp_i) ) {
		fread (&xc, ts_1_byte, 1, fp_i); x = (unsigned int) xc;
		fread (&yc, ts_1_byte, 1, fp_i); y = (unsigned int) yc;
		fread (&dir4, ts_4_bytes, 1, fp_i); dir = (float) dir4;
		fread (&spikesc, ts_1_byte, 1, fp_i); spikes = (unsigned int) spikesc;
	
		assert ( spikes < MAX_SPSZ);

		if(dir < 0.0)
			bad_dir_sample = 1;
		else
			bad_dir_sample = 0;

		if ((!x && !y) || (one_zero_bad && !x) || (one_zero_bad && !y))	
			bad_pix_sample = 1;
		else
			bad_pix_sample = 0;

		for (i=0; i<spikes; i++)  fread (&(spk_time[i + spk_num_sum]), ts_4_bytes, 1, fp_i);
	
		//Collecting data for averaging along time_step_factor time steps		
		if(bad_pix_sample){
			nmb_pix_unknown++;
		}
		if (bad_dir_sample){
			nmb_dir_unknown++;
		}
		if (!bad_pix_sample){
			x_sum += (x / scale_x);
			y_sum += (y / scale_y);
		}
		if(!bad_dir_sample){
			dir /= scale_dir;
			if(dir > 180.0)
				dir -=360.0;
			else if(dir < 0.0)
				dir += 360.0;
			dir_sum += dir;
		}

/*
		if (print_pix_nmb) {
			pixel [pixel_nmb][0] = y;
			pixel [pixel_nmb][1] = x;
			// pixel [pixel_nmb][0] = y / scale_y;
			// pixel [pixel_nmb][1] = x / scale_x;
			pixel_nmb++;
		} 
*/

		spk_num_sum += spikes;

		if (step % time_step_factor == 0 )  {
			if (nmb_pix_unknown == time_step_factor) {
				xc = (unsigned char) 0;
				yc = (unsigned char) 0;
			}
			if (nmb_dir_unknown == time_step_factor){
				dir4 = NO_DIRECTION;
			}
			if(nmb_pix_unknown != time_step_factor){ 
				xc = (unsigned char) (x_sum / (time_step_factor - nmb_pix_unknown));
				yc = (unsigned char) (y_sum / (time_step_factor - nmb_pix_unknown));
			}
			if(nmb_dir_unknown != time_step_factor){ 
				dir_sum /= (float)(time_step_factor - nmb_dir_unknown);
				if(dir_sum < 0.0)
					dir_sum += 360.0;
				dir4 = (int)(dir_sum);
			}

// printf("HERE %d %f %d %d %d \n",dir4, dir_sum, time_step_factor , nmb_pix_unknown, (time_step_factor - nmb_pix_unknown)); 
					 
//			printf ("\n****    x = %d\t y = %d spk_sum=%d\n", x, y, spk_num_sum);
//			getchar();
			assert(spk_num_sum < MAX_SPSZ); 

			if (accept_all || drand48() < 0.5) {
				fwrite((void *)&xc, ts_1_byte, 1, fp_o);
				fwrite((void *)&yc, ts_1_byte, 1, fp_o);
				fwrite((void *)&dir4, ts_4_bytes, 1, fp_o);
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

				count_dir_weights (direction, time_step_factor, direction_weight, &different_direction_nmb);
				printf ("\n different_direction_nmb:\t%d", different_direction_nmb);	
				printf("\n highest direction weight:\t%f", direction_weight[0]);
                        }

			x_sum = y_sum = spk_num_sum = pixel_nmb = direction_nmb = 0;
			dir_sum = 0.0;
			nmb_pix_unknown = 0;
			nmb_dir_unknown = 0;
		}
		step++;
//		end_while:
	}
	return;
}


void count_dir_weights (direction, time_step_factor, direction_weight, different_direction_nmb)
	int		*direction;
	unsigned int 	time_step_factor, *different_direction_nmb;
	double 		*direction_weight;	
{
	int			dir0;
	unsigned int 		i, j, direction_nmb, i_dummy;
	unsigned int 		unique_direction [time_step_factor];
	double			unique_direction_weight [time_step_factor];

	extern unsigned int 	max_xsz;

	void sort_d_0 (double *, unsigned int);

	
	for (i=0; i<time_step_factor; i++) unique_direction_weight[i] = 0.0;

	direction_nmb = 0; 

	for (i=0; i<time_step_factor ; i++) {
		if (direction[i] == NO_DIRECTION) continue;
		
		unique_direction[direction_nmb] = dir0 = direction[i];
		unique_direction_weight[direction_nmb] = 1;
		direction_nmb++; 
 
		for (j=i+1; j<time_step_factor; j++) {
			if (direction[j] == NO_DIRECTION)
				continue;
			else{ 
				if (direction [j] == dir0) {
					direction[j] = direction[j] = 0;
					unique_direction_weight [direction_nmb - 1]++;
				}
			}
		}
	}


//	direction_nmb   	- number of different directions

//	NORMALIZING WEIGHTS

	i_dummy = 0;
	for (i=0; i<direction_nmb; i++) {
		i_dummy += (unsigned int) unique_direction_weight[i];
		unique_direction_weight[i] = unique_direction_weight[i] / time_step_factor;
	}

//	printf ("\ncontrol_sum = %d", i_dummy);


//	SORTING WEIGHTS

//	printf("\n");
//	for (i=0; i<direction_nmb; i++) printf("\t unique_direction_weight [%d] = %f", i, unique_direction_weight[i]);

	sort_d_0 (unique_direction_weight, direction_nmb);

//			unique_direction_weight[0] has the greatest weight;
//	printf("\n");
//	for (i=0; i<direction_nmb; i++) printf("\t unique_direction_weight [%d] = %f", i, unique_direction_weight[i]);

	for (i=0; i<time_step_factor; i++) direction_weight[i] = unique_direction_weight[i];

	*different_direction_nmb = direction_nmb;

}
