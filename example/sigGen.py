from seccure import *

'''
Generates (r,s,H(m)) tupples for testing by abusing code from py-seccure.
'''

ctr = Crypto.Util.Counter.new(128, initial_value=0)

def ECDSA_sign(priv, md):
	'''
	Returns (r,s,H(m)) tupples made with cooked k values.
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
			buf = buf[:-1] + b'\xFF'
	
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
	numTupples = 100000

	rsmTupples = []

	curve = Curve.by_name('secp160r1')
	privkey = curve.passphrase_to_privkey(key)
	print "[+] Privkey is: " + str(privkey)
	
	for i in range(0,numTupples):
		message = "This is message number " + str(i)
		rsmTupples.append(ECDSA_sign(curve.passphrase_to_privkey(key), hashlib.sha512(message).digest()))

	f = open('rsmTuples', 'w')
	for t in rsmTupples:
		f.write(str(t[0]) + ' ') # r
		f.write(str(t[1]) + ' ') # s
		f.write(str(t[2]) + '\n') # H(m)
	f.close()

if __name__ == "__main__":
	main()

