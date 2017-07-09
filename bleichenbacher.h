#ifndef BLEICHENBACHER_H
#define BLEICHENBACHER_H

#include <NTL/ZZ_p.h>
#include <vector>

using namespace std;
using namespace NTL;

/* Configure these! */
#define NUM_CPUs 8
#define THREADS_PER_CPU 2


/*
 *	Note: It is the user's job to initialize the NTL modulus before using
 *	these functions. This can be done like this:
 *
 *		ZZ mod;
 *		mod = to_ZZ("123");
 *		ZZ_p::init(mod);
 *
 *	Set the modulus to the prime used by your curve.
 */


/**
	Implements preprocessing stage of attack.

	rsmTuples - 	Elements of the form (r,s,m) where r and s are 
			signature pairs and m is the hash of the corresponding
			message. 

	hcPairs - 	Vector to store resulting (h,c) pairs in.	
*/
void hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs);

/**
	Implements preprocessing stage of attack when some MSBs already known.

	rsmTuples - 	Elements of the form (r,s,m) where r and s are 
			signature pairs and m is the hash of the corresponding
			message. 

	hcPairs - 	Vector to store resulting (h,c) pairs in.

	numBits -	Number of MSBs of knownBits to feed back into 
			algorithm.

	knownBits -	Contains known MSBs of key.
*/
void hcFromRs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples, 
	vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int numBits,
	ZZ knownBits);

/**
	Implements Sort-And-Difference Algorithm.

	CAUTION: 2^l must fit inside of an int.

	hcPairs - 	Previously generated (h,c) pairs.

	l -		c_j < 2^l

	t -		Number of times to execute sort and difference loop.
*/
void sortAndDiff(vector<tuple<ZZ_p, ZZ_p>> *hcPairs, 
	int l,
	int t);

/**
	Does bias computation and returns top ten m values that maximize |Z_m|
	along with their biasses. Assumes that hcPairs has already been through 
	sortAndDiff. Use for first round of attack.

	CAUTION: 2^l must fit inside of an int.

	hcPairs - 	Previously generated (h,c) pairs.

	l -		c_j < 2^l
*/
vector<tuple<int, double>> maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int l);

/**
	Does bias computation and returns top ten m values that minimize |Z_m| 
	along with their biasses when given number of most significant bits are
	already known. Assumes that hcPairs has already been through 
	sortAndDiff and hcPairs created with hcFromRs for known bits. Use for
	all rounds of attack that are not the first.

	CAUTION: 2^l must fit inside of an int.

	hcPairs - 	Previously generated (h,c) pairs.

	bits -		Number of derived MSBs fed back into the algorithm.

	l -		c_j < 2^l
*/
vector<tuple<int, double>> maxM(vector<tuple<ZZ_p, ZZ_p>> *hcPairs,
	int bits,
	int l);

/**
	Returns average sample bias of round most recently executed.
*/
double avgBias();

/**
	Returns standard deviation of sample bias of round most recently 
	executed.
*/
double stdDevBias();

#endif
