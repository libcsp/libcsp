

/* Hash-based Message Authentication Code - based on code from libtom.org */

#include <csp/crypto/csp_hmac.h>

#include <string.h>

#include <csp/csp_buffer.h>
#include <csp/crypto/csp_sha1.h>

#define HMAC_KEY_LENGTH 16

/* HMAC key */
static uint8_t csp_hmac_key[HMAC_KEY_LENGTH];

/* HMAC state structure */
typedef struct {
	csp_sha1_state_t md;
	uint8_t key[CSP_SHA1_BLOCKSIZE];
} hmac_state;

static int csp_hmac_init(hmac_state * hmac, const uint8_t * key, uint32_t keylen) {
	uint32_t i;
	uint8_t buf[CSP_SHA1_BLOCKSIZE];

	/* NULL pointer and key check */
	if (!hmac || !key || keylen < 1)
		return CSP_ERR_INVAL;

	/* Make sure we have a large enough key */
	if (keylen > CSP_SHA1_BLOCKSIZE) {
		csp_sha1_memory(key, keylen, hmac->key);
		if (CSP_SHA1_DIGESTSIZE < CSP_SHA1_BLOCKSIZE)
			memset((hmac->key) + CSP_SHA1_DIGESTSIZE, 0, (CSP_SHA1_BLOCKSIZE - CSP_SHA1_DIGESTSIZE));
	} else {
		memcpy(hmac->key, key, keylen);
		if (keylen < CSP_SHA1_BLOCKSIZE)
			memset((hmac->key) + keylen, 0, (CSP_SHA1_BLOCKSIZE - keylen));
	}

	/* Create the initial vector */
	for (i = 0; i < CSP_SHA1_BLOCKSIZE; i++) {
		buf[i] = hmac->key[i] ^ 0x36;
	}

	/* Prepend to the hash data */
	csp_sha1_init(&hmac->md);
	csp_sha1_process(&hmac->md, buf, CSP_SHA1_BLOCKSIZE);

	return CSP_ERR_NONE;
}

static int csp_hmac_process(hmac_state * hmac, const uint8_t * in, uint32_t inlen) {

	/* NULL pointer check */
	if (!hmac || !in)
		return CSP_ERR_INVAL;

	/* Process data */
	csp_sha1_process(&hmac->md, in, inlen);

	return CSP_ERR_NONE;
}

static int csp_hmac_done(hmac_state * hmac, uint8_t * out) {

	if (!hmac || !out)
		return CSP_ERR_INVAL;

	/* Get the hash of the first HMAC vector plus the data */
	uint8_t isha[CSP_SHA1_DIGESTSIZE];
	csp_sha1_done(&hmac->md, isha);

	/* Create the second HMAC vector vector */
	uint8_t buf[CSP_SHA1_BLOCKSIZE];
	for (unsigned int i = 0; i < sizeof(buf); i++) {
		buf[i] = hmac->key[i] ^ 0x5C;
	}

	/* Now calculate the outer hash */
	csp_sha1_init(&hmac->md);
	csp_sha1_process(&hmac->md, buf, sizeof(buf));
	csp_sha1_process(&hmac->md, isha, sizeof(isha));
	csp_sha1_done(&hmac->md, buf);

	/* Copy to output  */
	memcpy(out, buf, CSP_SHA1_DIGESTSIZE);

	return CSP_ERR_NONE;
}

int csp_hmac_memory(const void * key, uint32_t keylen, const void * data, uint32_t datalen, uint8_t * hmac) {
	hmac_state state;

	/* NULL pointer check */
	if (!key || !data || !hmac)
		return CSP_ERR_INVAL;

	/* Init HMAC state */
	if (csp_hmac_init(&state, key, keylen) != 0)
		return CSP_ERR_INVAL;

	/* Process data */
	if (csp_hmac_process(&state, data, datalen) != 0)
		return CSP_ERR_INVAL;

	/* Output HMAC */
	if (csp_hmac_done(&state, hmac) != 0)
		return CSP_ERR_INVAL;

	return CSP_ERR_NONE;
}

int csp_hmac_set_key(const void * key, uint32_t keylen) {

	/* Use SHA1 as KDF */
	uint8_t hash[CSP_SHA1_DIGESTSIZE];
	csp_sha1_memory(key, keylen, hash);

	/* Copy key */
	memcpy(csp_hmac_key, hash, sizeof(csp_hmac_key));

	return CSP_ERR_NONE;
}

int csp_hmac_append(csp_packet_t * packet, bool include_header) {

	if ((packet->length + (unsigned int)CSP_HMAC_LENGTH) > sizeof(packet->data)) {
		return CSP_ERR_NOMEM;
	}

	/* Calculate HMAC */
	uint8_t hmac[CSP_SHA1_DIGESTSIZE];

	if (include_header) {

		/* If header is included, csp_id_prepend() must be called beforehand */
		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length, hmac);
		memcpy(&packet->frame_begin[packet->frame_length], hmac, CSP_HMAC_LENGTH);
		packet->frame_length += CSP_HMAC_LENGTH;
		packet->length += CSP_HMAC_LENGTH;

	} else {

		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->data, packet->length, hmac);
		memcpy(&packet->data[packet->length], hmac, CSP_HMAC_LENGTH);
		packet->length += CSP_HMAC_LENGTH;
	}

	return CSP_ERR_NONE;
}

int csp_hmac_verify(csp_packet_t * packet, bool include_header) {

	if (packet->length < (unsigned int)CSP_HMAC_LENGTH) {
		return CSP_ERR_HMAC;
	}

	uint8_t hmac[CSP_SHA1_DIGESTSIZE];

	/* Calculate HMAC */
	if (include_header) {

		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->frame_begin, packet->frame_length - CSP_HMAC_LENGTH, hmac);

		/* Compare calculated HMAC with packet header */
		if (memcmp(&packet->frame_begin[packet->frame_length] - CSP_HMAC_LENGTH, hmac, CSP_HMAC_LENGTH) != 0) {
			/* HMAC failed */
			return CSP_ERR_HMAC;
		}

		/* Strip HMAC */
		packet->frame_length -= CSP_HMAC_LENGTH;

	} else {
		csp_hmac_memory(csp_hmac_key, sizeof(csp_hmac_key), packet->data, packet->length - CSP_HMAC_LENGTH, hmac);

		/* Compare calculated HMAC with packet header */
		if (memcmp(&packet->data[packet->length] - CSP_HMAC_LENGTH, hmac, CSP_HMAC_LENGTH) != 0) {
			/* HMAC failed */
			return CSP_ERR_HMAC;
		}

		/* Strip HMAC */
		packet->length -= CSP_HMAC_LENGTH;
	}

	return CSP_ERR_NONE;
}
