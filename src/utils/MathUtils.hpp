#ifndef _MATH_UTILS
#define _MATH_UTILS

#include <vector>

#define MAX(a,b)((a>b?a:b))
#define MIN(a,b)((a<b?a:b))


inline static float map(float x, float sa, float sb, float ea, float eb)
{
	return (x - sa) / (sb - sa) * (eb - ea) + ea;
}

template <typename T>
inline static T modTrue(const T& a, const T& b)
{
	return ((a % b) + b) % b;
}

struct SampleInterpolation
{
	struct Buffer
	{
		double bfr[4] = {0, 0, 0, 0};

		void update(double x)
		{
			bfr[3] = bfr[2];
			bfr[2] = bfr[1];
			bfr[1] = bfr[0];
			bfr[0] = x;
		}

		double get(int i)
		{
			return bfr[i + 1];
		}
	};

	static float none(const std::vector<float>& samples, float index) {
		return samples[(size_t)index % samples.size()];
	}

	// Linear interpolation
	static float linear(const std::vector<float>& samples, float index) {
		size_t i0 = modTrue<int>((int)std::floor(index), (int)samples.size());
		size_t i1 = (i0 + 1) % samples.size();

		float t = index - floor(index);
		return samples[i0] * (1.0f - t) + samples[i1] * t;
	}

	static float optimal2X(const std::vector<float>& samples, float index)
	{
		float smpls;
		smpls = static_cast<float>(samples.size());
		index = fmod(index, smpls) + samples.size();
		index = fmod(index, smpls);

		int size = static_cast<int>(samples.size());
		int i0 = static_cast<int>(index) % size;
		int i1 = (i0 + 1) % size;

		float y0 = samples[i0];
		float y1 = samples[i1];

		// Taken directly from https://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf
		// Optimal 2x (2-point, 3rd-order) (z-form)
		float z = 1.0 - 1/2.0;
		float even1 = y1+y0, odd1 = y1-y0;
		float c0 = even1*0.50037842517188658;
		float c1 = odd1*1.00621089801788210;
		float c2 = even1*-0.004541102062639801;
		float c3 = odd1*-1.57015627178718420;
		return ((c3*z+c2)*z+c1)*z+c0;
	}

	static float optimal8X(const std::vector<float>& samples, float index)
	{
		double smpls;
		smpls = static_cast<float>(samples.size());
		index = fmod(index, smpls) + samples.size();
		index = fmod(index, smpls);

		int size = static_cast<int>(samples.size());
		int i0 = static_cast<int>(index) % size;
		int i1 = (i0 + 1) % size;
		int i2 = (i0 + 2) % size;
		int i3 = (i0 - 1 + size) % size;  // Wrap backward

		float y0 = samples[i3];
		float y1 = samples[i0];
		float y2 = samples[i1];
		float y3 = samples[i2];
		
		// Taken directly from https://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf
		// Optimal 8x (4-point, 2nd-order) (z-form)
		float z = 1.0 - 1/2.0;
		float even1 = y2+y1, odd1 = y2-y1;
		float even2 = y3+y0, odd2 = y3-y0;
		float c0 = even1*0.32852206663814043 + even2*0.17147870380790242;
		float c1 = odd1*-0.35252373075274990 + odd2*0.45113687946292658;
		float c2 = even1*-0.240052062078895181 + even2*0.24004281672637814;
		return (c2*z+c1)*z+c0;
	}

	static float optimal32X(const std::vector<float>& samples, float index)
	{
		double smpls;
		smpls = static_cast<float>(samples.size());
		index = fmod(index, smpls) + samples.size();
		index = fmod(index, smpls);

		int size = static_cast<int>(samples.size());
		int i0 = static_cast<int>(index) % size;
		int i1 = (i0 + 1) % size;
		int i2 = (i0 + 2) % size;
		int i3 = (i0 + 3) % size;
		int im1 = (i0 - 1 + size) % size;
		int im2 = (i0 - 1 + size) % size;

		float ym2 = samples[im2];
		float ym1 = samples[im1];
		float y0 = samples[i0];
		float y1 = samples[i1];
		float y2 = samples[i2];
		float y3 = samples[i3];

		// Taken directly from https://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf
		// Optimal 32x (6-point, 5th-order) (z-form)
		float z = 1.0 - 1/2.0;
		float even1 = y1+y0, odd1 = y1-y0;
		float even2 = y2+ym1, odd2 = y2-ym1;
		float even3 = y3+ym2, odd3 = y3-ym2;
		float c0 = even1*0.42685983409379380 + even2*0.07238123511170030
		+ even3*0.00075893079450573;
		float c1 = odd1*0.35831772348893259 + odd2*0.20451644554758297
		+ odd3*0.00562658797241955;
		float c2 = even1*-0.217009177221292431 + even2*0.20051376594086157
		+ even3*0.01649541128040211;
		float c3 = odd1*-0.25112715343740988 + odd2*0.04223025992200458
		+ odd3*0.02488727472995134;
		float c4 = even1*0.04166946673533273 + even2*-0.06250420114356986
		+ even3*0.02083473440841799;
		float c5 = odd1*0.08349799235675044 + odd2*-0.04174912841630993
		+ odd3*0.00834987866042734;
		return ((((c5*z+c4)*z+c3)*z+c2)*z+c1)*z+c0;
	}

	static float cubic(const std::vector<float>& samples, float index) {
		int size = static_cast<int>(samples.size());

		int i0 = static_cast<int>(index);
		int i1 = (i0 + 1) % size;
		int i2 = (i0 + 2) % size;
		int i3 = (i0 - 1 + size) % size;  // Wrap backward

		float t = index - i0;  // Fractional part of the index

		// Cubic Hermite spline
		float a0 = samples[i3];
		float a1 = samples[i0];
		float a2 = samples[i1];
		float a3 = samples[i2];

		float t2 = t * t;
		float t3 = t2 * t;

		return a1 + 0.5f * t * (a2 - a0) +
			0.5f * t2 * (2.0f * a0 - 5.0f * a1 + 4.0f * a2 - a3) +
			0.5f * t3 * (-a0 + 3.0f * a1 - 3.0f * a2 + a3);
	}
};

class LowPassFilter {
public:
	LowPassFilter()
	{

	}

    LowPassFilter(float sampleRate, float cutoffFreq) {
        setCoefficients(sampleRate, cutoffFreq);
    }

    float process(float sample) {
        // Direct Form 1 Biquad implementation
        float result = b0 * sample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        // Shift history
        x2 = x1;
        x1 = sample;
        y2 = y1;
        y1 = result;

        return result;
    }

    void setCoefficients(float sampleRate, float cutoffFreq) {
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = sin(omega) / (2.0f);
        
        float cosW = cos(omega);
        float norm = 1.0f / (1.0f + alpha);

        b0 = (1.0f - cosW) / 2.0f * norm;
        b1 = (1.0f - cosW) * norm;
        b2 = b0;
        a1 = -2.0f * cosW * norm;
        a2 = (1.0f - alpha) * norm;

        // Reset history
        x1 = x2 = y1 = y2 = 0.0f;
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
};


class HPFilter {
public:
    HPFilter()
        : sampleRate(0.0f), cutoffHz(0.0f), a(0.0f), prevX(0.0f), prevY(0.0f) {}

    HPFilter(float sampleRate, float cutoffHz)
        : sampleRate(0.0f), cutoffHz(0.0f), a(0.0f), prevX(0.0f), prevY(0.0f) {
        setCutoff(sampleRate, cutoffHz);
    }

    void setCutoff(float sampleRate, float cutoffHz) {
        this->sampleRate = sampleRate;
        this->cutoffHz = cutoffHz;

        if (sampleRate <= 0.0f || cutoffHz <= 0.0f) {
            a = 0.0f;
            return;
        }

        const float dt = 1.0f / sampleRate;
        const float RC = 1.0f / (2.0f * M_PI * cutoffHz);
        a = RC / (RC + dt);

        prevX = 0.0f;
        prevY = 0.0f;
    }

    float process(float x0) {
        // One-pole high-pass: y[n] = a * (y[n-1] + x[n] - x[n-1])
        float y0 = a * (prevY + x0 - prevX);
        prevX = x0;
        prevY = y0;
        return y0;
    }

    float getSampleRate() const {
        return sampleRate;
    }

private:
    float sampleRate;
    float cutoffHz;
    float a;
    float prevX;
    float prevY;
};

#endif // _MATH_UTILS