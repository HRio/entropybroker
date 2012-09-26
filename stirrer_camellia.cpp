// SVN: $Id$
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <openssl/camellia.h>

#include "error.h"
#include "random_source.h"
#include "utils.h"
#include "stirrer.h"
#include "stirrer_camellia.h"

stirrer_camellia::stirrer_camellia(random_source_t rs_in) : rs(rs_in)
{
}

stirrer_camellia::~stirrer_camellia()
{
}

int stirrer_camellia::get_stir_size() const
{
	return 256 / 8;
}

int stirrer_camellia::get_ivec_size() const
{
	return CAMELLIA_BLOCK_SIZE;
}

void stirrer_camellia::do_stir(unsigned char *ivec, unsigned char *target, int target_size, unsigned char *data_in, int data_in_size, unsigned char *temp_buffer, bool direction)
{
	unsigned char temp_key[32];

	if (data_in_size > get_stir_size())
		error_exit("Invalid stir-size %d (expected: %d)", data_in_size, get_stir_size());

	memcpy(temp_key, data_in, data_in_size);

	// these generated bytes are not counted in the entropy
	// estimation
	if (data_in_size > 24 && data_in_size < 32)
	{
		get_random(rs, &temp_key[data_in_size], 32 - data_in_size);

		data_in_size = 32;
	}
	else if (data_in_size > 16 && data_in_size < 24)
	{
		get_random(rs, &temp_key[data_in_size], 24 - data_in_size);

		data_in_size = 24;
	}
	else if (data_in_size < 16)
	{
		get_random(rs, &temp_key[data_in_size], 16 - data_in_size);

		data_in_size = 16;
	}

	CAMELLIA_KEY key;
	Camellia_set_key(temp_key, data_in_size * 8, &key);

	int ivec_offset = 0;
	Camellia_cfb8_encrypt(target, temp_buffer, target_size, &key, ivec, &ivec_offset, direction ? CAMELLIA_ENCRYPT : CAMELLIA_DECRYPT);
	memcpy(target, temp_buffer, target_size);
}
