/****************************************************************************/
/*                                                                          */
/*  Module:         jbicomp.c                                               */
/*                                                                          */
/*                  Copyright (C) Altera Corporation 1997-2001              */
/*                                                                          */
/*  Description:    Contains the code for compressing and uncompressing     */
/*                  Boolean array data.                                     */
/*                                                                          */
/*                  This algorithm works by searching previous bytes in the */
/*                  data that match the current data. If a match is found,  */
/*                  then the offset and length of the matching data can     */
/*                  replace the actual data in the output.                  */
/*                                                                          */
/*  Revisions:      2.2  fixed /W4 warnings                                 */
/*                                                                          */
/****************************************************************************/

#include "jbiport.h"
#include "jbiexprt.h"
#include "jbicomp.h"
#include "jbistub.h"

#define SHORT_BITS          16
#define CHAR_BITS           8
#define DATA_BLOB_LENGTH    3
#define MATCH_DATA_LENGTH   8192
#define JBI_ACA_REQUEST_SIZE 1024
#define JBI_ACA_BUFFER_SIZE (MATCH_DATA_LENGTH + JBI_ACA_REQUEST_SIZE)

unsigned long jbi_in_length = 0L;
unsigned long jbi_in_index = 0L;    /* byte index into compressed array */
unsigned int jbi_bits_avail = CHAR_BITS;

#if PORT == DOS
int jbi_current_variable_id = -1;
int jbi_current_page = -1;
int jbi_version = 0;
unsigned long jbi_out_length = 0L;
unsigned int jbi_out_index = 0; /* byte index into jbi_aca_out_buffer[] */
unsigned long jbi_aca_in_offset = 0L;
unsigned char jbi_aca_out_buffer[JBI_ACA_BUFFER_SIZE];
#endif

/****************************************************************************/
/*                                                                          */
/*  The following functions implement incremental decompression of Boolean  */
/*  array data, using a small memory window.                                */
/*                                                                          */
/*  This algorithm works by searching previous bytes in the data that match */
/*  the current data. If a match is found, then the offset and length of    */
/*  the matching data can replace the actual data in the output.            */
/*                                                                          */
/*  Memory usage is reduced by maintaining a "window" buffer which contains */
/*  the uncompressed data for one 8K page, plus some extra amount specified */
/*  by JBI_ACA_REQUEST_SIZE.  The function jbi_uncompress_page() is used to */
/*  request a subrange of the uncompressed data, starting at a particular   */
/*  bit position and extending a maximum of JBI_ACA_REQUEST_SIZE bytes.     */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */

unsigned int jbi_bits_required(unsigned int n)

/*                                                                          */
/*  Description:    Calculate the minimum number of bits required to        */
/*                  represent n.                                            */
/*                                                                          */
/*  Returns:        Number of bits.                                         */
/*                                                                          */
/****************************************************************************/
{
    unsigned int result = SHORT_BITS;

    if (n == 0)
    {
        result = 1;
    }
    else
    {
        /* Look for the highest non-zero bit position */
        while ((n & (1 << (SHORT_BITS - 1))) == 0)
        {
            n <<= 1;
            --result;
        }
    }

    return (result);
}

/****************************************************************************/
/*                                                                          */

unsigned int jbi_read_packed
(
#if PORT!=DOS
    unsigned char *buffer,
#endif
    unsigned int bits
)

/*                                                                          */
/*  Description:    Read the next value from the input array "buffer".      */
/*                  Read only "bits" bits from the array. The amount of     */
/*                  bits that have already been read from "buffer" is       */
/*                  stored internally to this function.                     */
/*                                                                          */
/*  Returns:        Up to 16 bit value. -1 if buffer overrun.               */
/*                                                                          */
/****************************************************************************/
{
    unsigned int result = 0;
    unsigned int shift = 0;
    unsigned int databyte = 0;

    while (bits > 0)
    {
#if PORT==DOS
        databyte = GET_BYTE(jbi_aca_in_offset + jbi_in_index);
#else
        databyte = buffer[jbi_in_index];
#endif
        result |= (((databyte >> (CHAR_BITS - jbi_bits_avail))
            & (0xFF >> (CHAR_BITS - jbi_bits_avail))) << shift);

        if (bits <= jbi_bits_avail)
        {
            result &= (0xFFFF >> (SHORT_BITS - (bits + shift)));
            jbi_bits_avail -= bits;
            bits = 0;
        }
        else
        {
            ++jbi_in_index;
            shift += jbi_bits_avail;
            bits -= jbi_bits_avail;
            jbi_bits_avail = CHAR_BITS;
        }
    }

    return (result);
}

#if PORT==DOS

/****************************************************************************/
/*                                                                          */

void jbi_uncompress_next_page(int version)

/*                                                                          */
/*  Description:    Uncompresses one page of compressed data, using         */
/*                  data page as reference for repeated sections.           */
/*                  Overwrites previous page of data in buffer.             */
/*                                                                          */
/*  Returns:        TRUE for success, FALSE if error encountered            */
/*                                                                          */
/****************************************************************************/
{
    unsigned int i, j, offset, length;
    unsigned int end_index;
    unsigned long tmp_in_index = jbi_in_index;
    unsigned int tmp_out_index = jbi_out_index;
    unsigned int tmp_bits_avail = jbi_bits_avail;
    unsigned int prev[3];
    unsigned long long_end;
    unsigned int match_data_length = MATCH_DATA_LENGTH;

    if (version > 0) --match_data_length;

    if (jbi_current_page < 0)
    {
        /* this is the first page of the array */
        jbi_current_page = 0;
        jbi_in_index = 4;   /* skip over length field */
        jbi_out_index = 0;
        end_index = (jbi_out_length < JBI_ACA_BUFFER_SIZE) ?
            (unsigned int) jbi_out_length : JBI_ACA_BUFFER_SIZE;
    }
    else
    {
        /* this is not the first page */
        ++jbi_current_page;
        jbi_out_index -= MATCH_DATA_LENGTH;
        long_end = jbi_out_length -
            ((long) jbi_current_page * (long) MATCH_DATA_LENGTH);
        end_index = (long_end < JBI_ACA_BUFFER_SIZE) ?
            (unsigned int) long_end : JBI_ACA_BUFFER_SIZE;

        /* copy extra data from end of circular buffer to beginning */
        for (i = 0; i < jbi_out_index; ++i)
        {
            jbi_aca_out_buffer[i] = jbi_aca_out_buffer[i + MATCH_DATA_LENGTH];
        }
    }

    while (jbi_out_index < end_index)
    {
        /* save state so we can undo the last packet when we reach the end */
        tmp_in_index = jbi_in_index;
        tmp_out_index = jbi_out_index;
        tmp_bits_avail = jbi_bits_avail;

        /* A 0 bit indicates literal data. */
        if (jbi_read_packed(1) == 0)
        {
            for (i = 0; i < DATA_BLOB_LENGTH; ++i)
            {
                if (jbi_out_index < end_index)
                {
                    if (version == 0)
                    {
                        prev[i] = jbi_aca_out_buffer[jbi_out_index] & 0xff;
                    }
                    jbi_aca_out_buffer[jbi_out_index++] =
                        (unsigned char) jbi_read_packed(CHAR_BITS);
                }
            }
        }
        else
        {
            /* A 1 bit indicates offset/length to follow. */
            offset = jbi_read_packed(jbi_bits_required(
                (jbi_current_page > 0) ? match_data_length :
                (jbi_out_index > match_data_length ? match_data_length :
                jbi_out_index)));
            length = jbi_read_packed(CHAR_BITS);

            if ((version == 0) && (offset == match_data_length + 3))
            {
                jbi_aca_out_buffer[jbi_out_index++] = (unsigned char) prev[0];
                jbi_aca_out_buffer[jbi_out_index++] = (unsigned char) prev[1];
                jbi_aca_out_buffer[jbi_out_index++] = (unsigned char) prev[2];
                length -= 3;
            }

            for (i = 0; i < length; ++i)
            {
                if (jbi_out_index < end_index)
                {
                    if (offset > jbi_out_index)
                    {
                        j = jbi_out_index + MATCH_DATA_LENGTH - offset;
                    }
                    else j = jbi_out_index - offset;
                    jbi_aca_out_buffer[jbi_out_index] = jbi_aca_out_buffer[j];
                    ++jbi_out_index;
                }
            }

            if (version == 0)
            {
                prev[0] = jbi_aca_out_buffer[jbi_out_index - 3] & 0xff;
                prev[1] = jbi_aca_out_buffer[jbi_out_index - 2] & 0xff;
                prev[2] = jbi_aca_out_buffer[jbi_out_index - 1] & 0xff;
            }
        }
    }

    /* restore the state before the previous packet */
    jbi_in_index = tmp_in_index;
    jbi_out_index = tmp_out_index;
    jbi_bits_avail = tmp_bits_avail;
}

/****************************************************************************/
/*                                                                          */

void jbi_uncompress_page
(
    int variable_id,
    int page,
    int version
)

/*                                                                          */
/*  Description:    Uncompress requested page of variable data.  Stores     */
/*                  uncompressed data in jbi_aca_out_buffer[].              */
/*                                                                          */
/*  Returns:        TRUE if successful, otherwise FALSE if:                 */
/*                      1) variable is not a compressed array               */
/*                      2) compressed data is illegal or corrupted          */
/*                      3) requested page is beyond the end of the array    */
/*                      4) internal error in the code                       */
/*                                                                          */
/****************************************************************************/
{
    unsigned long symbol_table;
    unsigned long data_section;
    unsigned long offset;
    unsigned long value;
    int delta = version * 2;

    if (variable_id != jbi_current_variable_id)
    {
        /* initialize to uncompress the desired variable */
        symbol_table = GET_DWORD(16 + (version * 8));
        data_section = GET_DWORD(20 + (version * 8));
        offset = symbol_table + ((11 + delta) * variable_id);
        value = GET_DWORD(offset + 3 + delta);
        jbi_current_variable_id = variable_id;
        jbi_current_page = -1;
        jbi_bits_avail = CHAR_BITS;
        jbi_in_length = GET_DWORD(offset + 7 + delta);
        jbi_out_length =
            (((unsigned long) GET_BYTE(data_section + value)) |
            (((unsigned long) GET_BYTE(data_section + value + 1)) << 8) |
            (((unsigned long) GET_BYTE(data_section + value + 2)) << 16) |
            (((unsigned long) GET_BYTE(data_section + value + 3)) << 24));
        jbi_in_index = 4;   /* skip over length field */
        jbi_out_index = 0;
        jbi_aca_in_offset = data_section + value;
    }

    /* to look back at an earlier page, start over at the beginning */
    if (page < jbi_current_page)
    {
        jbi_current_page = -1;
        jbi_in_index = 4;   /* skip over length field */
        jbi_bits_avail = CHAR_BITS;
    }

    /* uncompress sequentially up to the desired page */
    while (page > jbi_current_page)
    {
        jbi_uncompress_next_page(version);
    }
}

#else

/****************************************************************************/
/*                                                                          */

unsigned long jbi_uncompress
(
    unsigned char *in,
    unsigned long in_length,
    unsigned char *out,
    unsigned long out_length,
    int version
)

/*                                                                          */
/*  Description:    Uncompress data in "in" and write result to "out".      */
/*                                                                          */
/*  Returns:        Length of uncompressed data. -1 if:                     */
/*                      1) out_length is too small                          */
/*                      2) Internal error in the code                       */
/*                      3) in doesn't contain ACA compressed data.          */
/*                                                                          */
/****************************************************************************/
{
#ifdef CONFIG_64BIT
    unsigned int data_length = 0;
#else
    unsigned long data_length = 0L;
#endif
    unsigned long i, j;
    unsigned int offset, length;
    unsigned int match_data_length = MATCH_DATA_LENGTH;

    if (version > 0) --match_data_length;

    jbi_in_length = in_length;
    jbi_bits_avail = CHAR_BITS;
    jbi_in_index = 0L;
    for (i = 0; i < out_length; ++i) out[i] = 0;

    /* Read number of bytes in data. */
#ifdef CONFIG_64BIT
    for (i = 0; i < sizeof(unsigned int); ++i)
    {
        data_length = data_length | ((unsigned int)
            jbi_read_packed(in, CHAR_BITS) << (i * CHAR_BITS));
    }
#else
    for (i = 0; i < sizeof (in_length); ++i)
    {
        data_length = data_length | ((unsigned long)
            jbi_read_packed(in, CHAR_BITS) << (i * CHAR_BITS));
    }
#endif

    if (data_length > out_length)
    {
#ifdef CONFIG_64BIT
        jbi_dbg(DEBUG_ERR, "data_length(0x%x,0x%lx)\n",
            data_length, out_length);
        data_length = 0;
#else
        jbi_dbg(DEBUG_ERR, "data_length(0x%lx,0x%lx)\n",
            data_length, out_length);
        data_length = 0L;
#endif
    }
    else
    {
        i = 0;
        while (i < data_length)
        {
            /* A 0 bit indicates literal data. */
            if (jbi_read_packed(in, 1) == 0)
            {
                for (j = 0; j < DATA_BLOB_LENGTH; ++j)
                {
                    if (i < data_length)
                    {
                        out[i] = (unsigned char) jbi_read_packed(in, CHAR_BITS);
                        i++;
                    }
                }
            }
            else
            {
                /* A 1 bit indicates offset/length to follow. */
                offset = jbi_read_packed(in, jbi_bits_required((short) (i > match_data_length ? match_data_length : i)));
                length = jbi_read_packed(in, CHAR_BITS);

                for (j = 0; j < length; ++j)
                {
                    if (i < data_length)
                    {
                        out[i] = out[i - offset];
                        i++;
                    }
                }
            }
        }
    }

    return (data_length);
}

#endif
