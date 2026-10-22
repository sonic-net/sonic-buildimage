/****************************************************************************/
/*                                                                          */
/*  Module:         jbimain.c                                               */
/*                                                                          */
/*                  Copyright (C) Altera Corporation 1998-2001              */
/*                                                                          */
/*  Description:    Jam STAPL ByteCode Player (Interpreter)                 */
/*                                                                          */
/*  Revisions:      2.2  fixed /W4 warnings                                 */
/*                  2.0  added support for STAPL ByteCode format            */
/*                                                                          */
/****************************************************************************/

#include "jbiport.h"
#include "jbiexprt.h"
#include "jbijtag.h"
#include "jbicomp.h"
#include "jbistub.h"

/****************************************************************************/
/*                                                                          */
/*  MACROS                                                                  */
/*                                                                          */
/****************************************************************************/

#ifndef NULL
#define NULL 0
#endif

#define JBI_STACK_SIZE 128

#define JBIC_MESSAGE_LENGTH 1024

/*
*   This macro checks if enough parameters are available on the stack. The
*   argument is the number of parameters needed.
*/
#define IF_CHECK_STACK(x) \
    if (stack_ptr < (int) (x)) \
    { \
        status = JBIC_STACK_OVERFLOW; \
    } \
    else

/*
*   This macro checks if a code address is inside the code section
*/
#define CHECK_PC \
    if ((pc < code_section) || (pc >= debug_section)) \
    { \
        status = JBIC_BOUNDS_ERROR; \
    }

/****************************************************************************/
/*                                                                          */
/*  GLOBAL VARIABLES                                                        */
/*                                                                          */
/****************************************************************************/

#if PORT==DOS
/*
*   jbi_program is a global pointer used by macros GET_BYTE, GET_WORD, and
*   GET_DWORD to read data from the JBC file
*/
PROGRAM_PTR jbi_program;
#endif

/****************************************************************************/
/*                                                                          */
/*  UTILITY FUNCTIONS                                                       */
/*                                                                          */
/****************************************************************************/

int jbi_strlen(char *string)
{
    int len = 0;

    while (string[len] != '\0') ++len;

    return (len);
}

long jbi_atol(char *buffer)
{
    long result = 0L;
    int index = 0;

    while ((buffer[index] >= '0') && (buffer[index] <= '9'))
    {
        result = (result * 10) + (buffer[index] - '0');
        ++index;
    }

    return (result);
}

void jbi_ltoa(char *buffer, long number)
{
    int index = 0;
    int rev_index = 0;
    char reverse[32];

    if (number < 0L)
    {
        buffer[index++] = '-';
        number = 0 - number;
    }
    else if (number == 0)
    {
        buffer[index++] = '0';
    }

    while (number != 0)
    {
        reverse[rev_index++] = (char) ((number % 10) + '0');
        number /= 10;
    }

    while (rev_index > 0)
    {
        buffer[index++] = reverse[--rev_index];
    }

    buffer[index] = '\0';
}

char jbi_toupper(char ch)
{
    return ((char) (((ch >= 'a') && (ch <= 'z')) ? (ch + 'A' - 'a') : ch));
}

int jbi_stricmp(char *left, char *right)
{
    int result = 0;
    char l, r;

    do
    {
        l = jbi_toupper(*left);
        r = jbi_toupper(*right);
        result = l - r;
        ++left;
        ++right;
    }
    while ((result == 0) && (l != '\0') && (r != '\0'));

    return (result);
}

void jbi_strncpy(char *left, char *right, int count)
{
    char ch;

    do
    {
        *left = *right;
        ch = *right;
        ++left;
        ++right;
        --count;
    }
    while ((ch != '\0') && (count != 0));
}

void jbi_make_dword(unsigned char *buf, unsigned long num)
{
    buf[0] = (unsigned char) num;
    buf[1] = (unsigned char) (num >> 8L);
    buf[2] = (unsigned char) (num >> 16L);
    buf[3] = (unsigned char) (num >> 24L);
}

unsigned long jbi_get_dword(unsigned char *buf)
{
    return
        (((unsigned long) buf[0]) |
        (((unsigned long) buf[1]) << 8L) |
        (((unsigned long) buf[2]) << 16L) |
        (((unsigned long) buf[3]) << 24L));
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_execute
(
    PROGRAM_PTR program,
    long program_size,
    char *workspace,
    long workspace_size,
    char *action,
    char **init_list,
    int reset_jtag,
    long *error_address,
    int *exit_code,
    int *format_version
)

/*                                                                          */
/*  Description:                                                            */
/*                                                                          */
/*  Returns:                                                                */
/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned long first_word = 0L;
    unsigned long action_table = 0L;
    unsigned long proc_table = 0L;
    unsigned long string_table = 0L;
    unsigned long symbol_table = 0L;
    unsigned long data_section = 0L;
    unsigned long code_section = 0L;
    unsigned long debug_section = 0L;
    unsigned long action_count = 0L;
    unsigned long proc_count = 0L;
    unsigned long symbol_count = 0L;
    /*char message_buffer[JBIC_MESSAGE_LENGTH + 1];*/
    char *message_buffer;
    addr_t *variables = NULL;
    long *variable_size = NULL;
    char *attributes = NULL;
    unsigned char *proc_attributes = NULL;
    unsigned long pc;
    unsigned long opcode_address;
    unsigned long args[3];
    unsigned int opcode;
    unsigned long name_id;
    addr_t stack[JBI_STACK_SIZE] = {0};
    unsigned char charbuf[4];
    long long_temp;
    unsigned int variable_id;
    unsigned char *charptr_temp;
    unsigned char *charptr_temp2;
    long *longptr_temp;
    int version = 0;
    int delta = 0;
    int stack_ptr = 0;
    unsigned int arg_count;
    int done = 0;
    int bad_opcode = 0;
    unsigned int count;
    unsigned int index;
    unsigned int index2;
    long long_count;
    long long_index;
    long long_index2;
    unsigned int i;
    unsigned int j;
    unsigned long uncompressed_size, uncompressed_result;
    unsigned int offset;
    unsigned long value;
    int current_proc = 0;
    char *equal_ptr;
    int length;
    int reverse;

    unsigned long debug_cnt = 0;

#if PORT==DOS
    char name[33];
#else
    char *name;
#endif

    jbi_workspace = workspace;
    jbi_workspace_size = workspace_size;

#if PORT==DOS
    jbi_program = program;
#endif

    /* Resolve compilation warnings: the frame size of 1664 bytes is larger than 1024 bytes */
    message_buffer = (char *) kzalloc(JBIC_MESSAGE_LENGTH + 1, GFP_KERNEL);
    if (message_buffer == NULL) {
        jbi_dbg(DEBUG_DETAIL, "Memory not enough jbi_execute \n");
        return JBIC_OUT_OF_MEMORY;
    }

    /*
    *   Read header information
    */
    if (program_size > 52L)
    {
        first_word    = GET_DWORD(0);
        version = (int) (first_word & 1L);
        *format_version = version + 1;
        delta = version * 8;

        action_table  = GET_DWORD(4);
        proc_table    = GET_DWORD(8);
        string_table  = GET_DWORD(4 + delta);
        symbol_table  = GET_DWORD(16 + delta);
        data_section  = GET_DWORD(20 + delta);
        code_section  = GET_DWORD(24 + delta);
        debug_section = GET_DWORD(28 + delta);
        action_count  = GET_DWORD(40 + delta);
        proc_count    = GET_DWORD(44 + delta);
        symbol_count  = GET_DWORD(48 + (2 * delta));

        jbi_dbg(DEBUG_DETAIL, "version:       %d\n", version);
        jbi_dbg(DEBUG_DETAIL, "delta:         %d\n", delta);
        jbi_dbg(DEBUG_DETAIL, "action_table:  0x%08lx\n", action_table);
        jbi_dbg(DEBUG_DETAIL, "proc_table:    0x%08lx\n", proc_table);
        jbi_dbg(DEBUG_DETAIL, "string_table:  0x%08lx\n", string_table);
        jbi_dbg(DEBUG_DETAIL, "symbol_table:  0x%08lx\n", symbol_table);
        jbi_dbg(DEBUG_DETAIL, "data_section:  0x%08lx\n", data_section);
        jbi_dbg(DEBUG_DETAIL, "code_section:  0x%08lx\n", code_section);
        jbi_dbg(DEBUG_DETAIL, "debug_section: 0x%08lx\n", debug_section);
        jbi_dbg(DEBUG_DETAIL, "action_count:  0x%08lx\n", action_count);
        jbi_dbg(DEBUG_DETAIL, "proc_count:    0x%08lx\n", proc_count);
        jbi_dbg(DEBUG_DETAIL, "symbol_count:  0x%08lx\n", symbol_count);
        jbi_dbg(DEBUG_DETAIL, "\n");
    }

    if ((first_word != 0x4A414D00L) && (first_word != 0x4A414D01L))
    {
        jbi_dbg(DEBUG_ERR, "first_word 0x%lx\n", first_word);
        done = 1;
        status = JBIC_IO_ERROR;
    }

    if ((status == JBIC_SUCCESS) && (symbol_count > 0))
    {
        variables = (addr_t *) jbi_malloc(
            (unsigned int) symbol_count * sizeof(long));

        if (variables == NULL) status = JBIC_OUT_OF_MEMORY;

        if (status == JBIC_SUCCESS)
        {
            variable_size = (long *) jbi_malloc(
                (unsigned int) symbol_count * sizeof(long));

            if (variable_size == NULL) status = JBIC_OUT_OF_MEMORY;
        }

        if (status == JBIC_SUCCESS)
        {
            attributes = (char *) jbi_malloc((unsigned int) symbol_count);

            if (attributes == NULL) status = JBIC_OUT_OF_MEMORY;
        }

        if ((status == JBIC_SUCCESS) && (version > 0))
        {
            proc_attributes = (unsigned char *) jbi_malloc((unsigned int) proc_count);

            if (proc_attributes == NULL) status = JBIC_OUT_OF_MEMORY;
        }

        if (status == JBIC_SUCCESS)
        {
            delta = version * 2;

            for (i = 0; i < (unsigned int) symbol_count; ++i)
            {
                offset = (unsigned int) (symbol_table + ((11 + delta) * i));

                value = GET_DWORD(offset + 3 + delta);

                attributes[i] = GET_BYTE(offset);

                /* use bit 7 of attribute byte to indicate that this buffer */
                /* was dynamically allocated and should be freed later */
                attributes[i] &= 0x7f;

                variable_size[i] = GET_DWORD(offset + 7 + delta);

                jbi_dbg(DEBUG_NOISY, "symbol %03d: 0x%02x,0x%08lx,0x%08lx\n",
                    i, attributes[i], value, variable_size[i]);

                /*
                *   Attribute bits:
                *   bit 0:  0 = read-only, 1 = read-write
                *   bit 1:  0 = not compressed, 1 = compressed
                *   bit 2:  0 = not initialized, 1 = initialized
                *   bit 3:  0 = scalar, 1 = array
                *   bit 4:  0 = Boolean, 1 = integer
                *   bit 5:  0 = declared variable,
                *           1 = compiler created temporary variable
                */

                if ((attributes[i] & 0x0c) == 0x04)
                {
                    /* initialized scalar variable */
                    variables[i] = value;
                }
                else if ((attributes[i] & 0x1e) == 0x0e)
                {
                    /* initialized compressed Boolean array */
#if PORT==DOS
                    /* for DOS port, get the size but do not uncompress */
                    long_index = data_section + value;
                    uncompressed_size =
                        (((unsigned long) GET_BYTE(long_index)) |
                        (((unsigned long) GET_BYTE(long_index + 1L)) << 8L) |
                        (((unsigned long) GET_BYTE(long_index + 2L)) << 16L) |
                        (((unsigned long) GET_BYTE(long_index + 3L)) << 24L));
                    variable_size[i] = uncompressed_size;
#else
                    uncompressed_size = jbi_get_dword(
                        &program[data_section + value]);

                    /* allocate a buffer for the uncompressed data */
                    variables[i] = (addr_t) jbi_malloc(uncompressed_size);

                    if (variables[i] == (addr_t) 0L)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                    }
                    else
                    {
                        /* set flag so buffer will be freed later */
                        attributes[i] |= 0x80;

                        /* uncompress the data */
                        uncompressed_result =
                            jbi_uncompress(
                            &program[data_section + value],
                            variable_size[i],
                            (unsigned char *) variables[i],
                            uncompressed_size,
                            version);
                        if (uncompressed_result != uncompressed_size)
                        {
                            /* decompression failed */
                            jbi_dbg(DEBUG_ERR, "uncompress fail(0x%lx,0x%lx,0x%lx)(0x%lx)\n",
                                variable_size[i], uncompressed_result, uncompressed_size, value);
                            status = JBIC_IO_ERROR;
                        }
                        else
                        {
                            variable_size[i] = uncompressed_size * 8L;
                        }
                    }
#endif
                }
                else if ((attributes[i] & 0x1e) == 0x0c)
                {
                    /* initialized Boolean array */
#if PORT==DOS
                    /* flag attributes so that memory is freed */
                    attributes[i] |= 0x80;

                    if (variable_size[i] > 0)
                    {
                        unsigned int size = (unsigned int)
                            ((variable_size[i] + 7L) / 8L);

                        variables[i] = (long) jbi_malloc(size);

                        if (variables[i] == NULL)
                        {
                            status = JBIC_OUT_OF_MEMORY;
                        }
                        else
                        {
                            unsigned char *p = (unsigned char *) variables[i];
                            /* copy array values into buffer */
                            for (j = 0; j < size; ++j)
                            {
                                p[j] = GET_BYTE(data_section + value + j);
                            }
                        }
                    }
                    else
                    {
                        variables[i] = 0;
                    }
#else
                    variables[i] = value + data_section + (addr_t) program;
#endif
                }
                else if ((attributes[i] & 0x1c) == 0x1c)
                {
                    /* initialized integer array */
                    variables[i] = value + data_section;
                }
                else if ((attributes[i] & 0x0c) == 0x08)
                {
                    /* uninitialized array */

                    /* flag attributes so that memory is freed */
                    attributes[i] |= 0x80;

                    if (variable_size[i] > 0)
                    {
                        unsigned int size;

                        if (attributes[i] & 0x10)
                        {
                            /* integer array */
                            size = (unsigned int)
                                (variable_size[i] * sizeof(long));
                        }
                        else
                        {
                            /* Boolean array */
                            size = (unsigned int)
                                ((variable_size[i] + 7L) / 8L);
                        }

                        variables[i] = (addr_t) jbi_malloc(size);

                        if (variables[i] == (addr_t) NULL)
                        {
                            status = JBIC_OUT_OF_MEMORY;
                        }
                        else
                        {
                            /* zero out memory */
                            for (j = 0; j < size; ++j)
                            {
                                ((unsigned char *)(variables[i]))[j] = 0;
                            }
                        }
                    }
                    else
                    {
                        variables[i] = 0;
                    }
                }
                else
                {
                    variables[i] = 0;
                }

                jbi_dbg(DEBUG_NOISY, "  variables: 0x%08lx,0x%016llx\n",
                    variable_size[i], (long long) variables[i]);
            }
        }

        jbi_dbg(DEBUG_NOISY, "\n");
    }

    /*
    *   Initialize variables listed in init_list
    */
    if ((status == JBIC_SUCCESS) && (init_list != NULL) && (version == 0))
    {
        delta = version * 2;
        count = 0;
        while (init_list[count] != NULL)
        {
            equal_ptr = init_list[count];
            length = 0;
            while ((*equal_ptr != '=') && (*equal_ptr != '\0'))
            {
                ++equal_ptr;
                ++length;
            }
            if (*equal_ptr == '=')
            {
                ++equal_ptr;
                value = jbi_atol(equal_ptr);
                jbi_strncpy(message_buffer, init_list[count], length);
                message_buffer[length] = '\0';
                for (i = 0; i < (unsigned int) symbol_count; ++i)
                {
                    offset = (unsigned int) (symbol_table + ((11 + delta) * i));
                    name_id = (version == 0) ? GET_WORD(offset + 1) :
                        GET_DWORD(offset + 1);
#if PORT==DOS
                    for (j = 0; j < 32; ++j)
                    {
                        name[j] = GET_BYTE(string_table + name_id + j);
                    }
                    name[32] = '\0';
#else
                    name = (char *) &program[string_table + name_id];
#endif

                    if (jbi_stricmp(message_buffer, name) == 0)
                    {
                        variables[i] = value;
                    }

                    jbi_dbg(DEBUG_NOISY, "init_list %03d: 0x%08lx,%s,0x%016llx\n",
                        i, name_id, name, (long long) variables[i]);
                }
            }

            ++count;
        }

        jbi_dbg(DEBUG_NOISY, "\n");
    }

    if (status != JBIC_SUCCESS) done = 1;

    jbi_init_jtag();

    pc = code_section;
    message_buffer[0] = '\0';

    /*
    *   For JBC version 2, we will execute the procedures corresponding to
    *   the selected ACTION
    */
    if (version > 0)
    {
        if (action == NULL)
        {
            status = JBIC_ACTION_NOT_FOUND;
            done = 1;
        }
        else
        {
            int action_found = 0;

            for (i = 0; (i < action_count) && !action_found; ++i)
            {
                name_id = GET_DWORD(action_table + (12 * i));

#if PORT==DOS
                for (j = 0; j < 32; ++j)
                {
                    name[j] = GET_BYTE(string_table + name_id + j);
                }
                name[32] = '\0';
#else
                name = (char *) &program[string_table + name_id];
#endif

                if (jbi_stricmp(action, name) == 0)
                {
                    action_found = 1;
                    current_proc = (int) GET_DWORD(action_table + (12 * i) + 8);
                }

                jbi_dbg(DEBUG_NOISY, "action %03d: 0x%08lx,%s, %d,%d\n",
                        i, name_id, name, action_found, current_proc);
            }

            if (!action_found)
            {
                status = JBIC_ACTION_NOT_FOUND;
                done = 1;
            }
        }

        if (status == JBIC_SUCCESS)
        {
            int first_time = 1;
            i = current_proc;
            while ((i != 0) || first_time)
            {
                first_time = 0;
                /* check procedure attribute byte */
                proc_attributes[i] = (unsigned char)
                    (GET_BYTE(proc_table + (13 * i) + 8) & 0x03);

                jbi_dbg(DEBUG_NOISY, "  proc_attributes %03d: 0x%02x\n",
                     i, proc_attributes[i]);

                if (proc_attributes[i] != 0)
                {
                    /*
                    *   BIT0 - OPTIONAL
                    *   BIT1 - RECOMMENDED
                    *   BIT6 - FORCED OFF
                    *   BIT7 - FORCED ON
                    */
                    if (init_list != NULL)
                    {
                        name_id = GET_DWORD(proc_table + (13 * i));
#if PORT==DOS
                        for (j = 0; j < 32; ++j)
                        {
                            name[j] = GET_BYTE(string_table + name_id + j);
                        }
                        name[32] = '\0';
#else
                        name = (char *) &program[string_table + name_id];
#endif

                        jbi_dbg(DEBUG_NOISY, "    init_list %03d: 0x%08lx,%s\n",
                            i, name_id, name);

                        count = 0;
                        while (init_list[count] != NULL)
                        {
                            equal_ptr = init_list[count];
                            length = 0;
                            while ((*equal_ptr != '=') && (*equal_ptr != '\0'))
                            {
                                ++equal_ptr;
                                ++length;
                            }
                            if (*equal_ptr == '=')
                            {
                                ++equal_ptr;
                                jbi_strncpy(message_buffer, init_list[count], length);
                                message_buffer[length] = '\0';

                                if (jbi_stricmp(message_buffer, name) == 0)
                                {
                                    if (jbi_atol(equal_ptr) == 0)
                                    {
                                        proc_attributes[i] |= 0x40;
                                    }
                                    else
                                    {
                                        proc_attributes[i] |= 0x80;
                                    }
                                }
                            }

                            jbi_dbg(DEBUG_NOISY, "      proc_attributes %03d: 0x%02x\n",
                                i, proc_attributes[i]);

                            ++count;
                        }
                    }
                }

                i = (unsigned int) GET_DWORD(proc_table + (13 * i) + 4);
            }

            /*
            *   Set current_proc to the first procedure to be executed
            */
            i = current_proc;
            while ((i != 0) &&
                ((proc_attributes[i] == 1) ||
                ((proc_attributes[i] & 0xc0) == 0x40)))
            {
                i = (unsigned int) GET_DWORD(proc_table + (13 * i) + 4);
            }

            if ((i != 0) || ((i == 0) && (current_proc == 0) &&
                ((proc_attributes[0] != 1) &&
                ((proc_attributes[0] & 0xc0) != 0x40))))
            {
                current_proc = i;
                pc = code_section + GET_DWORD(proc_table + (13 * i) + 9);
                CHECK_PC;
            }
            else
            {
                /* there are no procedures to execute! */
                done = 1;
            }
        }

        jbi_dbg(DEBUG_NOISY, "\n");
    }

    message_buffer[0] = '\0';

    jbi_dbg(DEBUG_NOISY, "excute pc: 0x%lx,%d\n", pc, current_proc);
    while (!done)
    {
        opcode = (unsigned int) (GET_BYTE(pc) & 0xff);
        debug_cnt++;
        jbi_dbg(DEBUG_NOISY, "op: 0x%02x(%03d:0x%08lx,%08lx)",
            opcode, stack_ptr, pc, debug_cnt);
        opcode_address = pc;
        ++pc;

        arg_count = (opcode >> 6) & 3;
        jbi_dbg(DEBUG_NOISY, " - %u:", arg_count);
        for (i = 0; i < arg_count; ++i)
        {
            args[i] = GET_DWORD(pc);
            jbi_dbg(DEBUG_NOISY, " 0x%08lx", args[i]);
            pc += 4;
        }
        jbi_dbg(DEBUG_NOISY, "\n");

        switch (opcode)
        {
        case 0x00: /* NOP  */
            /* do nothing */
            break;

        case 0x01: /* DUP  */
            IF_CHECK_STACK(1)
            {
                stack[stack_ptr] = stack[stack_ptr - 1];
                ++stack_ptr;
            }
            break;

        case 0x02: /* SWP  */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[stack_ptr - 2];
                stack[stack_ptr - 2] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }
            break;

        case 0x03: /* ADD  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] += stack[stack_ptr];
            }
            break;

        case 0x04: /* SUB  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] -= stack[stack_ptr];
            }
            break;

        case 0x05: /* MULT */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] *= stack[stack_ptr];
            }
            break;

        case 0x06: /* DIV  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] /= stack[stack_ptr];
            }
            break;

        case 0x07: /* MOD  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] %= stack[stack_ptr];
            }
            break;

        case 0x08: /* SHL  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] <<= stack[stack_ptr];
            }
            break;

        case 0x09: /* SHR  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] >>= stack[stack_ptr];
            }
            break;

        case 0x0A: /* NOT  */
            IF_CHECK_STACK(1)
            {
                stack[stack_ptr - 1] ^= (-1L);
            }
            break;

        case 0x0B: /* AND  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] &= stack[stack_ptr];
            }
            break;

        case 0x0C: /* OR   */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] |= stack[stack_ptr];
            }
            break;

        case 0x0D: /* XOR  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] ^= stack[stack_ptr];
            }
            break;

        case 0x0E: /* INV */
            IF_CHECK_STACK(1)
            {
                stack[stack_ptr - 1] = stack[stack_ptr - 1] ? 0L : 1L;
            }
            break;

        case 0x0F: /* GT   */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] =
                    (stack[stack_ptr - 1] > stack[stack_ptr]) ? 1L : 0L;
            }
            break;

        case 0x10: /* LT   */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] =
                    (stack[stack_ptr - 1] < stack[stack_ptr]) ? 1L : 0L;
            }
            break;

        case 0x11: /* RET  */
            if ((version > 0) && (stack_ptr == 0))
            {
                /*
                *   We completed one of the main procedures of an ACTION.
                *   Find the next procedure to be executed and jump to it.
                *   If there are no more procedures, then EXIT.
                */
                i = (unsigned int) GET_DWORD(proc_table + (13 * current_proc) + 4);
                while ((i != 0) &&
                    ((proc_attributes[i] == 1) ||
                    ((proc_attributes[i] & 0xc0) == 0x40)))
                {
                    i = (unsigned int) GET_DWORD(proc_table + (13 * i) + 4);
                }

                if (i == 0)
                {
                    /* there are no procedures to execute! */
                    done = 1;
                    *exit_code = 0; /* success */
                }
                else
                {
                    current_proc = i;
                    pc = code_section + GET_DWORD(proc_table + (13 * i) + 9);
                    CHECK_PC;
                }
            }
            else IF_CHECK_STACK(1)
            {
                pc = stack[--stack_ptr] + code_section;
                CHECK_PC;
                if (pc == code_section)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
            }
            break;

        case 0x12: /* CMPS */
            /*
            *   Array short compare
            *   ...stack 0 is source 1 value
            *   ...stack 1 is source 2 value
            *   ...stack 2 is mask value
            *   ...stack 3 is count
            */
            IF_CHECK_STACK(4)
            {
                long a = stack[--stack_ptr];
                long b = stack[--stack_ptr];
                long_temp = stack[--stack_ptr];
                count = (unsigned int) stack[stack_ptr - 1];

                if ((count < 1) || (count > 32))
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
                    long_temp &= ((-1L) >> (32 - count));

                    stack[stack_ptr - 1] =
                        ((a & long_temp) == (b & long_temp)) ? 1L : 0L;
                }
            }
            break;

        case 0x13: /* PINT */
            /*
            *   PRINT add integer
            *   ...stack 0 is integer value
            */
            IF_CHECK_STACK(1)
            {
                jbi_ltoa(&message_buffer[jbi_strlen(message_buffer)],
                    stack[--stack_ptr]);
            }
            break;

        case 0x14: /* PRNT */
            /*
            *   PRINT finish
            */
            jbi_message(message_buffer);
            message_buffer[0] = '\0';
            break;

        case 0x15: /* DSS  */
            /*
            *   DRSCAN short
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_do_drscan(count, charbuf, 0);
            }
            break;

        case 0x16: /* DSSC */
            /*
            *   DRSCAN short with capture
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[--stack_ptr];
                count = (unsigned int) stack[stack_ptr - 1];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_swap_dr(count, charbuf, 0, charbuf, 0);
                stack[stack_ptr - 1] = jbi_get_dword(charbuf);
            }
            break;

        case 0x17: /* ISS  */
            /*
            *   IRSCAN short
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_do_irscan(count, charbuf, 0);
            }
            break;

        case 0x18: /* ISSC */
            /*
            *   IRSCAN short with capture
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[--stack_ptr];
                count = (unsigned int) stack[stack_ptr - 1];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_swap_ir(count, charbuf, 0, charbuf, 0);
                stack[stack_ptr - 1] = jbi_get_dword(charbuf);
            }
            break;

        case 0x19: /* VSS  */
            /*
            *   VECTOR short
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            bad_opcode = 1;
            break;

        case 0x1A: /* VSSC */
            /*
            *   VECTOR short with capture
            *   ...stack 0 is scan data
            *   ...stack 1 is count
            */
            bad_opcode = 1;
            break;

        case 0x1B: /* VMPF */
            /*
            *   VMAP finish
            */
            bad_opcode = 1;
            break;

        case 0x1C: /* DPR  */
            IF_CHECK_STACK(1)
            {
                count = (unsigned int) stack[--stack_ptr];
                status = jbi_set_dr_preamble(count, 0, NULL);
            }
            break;

        case 0x1D: /* DPRL */
            /*
            *   DRPRE with literal data
            *   ...stack 0 is count
            *   ...stack 1 is literal data
            */
            IF_CHECK_STACK(2)
            {
                count = (unsigned int) stack[--stack_ptr];
                long_temp = stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_set_dr_preamble(count, 0, charbuf);
            }
            break;

        case 0x1E: /* DPO  */
            /*
            *   DRPOST
            *   ...stack 0 is count
            */
            IF_CHECK_STACK(1)
            {
                count = (unsigned int) stack[--stack_ptr];
                status = jbi_set_dr_postamble(count, 0, NULL);
            }
            break;

        case 0x1F: /* DPOL */
            /*
            *   DRPOST with literal data
            *   ...stack 0 is count
            *   ...stack 1 is literal data
            */
            IF_CHECK_STACK(2)
            {
                count = (unsigned int) stack[--stack_ptr];
                long_temp = stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_set_dr_postamble(count, 0, charbuf);
            }
            break;

        case 0x20: /* IPR  */
            IF_CHECK_STACK(1)
            {
                count = (unsigned int) stack[--stack_ptr];
                status = jbi_set_ir_preamble(count, 0, NULL);
            }
            break;

        case 0x21: /* IPRL */
            /*
            *   IRPRE with literal data
            *   ...stack 0 is count
            *   ...stack 1 is literal data
            */
            IF_CHECK_STACK(2)
            {
                count = (unsigned int) stack[--stack_ptr];
                long_temp = stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_set_ir_preamble(count, 0, charbuf);
            }
            break;

        case 0x22: /* IPO  */
            /*
            *   IRPOST
            *   ...stack 0 is count
            */
            IF_CHECK_STACK(1)
            {
                count = (unsigned int) stack[--stack_ptr];
                status = jbi_set_ir_postamble(count, 0, NULL);
            }
            break;

        case 0x23: /* IPOL */
            /*
            *   IRPOST with literal data
            *   ...stack 0 is count
            *   ...stack 1 is literal data
            */
            IF_CHECK_STACK(2)
            {
                count = (unsigned int) stack[--stack_ptr];
                long_temp = stack[--stack_ptr];
                jbi_make_dword(charbuf, long_temp);
                status = jbi_set_ir_postamble(count, 0, charbuf);
            }
            break;

        case 0x24: /* PCHR */
            IF_CHECK_STACK(1)
            {
                unsigned char ch;
                count = jbi_strlen(message_buffer);
                ch = (char) stack[--stack_ptr];
                if ((ch < 1) || (ch > 127))
                {
                    /* character code out of range */
                    /* instead of flagging an error, force the value to 127 */
                    ch = 127;
                }
                message_buffer[count] = ch;
                message_buffer[count + 1] = '\0';
            }
            break;

        case 0x25: /* EXIT */
            IF_CHECK_STACK(1)
            {
                *exit_code = (int) stack[--stack_ptr];
            }
            done = 1;
            break;

        case 0x26: /* EQU  */
            IF_CHECK_STACK(2)
            {
                --stack_ptr;
                stack[stack_ptr - 1] =
                    (stack[stack_ptr - 1] == stack[stack_ptr]) ? 1L : 0L;
            }
            break;

        case 0x27: /* POPT  */
            IF_CHECK_STACK(1)
            {
                --stack_ptr;
            }
            break;

        case 0x28: /* TRST  */
            bad_opcode = 1;
            break;

        case 0x29: /* FRQ   */
            bad_opcode = 1;
            break;

        case 0x2A: /* FRQU  */
            bad_opcode = 1;
            break;

        case 0x2B: /* PD32  */
            bad_opcode = 1;
            break;

        case 0x2C: /* ABS   */
            IF_CHECK_STACK(1)
            {
                if (stack[stack_ptr - 1] < 0)
                {
                    stack[stack_ptr - 1] = 0 - stack[stack_ptr - 1];
                }
            }
            break;

        case 0x2D: /* BCH0  */
            /*
            *   Batch operation 0
            *   SWP
            *   SWPN 7
            *   SWP
            *   SWPN 6
            *   DUPN 8
            *   SWPN 2
            *   SWP
            *   DUPN 6
            *   DUPN 6
            */

            /* SWP  */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[stack_ptr - 2];
                stack[stack_ptr - 2] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* SWPN 7 */
            index = 7 + 1;
            IF_CHECK_STACK(index)
            {
                long_temp = stack[stack_ptr - index];
                stack[stack_ptr - index] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* SWP  */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[stack_ptr - 2];
                stack[stack_ptr - 2] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* SWPN 6 */
            index = 6 + 1;
            IF_CHECK_STACK(index)
            {
                long_temp = stack[stack_ptr - index];
                stack[stack_ptr - index] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* DUPN 8 */
            index = 8 + 1;
            IF_CHECK_STACK(index)
            {
                stack[stack_ptr] = stack[stack_ptr - index];
                ++stack_ptr;
            }

            /* SWPN 2 */
            index = 2 + 1;
            IF_CHECK_STACK(index)
            {
                long_temp = stack[stack_ptr - index];
                stack[stack_ptr - index] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* SWP  */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[stack_ptr - 2];
                stack[stack_ptr - 2] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }

            /* DUPN 6 */
            index = 6 + 1;
            IF_CHECK_STACK(index)
            {
                stack[stack_ptr] = stack[stack_ptr - index];
                ++stack_ptr;
            }

            /* DUPN 6 */
            index = 6 + 1;
            IF_CHECK_STACK(index)
            {
                stack[stack_ptr] = stack[stack_ptr - index];
                ++stack_ptr;
            }
            break;

        case 0x2E: /* BCH1  */
            /*
            *   Batch operation 1
            *   SWPN 8
            *   SWP
            *   SWPN 9
            *   SWPN 3
            *   SWP
            *   SWPN 2
            *   SWP
            *   SWPN 7
            *   SWP
            *   SWPN 6
            *   DUPN 5
            *   DUPN 5
            */
            bad_opcode = 1;
            break;

        case 0x2F: /* PSH0  */
            stack[stack_ptr++] = 0;
            break;

        case 0x40: /* PSHL */
            stack[stack_ptr++] = (long) args[0];
            break;

        case 0x41: /* PSHV */
            stack[stack_ptr++] = variables[args[0]];
            break;

        case 0x42: /* JMP  */
            pc = args[0] + code_section;
            CHECK_PC;
            break;

        case 0x43: /* CALL */
            stack[stack_ptr++] = pc;
            pc = args[0] + code_section;
            CHECK_PC;
            break;

        case 0x44: /* NEXT */
            /*
            *   Process FOR / NEXT loop
            *   ...argument 0 is variable ID
            *   ...stack 0 is step value
            *   ...stack 1 is end value
            *   ...stack 2 is top address
            */
            IF_CHECK_STACK(3)
            {
                long step = stack[stack_ptr - 1];
                long end = stack[stack_ptr - 2];
                long top = stack[stack_ptr - 3];
                long iterator = variables[args[0]];
                int break_out = 0;

                if (step < 0)
                {
                    if (iterator <= end) break_out = 1;
                }
                else
                {
                    if (iterator >= end) break_out = 1;
                }

                if (break_out)
                {
                    stack_ptr -= 3;
                }
                else
                {
                    variables[args[0]] = iterator + step;
                    pc = top + code_section;
                    CHECK_PC;
                }
            }
            break;

        case 0x45: /* PSTR */
            /*
            *   PRINT add string
            *   ...argument 0 is string ID
            */
#if PORT==DOS
            long_index = string_table + args[0];
            index2 = jbi_strlen(message_buffer);

            do
            {
                i = GET_BYTE(long_index);
                message_buffer[index2] = (char) i;
                ++long_index;
                ++index2;
            }
            while ((i != '\0') && (index2 < JBIC_MESSAGE_LENGTH));
#else
            count = jbi_strlen(message_buffer);
            jbi_strncpy(&message_buffer[count],
                (char *) &program[string_table + args[0]],
                JBIC_MESSAGE_LENGTH - count);
#endif
            message_buffer[JBIC_MESSAGE_LENGTH] = '\0';
            break;

        case 0x46: /* VMAP */
            /*
            *   VMAP add signal name
            *   ...argument 0 is string ID
            */
            bad_opcode = 1;
            break;

        case 0x47: /* SINT */
            /*
            *   STATE intermediate state
            *   ...argument 0 is state code
            */
            status = jbi_goto_jtag_state((int) args[0]);
            break;

        case 0x48: /* ST   */
            /*
            *   STATE final state
            *   ...argument 0 is state code
            */
            status = jbi_goto_jtag_state((int) args[0]);
            break;

        case 0x49: /* ISTP */
            /*
            *   IRSTOP state
            *   ...argument 0 is state code
            */
            status = jbi_set_irstop_state((int) args[0]);
            break;

        case 0x4A: /* DSTP */
            /*
            *   DRSTOP state
            *   ...argument 0 is state code
            */
            status = jbi_set_drstop_state((int) args[0]);
            break;

        case 0x4B: /* SWPN */
            /*
            *   Exchange top with Nth stack value
            *   ...argument 0 is 0-based stack entry to swap with top element
            */
            index = ((int) args[0]) + 1;
            IF_CHECK_STACK(index)
            {
                long_temp = stack[stack_ptr - index];
                stack[stack_ptr - index] = stack[stack_ptr - 1];
                stack[stack_ptr - 1] = long_temp;
            }
            break;

        case 0x4C: /* DUPN */
            /*
            *   Duplicate Nth stack value
            *   ...argument 0 is 0-based stack entry to duplicate
            */
            index = ((int) args[0]) + 1;
            IF_CHECK_STACK(index)
            {
                stack[stack_ptr] = stack[stack_ptr - index];
                ++stack_ptr;
            }
            break;

        case 0x4D: /* POPV */
            /*
            *   Pop stack into scalar variable
            *   ...argument 0 is variable ID
            *   ...stack 0 is value
            */
            IF_CHECK_STACK(1)
            {
                variables[args[0]] = stack[--stack_ptr];
            }
            break;

        case 0x4E: /* POPE */
            /*
            *   Pop stack into integer array element
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is value
            */
            IF_CHECK_STACK(2)
            {
                variable_id = (unsigned int) args[0];

                /*
                *   If variable is read-only, convert to writable array
                */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x1c))
                {
                    /*
                    *   Allocate a writable buffer for this array
                    */
                    count = (unsigned int) variable_size[variable_id];
                    long_temp = variables[variable_id];
                    longptr_temp = (long *) jbi_malloc(count * sizeof(long));
                    variables[variable_id] = (addr_t) longptr_temp;

                    if (variables[variable_id] == (addr_t) NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                        break;
                    }
                    else
                    {
                        /* copy previous contents into buffer */
                        for (i = 0; i < count; ++i)
                        {
                            longptr_temp[i] = GET_DWORD(long_temp);
                            long_temp += 4L;
                        }

                        /* set bit 7 - buffer was dynamically allocated */
                        attributes[variable_id] |= 0x80;

                        /* clear bit 2 - variable is writable */
                        attributes[variable_id] &= ~0x04;
                        attributes[variable_id] |= 0x01;
                    }
                }

#if PORT==DOS
                /* for 16-bit version, allow writing in allocated buffers */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x9c))
                {
                    attributes[variable_id] &= ~0x04;
                    attributes[variable_id] |= 0x01;
                }
#endif

                /* check that variable is a writable integer array */
                if ((attributes[variable_id] & 0x1c) != 0x18)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
                    longptr_temp = (long *) variables[variable_id];

                    /* pop the array index */
                    index = (unsigned int) stack[--stack_ptr];

                    /* pop the value and store it into the array */
                    longptr_temp[index] = stack[--stack_ptr];
                }
            }
            break;

        case 0x4F: /* POPA */
            /*
            *   Pop stack into Boolean array
            *   ...argument 0 is variable ID
            *   ...stack 0 is count
            *   ...stack 1 is array index
            *   ...stack 2 is value
            */
            IF_CHECK_STACK(3)
            {
                variable_id = (unsigned int) args[0];

                /*
                *   If variable is read-only, convert to writable array
                */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x0c))
                {
                    /*
                    *   Allocate a writable buffer for this array
                    */
                    long_temp = (variable_size[variable_id] + 7L) >> 3L;
                    charptr_temp2 = (unsigned char *) variables[variable_id];
                    charptr_temp = jbi_malloc((unsigned int) long_temp);
                    variables[variable_id] = (addr_t) charptr_temp;

                    if (variables[variable_id] == (addr_t) NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                    }
                    else
                    {
                        /* zero the buffer */
                        for (long_index = 0L;
                            long_index < long_temp;
                            ++long_index)
                        {
                            charptr_temp[long_index] = 0;
                        }

                        /* copy previous contents into buffer */
                        for (long_index = 0L;
                            long_index < variable_size[variable_id];
                            ++long_index)
                        {
#if PORT==DOS
                            if ((attributes[variable_id] & 0x02) &&
                                ((long_index & 0x0000FFFF) == 0L))
                            {
                                /* initialized compressed Boolean array */
                                jbi_uncompress_page(variable_id,
                                    (int) (long_index >> 16), version);
                                charptr_temp = jbi_aca_out_buffer;
                                long_index2 = long_index & 0xFFFF;
                            }
#else
                            long_index2 = long_index;
#endif

                            if (charptr_temp2[long_index2 >> 3] &
                                (1 << (long_index2 & 7)))
                            {
                                charptr_temp[long_index >> 3] |=
                                    (1 << (long_index & 7));
                            }
                        }

                        /* set bit 7 - buffer was dynamically allocated */
                        attributes[variable_id] |= 0x80;

                        /* clear bit 2 - variable is writable */
                        attributes[variable_id] &= ~0x04;
                        attributes[variable_id] |= 0x01;
                    }
                }

#if PORT==DOS
                /* for 16-bit version, allow writing in allocated buffers */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x8c))
                {
                    attributes[variable_id] &= ~0x04;
                    attributes[variable_id] |= 0x01;
                }
#endif

                /* check that variable is a writable Boolean array */
                if ((attributes[variable_id] & 0x1c) != 0x08)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
                    charptr_temp = (unsigned char *) variables[variable_id];

                    /* pop the count (number of bits to copy) */
                    long_count = stack[--stack_ptr];

                    /* pop the array index */
                    long_index = stack[--stack_ptr];

                    reverse = 0;

                    if (version > 0)
                    {
                        /* stack 0 = array right index */
                        /* stack 1 = array left index */

                        if (long_index > long_count)
                        {
                            reverse = 1;
                            long_temp = long_count;
                            long_count = 1 + long_index - long_count;
                            long_index = long_temp;

                            /* reverse POPA is not supported */
                            status = JBIC_BOUNDS_ERROR;
                            break;
                        }
                        else
                        {
                            long_count = 1 + long_count - long_index;
                        }
                    }

                    /* pop the data */
                    long_temp = stack[--stack_ptr];

                    if (long_count < 1)
                    {
                        status = JBIC_BOUNDS_ERROR;
                    }
                    else
                    {
                        for (i = 0; i < (unsigned int) long_count; ++i)
                        {
                            if (long_temp & (1L << (long) i))
                            {
                                charptr_temp[long_index >> 3L] |=
                                    (1L << (long_index & 7L));
                            }
                            else
                            {
                                charptr_temp[long_index >> 3L] &=
                                    ~ (unsigned int) (1L << (long_index & 7L));
                            }
                            ++long_index;
                        }
                    }
                }
            }
            break;

        case 0x50: /* JMPZ */
            /*
            *   Pop stack and branch if zero
            *   ...argument 0 is address
            *   ...stack 0 is condition value
            */
            IF_CHECK_STACK(1)
            {
                if (stack[--stack_ptr] == 0)
                {
                    pc = args[0] + code_section;
                    CHECK_PC;
                }
            }
            break;

        case 0x51: /* DS   */
        case 0x52: /* IS   */
            /*
            *   DRSCAN
            *   IRSCAN
            *   ...argument 0 is scan data variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                long_index = stack[--stack_ptr];
                long_count = stack[--stack_ptr];

                reverse = 0;

                if (version > 0)
                {
                    /* stack 0 = array right index */
                    /* stack 1 = array left index */
                    /* stack 2 = count */
                    long_temp = long_count;
                    long_count = stack[--stack_ptr];

                    if (long_index > long_temp)
                    {
                        reverse = 1;
                        long_index = long_temp;
                    }
                }

#if PORT==DOS
                if (((long_index & 0xFFFF0000) == 0) &&
                    ((long_count & 0xFFFF0000) == 0))
                {
                    variable_id = (unsigned int) args[0];
                    if ((attributes[variable_id] & 0x1e) == 0x0e)
                    {
                        /* initialized compressed Boolean array */
                        jbi_uncompress_page(variable_id,
                            (int) (long_index >> 16), version);
                        long_index &= 0x0000ffff;
                        charptr_temp = jbi_aca_out_buffer;
                    }
                    else
                    {
                        charptr_temp = (unsigned char *) variables[variable_id];
                    }

                    if (reverse)
                    {
                        /* allocate a buffer and reverse the data order */
                        charptr_temp2 = charptr_temp;
                        charptr_temp = jbi_malloc((unsigned int)
                            ((long_count >> 3L) + 1L));

                        if (charptr_temp == NULL)
                        {
                            status = JBIC_OUT_OF_MEMORY;
                            break;
                        }
                        else
                        {
                            long_temp = long_index + long_count - 1;
                            long_index2 = 0;
                            while (long_index2 < long_count)
                            {
                                if (charptr_temp2[long_temp >> 3] &
                                    (1 << (long_temp & 7)))
                                {
                                    charptr_temp[long_index2 >> 3] |=
                                        (1 << (long_index2 & 7));
                                }
                                else
                                {
                                    charptr_temp[long_index2 >> 3] &=
                                        ~(1 << (long_index2 & 7));
                                }

                                --long_temp;
                                ++long_index2;
                            }
                        }
                    }

                    if (opcode == 0x51) /* DS */
                    {
                        status = jbi_do_drscan((unsigned int) long_count,
                            charptr_temp, (unsigned long) long_index);
                    }
                    else    /* IS */
                    {
                        status = jbi_do_irscan((unsigned int) long_count,
                            charptr_temp, (unsigned int) long_index);
                    }

                    if (reverse) jbi_free(charptr_temp);
                }
                else if ((opcode == 0x51) && !reverse)
                {
                    status = jbi_do_drscan_multi_page(
                        (unsigned int) args[0],
                        (unsigned long) long_count,
                        (unsigned long) long_index, version);
                }
                else
                {
                    /* reverse multi-page scans are not supported */
                    /* multi-page IR scans are not supported */
                    status = JBIC_BOUNDS_ERROR;
                }
#else
                charptr_temp = (unsigned char *) variables[args[0]];

                if (reverse)
                {
                    /* allocate a buffer and reverse the data order */
                    charptr_temp2 = charptr_temp;
                    charptr_temp = jbi_malloc((long_count >> 3) + 1);
                    if (charptr_temp == NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                        break;
                    }
                    else
                    {
                        long_temp = long_index + long_count - 1;
                        long_index2 = 0;
                        while (long_index2 < long_count)
                        {
                            if (charptr_temp2[long_temp >> 3] &
                                (1 << (long_temp & 7)))
                            {
                                charptr_temp[long_index2 >> 3] |=
                                    (1 << (long_index2 & 7));
                            }
                            else
                            {
                                charptr_temp[long_index2 >> 3] &=
                                    ~(1 << (long_index2 & 7));
                            }

                            --long_temp;
                            ++long_index2;
                        }
                    }
                }

                if (opcode == 0x51) /* DS */
                {
                    status = jbi_do_drscan((unsigned int) long_count,
                        charptr_temp, (unsigned long) long_index);
                }
                else    /* IS */
                {
                    status = jbi_do_irscan((unsigned int) long_count,
                        charptr_temp, (unsigned int) long_index);
                }
#endif

                if (reverse && (charptr_temp != NULL))
                {
                    jbi_free(charptr_temp);
                }
            }
            break;

        case 0x53: /* DPRA */
            /*
            *   DRPRE with array data
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                index = (unsigned int) stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];

                if (version > 0)
                {
                    /* stack 0 = array right index */
                    /* stack 1 = array left index */
                    count = 1 + count - index;
                }

                charptr_temp = (unsigned char *) variables[args[0]];
                status = jbi_set_dr_preamble(count, index, charptr_temp);
            }
            break;

        case 0x54: /* DPOA */
            /*
            *   DRPOST with array data
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                index = (unsigned int) stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];

                if (version > 0)
                {
                    /* stack 0 = array right index */
                    /* stack 1 = array left index */
                    count = 1 + count - index;
                }

                charptr_temp = (unsigned char *) variables[args[0]];
                status = jbi_set_dr_postamble(count, index, charptr_temp);
            }
            break;

        case 0x55: /* IPRA */
            /*
            *   IRPRE with array data
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                index = (unsigned int) stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];

                if (version > 0)
                {
                    /* stack 0 = array right index */
                    /* stack 1 = array left index */
                    count = 1 + count - index;
                }

                charptr_temp = (unsigned char *) variables[args[0]];
                status = jbi_set_ir_preamble(count, index, charptr_temp);
            }
            break;

        case 0x56: /* IPOA */
            /*
            *   IRPOST with array data
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            *   ...stack 1 is count
            */
            IF_CHECK_STACK(2)
            {
                index = (unsigned int) stack[--stack_ptr];
                count = (unsigned int) stack[--stack_ptr];

                if (version > 0)
                {
                    /* stack 0 = array right index */
                    /* stack 1 = array left index */
                    count = 1 + count - index;
                }

                charptr_temp = (unsigned char *) variables[args[0]];
                status = jbi_set_ir_postamble(count, index, charptr_temp);
            }
            break;

        case 0x57: /* EXPT */
            /*
            *   EXPORT
            *   ...argument 0 is string ID
            *   ...stack 0 is integer expression
            */
            IF_CHECK_STACK(1)
            {
#if PORT==DOS
                name_id = args[0];
                for (j = 0; j < 32; ++j)
                {
                    name[j] = GET_BYTE(string_table + name_id + j);
                }
                name[32] = '\0';
#else
                name = (char *) &program[string_table + args[0]];
#endif
                long_temp = stack[--stack_ptr];
                jbi_export_integer(name, long_temp);
            }
            break;

        case 0x58: /* PSHE */
            /*
            *   Push integer array element
            *   ...argument 0 is variable ID
            *   ...stack 0 is array index
            */
            IF_CHECK_STACK(1)
            {
                variable_id = (unsigned int) args[0];
                index = (unsigned int) stack[stack_ptr - 1];

                /* check variable type */
                if ((attributes[variable_id] & 0x1f) == 0x19)
                {
                    /* writable integer array */
                    longptr_temp = (long *) variables[variable_id];
                    stack[stack_ptr - 1] = longptr_temp[index];
                }
                else if ((attributes[variable_id] & 0x1f) == 0x1c)
                {
                    /* read-only integer array */
                    long_temp = variables[variable_id] + (4L * index);
                    stack[stack_ptr - 1] = GET_DWORD(long_temp);
                }
                else
                {
                    status = JBIC_BOUNDS_ERROR;
                }
            }
            break;

        case 0x59: /* PSHA */
            /*
            *   Push Boolean array
            *   ...argument 0 is variable ID
            *   ...stack 0 is count
            *   ...stack 1 is array index
            */
            IF_CHECK_STACK(2)
            {
                variable_id = (unsigned int) args[0];

                /* check that variable is a Boolean array */
                if ((attributes[variable_id] & 0x18) != 0x08)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
                    charptr_temp = (unsigned char *) variables[variable_id];

                    /* pop the count (number of bits to copy) */
                    count = (unsigned int) stack[--stack_ptr];

                    /* pop the array index */
                    index = (unsigned int) stack[stack_ptr - 1];

                    if (version > 0)
                    {
                        /* stack 0 = array right index */
                        /* stack 1 = array left index */
                        count = 1 + count - index;
                    }

                    if ((count < 1) || (count > 32))
                    {
                        status = JBIC_BOUNDS_ERROR;
                    }
                    else
                    {
#if PORT==DOS
                        if ((attributes[variable_id] & 0x1e) == 0x0e)
                        {
                            /* initialized compressed Boolean array */
                            jbi_uncompress_page(variable_id,
                                (int) (stack[stack_ptr - 1] >> 16), version);
                            charptr_temp = jbi_aca_out_buffer;
                        }
#endif
                        long_temp = 0L;

                        for (i = 0; i < count; ++i)
                        {
                            if (charptr_temp[(i + index) >> 3] &
                                (1 << ((i + index) & 7)))
                            {
                                long_temp |= (1L << i);
                            }
                        }

                        stack[stack_ptr - 1] = long_temp;
                    }
                }
            }
            break;

        case 0x5A: /* DYNA */
            /*
            *   Dynamically change size of array
            *   ...argument 0 is variable ID
            *   ...stack 0 is new size
            */
            IF_CHECK_STACK(1)
            {
                variable_id = (unsigned int) args[0];
                long_temp = stack[--stack_ptr];

                if (long_temp > variable_size[variable_id])
                {
                    variable_size[variable_id] = long_temp;

                    if (attributes[variable_id] & 0x10)
                    {
                        /* allocate integer array */
                        long_temp *= 4;
                    }
                    else
                    {
                        /* allocate Boolean array */
                        long_temp = (long_temp + 7) >> 3;
                    }

                    /*
                    *   If the buffer was previously allocated, free it
                    */
                    if ((attributes[variable_id] & 0x80) &&
                        (variables[variable_id] != (addr_t) NULL))
                    {
                        jbi_free((void *) variables[variable_id]);
                        variables[variable_id] = (addr_t) NULL;
                    }

                    /*
                    *   Allocate a new buffer of the requested size
                    */
                    variables[variable_id] = (addr_t)
                        jbi_malloc((unsigned int) long_temp);

                    if (variables[variable_id] == (addr_t) NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                    }
                    else
                    {
                        /*
                        *   Set the attribute bit to indicate that this buffer
                        *   was dynamically allocated and should be freed later
                        */
                        attributes[variable_id] |= 0x80;

                        /* zero out memory */
                        count = (unsigned int)
                            ((variable_size[variable_id] + 7L) / 8L);
                        charptr_temp = (unsigned char *)
                            (variables[variable_id]);
                        for (index = 0; index < count; ++index)
                        {
                            charptr_temp[index] = 0;
                        }
                    }
                }
            }
            break;

        case 0x5B: /* EXPR */
            bad_opcode = 1;
            break;

        case 0x5C: /* EXPV */
            /*
            *   Export Boolean array
            *   ...argument 0 is string ID
            *   ...stack 0 is variable ID
            *   ...stack 1 is array right index
            *   ...stack 2 is array left index
            */
            IF_CHECK_STACK(3)
            {
                if (version == 0)
                {
                    /* EXPV is not supported in JBC 1.0 */
                    bad_opcode = 1;
                    break;
                }
#if PORT==DOS
                name_id = args[0];
                for (j = 0; j < 32; ++j)
                {
                    name[j] = GET_BYTE(string_table + name_id + j);
                }
                name[32] = '\0';
#else
                name = (char *) &program[string_table + args[0]];
#endif
                variable_id = (unsigned int) stack[--stack_ptr];
                long_index = stack[--stack_ptr];    /* right index */
                long_index2 = stack[--stack_ptr];   /* left index */

                if (long_index > long_index2)
                {
                    /* reverse indices not supported */
                    status = JBIC_BOUNDS_ERROR;
                    break;
                }

                long_count = 1 + long_index2 - long_index;

                charptr_temp = (unsigned char *) variables[variable_id];
                charptr_temp2 = NULL;

#if PORT==DOS
                if ((attributes[variable_id] & 0x1e) == 0x0e)
                {
                    /* initialized compressed Boolean array */
                    jbi_uncompress_page(variable_id,
                        (int) (long_index >> 16), version);
                    charptr_temp = jbi_aca_out_buffer;
                    long_index &= 0x0000FFFF;
                }
#endif

                if ((long_index & 7L) != 0)
                {
                    charptr_temp2 = jbi_malloc((unsigned int)
                        ((long_count + 7L) / 8L));
                    if (charptr_temp2 == NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                        break;
                    }
                    else
                    {
                        long k = long_index;
                        for (i = 0; i < (unsigned int) long_count; ++i)
                        {
                            if (charptr_temp[k >> 3] & (1 << (k & 7)))
                            {
                                charptr_temp2[i >> 3] |= (1 << (i & 7));
                            }
                            else
                            {
                                charptr_temp2[i >> 3] &= ~(1 << (i & 7));
                            }

                            ++k;
                        }
                        charptr_temp = charptr_temp2;
                    }
                }
                else if (long_index != 0)
                {
                    charptr_temp = &charptr_temp[long_index >> 3];
                }

                jbi_export_boolean_array(name, charptr_temp, long_count);

                /* free allocated buffer */
                if (((long_index & 7L) != 0) && (charptr_temp2 != NULL))
                {
                    jbi_free(charptr_temp2);
                }
            }
            break;

        case 0x80: /* COPY */
            /*
            *   Array copy
            *   ...argument 0 is dest ID
            *   ...argument 1 is source ID
            *   ...stack 0 is count
            *   ...stack 1 is dest index
            *   ...stack 2 is source index
            */
            IF_CHECK_STACK(3)
            {
                long copy_count = stack[--stack_ptr];
                long copy_index = stack[--stack_ptr];
                long copy_index2 = stack[--stack_ptr];
                long destleft;
                long src_count;
                long dest_count;
                int src_reverse = 0;
                int dest_reverse = 0;

                reverse = 0;

                if (version > 0)
                {
                    /* stack 0 = source right index */
                    /* stack 1 = source left index */
                    /* stack 2 = destination right index */
                    /* stack 3 = destination left index */
                    destleft = stack[--stack_ptr];

                    if (copy_count > copy_index)
                    {
                        src_reverse = 1;
                        reverse = 1;
                        src_count = 1 + copy_count - copy_index;
                        /* copy_index = source start index */
                    }
                    else
                    {
                        src_count = 1 + copy_index - copy_count;
                        copy_index = copy_count;    /* source start index */
                    }

                    if (copy_index2 > destleft)
                    {
                        dest_reverse = 1;
                        reverse = !reverse;
                        dest_count = 1 + copy_index2 - destleft;
                        copy_index2 = destleft; /* destination start index */
                    }
                    else
                    {
                        dest_count = 1 + destleft - copy_index2;
                        /* copy_index2 = destination start index */
                    }

                    copy_count = (src_count < dest_count) ? src_count : dest_count;

                    if ((src_reverse || dest_reverse) &&
                        (src_count != dest_count))
                    {
                        /* If either the source or destination is reversed, */
                        /* we can't tolerate a length mismatch, because we  */
                        /* "left justify" the arrays when copying.  This    */
                        /* won't work correctly with reversed arrays.       */
                        status = JBIC_BOUNDS_ERROR;
                    }
                }

                count = (unsigned int) copy_count;
                index = (unsigned int) copy_index;
                index2 = (unsigned int) copy_index2;

                /*
                *   If destination is a read-only array, allocate a buffer
                *   and convert it to a writable array
                */
                variable_id = (unsigned int) args[1];
                if ((version > 0) && ((attributes[variable_id] & 0x9c) == 0x0c))
                {
                    /*
                    *   Allocate a writable buffer for this array
                    */
                    long_temp = (variable_size[variable_id] + 7L) >> 3L;
                    charptr_temp2 = (unsigned char *) variables[variable_id];
                    charptr_temp = jbi_malloc((unsigned int) long_temp);
                    variables[variable_id] = (addr_t) charptr_temp;

                    if (variables[variable_id] == (addr_t) NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                        break;
                    }
                    else
                    {
                        /* zero the buffer */
                        for (long_index = 0L;
                            long_index < long_temp;
                            ++long_index)
                        {
                            charptr_temp[long_index] = 0;
                        }

                        /* copy previous contents into buffer */
                        for (long_index = 0L;
                            long_index < variable_size[variable_id];
                            ++long_index)
                        {
#if PORT==DOS
                            if ((attributes[variable_id] & 0x02) &&
                                ((long_index & 0x0000FFFF) == 0L))
                            {
                                /* initialized compressed Boolean array */
                                jbi_uncompress_page(variable_id,
                                    (int) (long_index >> 16), version);
                                charptr_temp = jbi_aca_out_buffer;
                                long_index2 = long_index & 0xFFFF;
                            }
#else
                            long_index2 = long_index;
#endif

                            if (charptr_temp2[long_index2 >> 3] &
                                (1 << (long_index2 & 7)))
                            {
                                charptr_temp[long_index >> 3] |=
                                    (1 << (long_index & 7));
                            }
                        }

                        /* set bit 7 - buffer was dynamically allocated */
                        attributes[variable_id] |= 0x80;

                        /* clear bit 2 - variable is writable */
                        attributes[variable_id] &= ~0x04;
                        attributes[variable_id] |= 0x01;
                    }
                }

#if PORT==DOS
                /* for 16-bit version, allow writing in allocated buffers */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x8c))
                {
                    attributes[variable_id] &= ~0x04;
                    attributes[variable_id] |= 0x01;
                }
#endif

                charptr_temp = (unsigned char *) variables[args[1]];
                charptr_temp2 = (unsigned char *) variables[args[0]];

#if PORT==DOS
                variable_id = (unsigned int) args[0];
                if ((attributes[variable_id] & 0x1e) == 0x0e)
                {
                    /* initialized compressed Boolean array */
                    jbi_uncompress_page(variable_id,
                        (int) (copy_index >> 16), version);
                    charptr_temp2 = jbi_aca_out_buffer;
                }
#endif

                /* check that destination is a writable Boolean array */
                if ((attributes[args[1]] & 0x1c) != 0x08)
                {
                    status = JBIC_BOUNDS_ERROR;
                    break;
                }

                if (count < 1)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
                    if (reverse)
                    {
                        index2 += (count - 1);
                    }

                    for (i = 0; i < count; ++i)
                    {
                        if (charptr_temp2[index >> 3] & (1 << (index & 7)))
                        {
                            charptr_temp[index2 >> 3] |= (1 << (index2 & 7));
                        }
                        else
                        {
                            charptr_temp[index2 >> 3] &=
                                ~(unsigned int) (1 << (index2 & 7));
                        }
                        ++index;
                        if (reverse) --index2; else ++index2;
                    }
                }
            }
            break;

        case 0x81: /* REVA */
            /*
            *   ARRAY COPY reversing bit order
            *   ...argument 0 is dest ID
            *   ...argument 1 is source ID
            *   ...stack 0 is dest index
            *   ...stack 1 is source index
            *   ...stack 2 is count
            */
            bad_opcode = 1;
            break;

        case 0x82: /* DSC  */
        case 0x83: /* ISC  */
            /*
            *   DRSCAN with capture
            *   IRSCAN with capture
            *   ...argument 0 is scan data variable ID
            *   ...argument 1 is capture variable ID
            *   ...stack 0 is capture index
            *   ...stack 1 is scan data index
            *   ...stack 2 is count
            */
            IF_CHECK_STACK(3)
            {
                long scan_right, scan_left;
                long capture_count = 0;
                long scan_count = 0;
                long capture_index = stack[--stack_ptr];
                long scan_index = stack[--stack_ptr];
                if (version > 0)
                {
                    /* stack 0 = capture right index */
                    /* stack 1 = capture left index */
                    /* stack 2 = scan right index */
                    /* stack 3 = scan left index */
                    /* stack 4 = count */
                    scan_right = stack[--stack_ptr];
                    scan_left = stack[--stack_ptr];
                    capture_count = 1 + scan_index - capture_index;
                    scan_count = 1 + scan_left - scan_right;
                    scan_index = scan_right;
                }
                long_count = stack[--stack_ptr];

                /*
                *   If capture array is read-only, allocate a buffer
                *   and convert it to a writable array
                */
                variable_id = (unsigned int) args[1];
                if ((version > 0) && ((attributes[variable_id] & 0x9c) == 0x0c))
                {
                    /*
                    *   Allocate a writable buffer for this array
                    */
                    long_temp = (variable_size[variable_id] + 7L) >> 3L;
                    charptr_temp2 = (unsigned char *) variables[variable_id];
                    charptr_temp = jbi_malloc((unsigned int) long_temp);
                    variables[variable_id] = (addr_t) charptr_temp;

                    if (variables[variable_id] == (addr_t) NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                        break;
                    }
                    else
                    {
                        /* zero the buffer */
                        for (long_index = 0L;
                            long_index < long_temp;
                            ++long_index)
                        {
                            charptr_temp[long_index] = 0;
                        }

                        /* copy previous contents into buffer */
                        for (long_index = 0L;
                            long_index < variable_size[variable_id];
                            ++long_index)
                        {
#if PORT==DOS
                            if ((attributes[variable_id] & 0x02) &&
                                ((long_index & 0x0000FFFF) == 0L))
                            {
                                /* initialized compressed Boolean array */
                                jbi_uncompress_page(variable_id,
                                    (int) (long_index >> 16), version);
                                charptr_temp = jbi_aca_out_buffer;
                                long_index2 = long_index & 0xFFFF;
                            }
#else
                            long_index2 = long_index;
#endif

                            if (charptr_temp2[long_index2 >> 3] &
                                (1 << (long_index2 & 7)))
                            {
                                charptr_temp[long_index >> 3] |=
                                    (1 << (long_index & 7));
                            }
                        }

                        /* set bit 7 - buffer was dynamically allocated */
                        attributes[variable_id] |= 0x80;

                        /* clear bit 2 - variable is writable */
                        attributes[variable_id] &= ~0x04;
                        attributes[variable_id] |= 0x01;
                    }
                }

#if PORT==DOS
                /* for 16-bit version, allow writing in allocated buffers */
                if ((version > 0) &&
                    ((attributes[variable_id] & 0x9c) == 0x8c))
                {
                    attributes[variable_id] &= ~0x04;
                    attributes[variable_id] |= 0x01;
                }
#endif

                charptr_temp = (unsigned char *) variables[args[0]];
                charptr_temp2 = (unsigned char *) variables[args[1]];

#if PORT==DOS
                variable_id = (unsigned int) args[0];
                if ((attributes[variable_id] & 0x1e) == 0x0e)
                {
                    /* initialized compressed Boolean array */
                    jbi_uncompress_page(variable_id,
                        (int) (scan_index >> 16), version);
                    scan_index &= 0x0000ffff;
                    charptr_temp = jbi_aca_out_buffer;
                }
#endif

                if ((version > 0) &&
                    ((long_count > capture_count) || (long_count > scan_count)))
                {
                    status = JBIC_BOUNDS_ERROR;
                }

                /* check that capture array is a writable Boolean array */
                if ((attributes[args[1]] & 0x1c) != 0x08)
                {
                    status = JBIC_BOUNDS_ERROR;
                }

                if (status == JBIC_SUCCESS)
                {
                    if (opcode == 0x82) /* DSC */
                    {
                        status = jbi_swap_dr((unsigned int) long_count,
                            charptr_temp, (unsigned long) scan_index,
                            charptr_temp2, (unsigned int) capture_index);
                    }
                    else /* ISC */
                    {
                        status = jbi_swap_ir((unsigned int) long_count,
                            charptr_temp, (unsigned int) scan_index,
                            charptr_temp2, (unsigned int) capture_index);
                    }
                }
            }
            break;

        case 0x84: /* WAIT */
            /*
            *   WAIT
            *   ...argument 0 is wait state
            *   ...argument 1 is end state
            *   ...stack 0 is cycles
            *   ...stack 1 is microseconds
            */
            IF_CHECK_STACK(2)
            {
                long_temp = stack[--stack_ptr];

                if (long_temp != 0L)
                {
                    status = jbi_do_wait_cycles(long_temp, (unsigned int) args[0]);
                }

                long_temp = stack[--stack_ptr];

                if ((status == JBIC_SUCCESS) && (long_temp != 0L))
                {
                    status = jbi_do_wait_microseconds(long_temp, (unsigned int) args[0]);
                }

                if ((status == JBIC_SUCCESS) && (args[1] != args[0]))
                {
                    status = jbi_goto_jtag_state((unsigned int) args[1]);
                }

                if (version > 0)
                {
                    --stack_ptr;    /* throw away MAX cycles */
                    --stack_ptr;    /* throw away MAX microseconds */
                }
            }
            break;

        case 0x85: /* VS   */
            /*
            *   VECTOR
            *   ...argument 0 is dir data variable ID
            *   ...argument 1 is scan data variable ID
            *   ...stack 0 is dir array index
            *   ...stack 1 is scan array index
            *   ...stack 2 is count
            */
            bad_opcode = 1;
            break;

        case 0xC0: /* CMPA */
            /*
            *   Array compare
            *   ...argument 0 is source 1 ID
            *   ...argument 1 is source 2 ID
            *   ...argument 2 is mask ID
            *   ...stack 0 is source 1 index
            *   ...stack 1 is source 2 index
            *   ...stack 2 is mask index
            *   ...stack 3 is count
            */
            IF_CHECK_STACK(4)
            {
                long a, b;
                unsigned char *source1 = (unsigned char *) variables[args[0]];
                unsigned char *source2 = (unsigned char *) variables[args[1]];
                unsigned char *mask    = (unsigned char *) variables[args[2]];
                unsigned long index1 = stack[--stack_ptr];
                unsigned long index2 = stack[--stack_ptr];
                unsigned long mask_index = stack[--stack_ptr];
                long_count = stack[--stack_ptr];

                if (version > 0)
                {
                    /* stack 0 = source 1 right index */
                    /* stack 1 = source 1 left index */
                    /* stack 2 = source 2 right index */
                    /* stack 3 = source 2 left index */
                    /* stack 4 = mask right index */
                    /* stack 5 = mask left index */
                    long mask_right = stack[--stack_ptr];
                    long mask_left = stack[--stack_ptr];
                    a = 1 + index2 - index1; /* source 1 count */
                    b = 1 + long_count - mask_index; /* source 2 count */
                    a = (a < b) ? a : b;
                    b = 1 + mask_left - mask_right; /* mask count */
                    a = (a < b) ? a : b;
                    index2 = mask_index;    /* source 2 start index */
                    mask_index = mask_right;    /* mask start index */
                    long_count = a;
                }

                long_temp = 1L;

                if (long_count < 1)
                {
                    status = JBIC_BOUNDS_ERROR;
                }
                else
                {
#if PORT==DOS
                    variable_id = (unsigned int) args[0];
                    if ((attributes[variable_id] & 0x1e) == 0x0e)
                    {
                        jbi_uncompress_page(variable_id,
                            (int) (index1 >> 16), version);
                        index1 &= 0x0000ffff;
                        source1 = jbi_aca_out_buffer;
                    }

                    variable_id = (unsigned int) args[1];
                    if ((attributes[variable_id] & 0x1e) == 0x0e)
                    {
                        jbi_uncompress_page(variable_id,
                            (int) (index2 >> 16), version);
                        index2 &= 0x0000ffff;
                        source2 = jbi_aca_out_buffer;
                    }
#endif
                    count = (unsigned int) long_count;

                    for (i = 0; i < count; ++i)
                    {
                        if (mask[mask_index >> 3] & (1 << (mask_index & 7)))
                        {
                            a = source1[index1 >> 3] & (1 << (index1 & 7))
                                ? 1 : 0;
                            b = source2[index2 >> 3] & (1 << (index2 & 7))
                                ? 1 : 0;

                            if (a != b) long_temp = 0L; /* failure */
                        }
                        ++index1;
                        ++index2;
                        ++mask_index;
                    }
                }

                stack[stack_ptr++] = long_temp;
            }
            break;

        case 0xC1: /* VSC  */
            /*
            *   VECTOR with capture
            *   ...argument 0 is dir data variable ID
            *   ...argument 1 is scan data variable ID
            *   ...argument 2 is capture variable ID
            *   ...stack 0 is capture index
            *   ...stack 1 is scan data index
            *   ...stack 2 is dir data index
            *   ...stack 3 is count
            */
            bad_opcode = 1;
            break;

        default:
            /*
            *   Unrecognized opcode -- ERROR!
            */
            bad_opcode = 1;
            break;
        }

        if (bad_opcode)
        {
            status = JBIC_ILLEGAL_OPCODE;
        }

        if ((stack_ptr < 0) || (stack_ptr >= JBI_STACK_SIZE))
        {
            status = JBIC_STACK_OVERFLOW;
        }

        if (status != JBIC_SUCCESS)
        {
            done = 1;
            *error_address = (long) (opcode_address - code_section);
        }
    }
    jbi_dbg(DEBUG_DETAIL, "debug_cnt(total): 0x%lx\n", debug_cnt);

    jbi_dbg(DEBUG_NOISY, "jbi_free_jtag_padding_buffers\n");
    jbi_free_jtag_padding_buffers(reset_jtag);

    /*
    *   Free all dynamically allocated arrays
    */
    jbi_dbg(DEBUG_NOISY, "jbi_free_attributes\n");
    if ((attributes != NULL) && (variables != NULL))
    {
        for (i = 0; i < (unsigned int) symbol_count; ++i)
        {
            if ((attributes[i] & 0x80) && (variables[i] != (addr_t) NULL)
                && (variables[i] != (addr_t) 1))
            {
                jbi_free((void *) variables[i]);
            }
        }
    }

    if (variables != NULL) jbi_free(variables);

    if (variable_size != NULL) jbi_free(variable_size);

    if (attributes != NULL) jbi_free(attributes);

    if (proc_attributes != NULL) jbi_free(proc_attributes);

    jbi_dbg(DEBUG_NOISY, "return status %d\n", status);
    kfree(message_buffer);
    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_get_note
(
    PROGRAM_PTR program,
    long program_size,
    long *offset,
    char *key,
    char *value,
    int length
)

/*                                                                          */
/*  Description:    Gets key and value of NOTE fields in the JBC file.      */
/*                  Can be called in two modes:  if offset pointer is NULL, */
/*                  then the function searches for note fields which match  */
/*                  the key string provided.  If offset is not NULL, then   */
/*                  the function finds the next note field of any key,      */
/*                  starting at the offset specified by the offset pointer. */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_UNEXPECTED_END;
    unsigned long note_strings = 0L;
    unsigned long note_table = 0L;
    unsigned long note_count = 0L;
    unsigned long first_word = 0L;
    int version = 0;
    int delta = 0;
    char *key_ptr;
    char *value_ptr;
    int i;

#if PORT==DOS
    int count = 0;
    int done = 0;
    long long_index = 0;
    char key_buffer[256];
    char value_buffer[256];

    jbi_program = program;
#endif

    /*
    *   Read header information
    */
    if (program_size > 52L)
    {
        first_word    = GET_DWORD(0);
        version = (int) (first_word & 1L);
        delta = version * 8;

        note_strings  = GET_DWORD(8 + delta);
        note_table    = GET_DWORD(12 + delta);
        note_count    = GET_DWORD(44 + (2 * delta));
    }

    if ((first_word != 0x4A414D00L) && (first_word != 0x4A414D01L))
    {
        status = JBIC_IO_ERROR;
    }
    else if (note_count > 0L)
    {
        if (offset == NULL)
        {
            /*
            *   We will search for the first note with a specific key, and
            *   return only the value
            */
            for (i = 0; (i < (int) note_count) && (status != JBIC_SUCCESS); ++i)
            {
#if PORT==DOS
                done = 0;
                count = 0;
                long_index = note_strings + GET_DWORD(note_table + (8 * i));
                while ((count < 255) && !done)
                {
                    key_buffer[count] = GET_BYTE(long_index);
                    if (key_buffer[count] == '\0') done = 1;
                    ++long_index;
                    ++count;
                }
                key_buffer[255] = '\0';
                key_ptr = key_buffer;
#else
                key_ptr = (char *) &program[note_strings +
                    GET_DWORD(note_table + (8 * i))];
#endif
                if ((key != NULL) && (jbi_stricmp(key, key_ptr) == 0))
                {
                    status = JBIC_SUCCESS;

#if PORT==DOS
                    done = 0;
                    count = 0;
                    long_index = note_strings + GET_DWORD(note_table + (8 * i) + 4);
                    while ((count < 255) && !done)
                    {
                        value_buffer[count] = GET_BYTE(long_index);
                        if (value_buffer[count] == '\0') done = 1;
                        ++long_index;
                        ++count;
                    }
                    value_buffer[255] = '\0';
                    value_ptr = value_buffer;
#else
                    value_ptr = (char *) &program[note_strings +
                        GET_DWORD(note_table + (8 * i) + 4)];
#endif

                    if (value != NULL)
                    {
                        jbi_strncpy(value, value_ptr, length);
                    }
                }
            }
        }
        else
        {
            /*
            *   We will search for the next note, regardless of the key, and
            *   return both the value and the key
            */

            i = (int) *offset;

            if ((i >= 0) && (i < (int) note_count))
            {
                status = JBIC_SUCCESS;

                if (key != NULL)
                {
#if PORT==DOS
                    done = 0;
                    count = 0;
                    long_index = note_strings +
                        GET_DWORD(note_table + (8 * i));

                    while ((count < length) && !done)
                    {
                        key[count] = GET_BYTE(long_index);
                        if (key[count] == '\0') done = 1;
                        ++long_index;
                        ++count;
                    }
#else
                    jbi_strncpy(key, (char *) &program[note_strings +
                        GET_DWORD(note_table + (8 * i))], length);
#endif
                }

                if (value != NULL)
                {
#if PORT==DOS
                    done = 0;
                    count = 0;
                    long_index = note_strings +
                        GET_DWORD(note_table + (8 * i) + 4);

                    while ((count < length) && !done)
                    {
                        value[count] = GET_BYTE(long_index);
                        if (value[count] == '\0') done = 1;
                        ++long_index;
                        ++count;
                    }
#else
                    jbi_strncpy(value, (char *) &program[note_strings +
                        GET_DWORD(note_table + (8 * i) + 4)], length);
#endif
                }

                *offset = i + 1;
            }
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_check_crc
(
    PROGRAM_PTR program,
    long program_size,
    unsigned short *expected_crc,
    unsigned short *actual_crc
)

/*                                                                          */
/*  Description:    This function reads the entire input file and computes  */
/*                  the CRC of everything up to the CRC field.              */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, JBIC_CRC_ERROR for failure    */
/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned short local_expected, local_actual, shift_reg = 0xffff;
    int bit, feedback;
    unsigned char databyte;
    unsigned long i;
    unsigned long crc_section = 0L;
    unsigned long first_word = 0L;
    int version = 0;
    int delta = 0;

#if PORT==DOS
    jbi_program = program;
#endif

    if (program_size > 52L)
    {
        first_word  = GET_DWORD(0);
        version = (int) (first_word & 1L);
        delta = version * 8;

        crc_section = GET_DWORD(32 + delta);
    }

    if ((first_word != 0x4A414D00L) && (first_word != 0x4A414D01L))
    {
        status = JBIC_IO_ERROR;
    }

    if (crc_section >= (unsigned long) program_size)
    {
        status = JBIC_IO_ERROR;
    }

    if (status == JBIC_SUCCESS)
    {
        local_expected = (unsigned short) GET_WORD(crc_section);
        if (expected_crc != NULL) *expected_crc = local_expected;

        for (i = 0; i < crc_section; ++i)
        {
            databyte = GET_BYTE(i);
            for (bit = 0; bit < 8; bit++)   /* compute for each bit */
            {
                feedback = (databyte ^ shift_reg) & 0x01;
                shift_reg >>= 1;    /* shift the shift register */
                if (feedback) shift_reg ^= 0x8408;  /* invert selected bits */
                databyte >>= 1;     /* get the next bit of input_byte */
            }
        }

        local_actual = (unsigned short) ~shift_reg;
        if (actual_crc != NULL) *actual_crc = local_actual;

        if (local_expected != local_actual)
        {
            status = JBIC_CRC_ERROR;
        }
    }

    return (status);
}

JBI_RETURN_TYPE jbi_get_file_info
(
    PROGRAM_PTR program,
    long program_size,
    int *format_version,
    int *action_count,
    int *procedure_count
)
{
    JBI_RETURN_TYPE status = JBIC_IO_ERROR;
    unsigned long first_word = 0;
    int version = 0;

#if PORT==DOS
    jbi_program = program;
#endif

    /*
    *   Read header information
    */
    if (program_size > 52L)
    {
        first_word = GET_DWORD(0);

        if ((first_word == 0x4A414D00L) || (first_word == 0x4A414D01L))
        {
            status = JBIC_SUCCESS;

            version = (int) (first_word & 1L);
            *format_version = version + 1;

            if (version > 0)
            {
                *action_count = (int) GET_DWORD(48);
                *procedure_count = (int) GET_DWORD(52);
            }
        }

    }

    return (status);
}

JBI_RETURN_TYPE jbi_get_action_info
(
    PROGRAM_PTR program,
    long program_size,
    int index,
    char **name,
    char **description,
    JBI_PROCINFO **procedure_list
)
{
    JBI_RETURN_TYPE status = JBIC_IO_ERROR;
    JBI_PROCINFO *procptr = NULL;
    JBI_PROCINFO *tmpptr = NULL;
    unsigned long first_word = 0L;
    unsigned long action_table = 0L;
    unsigned long proc_table = 0L;
    unsigned long string_table = 0L;
    unsigned long note_strings = 0L;
    unsigned long action_count = 0L;
    unsigned long proc_count = 0L;
    unsigned long act_name_id = 0L;
    unsigned long act_desc_id = 0L;
    unsigned long act_proc_id = 0L;
    unsigned long act_proc_name = 0L;
    unsigned char act_proc_attribute = 0;

#if PORT==DOS
    int i, length;
    jbi_program = program;
#endif

    /*
    *   Read header information
    */
    if (program_size > 52L)
    {
        first_word = GET_DWORD(0);

        if (first_word == 0x4A414D01L)
        {
            action_table = GET_DWORD(4);
            proc_table   = GET_DWORD(8);
            string_table = GET_DWORD(12);
            note_strings = GET_DWORD(16);
            action_count = GET_DWORD(48);
            proc_count   = GET_DWORD(52);

            if (index < (int) action_count)
            {
                act_name_id = GET_DWORD(action_table + (12 * index));
                act_desc_id = GET_DWORD(action_table + (12 * index) + 4);
                act_proc_id = GET_DWORD(action_table + (12 * index) + 8);

#if PORT==DOS
                length = 0;
                while (GET_BYTE(string_table + act_name_id + length) != 0) ++length;
                *name = jbi_malloc(length + 1);
                if (*name == NULL)
                {
                    status = JBIC_OUT_OF_MEMORY;
                }
                else
                {
                    for (i = 0; i < length; ++i)
                    {
                        (*name)[i] = GET_BYTE(string_table + act_name_id + i);
                    }
                    (*name)[length] = '\0';
                }
#else
                *name = (char *) &program[string_table + act_name_id];
#endif

                if (act_desc_id < (note_strings - string_table))
                {
#if PORT==DOS
                    length = 0;
                    while (GET_BYTE(string_table + act_desc_id + length) != 0) ++length;
                    *description = jbi_malloc(length + 1);
                    if (*description == NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                    }
                    else
                    {
                        for (i = 0; i < length; ++i)
                        {
                            (*description)[i] = GET_BYTE(string_table + act_desc_id + i);
                        }
                        (*description)[length] = '\0';
                    }
#else
                    *description = (char *) &program[string_table + act_desc_id];
#endif
                }

                do
                {
                    act_proc_name = GET_DWORD(proc_table + (13 * act_proc_id));
                    act_proc_attribute = (unsigned char)
                        (GET_BYTE(proc_table + (13 * act_proc_id) + 8) & 0x03);

                    procptr = (JBI_PROCINFO *) jbi_malloc(sizeof(JBI_PROCINFO));

                    if (procptr == NULL)
                    {
                        status = JBIC_OUT_OF_MEMORY;
                    }
                    else
                    {
#if PORT==DOS
                        length = 0;
                        while (GET_BYTE(string_table + act_proc_name + length) != 0) ++length;
                        procptr->name = jbi_malloc(length + 1);
                        if (procptr->name == NULL)
                        {
                            status = JBIC_OUT_OF_MEMORY;
                        }
                        else
                        {
                            for (i = 0; i < length; ++i)
                            {
                                procptr->name[i] =
                                    GET_BYTE(string_table + act_proc_name + i);
                            }
                            procptr->name[length] = '\0';
                        }
#else
                        procptr->name = (char *)
                            &program[string_table + act_proc_name];
#endif
                        procptr->attributes = act_proc_attribute;
                        procptr->next = NULL;

                        /* add record to end of linked list */
                        if (*procedure_list == NULL)
                        {
                            *procedure_list = procptr;
                        }
                        else
                        {
                            tmpptr = *procedure_list;
                            while (tmpptr->next != NULL) tmpptr = tmpptr->next;
                            tmpptr->next = procptr;
                        }
                    }

                    act_proc_id =
                        GET_DWORD(proc_table + (13 * act_proc_id) + 4);
                }
                while ((act_proc_id != 0) && (act_proc_id < proc_count));
            }
        }

    }

    return (status);
}
