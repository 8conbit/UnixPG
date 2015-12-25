#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>


int encrypt(unsigned char* cipher, unsigned char* plain, unsigned int plainLen, unsigned char* key, unsigned char *iv){
	EVP_CIPHER_CTX ctx;//cipher context = ctx
	int addLen=0, outLen=0;
	unsigned long err=0;

	ERR_load_crypto_strings();
	EVP_CIPHER_CTX_init(&ctx);

	if(EVP_EncryptInit_ex(&ctx, EVP_aes_128_gcm(), NULL, key, iv) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_Encryptinit - %s\n", ERR_error_string(err,NULL));

		return -1;
	}

	if(EVP_EncryptUpdate(&ctx, cipher, &outLen, plain, plainLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_EncryptUpdate - %s\n", ERR_error_string(err, NULL));

		return -1;
	}
	
	if(EVP_EncryptFinal_ex(&ctx, &cipher[outLen], &addLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_EncryptFinal() - %s\n", ERR_error_string(err, NULL));

		return -1;
	}
			
	EVP_CIPHER_CTX_cleanup(&ctx);
	ERR_free_strings();
	printf("addLen = %d, outLen = %d\n", addLen, outLen);

	return addLen + outLen;
}

int decrypt(unsigned char *plain, unsigned char *cipher, unsigned int cipherLen, unsigned char *key, unsigned char *iv){
	EVP_CIPHER_CTX ctx;
	unsigned long err=0;
	int subLen = 0, outLen=0;
	int e = 0;
	
	ERR_load_crypto_strings();
	EVP_CIPHER_CTX_init(&ctx);

	if (EVP_DecryptInit_ex(&ctx, EVP_aes_128_gcm(), NULL, key, iv) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_Decryptinit - %s\n", ERR_error_string(err,NULL));
		return -1;
	}
	if (EVP_DecryptUpdate(&ctx, plain, &outLen, cipher, cipherLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_DecryptUpdate - %s\n", ERR_error_string(err,NULL));
		return -1;
	}
	printf("subLen = %d, outLen = %d\n", subLen, outLen);
	e = EVP_DecryptFinal_ex(&ctx, &plain[outLen], &subLen);
	printf("errcode = %d\n", e);
	if(e!= 1){
		err = ERR_get_error();
		printf("ERR:EVP_DecryptFinal() - %s\n", ERR_error_string(err, NULL));
		return -1;
	}

	EVP_CIPHER_CTX_cleanup(&ctx);
	ERR_free_strings();
	printf("subLen = %d, outLen = %d\n", subLen, outLen);
	
	return subLen+outLen;

}

int main(void){
	int clen;
	char iv[16] = {0}; 
	char *p = "1234safdkjsadfsaflsafjlsajakjljljkjljkjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjlskgdjls;adfjlsadjflsadfjsladkfjlskadjflsadkjflksdajfl56789a123456789a123456123141984712983718924719827491827498172398172398714917249817378871273917192461969218313";
	char *c = (char*)calloc(1024, sizeof(char));
	char *p2 = (char*)calloc(1024, sizeof(char));
	char *key = "123456789112345";
	clen = encrypt(c, p, strlen(p), key, iv);	
	printf("%s\n", c);
	decrypt(p2, c, clen, key, iv);
	printf("%s\n", p2);
	return 0;
}
