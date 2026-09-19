#define __STDC_CONSTANT_MACROS
#include "codebook.h"

#include <string.h>


int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

unsigned int _book_maptype1_quantvals1(unsigned int entries, unsigned int dimensions){
  /* get us a starting hint, we'll polish it below */
  int bits=ilog(entries);
  int vals=entries>>((bits-1)*(dimensions-1)/dimensions);

  while(1){
    unsigned long acc=1;
    unsigned long acc1=1;
    unsigned int i;
    for(i=0;i<dimensions;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=entries && acc1>entries){
      return(vals);
    }else{
      if(acc>entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

void codebook_library_init(codebook_library *cb_lib)
{
    cb_lib->codebook_data = NULL;
    cb_lib->codebook_offsets = NULL;
    cb_lib->codebook_count = 0;
}

void codebook_library_destroy(codebook_library* cb_lib)
{
    free(cb_lib->codebook_data);
    free(cb_lib->codebook_offsets);
}

const char* codebook_library_get_codebook(codebook_library* cb_lib, int i)
{
    if (!cb_lib->codebook_data || !cb_lib->codebook_offsets)
    {
        fprintf(stderr, "codebook library not loaded\n");
        return NULL;
    }
    if (i >= cb_lib->codebook_count - 1 || i < 0) return NULL;
    return &cb_lib->codebook_data[cb_lib->codebook_offsets[i]];
}

long codebook_library_get_codebook_size(codebook_library* cb_lib, int i)
{
    if (!cb_lib->codebook_data || !cb_lib->codebook_offsets)
    {
        fprintf(stderr, "codebook library not loaded\n");
        return -1;
    }
    if (i >= cb_lib->codebook_count - 1 || i < 0) return -1;
    return cb_lib->codebook_offsets[i + 1] - cb_lib->codebook_offsets[i];
}

void codebook_library_init_from_data(codebook_library *cb_lib, const uint8_t* data, unsigned long length)
{
    cb_lib->codebook_data = NULL;
    cb_lib->codebook_offsets = NULL;
    cb_lib->codebook_count = 0;

    cb_lib->codebook_data = malloc(length);
    memcpy(cb_lib->codebook_data, data, length);

    long offset_offset = read_32_le_buf((unsigned char*)cb_lib->codebook_data + length - 4);
    cb_lib->codebook_count = (length - offset_offset) / 4;
    cb_lib->codebook_offsets = malloc(cb_lib->codebook_count * sizeof(long));

    for (long i = 0; i < cb_lib->codebook_count; i++)
    {
        cb_lib->codebook_offsets[i] = read_32_le_buf((unsigned char*)cb_lib->codebook_data+offset_offset+i*4);
    }
}

void codebook_library_rebuild_ogg(codebook_library *cb_lib, int i, Bit_oggstream* bos)
{
    const char * cb = codebook_library_get_codebook(cb_lib, i);
    unsigned long cb_size;

    {
        long signed_cb_size = codebook_library_get_codebook_size(cb_lib, i);
        if (!cb || -1 == signed_cb_size) {
            fprintf(stderr, "Invalid codebook id %d\n", i);
            return;
        }
        cb_size = signed_cb_size;
    }

    Bit_stream bis;
    Bit_stream_init(&bis, (char **)&cb);

    codebook_library_rebuild(cb_lib, &bis, cb_size, bos);
}

/* cb_size == 0 to not check size (for an inline bitstream) */
void codebook_library_copy(codebook_library *cb_lib, Bit_stream *bis, Bit_oggstream *bos)
{
    /* IN: 24 bit identifier, 16 bit dimensions, 24 bit entry count */

    (void)cb_lib;

    Bit_uint id = {0, 24};
    Bit_uint dimensions = {0, 16};
    Bit_uint entries = {0, 24};

    Bit_stream_right_shift_bit_uint(bis, &id);
    Bit_stream_right_shift_bit_uint(bis, &dimensions);
    Bit_stream_right_shift_bit_uint(bis, &entries);

    if (0x564342 != id.total)
    {
        fprintf(stderr, "invalid codebook identifier\n");
        return;
    }

    //cout << "Codebook with " << dimensions << " dimensions, " << entries << " entries" << endl;

    /* OUT: 24 bit identifier, 16 bit dimensions, 24 bit entry count */
    Bit_oggstream_left_shift_bit_uint(bos, &id);
    Bit_oggstream_left_shift_bit_uint(bos, &dimensions);
    Bit_oggstream_left_shift_bit_uint(bos, &entries);

    // gather codeword lengths

    /* IN/OUT: 1 bit ordered flag */
    Bit_uint ordered = {0, 1};
    Bit_stream_right_shift_bit_uint(bis, &ordered);
    Bit_oggstream_left_shift_bit_uint(bos, &ordered);
    if (ordered.total)
    {
        //cout << "Ordered " << endl;

        /* IN/OUT: 5 bit initial length */
        Bit_uint initial_length = {0, 5};
        Bit_stream_right_shift_bit_uint(bis, &initial_length);
        Bit_oggstream_left_shift_bit_uint(bos, &initial_length);

        unsigned int current_entry = 0;
        while (current_entry < entries.total)
        {
            /* IN/OUT: ilog(entries-current_entry) bit count w/ given length */
            Bit_uintv number;
            Bit_uintv_init(&number, ilog(entries.total-current_entry));
            Bit_stream_right_shift_bit_uintv(bis, &number);
            Bit_oggstream_left_shift_bit_uintv(bos, &number);
            current_entry += number.total;
        }
        if (current_entry > entries.total) {
            fprintf(stderr, "current_entry out of range\n");
            return;
        }
    }
    else
    {
        /* IN/OUT: 1 bit sparse flag */
        Bit_uint sparse = {0, 1};
        Bit_stream_right_shift_bit_uint(bis, &sparse);
        Bit_oggstream_left_shift_bit_uint(bos, &sparse);

        //cout << "Unordered, ";

        //if (sparse)
        //{
        //    cout << "Sparse" << endl;
        //}
        //else
        //{
        //    cout << "Nonsparse" << endl;
        //}

        for (unsigned int i = 0; i < entries.total; i++)
        {
            bool present_bool = true;

            if (sparse.total)
            {
                /* IN/OUT 1 bit sparse presence flag */
                Bit_uint present = {0, 1};
                Bit_stream_right_shift_bit_uint(bis, &present);
                Bit_oggstream_left_shift_bit_uint(bos, &present);

                present_bool = (0 != present.total);
            }

            if (present_bool)
            {
                /* IN/OUT: 5 bit codeword length-1 */
                Bit_uint codeword_length = {0, 5};
                Bit_stream_right_shift_bit_uint(bis, &codeword_length);
                Bit_oggstream_left_shift_bit_uint(bos, &codeword_length);
            }
        }
    } // done with lengths


    // lookup table

    /* IN/OUT: 4 bit lookup type */
    Bit_uint lookup_type = {0, 4};
    Bit_stream_right_shift_bit_uint(bis, &lookup_type);
    Bit_oggstream_left_shift_bit_uint(bos, &lookup_type);

    if (0 == lookup_type.total)
    {
        //cout << "no lookup table" << endl;
    }
    else if (1 == lookup_type.total)
    {
        //cout << "lookup type 1" << endl;

        /* IN/OUT: 32 bit minimum length, 32 bit maximum length, 4 bit value length-1, 1 bit sequence flag */
        Bit_uint min = {0, 32}, max = {0, 32};
        Bit_uint value_length = {0, 4};
        Bit_uint sequence_flag = {0, 1};
        Bit_stream_right_shift_bit_uint(bis, &min);
        Bit_stream_right_shift_bit_uint(bis, &max);
        Bit_stream_right_shift_bit_uint(bis, &value_length);
        Bit_stream_right_shift_bit_uint(bis, &sequence_flag);
        Bit_oggstream_left_shift_bit_uint(bos, &min);
        Bit_oggstream_left_shift_bit_uint(bos, &max);
        Bit_oggstream_left_shift_bit_uint(bos, &value_length);
        Bit_oggstream_left_shift_bit_uint(bos, &sequence_flag);
        unsigned int quantvals = _book_maptype1_quantvals1(entries.total, dimensions.total);
        for (unsigned int i = 0; i < quantvals; i++)
        {
            /* IN/OUT: n bit value */
            Bit_uintv val;
            Bit_uintv_init(&val, value_length.total + 1);
            Bit_stream_right_shift_bit_uintv(bis, &val);
            Bit_oggstream_left_shift_bit_uintv(bos, &val);
        }
    }
    else if (2 == lookup_type.total)
    {
        fprintf(stderr, "didn't expect lookup type 2\n");
        return;
    }
    else
    {
        fprintf(stderr, "invalid lookup type\n");
        return;
    }

    //cout << "total bits read = " << bis.get_total_bits_read() << endl;
}

/* cb_size == 0 to not check size (for an inline bitstream) */
void codebook_library_rebuild(codebook_library *cb_lib, Bit_stream *bis, unsigned long cb_size, Bit_oggstream* bos)
{
    /* IN: 4 bit dimensions, 14 bit entry count */
	(void)cb_lib;

    Bit_uint dimensions = {0, 4};
    Bit_uint entries = {0, 14};

    Bit_stream_right_shift_bit_uint(bis, &dimensions);
    Bit_stream_right_shift_bit_uint(bis, &entries);

    //cout << "Codebook " << i << ", " << dimensions << " dimensions, " << entries << " entries" << endl;
    //cout << "Codebook with " << dimensions << " dimensions, " << entries << " entries" << endl;

    /* OUT: 24 bit identifier, 16 bit dimensions, 24 bit entry count */
    Bit_uint id = {0, 24};
    id.total = 0x564342; // "VCB" for "Vorbis CodeBook"
    Bit_oggstream_left_shift_bit_uint(bos, &id);
    Bit_uint dimensions_out = { dimensions.total, 16 };
    Bit_oggstream_left_shift_bit_uint(bos, &dimensions_out);
    Bit_uint entries_out = { entries.total, 24 };
    Bit_oggstream_left_shift_bit_uint(bos, &entries_out);

    // gather codeword lengths

    /* IN/OUT: 1 bit ordered flag */
    Bit_uint ordered = {0, 1};
    Bit_stream_right_shift_bit_uint(bis, &ordered);
    Bit_oggstream_left_shift_bit_uint(bos, &ordered);
    if (ordered.total)
    {
        //cout << "Ordered " << endl;

        /* IN/OUT: 5 bit initial length */
        Bit_uint initial_length = {0, 5};
        Bit_stream_right_shift_bit_uint(bis, &initial_length);
        Bit_oggstream_left_shift_bit_uint(bos, &initial_length);

        unsigned int current_entry = 0;
        while (current_entry < entries.total)
        {
            /* IN/OUT: ilog(entries-current_entry) bit count w/ given length */
            Bit_uintv number;
            Bit_uintv_init(&number, ilog(entries.total-current_entry));
            Bit_stream_right_shift_bit_uintv(bis, &number);
            Bit_oggstream_left_shift_bit_uintv(bos, &number);
            current_entry += number.total;
        }
        if (current_entry > entries.total) {
            fprintf(stderr, "current_entry out of range\n");
            return;
        }
    }
    else
    {
        /* IN: 3 bit codeword length length, 1 bit sparse flag */
        Bit_uint codeword_length_length = {0, 3};
        Bit_uint sparse = {0, 1};
        Bit_stream_right_shift_bit_uint(bis, &codeword_length_length);
        Bit_stream_right_shift_bit_uint(bis, &sparse);

        //cout << "Unordered, " << codeword_length_length << " bit lengths, ";

        if (0 == codeword_length_length.total || 5 < codeword_length_length.total)
        {
            fprintf(stderr, "nonsense codeword length length\n");
            return;
        }

        /* OUT: 1 bit sparse flag */
        Bit_oggstream_left_shift_bit_uint(bos, &sparse);
        //if (sparse)
        //{
        //    cout << "Sparse" << endl;
        //}
        //else
        //{
        //    cout << "Nonsparse" << endl;
        //}

        for (unsigned int i = 0; i < entries.total; i++)
        {
            bool present_bool = true;

            if (sparse.total)
            {
                /* IN/OUT 1 bit sparse presence flag */
                Bit_uint present = {0, 1};
                Bit_stream_right_shift_bit_uint(bis, &present);
                Bit_oggstream_left_shift_bit_uint(bos, &present);

                present_bool = (0 != present.total);
            }

            if (present_bool)
            {
                /* IN: n bit codeword length-1 */
                Bit_uintv codeword_length;
                Bit_uintv_init(&codeword_length, codeword_length_length.total);
                Bit_stream_right_shift_bit_uintv(bis, &codeword_length);

                /* OUT: 5 bit codeword length-1 */
                Bit_uint codeword_length_out = {codeword_length.total, 5};
                Bit_oggstream_left_shift_bit_uint(bos, &codeword_length_out);
            }
        }
    } // done with lengths


    // lookup table

    /* IN: 1 bit lookup type */
    Bit_uint lookup_type = {0, 1};
    Bit_stream_right_shift_bit_uint(bis, &lookup_type);
    /* OUT: 4 bit lookup type */
    Bit_uint lookup_type_out = {lookup_type.total, 4};
    Bit_oggstream_left_shift_bit_uint(bos, &lookup_type_out);

    if (0 == lookup_type.total)
    {
        //cout << "no lookup table" << endl;
    }
    else if (1 == lookup_type.total)
    {
        //cout << "lookup type 1" << endl;

        /* IN/OUT: 32 bit minimum length, 32 bit maximum length, 4 bit value length-1, 1 bit sequence flag */
        Bit_uint min = {0, 32}, max = {0, 32};
        Bit_uint value_length = {0, 4};
        Bit_uint sequence_flag = {0, 1};
        Bit_stream_right_shift_bit_uint(bis, &min);
        Bit_stream_right_shift_bit_uint(bis, &max);
        Bit_stream_right_shift_bit_uint(bis, &value_length);
        Bit_stream_right_shift_bit_uint(bis, &sequence_flag);
        Bit_oggstream_left_shift_bit_uint(bos, &min);
        Bit_oggstream_left_shift_bit_uint(bos, &max);
        Bit_oggstream_left_shift_bit_uint(bos, &value_length);
        Bit_oggstream_left_shift_bit_uint(bos, &sequence_flag);
        unsigned int quantvals = _book_maptype1_quantvals1(entries.total, dimensions.total);
        for (unsigned int i = 0; i < quantvals; i++)
        {
            /* IN/OUT: n bit value */
            Bit_uintv val;
            Bit_uintv_init(&val, value_length.total + 1);
            Bit_stream_right_shift_bit_uintv(bis, &val);
            Bit_oggstream_left_shift_bit_uintv(bos, &val);
        }
    }
    else if (2 == lookup_type.total)
    {
        fprintf(stderr, "didn't expect lookup type 2\n");
        return;
    }
    else
    {
        fprintf(stderr, "invalid lookup type\n");
        return;
    }

    //cout << "total bits read = " << bis.get_total_bits_read() << endl;

    /* check that we used exactly all bytes */
    /* note: if all bits are used in the last byte there will be one extra 0 byte */
    if ( 0 != cb_size && bis->total_bits_read/8+1 != cb_size )
    {
        fprintf(stderr, "codebook size mismatch: expected %lu, read %lu\n", cb_size,bis->total_bits_read/8+1);
        return;
    }
}
