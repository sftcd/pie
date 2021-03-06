/*
Copyright (c) 2013, Stephen Farrell, stephen@tolerantnetworks.com
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

int errno=-99;

/*!
 * @brief inefficiently read a file into a buffer
 * @param fp is a file pointer (in)
 * @param blen is the buffer length (in/out)
 * @param buf is the buffer 
 * @return zero on success, non-zero for error
 *
 * Given an ni name, open a file, hash it and add the hash to
 * the URI (somewhere, details TBD)
 */
static int fname2buf(FILE *fp,long *blen, unsigned char **buf)
{
	if (!blen || !buf) return(-1);
	int rv=fseek(fp,0,SEEK_END);
	if (rv) {
		fclose(fp);
		return(errno);
	}
	long flen=ftell(fp);
	rv=fseek(fp,0,SEEK_SET);
	if (rv) {
		fclose(fp);
		return(errno);
	}
	unsigned char *lbuf=(unsigned char*)malloc((flen+1)*sizeof(char));
	if (!lbuf) {
		fclose(fp);
		return(-1);
	}
	size_t read=fread(lbuf,flen,1,fp);
	if (read!=1) {
		free(lbuf);
		fclose(fp);
		return(-1);
	}
	fclose(fp);
	*blen=flen;
	*buf=lbuf;
	return(0);
}

/// LEFT Nibble of an octet as an ASCII hex digit
#define LNIB(x) ((((x)/16)>=10)?('a'-10+((x)/16)):('0'+((x)/16)))

/// RIGHT Nibble of an octet as an ASCII hex digit
#define RNIB(x) ((((x)%16)>=10)?('a'-10+((x)%16)):('0'+((x)%16)))

/*!
 * @brief base 16 (ascii hex) encode a value
 * @param ilen is the input length
 * @param ibuf is the input buffer
 * @param olen is the output length (in/out)
 * @param obuf is the output buffer (allocated by caller)
 * @return zero on success, non-zero for error
 *
 */
static int b16_enc(long ilen, const unsigned char *ibuf, long *olen, unsigned char *obuf)
{
	long i;
	if (*olen < 2*ilen) return(-1);
	for (i=0;i!=ilen;i++) {
		obuf[2*i]=LNIB(ibuf[i]);
		obuf[(2*i)+1]=RNIB(ibuf[i]);
	}
	*olen=2*ilen;
	return(0);
}

/*!
 * @brief hash a buffer
 * @param blen is the buffer length
 * @param buf is the buffer 
 * @param olen is the output buffer length (out)
 * @param obuf is the output buffer  (better be >256 bits long)
 * @return zero on success, non-zero for error
 *
 * Given an ni name, hash a buffer and add the hash to
 * the URI (somewhere, details TBD)
 */
int hashbuf(long blen, unsigned char *buf, int *olen, unsigned char *obuf)
{
	long hashlen=SHA256_DIGEST_LENGTH;
	if (*olen < hashlen) return(1);
	*olen=hashlen;
	SHA256_CTX c;
	SHA256_Init(&c);
	SHA256_Update(&c,buf,blen);
	SHA256_Final(obuf,&c);
	return(0);
}


main(int argc, char *argv[])
{
	if (argc != 3) { printf("Usage: ./pie N <file>\n"); exit(-1);}

	// not safe but good enough
	int N=atoi(argv[1]);
	char *fname=argv[2];

	if (N<=0 || N>128) { printf("Negative or >128 values for N are stupid. You said %d\n",N);exit(-2);}

	FILE *fp=fopen(fname,"rb");
	if (fp==NULL) { printf("Can't open file: %s\n",fname); exit(-3);}
	long blen;
	unsigned char *buf;
	int rv=fname2buf(fp,&blen,&buf);
	if (rv!=0) { printf("Can't read file: %s\n",fname); exit(-4);}

	int hlen=32;
	unsigned char hbuf[32];

	rv=hashbuf(blen,buf,&hlen,hbuf);
	if (rv!=0) { printf("Can't hash file: %s\n",fname); exit(-5);}

	// now pick N randoms less than 8*hlen
	unsigned char rndarr[N];
	rv=RAND_bytes(rndarr,N);
	if (rv!=1) { printf("Can't generate randoms.\n"); exit(-6);}


	// get bits
	int bal=N/8+((N%8?1:0));
	unsigned char bitsarr[bal];
	memset(bitsarr,0,bal);
	for (int i=0;i!=bal-1;i++) {
		for (int j=0;j!=8;j++) {
			int rndbit=rndarr[8*i+j];
			unsigned char rndch=hbuf[rndbit/8];
			unsigned char thebit=rndch&(8>>(rndbit%8));
			if (thebit) {
				bitsarr[i] |= (0x80>>j);
			}
		}
	}
	// one more inner for the last few bits
	int bitsleft=(N%8?(N%8):8);
		for (int j=0;j!=bitsleft;j++) {
			int rndbit=rndarr[8*(bal-1)+j];
			unsigned char rndch=hbuf[rndbit/8];
			unsigned char thebit=rndch&(8>>(rndbit%8));
			if (thebit) {
				bitsarr[(bal-1)] |= (0x80>>j);
			}
		}


	printf("Debug stuff\n");

	printf("Hash(PIE-crust)\n");
	for (int i=0;i!=hlen;i++) {
		if (i && !(i%4)) { printf(" ");}
		printf("%02x",hbuf[i]);
	}
	printf("\n");

	printf("Hash(PIE-crust, no spaces)\n");
	for (int i=0;i!=hlen;i++) {
		printf("%02x",hbuf[i]);
	}
	printf("\n");

	// binary
	printf("N=%d",N);
	printf("\nrands(hex):");
	for (int i=0;i!=N;i++){
		printf("%02x",rndarr[i]);
	}
	printf("\nrands(decimal):");
	for (int i=0;i!=N;i++){
		printf("%d,",rndarr[i]);
	}
	printf("\nrandom bits:");
	for (int i=0;i!=bal;i++) {
		printf("%02x",bitsarr[i]);
	}
	printf("\n");

	// ASCII
	printf("ASCII version\n");
	long ahr_len=2*N;
	unsigned char ah_rnds[ahr_len+1];
	memset(ah_rnds,0,ahr_len+1);
	rv=b16_enc(N,rndarr,&ahr_len,ah_rnds);
	if (rv!=0) { printf("Can't AH encode file: %s\n",fname); exit(-7);}
	long ahb_len=2*bal;
	unsigned char ah_bits[ahb_len+1];
	memset(ah_bits,0,ahb_len+1);
	rv=b16_enc(bal,bitsarr,&ahb_len,ah_bits);
	if (rv!=0) { printf("Can't AH encode file: %s\n",fname); exit(-7);}
	printf(":%d:%s:%s\n",N,ah_rnds,ah_bits);

	// Binary
	printf("Binary version\n");
	printf("%02x",N);
	for (int i=0;i!=N;i++){
		printf("%02x",rndarr[i]);
	}
	for (int i=0;i!=bal;i++) {
		printf("%02x",bitsarr[i]);
	}
	printf("\n");


	return(0);
}

