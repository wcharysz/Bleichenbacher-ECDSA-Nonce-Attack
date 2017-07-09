import random

'''
Generates fake (r,s,H(m)) tuples.
Saves tuples to file called 'rsmTuples-fake'
'''
def ECDSA_sign():
	r = random.getrandbits(160)
	s = random.getrandbits(160)
	m = random.getrandbits(511)
	return (r,s,m) 

def main():

	numTuples = 100000

	rsmTuples = []
	
	for i in range(0,numTuples):
		rsmTuples.append(ECDSA_sign())

	f = open('rsmTuples-fake', 'w')
	for t in rsmTuples:
		f.write(str(t[0]) + ' ') # r
		f.write(str(t[1]) + ' ') # s
		f.write(str(t[2]) + '\n') # H(m)
	f.close()

if __name__ == "__main__":
	main()

