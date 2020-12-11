# resampler
A continously variable audio resampler.  Also, an SSE optimized IIR filter.

## c_biquad4
This is an SSE-optimized implementation of an 8th order IIR filter.  4 biquad filters (transposed direct form II) are processed in parallel.

Filter coefficents are generated with Matlab:
```
/*
lowpass elliptical, 20kHz
d = fdesign.lowpass('N,Fp,Ap,Ast', 8, 20000, .1, 80, 1786840);
Hd = design(d, 'ellip', 'FilterStructure', 'df2tsos');
set(Hd, 'Arithmetic', 'single');
g = regexprep(num2str(reshape(Hd.ScaleValues(1:4), [1 4]), '%.16ff '), '\s+', ',')
b2 = regexprep(num2str(Hd.sosMatrix(5:8), '%.16ff '), '\s+', ',')
a2 = regexprep(num2str(Hd.sosMatrix(17:20), '%.16ff '), '\s+', ',')
a3 = regexprep(num2str(Hd.sosMatrix(21:24), '%.16ff '), '\s+', ',')
*/
lpf = new c_biquad4(
	{ 0.5086284279823303f,0.3313708603382111f,0.1059221103787422f,0.0055782101117074f },
	{ -1.9872593879699707f,-1.9750031232833862f,-1.8231037855148315f,-1.9900115728378296f },
	{ -1.9759204387664795f,-1.9602127075195313f,-1.9470522403717041f,-1.9888486862182617f },
	{ 0.9801648259162903f,0.9627774357795715f,0.9480593800544739f,0.9940192103385925f }
);
```
