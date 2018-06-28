#include <NTL/ZZ_p.h>
#include <NTL/xdouble.h>
#include <sstream>
#include <vector>
#include <tuple>
#include <complex>
//#include <parallel/algorithm>
#include <fftw3.h>
#include "bleichenbacher.h"
#include <dispatch/dispatch.h>
#include <boost/compute.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/sort/sort.hpp>

using namespace std;
using namespace NTL;

xdouble average, stdDev_accum;

bool compareHCtuple(tuple<ZZ_p, ZZ_p> a,
                    tuple<ZZ_p, ZZ_p> b)
{
    ZZ a_ZZ, b_ZZ;
    
    a_ZZ = rep(get<1>(a));
    b_ZZ = rep(get<1>(b));
    
    return a_ZZ < b_ZZ;
}


template<class To, class From>
To NTLtoOther(From *x)
{
	stringstream ss;
	ss << *x;
	return atof(ss.str().c_str());
}

double Z_exp(ZZ_p h);
vector<tuple<int, double>> internal_maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int Z_index_divisor,
	int l);
void internal_hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int numBits,
	ZZ knownBits,
	bool specialH);

void hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int numBits,
	ZZ knownBits)
{
	internal_hcFromRs(rsmTuples,
		hcPairs,
		numBits,
		knownBits,
		true);
}

void hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs)
{
	ZZ zero;
	zero = 0;
	internal_hcFromRs(rsmTuples,
		hcPairs,
		0,
		zero,
		false);
}

void internal_hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int numBits,
	ZZ knownBits,
	bool specialH)
{
	for(vector<tuple<ZZ_p, ZZ_p, ZZ_p>>::iterator it = rsmTuples->begin(); 
		it != rsmTuples->end(); ++it)
	{
		ZZ_p h, c, r, s, m, sInverse;

		r = get<0>(*it);
		s = get<1>(*it);
		m = get<2>(*it);

		if(s == 0) continue;

		power(sInverse, s, -1);

		c = r * sInverse;

		if(specialH)
		{
			ZZ modulus, keyBits;

			/* Place known key bits in MSBs of modulus */
			keyBits = knownBits;
			keyBits >>= (NumBits(keyBits) - numBits); 
			keyBits <<= (NumBits(r.modulus()) - numBits);		

			h = m * sInverse + c * to_ZZ_p(keyBits);

		} else {
			h = m * sInverse;
		}

		hcPairs->push_back(make_tuple(h, c));
	}
}

void sortAndDiff(vector<tuple<ZZ_p, ZZ_p>> *hcPairs, 
	int l,
	int t)
{	
	ZZ comp; 
	ZZ_p zero;
	unsigned long S;

	S = hcPairs->size();
	//omp_set_nested(1);
	//omp_set_num_threads(NUM_CPUs * THREADS_PER_CPU);
    namespace compute = boost::compute;

    dispatch_queue_t c_queue = dispatch_queue_create("myConcurrentQueue",
                                                     DISPATCH_QUEUE_CONCURRENT);
    
    
    dispatch_apply(t, c_queue, ^(size_t i) {        
        //std::sort(hcPairs->begin(), hcPairs->end(), compareHCtuple);
        //__gnu_parallel::sort(hcPairs->begin(), hcPairs->end(),
        //    compareHCtuple);
        boost::sort::block_indirect_sort(hcPairs->begin(), hcPairs->end(), compareHCtuple
                                         );
        
        for(int j = 0; j <= S-t; j++)
        {
            ZZ_p new_h, new_c;
            
            sub(new_h, get<0>((*hcPairs)[j+1]),
                get<0>((*hcPairs)[j]));
            sub(new_c, get<1>((*hcPairs)[j+1]),
                get<1>((*hcPairs)[j]));
            
            hcPairs->at(j) = make_tuple(new_h, new_c);
        }
    });

	/* Remove (h,c) pairs with c < 2^l */
	
	comp = 2;
	zero = 0;
	power(comp, comp, l);

	for(int i = 0; i < S; i++)
	{
		if(rep(get<1>((*hcPairs)[i])) >= comp)		
			hcPairs->at(i) = make_tuple(zero, zero);
	}

    //sort(hcPairs->begin(), hcPairs->end(), compareHCtuple);
	//__gnu_parallel::sort(hcPairs->begin(), hcPairs->end(),
	//		compareHCtuple);
    
    
    // get the default compute device
    //compute::device gpu = compute::system::default_device();
    
    // create a compute context and command queue
    //compute::context ctx(gpu);
    //compute::command_queue queue(ctx, gpu);
    
    //compute::vector<tuple<ZZ_p, ZZ_p>> device_vector(S, ctx);
    //compute::copy(hcPairs->begin(), hcPairs->end(), device_vector.begin(), queue);
    //compute::sort(device_vector.begin(), device_vector.end(), compareHCtuple, queue);
    //compute::copy(device_vector.begin(), device_vector.end(), hcPairs->begin(), queue);
    
    boost::sort::block_indirect_sort(hcPairs->begin(), hcPairs->end(), compareHCtuple
                                     );
	
	if(rep(get<1>((*hcPairs)[S-1])) == rep(zero))
	{
		hcPairs->clear();
		return;
	}

	for(int i = 0; i < S; i++)
	{
		if(rep(get<1>((*hcPairs)[i])) != rep(zero)) 
		{
			hcPairs->erase(hcPairs->begin(), 
				(hcPairs->begin()) + i);
			break;
		}
	}
}

vector<tuple<int, double>> maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int l)
{
	return internal_maxM(hcPairs, 1, l);
}

vector<tuple<int, double>> maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int bits,
	int l)
{
	return internal_maxM(hcPairs, bits, l);
}

vector<tuple<int, double>> internal_maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int Z_index_divisor,
	int l)
{
	int size;
	fftw_complex *in, *out;
    	fftw_plan p;
	vector<tuple<int, double>> indexValuePairs;

	size = pow(2,l)/Z_index_divisor;

	in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
    	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * size);
	p = fftw_plan_dft_1d(size, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

	/* Compute and store Z values using Euler's Formula */
	for(vector<tuple<ZZ_p, ZZ_p>>::iterator it = hcPairs->begin(); 
		it != hcPairs->end(); ++it)
	{
		int c;
		double z_exp;
		ZZ zz_c;
		
		zz_c = rep(get<1>(*it));
		c = NTLtoOther<int, ZZ>(&zz_c);
		z_exp = Z_exp(get<0>(*it));	

		in[c/Z_index_divisor][0] += cos(z_exp); // Re
		in[c/Z_index_divisor][1] += sin(z_exp); // Im
	}

	/* Do FFT */
	fftw_execute(p);

	/* Get max |Z_m| */
	average = 0;
	for(int i = 0; i < size; i++)
	{
		double magnitude;

		magnitude = abs(complex<double>((out[i])[0], (out[i])[1]));
		average += magnitude;		

		if(indexValuePairs.size() < 10)
		{
			indexValuePairs.push_back(make_tuple(i, magnitude));
		} else {
			for(int j = 0; j < 10; j++)
			{
				if(magnitude > get<1>(indexValuePairs[j]))
				{
					indexValuePairs.at(j) = 
						make_tuple(i, magnitude);
					break;
				}
			}
		}
	}
	average /= size;

	/* Multiply the 1/L back in */
	average /= hcPairs->size();
	for(int i = 0; i < 10; i++)
	{
		int index;
		double B;

		index = get<0>(indexValuePairs[i]);
		B = get<1>(indexValuePairs[i]) / hcPairs->size();

		indexValuePairs.at(i) = make_tuple(index, B);
	}

	/* Compute square of standard deviation */
	stdDev_accum = 0;	
	for(int i = 0; i < size; i++)
	{
		xdouble term;
		term = abs(complex<double>((out[i])[0], (out[i])[1])) 
			/ hcPairs->size();
		term -= average;
		power(term, term, 2);
		stdDev_accum += term;
	}
	stdDev_accum /= size;

	fftw_destroy_plan(p);
	fftw_free(in); 
	fftw_free(out);
	
	return indexValuePairs;
}

double avgBias()
{
	double ret;
	conv(ret, average);
	return ret;
}

double stdDevBias()
{
	double ret;
	conv(ret, stdDev_accum);
	ret = sqrt(ret);
	return ret;
}

double Z_exp(ZZ_p h)
{
	ZZ n;
	xdouble h_doub, n_doub, x, pi2;

	pi2 = 2 * M_PI;

	n = h.modulus();
	n_doub = to_xdouble(n);
	h_doub = to_xdouble(rep(h));
	x = pi2 * h_doub / n_doub;

	return NTLtoOther<double, xdouble>(&x);
}
