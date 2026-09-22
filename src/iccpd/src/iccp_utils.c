#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stddef.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/iccp_utils.h"
#include "../include/msg_format.h"

extern char **environ;

int iccp_exec_command(char *const argv[], bool suppress_output)
{
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t *actions_ptr = NULL;
    pid_t pid;
    int status;
    int rc;

    if (!argv || !argv[0])
        return -1;

    if (suppress_output)
    {
        rc = posix_spawn_file_actions_init(&actions);
        if (rc != 0)
            return -1;

        actions_ptr = &actions;
        rc = posix_spawn_file_actions_addopen(
            &actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
        if (rc == 0)
        {
            rc = posix_spawn_file_actions_addopen(
                &actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
        }
        if (rc != 0)
        {
            posix_spawn_file_actions_destroy(&actions);
            return -1;
        }
    }

    rc = posix_spawnp(&pid, argv[0], actions_ptr, NULL, argv, environ);
    if (actions_ptr)
        posix_spawn_file_actions_destroy(&actions);
    if (rc != 0)
        return -1;

    do
    {
        rc = waitpid(pid, &status, 0);
    } while (rc == -1 && errno == EINTR);

    if (rc == -1 || !WIFEXITED(status))
        return -1;

    return WEXITSTATUS(status);
}

bool iccp_is_interface_name_valid(const char *name, size_t name_len)
{
    size_t index;

    if (!name || name_len == 0 || name_len >= MAX_L_PORT_NAME)
        return false;

    if ((name_len == 1 && name[0] == '.') ||
        (name_len == 2 && name[0] == '.' && name[1] == '.'))
    {
        return false;
    }

    for (index = 0; index < name_len; ++index)
    {
        unsigned char character = (unsigned char)name[index];
        bool is_alphanumeric =
            (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9');

        if (is_alphanumeric || character == '_' || character == '.' ||
            (index > 0 && character == '-'))
        {
            continue;
        }

        return false;
    }

    return true;
}

int iccp_get_tlv_type(
    const char *msg_buf,
    size_t msg_len,
    uint16_t *tlv_type)
{
    ICCParameter icc_param;

    if (!msg_buf || !tlv_type ||
        msg_len < sizeof(ICCHdr) + sizeof(icc_param))
    {
        return -1;
    }

    memcpy(&icc_param, &msg_buf[sizeof(ICCHdr)], sizeof(icc_param));
    *tlv_type = icc_param.type;
    return 0;
}

int iccp_validate_message(
    const char *msg_buf,
    size_t msg_len,
    uint16_t msg_type)
{
    size_t minimum_len = sizeof(ICCHdr);
    size_t available_tlv_len;
    size_t declared_tlv_len;
    uint16_t parameter_len;

    if (!msg_buf || msg_len < minimum_len)
        return -1;

    switch (msg_type)
    {
        case MSG_T_CAPABILITY:
            minimum_len =
                sizeof(LDPHdr) + sizeof(LDPICCPCapabilityTLV);
            break;
        case MSG_T_RG_APP_DATA:
            minimum_len += sizeof(ICCParameter);
            break;
        case MSG_T_NOTIFICATION:
            minimum_len += sizeof(NAKTLV);
            break;
        case MSG_T_RG_CONNECT:
            minimum_len += sizeof(ICCParameter);
            if (msg_len < minimum_len)
                return -1;

            memcpy(&parameter_len,
                &msg_buf[sizeof(ICCHdr) + offsetof(ICCParameter, len)],
                sizeof(parameter_len));
            available_tlv_len = msg_len - sizeof(ICCHdr);
            declared_tlv_len =
                sizeof(ICCParameter) + ntohs(parameter_len);
            return declared_tlv_len <= available_tlv_len &&
                   declared_tlv_len <= sizeof(ICCSenderNameTLV)
                       ? 0 : -1;
        case MSG_T_RG_DISCONNECT:
            minimum_len += sizeof(DisconnectCodeTLV);
            break;
        default:
            break;
    }

    return msg_len >= minimum_len ? 0 : -1;
}

int iccp_validate_tlv(
    const char *msg_buf,
    size_t msg_len,
    size_t minimum_tlv_len,
    uint16_t *tlv_type,
    size_t *tlv_len)
{
    ICCParameter icc_param;
    size_t available_tlv_len;
    size_t declared_tlv_len;
    uint16_t parameter_len;

    if (!msg_buf || minimum_tlv_len < sizeof(ICCParameter) ||
        msg_len < sizeof(ICCHdr) + sizeof(icc_param))
    {
        return -1;
    }

    memcpy(&icc_param, &msg_buf[sizeof(ICCHdr)], sizeof(icc_param));
    memcpy(&parameter_len, &icc_param.len, sizeof(parameter_len));
    available_tlv_len = msg_len - sizeof(ICCHdr);
    declared_tlv_len = sizeof(ICCParameter) + ntohs(parameter_len);

    if (declared_tlv_len < minimum_tlv_len ||
        declared_tlv_len > available_tlv_len)
    {
        return -1;
    }

    if (tlv_type)
        *tlv_type = icc_param.type;
    if (tlv_len)
        *tlv_len = declared_tlv_len;
    return 0;
}

int iccp_parse_agg_config_tlv(
    const char *msg_buf,
    size_t msg_len,
    mLACPAggConfigTLV *portconf)
{
    const mLACPAggConfigTLV *wire_portconf;
    size_t fixed_tlv_len = offsetof(mLACPAggConfigTLV, agg_name);
    size_t available_tlv_len;
    size_t declared_tlv_len;
    size_t name_len;
    uint16_t parameter_len;

    if (!msg_buf || !portconf ||
        msg_len < sizeof(ICCHdr) + fixed_tlv_len)
    {
        return -1;
    }

    wire_portconf =
        (const mLACPAggConfigTLV *)&msg_buf[sizeof(ICCHdr)];
    available_tlv_len = msg_len - sizeof(ICCHdr);
    memcpy(&parameter_len, &wire_portconf->icc_parameter.len,
        sizeof(parameter_len));
    declared_tlv_len = sizeof(ICCParameter) + ntohs(parameter_len);
    name_len = wire_portconf->agg_name_len;

    if (declared_tlv_len < fixed_tlv_len ||
        declared_tlv_len > available_tlv_len ||
        name_len > declared_tlv_len - fixed_tlv_len ||
        !iccp_is_interface_name_valid(wire_portconf->agg_name, name_len) ||
        memchr(wire_portconf->agg_name, '\0', name_len) != NULL)
    {
        return -1;
    }

    if (wire_portconf->flags != 0x01 && wire_portconf->flags != 0x02)
        return -1;

    memset(portconf, 0, sizeof(*portconf));
    memcpy(portconf, wire_portconf, fixed_tlv_len);
    memcpy(portconf->agg_name, wire_portconf->agg_name, name_len);
    portconf->agg_name_len = name_len;
    return 0;
}
