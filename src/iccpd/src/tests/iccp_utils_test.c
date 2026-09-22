#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../include/iccp_utils.h"
#include "../../include/msg_format.h"

static int child_main(int argc, char **argv)
{
    FILE *output;

    if (argc != 4)
        return 2;

    output = fopen(argv[2], "w");
    if (!output)
        return 3;

    if (fputs(argv[3], output) == EOF || fclose(output) != 0)
        return 4;

    return 7;
}

static void test_exec_command(const char *program)
{
    static const char payload[] = "PortChannel1; touch /tmp/f079-pwned";
    char output_path[] = "/tmp/iccp-utils-test-XXXXXX";
    char *command[] = {
        (char *)program,
        "--child",
        output_path,
        (char *)payload,
        NULL
    };
    char output[sizeof(payload)] = { 0 };
    FILE *file;
    int fd;

    unlink("/tmp/f079-pwned");
    fd = mkstemp(output_path);
    assert(fd >= 0);
    close(fd);

    assert(iccp_exec_command(command, true) == 7);
    file = fopen(output_path, "r");
    assert(file != NULL);
    assert(fread(output, 1, sizeof(output) - 1, file) == sizeof(payload) - 1);
    assert(fclose(file) == 0);
    assert(strcmp(output, payload) == 0);
    assert(access("/tmp/f079-pwned", F_OK) == -1);

    unlink(output_path);
}

static size_t build_agg_config_msg(
    char *buffer,
    size_t buffer_size,
    const char *name,
    size_t name_len,
    size_t declared_tlv_len,
    uint8_t flags)
{
    mLACPAggConfigTLV *portconf;
    size_t fixed_tlv_len = offsetof(mLACPAggConfigTLV, agg_name);
    size_t msg_len = sizeof(ICCHdr) + declared_tlv_len;

    assert(buffer_size >= msg_len);
    memset(buffer, 0, buffer_size);
    portconf = (mLACPAggConfigTLV *)&buffer[sizeof(ICCHdr)];
    portconf->icc_parameter.len =
        htons(declared_tlv_len - sizeof(ICCParameter));
    portconf->flags = flags;
    portconf->agg_name_len = name_len;
    if (name && name_len <= buffer_size - sizeof(ICCHdr) - fixed_tlv_len)
        memcpy(portconf->agg_name, name, name_len);

    return msg_len;
}

static void test_agg_config_parser(void)
{
    static const char valid_name[] = "PortChannel100";
    static const char unsafe_name[] = "PortChannel1;id";
    char buffer[sizeof(ICCHdr) + sizeof(mLACPAggConfigTLV)] = { 0 };
    mLACPAggConfigTLV parsed;
    size_t fixed_tlv_len = offsetof(mLACPAggConfigTLV, agg_name);
    size_t msg_len;

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name), 0x01);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == 0);
    assert(strcmp(parsed.agg_name, valid_name) == 0);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        sizeof(mLACPAggConfigTLV), 0x02);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == 0);

    assert(iccp_parse_agg_config_tlv(
        buffer, sizeof(ICCHdr) + fixed_tlv_len - 1, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name), 0x01);
    ((mLACPAggConfigTLV *)&buffer[sizeof(ICCHdr)])->icc_parameter.len =
        htons(sizeof(mLACPAggConfigTLV));
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name) - 1, 0x01);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), NULL, MAX_L_PORT_NAME,
        sizeof(mLACPAggConfigTLV), 0x01);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), unsafe_name, strlen(unsafe_name),
        fixed_tlv_len + strlen(unsafe_name), 0x01);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), "Port\0Channel", 12,
        fixed_tlv_len + 12, 0x01);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name), 0x00);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name), 0x03);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);

    msg_len = build_agg_config_msg(
        buffer, sizeof(buffer), valid_name, strlen(valid_name),
        fixed_tlv_len + strlen(valid_name), 0x05);
    assert(iccp_parse_agg_config_tlv(buffer, msg_len, &parsed) == -1);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--child") == 0)
        return child_main(argc, argv);

    test_exec_command(argv[0]);
    test_agg_config_parser();
    return 0;
}
