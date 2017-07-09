#include "../bleichenbacher.h"
#include <NTL/ZZ_p.h>
#include <NTL/ZZ.h>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace NTL;

void loadSigs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples);
ZZ MSBguessFromM(int m, int l, ZZ mod);

int main(int argc, char *argv[])
{
	vector<tuple<ZZ_p, ZZ_p, ZZ_p>> rsmTuples;
	vector<tuple<ZZ_p, ZZ_p>> hcPairs;
	vector<tuple<int, double>> mValues;
	ZZ guess;

	/* Initialize NTL modulus for secp160r1.
	   n = FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF 7FFFFFFF
	   Citation: http://www.secg.org/SEC2-Ver-1.0.pdf */
	ZZ mod;
	mod = to_ZZ("1461501637330902918203684832716283019653785059327");
	ZZ_p::init(mod);

	/* Load the (r,s,H(m)) tupples */
	cout << "[+] Loading (r,s,H(m)) tupples...\n";
	loadSigs(&rsmTuples);

	/*
	 *	Round 1
	 */

	/* Make (h,c) pairs */
	cout << "[+] Making (h,c) pairs for round 1...\n";
	hcFromRs(&rsmTuples, &hcPairs);

	/* Sort & Diff - we must get |c_i| <= 30 bits */
	cout << "[+] Starting sort & diff for round 1...\n";
	sortAndDiff(&hcPairs, 27, 20);
	cout << "\t" << hcPairs.size() << " pairs left.\n";

	/* Get max m value */
	cout << "[+] Finding top ten bias:MSB guesses for round 1...\n";
	mValues = maxM(&hcPairs, 27);
	
	for(int i = 0; i < 10; i++)
	{
		guess = MSBguessFromM(get<0>(mValues[i]), 27, mod);
		cout << "\t" << get<1>(mValues[i]) << " : " << guess << "\n";
	}

	cout << "[+] Average bias for round 1: " << avgBias() << "\n";
	cout << "[+] Standard deviation of round 1 bias: " << stdDevBias() 
		<< "\n";
	
	hcPairs.clear();


	/*
	 *	Round 2
	 *
	 *	All rounds after round 1 should look like this.
	 */

	/* In your implementation get the real 20 MSBs from round 1 
	results... */

	cout << "[+] Reinjecting correct 20 MSBs for demonstration... :P" 
		<< "\n";

	// >>> bin(991662256230238939367140194553270109876310963800)[:22]
	// '0b10101101101100111010'
	// >>> int('10101101101100111010', 2)
	// 711482

	ZZ knownBits;
	knownBits = 711482;

	/* Make (h,c) pairs */
	cout << "[+] Making (h,c) pairs for round 2...\n";
	hcFromRs(&rsmTuples, &hcPairs, 20, knownBits);

	/* Sort & Diff - we must get |c_i| <= 30 bits */
	cout << "[+] Starting sort & diff for round 2...\n";
	sortAndDiff(&hcPairs, 27, 20);
	cout << "\t" << hcPairs.size() << " pairs left.\n";
	
	/* Get max m value */
	cout << "[+] Finding top ten bias:MSB guesses for round 2...\n";
	mValues = maxM(&hcPairs, 20, 27);

	for(int i = 0; i < 10; i++)
	{
		guess = MSBguessFromM(get<0>(mValues[i]), 27, mod);
		cout << "\t" << get<1>(mValues[i]) << " : " << guess << "\n";
	}

	cout << "[+] Average bias for round 2: " << avgBias() << "\n";
	cout << "[+] Standard deviation of round 2 bias: " << stdDevBias() 
		<< "\n";
	
	hcPairs.clear();
}

ZZ MSBguessFromM(int m, int l, ZZ mod)
{
	ZZ zz_m, guess;

	zz_m = m;
	mul(guess, m, mod);

	guess >>= l;
	
	return guess;
}

void loadSigs(vector<tuple<ZZ_p, ZZ_p, ZZ_p>> *rsmTuples)
{
	ifstream in("rsmTuples");
	string line;

	while(getline(in, line))
	{
		string r, s, m;
		ZZ zz_r, zz_s, zz_m;
		stringstream lineStream(line);

		lineStream >> r;
		lineStream >> s;
		lineStream >> m;

		conv(zz_r, r.c_str());
		conv(zz_s, s.c_str());
		conv(zz_m, m.c_str());

		rsmTuples->push_back(
			make_tuple(
				to_ZZ_p(zz_r),
				to_ZZ_p(zz_s),
				to_ZZ_p(zz_m)
			)
		);
	}
}
