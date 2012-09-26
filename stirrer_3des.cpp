// SVN: $Id$
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <openssl/des.h>

#include "error.h"
#include "random_source.h"
#include "log.h"
#include "utils.h"
#include "stirrer.h"
#include "stirrer_3des.h"

stirrer_3des::stirrer_3des(random_source_t rs_in) : rs(rs_in)
{
}

stirrer_3des::~stirrer_3des()
{
}

int stirrer_3des::get_stir_size() const
{
	return 24; // it is in reality somewhat less (21 bytes) but for ease of use we just throw away those 24 bits
}

int stirrer_3des::get_ivec_size() const
{
	return 8;
}

void stirrer_3des::set_3des_key(DES_key_schedule *ks, unsigned char key_in[8])
{
	DES_cblock dk;

	memcpy(dk, key_in, 8);
	DES_set_odd_parity(&dk);

	int attempts = 0;
	while(attempts < SET_KEY_ATTEMPTS && DES_is_weak_key(&dk) == 1)
	{
		unsigned char buffer[8];

		get_random(rs, buffer, sizeof buffer);

		for(unsigned int index=0; index<8; index++)
			key_in[index] ^= buffer[index];

		memcpy(dk, key_in, 8);
		DES_set_odd_parity(&dk);

		attempts++;
	}

	if (attempts == SET_KEY_ATTEMPTS)
		dolog(LOG_WARNING, "3DES stirrer: even after %d attempts, the key stays weak", attempts);

	// it is possible that the key still is weak
	// (theoretically)
	DES_set_key_unchecked(&dk, ks);
}

void stirrer_3des::do_stir(unsigned char *ivec, unsigned char *target, int target_size, unsigned char *data_in, int data_in_size, unsigned char *temp_buffer, bool direction)
{
	unsigned char temp_key[32];

	if (data_in_size > get_stir_size())
		error_exit("Invalid stir-size %d (expected: %d)", data_in_size, get_stir_size());

	memcpy(temp_key, data_in, data_in_size);

	// these generated bytes are not counted in the entropy
	// estimation
	if (data_in_size < 24)
		get_random(rs, &temp_key[data_in_size], 24 - data_in_size);

	DES_key_schedule ks1, ks2, ks3;
	set_3des_key(&ks1, &temp_key[ 0]);
	set_3des_key(&ks2, &temp_key[ 8]);
	set_3des_key(&ks3, &temp_key[16]);

	// retrieve bit7 of each key-byte and mix it into
	// the ivec as it is not used by the DES key
	int io = 0, bits = 0;
	unsigned char byte = 0;
	for(int index=0; index<24; index++)
	{
		byte <<= 1;

		if (temp_key[index] & 128)
			byte |= 1;

		bits++;

		if (bits == 8)
		{
			ivec[io++] ^= byte;

			bits = 0;
		}
	}

	DES_cblock iv;
	memcpy(iv, ivec, 8);

	int ivec_offset = 0;
	DES_ede3_cfb64_encrypt(target, temp_buffer, target_size, &ks1, &ks2, &ks3, &iv, &ivec_offset, direction ? DES_ENCRYPT : DES_DECRYPT);
	memcpy(target, temp_buffer, target_size);
}
