#include "m_pd.h"
#include <math.h>

//benCompressor~ is a basic compression algorithm implemented as a puredata external.
//Ben Mangold

static t_class *benCompressor_tilde_class;

typedef struct _benCompressor_tilde { //dataspace
  t_object  x_obj;
  t_int sr; //samplerate
  t_int n; //block size
  
  t_float f_coeff;
  t_sample f;
  t_float f_thresh;
  t_float f_ratio;
  t_float f_attack;
  t_float f_release;
  t_int attackFlag;
  t_float f_gain_change; // gain increment
  t_float f_gain_target;
  t_int i_fadesamps; 

} t_benCompressor_tilde;


t_float benCompressor_tilde_rms (t_sample *block, t_int n){
	
	t_float rms, sum;
	t_int i;
	t_sample blockCopy[n];
	
	for(i=0; i<n; i++)
		blockCopy[i] = 0.0;
		
	for(i=0; i<n; i++)
		blockCopy[i] = block[i] * block[i];	
	
	sum = 0;
	
	for(i=0; i<n; i++)
		sum += blockCopy[i];
		
	rms = sum/(t_float)n;
	
	rms = sqrt(rms);	
	
	return(rms);
}

void benCompressor_tilde_print (t_benCompressor_tilde *x){

	post("THRESH: %f", x->f_thresh);
	post("RATIO: %f", x->f_ratio);
	post("ATTACK: %f", x->f_attack);
	post("RELEASE: %f", x->f_release);

}

void benCompressor_tilde_thresh (t_benCompressor_tilde *x, t_floatarg thresh){
	x->f_thresh = thresh;
}

void benCompressor_tilde_ratio (t_benCompressor_tilde *x, t_floatarg ratio){
	
	if(ratio < 1){
		ratio = 1;
	}
	
	x->f_ratio = ratio;
	post("ratio change: %f",x->f_ratio);
}

void benCompressor_tilde_attack (t_benCompressor_tilde *x, t_floatarg attack){
	
	if(attack < 1){
		attack = 1;
	}
	
	
	x->f_attack = attack;
	post("attack change: %f",x->f_attack);
}

void benCompressor_tilde_release (t_benCompressor_tilde *x, t_floatarg release){
	
	if (release < 1){
		release = 1;
	}
	
	x->f_release = release;
	post("release change: %f",x->f_release);
}

//Ramp function to be used in attack and release
void benCompressor_tilde_rampcmd (t_benCompressor_tilde *x, t_floatarg target, t_floatarg time){
	
	t_float ampdif;  //
	t_int fadesamps; //fadetime in samples

	x->f_gain_target = (target<0)?0.0:target;
	x->f_gain_target = (x->f_gain_target>1)?1.0:x->f_gain_target;

//	post("target: %f, time: %f", x->f_gain_target, time);

	ampdif = x->f_gain_target - x->f_coeff;
	fadesamps  = x->sr * (time/1000);
	
	x->f_gain_change = ampdif/(t_float)(fadesamps-1);
	x->i_fadesamps = fadesamps;
	
}

// DSP Perform Routine
t_int *benCompressor_tilde_perform(t_int *w)          
{

	t_benCompressor_tilde *x = (t_benCompressor_tilde *)(w[1]);
	t_sample  *in =    (t_sample *)(w[2]);
	t_sample  *out =    (t_sample *)(w[3]);
	t_float current_rms = 0;
	
	t_int n = (int)(w[4]);
	
	
	current_rms = benCompressor_tilde_rms(in, n);
//	post("RMS: %f", current_rms);

// calculate a target amp if we're over the threshold. use the current attack time provided by the user as the ramp duration

/*
if attackFlag == 0 && RMS > thresh,
	attackFlag =1;
	ramp gain down;
else if attackFlag ==1 && RMS > thresh,
	adjust gain;
else if RMS < thresh,
	attackFlag = 0;
	ramp gain back to unity
*/

	if(current_rms > x->f_thresh && x->attackFlag == 0)
	{
		t_float overage, targetAmp;
		
		x->attackFlag = 1;

		overage = current_rms - x->f_thresh;
		overage /= x->f_ratio;
		targetAmp = x->f_thresh + overage;

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, targetAmp/current_rms, x->f_attack);
	}
	else if(current_rms > x->f_thresh && x->attackFlag == 1)
	{

		t_float overage, targetAmp;

		overage = current_rms - x->f_thresh;
		overage /= x->f_ratio;
		targetAmp = x->f_thresh + overage;

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, targetAmp/current_rms, x->f_attack);
	}
	else if(current_rms < x->f_thresh)
	{

		// rampcmd function takes amplitude in RMS, time in ms
		benCompressor_tilde_rampcmd(x, 1.0, x->f_release);
		x->attackFlag = 0;
	}
	
	t_sample coeff = x->f_coeff;
	
	while (n--){
		if(x->i_fadesamps-- > 1)
			coeff = coeff + x->f_gain_change;
		else
		{
			x->i_fadesamps = 0;
			coeff = x->f_gain_target;
		}

		
		// makeup gain stage here	
		*out++ = (*in++) * coeff;		
	}
	
	x->f_coeff = coeff;
	
	return (w+5);
}

// Adds DSP perform routine
void benCompressor_tilde_dsp(t_benCompressor_tilde *x, t_signal **sp)   //(pointer to class-dataspace, pointer to an array of signals)
{
  //Adds dsp perform routine to the DSP-tree. dsp_add(dsp perform routine, # of following pointers, sp[0] is first in-signal, sp[1] is second in-signal,sp[3] points to out-signal)

	dsp_add(
		benCompressor_tilde_perform,
		4,
		x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[0]->s_n
	);
	x->sr = sp[0]->s_sr;
	x->n = sp[0]->s_n;

}

void *benCompressor_tilde_new()
{
  t_benCompressor_tilde *x = (t_benCompressor_tilde *)pd_new(benCompressor_tilde_class);
  

	x->f_thresh = 0.05;
	x->f_ratio = 10;

	x->f_attack = 50;
	x->f_release = 50;
	
	x->f_coeff = 1.0;
	x->f_gain_target = x->f_coeff;

	x->i_fadesamps = 0;
	x->f_gain_change=0;
	x->attackFlag = 0;
	
	outlet_new(&x->x_obj, &s_signal);
	
	return (void *)x;
}

void benCompressor_tilde_setup(void) {
	benCompressor_tilde_class = class_new(
		
		gensym("benCompressor~"),
		(t_newmethod)benCompressor_tilde_new,
        0,
        sizeof(t_benCompressor_tilde),
        CLASS_DEFAULT, A_DEFFLOAT,
        0
    );

class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_thresh,
        gensym("thresh"),
	A_DEFFLOAT,
        0
    );

class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_ratio,
        gensym("ratio"),
	A_DEFFLOAT,
        0
    );

class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_attack,
        gensym("attack"),
	A_DEFFLOAT,
        0
    );

class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_release,
        gensym("release"),
	A_DEFFLOAT,
        0
    );

class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_print,
        gensym("print"),
        0
    );


	class_addmethod(
		
		benCompressor_tilde_class,
        (t_method)benCompressor_tilde_dsp,
        gensym("dsp"),
        0
    );

    CLASS_MAINSIGNALIN(benCompressor_tilde_class, t_benCompressor_tilde, f);

}

