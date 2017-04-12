/*
Copyright(c) 2017, James Slepicka
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include <cmath>
#include <memory>
#include <cstdint>

class c_resampler
{
public:
	c_resampler(float input_rate, float output_rate);
	virtual ~c_resampler();
	void set_output_rate(float output_rate);
	void process(float sample);
	int get_output_buf(const int16_t **out);
private:
	float input_rate;
	float output_rate;
	float m, mf;
	int samples_required;
	static const int OUTPUT_BUF_LEN = 1024;
	static const int INPUT_BUF_LEN = 4;
	int output_buf_index;
	int input_buf_index;
	short *output_buf;
	float *input_buf;
};

c_resampler::c_resampler(float input_rate, float output_rate)
{
	this->input_rate = input_rate;
	this->output_rate = output_rate;
	m = input_rate / output_rate;
	mf = 0.0f;
	output_buf_index = 0;
	input_buf_index = INPUT_BUF_LEN - 1;

	mf = m - (int)m;
	samples_required = (int)m + 2;

	output_buf = new short[OUTPUT_BUF_LEN];
	input_buf = new float[INPUT_BUF_LEN * 2];

	memset(output_buf, 0, sizeof(output_buf));

	for (int i = 0; i < INPUT_BUF_LEN * 2; i++)
		input_buf[i] = 0.0f;
}

c_resampler::~c_resampler()
{
	delete[] output_buf;
	delete[] input_buf;
}

void c_resampler::set_output_rate(float output_rate)
{
	this->output_rate = output_rate;
	m = input_rate / output_rate;
}

int c_resampler::get_output_buf(const int16_t **out)
{
	*out = this->output_buf;
	int x = output_buf_index;
	output_buf_index = 0;
	return x;
}

void c_resampler::process(float sample)
{
	input_buf[input_buf_index] = 
		input_buf[input_buf_index + INPUT_BUF_LEN] = sample;

	if (--samples_required == 0)
	{
		float y2 = input_buf[input_buf_index];
		float y1 = input_buf[input_buf_index + 1];
		float y0 = input_buf[input_buf_index + 2];
		float ym = input_buf[input_buf_index + 3];

		//4-point, 3rd-order B-spline (x-form)
		//see deip.pdf
		float ym1py1 = ym + y1;
		float c0 = 1.0f / 6.0f*ym1py1 + 2.0f / 3.0f*y0;
		float c1 = 1.0f / 2.0f*(y1 - ym);
		float c2 = 1.0f / 2.0f*ym1py1 - y0;
		float c3 = 1.0f / 2.0f*(y0 - y1) + 1.0f / 6.0f*(y2 - ym);
		float j = ((c3*mf + c2)*mf + c1)*mf + c0;

		float extra = 2.0f - mf;
		float n = m - extra;
		mf = n - (int)n;
		samples_required = (int)n + 2;
		static const float max_out = 32767.0f;
		int s = (int)round(j * max_out);
		
		if (s > 32767)
			s = 32767;
		else if (s < -32768)
			s = -32768;
		output_buf[output_buf_index++] = (short)s;
	}
	input_buf_index = (input_buf_index - 1) & 0x3;
}
