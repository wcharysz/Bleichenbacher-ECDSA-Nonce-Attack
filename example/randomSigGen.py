from seccure import *
from random import randint

'''
Generates (r,s,H(m)) tuples for testing by abusing code from py-seccure.
Saves tuples to file called 'rsmTuples-[key]-[randomint]'
'''

ctr = Crypto.Util.Counter.new(128, initial_value=randint(0, 2 ** 128))

def ECDSA_sign(priv, md):
	'''
	Returns (r,s,H(m)) tuples made with cooked k values.
	'''
	order = priv.curve.order
	hmk = serialize_number(priv.e, SER_BINARY, priv.curve.order_len_bin)
	h = hmac.new(hmk, digestmod=hashlib.sha256)
	h.update(md)
	cprng = Crypto.Cipher.AES.new(h.digest(), Crypto.Cipher.AES.MODE_CTR, counter=ctr)
	r = 0
	s = 0
	while s == 0:
		while r == 0:

			# Here's where we add the bias
			buf = cprng.encrypt(b'\0' * priv.curve.order_len_bin)
			buf = buf[:-2] + b'\xFF\xFF'
	
			k = priv.curve._buf_to_exponent(buf)
			p1 = priv.curve.base * k
			r = p1.x % order
		e = deserialize_number(md, SER_BINARY)
		e = (e % order)
		s = (priv.e * r) % order
		s = (s + e) % order
		e = gmpy.invert(k, order)
		s = (s * e) % order
	return (r,s,deserialize_number(md, SER_BINARY)) 

def main():

	# You can change this if you want...	
	key = b'SuperSecretDoNotTell'
	numTuples = 100000

	rsmTuples = []

	curve = Curve.by_name('secp160r1')
	privkey = curve.passphrase_to_privkey(key) 
	
	for i in range(0,numTuples):
		message = "This is secret. " + str(randint(0, 2 ** 128))
		rsmTuples.append(ECDSA_sign(privkey, hashlib.sha512(message).digest()))

	f = open('rsmTuples-' + str(privkey) + "-" + str(randint(0, 2 ** 30)), 'w')
	for t in rsmTuples:
		f.write(str(t[0]) + ' ') # r
		f.write(str(t[1]) + ' ') # s
		f.write(str(t[2]) + '\n') # H(m)
	f.close()

if __name__ == "__main__":
	main()

