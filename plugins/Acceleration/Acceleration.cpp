/* ========================================
 *  Acceleration - Acceleration.h
 *  Created 8/12/11 by SPIAdmin 
 *  Copyright (c) 2011 __MyCompanyName__, All rights reserved
 * ======================================== */

#ifndef ACCELERATION_H
#define ACCELERATION_H

#include <set>
#include <string>
#include <math.h>

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

enum {
      kParamA = 0,
      kParamB = 1,
      kNumParameters = 2
};

const int kNumPrograms = 0;
const int kNumInputs   = 2;
const int kNumOutputs  = 2;

class Acceleration : public Plugin 
{
public:
  Acceleration() : Plugin(kNumParameters, 0, 0) {
    // clear all parameters
    std::memset(fParameters, 0, sizeof(float)*kNumParameters);
    
    // we can know buffer-size right at the start
    fParameters[kParameterBufferSize] = getBufferSize();
  }
protected:
  const char* getLabel() const override
  {
    return "Acceleration";
  }
  const char* getDescription() const override
  {
    return "Acceleration AirWindows plugin.";
  }
  const char* getMaker() const override
  {
    return "AirWindows";
  }
  const char* getHomePage() const override
  {
    return "https://github.com/airwindows/airwindows";
  }
  const char* getLicense() const override
  {
    return "MIT";
  }
  uint32_t getVersion() const override
  {
    return d_version(1, 0, 0);
  }
  int64_t getUniqueId() const override
  {
    return d_cconst('d', 'a', 'c', 'c');
  }
  void bufferSizeChanged(uint32_t newBufferSize) override
  {
    fParameters[kParameterBufferSize] = newBufferSize;
  }
  float getParameterValue(uint32_t index) const override
  {
    return fParameters[index];
  }
  void setParameterValue(uint32_t index, float value) override
  {
    fParameters[index] = value;
  }
  void initParameter(uint32_t index, Parameter& parameter) override {
    parameter.hints = kParameterIsAutomable;
    parameter.ranges.min = 0.0f;
    parameter.ranges.max = 1.0f;
    parameter.ranges.def = 0.0f;

    switch (index) {
    case kParamA:
      parameter.name   = "Intensity";
      parameter.symbol = "intensity";
      break;
    case kParamB:
      parameter.name   = "Dry/Wet";
      parameter.symbol = "dry-wet";
      break;
    }
  }

  void run(const float** inputs, float** outputs, uint32_t frames) override {
    float* in1  =  (float *)inputs[0];
    float* in2  =  (float *)inputs[1];
    float* out1 = outputs[0];
    float* out2 = outputs[1];

    double overallscale = 1.0;
    overallscale /= 44100.0;
    overallscale *= getSampleRate();
	
    double intensity = pow(A,3)*(32/overallscale);
    double wet = B;
    double dry = 1.0 - wet;
	
    double senseL;
    double smoothL;
    double senseR;
    double smoothR;
    double accumulatorSample;
    double drySampleL;
    double drySampleR;
    long double inputSampleL;
    long double inputSampleR;
	    
    while (--frames != 0)
      {
	inputSampleL = *in1;
	inputSampleR = *in2;
	if (inputSampleL<1.2e-38 && -inputSampleL<1.2e-38) {
	  static int noisesource = 0;
	  //this declares a variable before anything else is compiled. It won't keep assigning
	  //it to 0 for every sample, it's as if the declaration doesn't exist in this context,
	  //but it lets me add this denormalization fix in a single place rather than updating
	  //it in three different locations. The variable isn't thread-safe but this is only
	  //a random seed and we can share it with whatever.
	  noisesource = noisesource % 1700021; noisesource++;
	  int residue = noisesource * noisesource;
	  residue = residue % 170003; residue *= residue;
	  residue = residue % 17011; residue *= residue;
	  residue = residue % 1709; residue *= residue;
	  residue = residue % 173; residue *= residue;
	  residue = residue % 17;
	  double applyresidue = residue;
	  applyresidue *= 0.00000001;
	  applyresidue *= 0.00000001;
	  inputSampleL = applyresidue;
	}
	if (inputSampleR<1.2e-38 && -inputSampleR<1.2e-38) {
	  static int noisesource = 0;
	  noisesource = noisesource % 1700021; noisesource++;
	  int residue = noisesource * noisesource;
	  residue = residue % 170003; residue *= residue;
	  residue = residue % 17011; residue *= residue;
	  residue = residue % 1709; residue *= residue;
	  residue = residue % 173; residue *= residue;
	  residue = residue % 17;
	  double applyresidue = residue;
	  applyresidue *= 0.00000001;
	  applyresidue *= 0.00000001;
	  inputSampleR = applyresidue;
	  //this denormalization routine produces a white noise at -300 dB which the noise
	  //shaping will interact with to produce a bipolar output, but the noise is actually
	  //all positive. That should stop any variables from going denormal, and the routine
	  //only kicks in if digital black is input. As a final touch, if you save to 24-bit
	  //the silence will return to being digital black again.
	}
	drySampleL = inputSampleL;
	drySampleR = inputSampleR;
		
	s3L = s2L;
	s2L = s1L;
	s1L = inputSampleL;
	smoothL = (s3L + s2L + s1L) / 3.0;
	m1L = (s1L-s2L)*((s1L-s2L)/1.3);
	m2L = (s2L-s3L)*((s1L-s2L)/1.3);
	senseL = fabs(m1L-m2L);
	senseL = (intensity*intensity*senseL);
	o3L = o2L;
	o2L = o1L;
	o1L = senseL;
	if (o2L > senseL) senseL = o2L;
	if (o3L > senseL) senseL = o3L;
	//sense on the most intense
		
	s3R = s2R;
	s2R = s1R;
	s1R = inputSampleR;
	smoothR = (s3R + s2R + s1R) / 3.0;
	m1R = (s1R-s2R)*((s1R-s2R)/1.3);
	m2R = (s2R-s3R)*((s1R-s2R)/1.3);
	senseR = fabs(m1R-m2R);
	senseR = (intensity*intensity*senseR);
	o3R = o2R;
	o2R = o1R;
	o1R = senseR;
	if (o2R > senseR) senseR = o2R;
	if (o3R > senseR) senseR = o3R;
	//sense on the most intense
		
	if (senseL > 1.0) senseL = 1.0;
	if (senseR > 1.0) senseR = 1.0;
		
	inputSampleL *= (1.0-senseL);
	inputSampleR *= (1.0-senseR);
		
	inputSampleL += (smoothL*senseL);
	inputSampleR += (smoothR*senseR);
		
	senseL /= 2.0;
	senseR /= 2.0;
		
	accumulatorSample = (ataLastOutL*senseL)+(inputSampleL*(1.0-senseL));
	ataLastOutL = inputSampleL;
	inputSampleL = accumulatorSample;
		
	accumulatorSample = (ataLastOutR*senseR)+(inputSampleR*(1.0-senseR));
	ataLastOutR = inputSampleR;
	inputSampleR = accumulatorSample;		
		
	if (wet !=1.0) {
	  inputSampleL = (inputSampleL * wet) + (drySampleL * dry);
	  inputSampleR = (inputSampleR * wet) + (drySampleR * dry);
	}
		
	//stereo 32 bit dither, made small and tidy.
	int expon; frexpf((float)inputSampleL, &expon);
	long double dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
	inputSampleL += (dither-fpNShapeL); fpNShapeL = dither;
	frexpf((float)inputSampleR, &expon);
	dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
	inputSampleR += (dither-fpNShapeR); fpNShapeR = dither;
	//end 32 bit dither

	*out1 = inputSampleL;
	*out2 = inputSampleR;

	*in1++;
	*in2++;
	*out1++;
	*out2++;
      }
  }
private:
  // Parameters
  float fParameters[kParameterCount];
  
  long double fpNShapeL;
  long double fpNShapeR;
  //default stuff
  double ataLastOutL;
  double s1L;
  double s2L;
  double s3L;
  double o1L;
  double o2L;
  double o3L;
  double m1L;
  double m2L;
  double desL;
  
  double ataLastOutR;
  double s1R;
  double s2R;
  double s3R;
  double o1R;
  double o2R;
  double o3R;
  double m1R;
  double m2R;
  double desR;
  
  float A;
  float B;
  
  DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Acceleration)
};

Plugin* createPlugin() {
  return new Acceleration();
}

END_NAMESPACE_DISTRHO

#endif
