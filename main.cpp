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

#include "biquad4.hpp"
#include "resampler.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <cstdio>

#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "sdl2main.lib")
#include "SDL.h"
#include "SDL_audio.h"

static const int AUDIO_LENGTH = 10;
static const int INPUT_RATE = 1000000;
static const int OUTPUT_RATE = 44100;

int audio_len;
int audio_pos = 0;
std::vector<int16_t> output_buf;
int16_t *out_buf;
int out_buf_pos = 0;

void callback(void *udata, Uint8 *stream, int len)
{
	/* Only play if we have data left */
	if (audio_len == 0)
		return;

	/* Mix as much data as possible */
	len = (len > audio_len ? audio_len : len);
	memcpy(stream, reinterpret_cast<Uint8*>(output_buf.data()) + audio_pos, len);
	audio_pos += len;
	audio_len -= len;
}

int main(int argc, char *argv[])
{
	SDL_AudioSpec wav_spec;
	wav_spec.freq = OUTPUT_RATE;
	wav_spec.format = AUDIO_S16;
	wav_spec.channels = 1;
	wav_spec.samples = 4096;
	wav_spec.callback = callback;
	wav_spec.userdata = NULL;

	if (SDL_OpenAudio(&wav_spec, NULL) < 0)
		return 1;

	output_buf.reserve(OUTPUT_RATE*AUDIO_LENGTH);

	/*
	% MATLAB code for generating filter coefficients

	Fs = 1000000;  % Sampling Frequency

	N     = 8;      % Order
	Fpass = 20000;  % Passband Frequency
	Apass = 0.1;    % Passband Ripple (dB)
	Astop = 96;     % Stopband Attenuation (dB)

	% Construct an FDESIGN object and call its ELLIP method.
	h  = fdesign.lowpass('N,Fp,Ap,Ast', N, Fpass, Apass, Astop, Fs);
	Hd = design(h, 'ellip', 'FilterStructure', 'df2tsos');
	set(Hd, 'Arithmetic', 'single');
	*/

	const float g[4] = { 0.361650943756104f,0.222894310951233f,0.073932819068432f,0.003100576112047f };
	const float b2[4] = { -1.941264033317566f,-1.880778431892395f,-1.240429520606995f,-1.955172181129456f };
	const float a2[4] = { -1.948359489440918f,-1.926492333412170f,-1.911195635795593f,-1.971058130264282f };
	const float a3[4] = { 0.961415290832520f,0.933920145034790f,0.913973271846771f,0.987604498863220f };

	c_resampler resampler(INPUT_RATE, OUTPUT_RATE);
	c_biquad4 filter(g, b2, a2, a3);

	auto fill_output_buf = [&]() {
		const int16_t *buf;
		int avail_samples = resampler.get_output_buf(&buf);
		for (int x = 0; x < avail_samples; x++)
			output_buf.push_back(*buf++);
	};

	std::chrono::time_point < std::chrono::high_resolution_clock> start, end;
	start = std::chrono::high_resolution_clock::now();
	int out = 0;
	int i = 0;
	int num_samples = INPUT_RATE * AUDIO_LENGTH;
	do
	{
		resampler.process(filter.process(1.0f * out));
		i++;
		if ((i & 4095) == 0)
		{
			fill_output_buf();
			out ^= 1;
		}
	} while (i <= num_samples);
	fill_output_buf();
	end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed = end - start;
	printf("Processed %i samples in %.4f seconds (%.2f samples/second)\n", num_samples, elapsed.count(), num_samples / elapsed.count());

	audio_len = output_buf.size() * 2;
	SDL_PauseAudio(0);
	while (audio_len > 0)
		SDL_Delay(10);
	SDL_CloseAudio();
	return 0;
}