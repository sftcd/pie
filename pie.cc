/*
 * Copyright (c) Stephen Farrell 2013, all rights reserved
 * TODO: Add BSD license
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
 * @param obuf is the output buffer (allocated by calle)
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

	for (int i=0;i!=hlen;i++) {
		if (i && !(i%4)) { printf(" ");}
		printf("%02x",hbuf[i]);
	}
	printf("\n");

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
		for (int j=0;j!=N%8;j++) {
			int rndbit=rndarr[8*(bal-1)+j];
			unsigned char rndch=hbuf[rndbit/8];
			unsigned char thebit=rndch&(8>>(rndbit%8));
			if (thebit) {
				bitsarr[(bal-1)] |= (0x80>>j);
			}
		}
	
	// binary
	printf("%d",N);
	printf("\n");
	for (int i=0;i!=N;i++){
		printf("%02x",rndarr[i]);
	}
	printf("\n");
	for (int i=0;i!=N;i++){
		printf("%d,",rndarr[i]);
	}
	printf("\n");
	for (int i=0;i!=bal;i++) {
		printf("%02x",bitsarr[i]);
	}
	printf("\n");
	return(0);
}

