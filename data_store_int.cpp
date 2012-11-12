#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "error.h"
#include "data_store_int.h"

data_store_int::data_store_int(int n_samples_in, int interval_in) : n_samples(n_samples_in), interval(interval_in), prev_index(-1)
{
	values = (long long int *)malloc(n_samples * sizeof(long long int));
	counts = (int *)malloc(n_samples * sizeof(int));
	valid = (bool *)malloc(n_samples * sizeof(bool));

	cur_t = 0;
}

data_store_int::data_store_int(std::string file) : prev_index(-1)
{
	FILE *fh = fopen(file.c_str(), "r");
	if (!fh)
		error_exit("failed to open %s", file.c_str());

	n_samples = get_int(fh);
	interval = get_int(fh);
	cur_t = get_int(fh);

	values = (long long int *)malloc(n_samples * sizeof(long long int));
	counts = (int *)malloc(n_samples * sizeof(int));
	valid = (bool *)malloc(n_samples * sizeof(bool));

	for(int index=0; index<n_samples; index++)
	{
		values[index] = get_long_long_int(fh);
		counts[index] = get_int(fh);
		valid[index] = get_bool(fh);
	}

	fclose(fh);
}

data_store_int::~data_store_int()
{
	free(counts);
	free(values);
	free(valid);
}

void data_store_int::dump(std::string file)
{
	FILE *fh = fopen(file.c_str(), "wb");
	if (!fh)
		error_exit("faile to create file %s", file.c_str());

	put_int(fh, n_samples);
	put_int(fh, interval);
	put_int(fh, cur_t);

	for(int index=0; index<n_samples; index++)
	{
		put_long_long_int(fh, values[index]);
		put_int(fh, counts[index]);
		put_bool(fh, valid[index]);
	}

	fclose(fh);
}

bool data_store_int::get_bool(FILE *fh)
{
	if (fgetc(fh))
		return true;

	return false;
}

int data_store_int::get_int(FILE *fh)
{
	unsigned char buffer[4];

	if (fread((char *)buffer, 4, 1, fh) != 1)
		error_exit("short read on data_store_int dump-file");

	return (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
}

// assuming 2x 32bit ints and 64bit long long int
long long int data_store_int::get_long_long_int(FILE *fh)
{
	unsigned i1, i2;

	i1 = get_int(fh);
	i2 = get_int(fh);

	return ((long long int)i1 << 32) + i2;
}

void data_store_int::put_bool(FILE *fh, bool value)
{
	fputc(value ? 1 : 0, fh);
}

void data_store_int::put_int(FILE *fh, int value)
{
	unsigned char buffer[4];

	buffer[0] = (value >> 24) && 255;
	buffer[1] = (value >> 16) && 255;
	buffer[2] = (value >>  8) && 255;
	buffer[3] = (value      ) && 255;

	if (fwrite((char *)buffer, 4, 1, fh) != 1)
		error_exit("problem writing to data_store_int dump-file");
}

void data_store_int::put_long_long_int(FILE *fh, long long int value)
{
	put_int(fh, value >> 32);
	put_int(fh, value & 0xffffffff);
}

int data_store_int::init_data(int t)
{
	int index = (t / interval) % n_samples;

	if (t != cur_t)
	{
		// if the interval between now and previous value
		// is more than a second, then invalidate values
		// in between
		for(int temp_t=cur_t + 1; temp_t < t; temp_t++)
		{
			int temp_index = (temp_t / interval) % n_samples;

			values[temp_index] = 0;
			counts[temp_index] = 0;
			valid[temp_index] = false;
		}

		values[index] = 0;
		counts[index] = 0;
		valid[index] = true;
	}

	cur_t = t;

	return index;
}

void data_store_int::add_avg(int t, int value)
{
	int index = init_data(t);

	values[index] += value;
	counts[index]++;
}

void data_store_int::add_sum(int t, int value)
{
	int index = init_data(t);

	values[index] += value;
	counts[index] = 1;
}

bool data_store_int::get(int index, double *value)
{
	if (counts[index])
	{
		*value = double(values[index]) / double(counts[index]);

		return true;
	}

	return false;
}
