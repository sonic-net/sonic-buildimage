#include <string>               // std::string
#include <vector>               // std::vector

#include "hamd_gecos.h"         // gecos_c
#include "../shared/utils.h"    // join()

gecos_c::gecos_c()
{
    // Allocate a minimum amount of members to account for
    // Full Name, Room Address, Work Phone, Home Phone,
    // and roles (i.e 5 elements).
    members_m.resize(5, "");
    roles_index_m = 4;
    roles_m.init(members_m[roles_index_m]);
}

gecos_c::gecos_c(const char * pw_gecos)
{
    init(pw_gecos);
}

void gecos_c::init(const char * pw_gecos)
{
    text_m        = pw_gecos;
    members_m     = split_exact(pw_gecos, ",", 5/*min_vector_size*/);
    roles_index_m = 4;

    bool roles_found = false;
    for (size_t i = roles_index_m; i < members_m.size(); i++)
    {
        if (startswith(members_m[i].c_str(), "roles="))
        {
            roles_found = true;
            roles_index_m = i;
            break;
        }
    }

    if (!roles_found)
    {
        if (*members_m.end() != "")
        {
            // If last element is not an empty string and
            // we haven't found the "roles=" keyword, then
            // we have other fields in the GECOS that are
            // not the roles attribute. So we need to reserve
            // an extra element at the end that we hold the roles.
            members_m.push_back("");
            roles_index_m = members_m.size() - 1;
        }
    }

    roles_m.init(members_m[roles_index_m]);
}

void gecos_c::set(const char * roles_p,
                  const char * full_name_p,
                  const char * room_addr_p,
                  const char * home_phone_p,
                  const char * work_phone_p)
{
    if (roles_p      != nullptr) members_m[roles_index_m] = roles_p;
    if (full_name_p  != nullptr) members_m[0] = full_name_p;
    if (room_addr_p  != nullptr) members_m[1] = room_addr_p;
    if (home_phone_p != nullptr) members_m[2] = home_phone_p;
    if (work_phone_p != nullptr) members_m[3] = work_phone_p;

    // Find the last non-empty member of the "other" fields.
    size_t  work_phone_index = 3;
    size_t  index       = members_m.size() - 1;
    for (; index > work_phone_index; index--)
    {
        if (!members_m[index].empty()) break;
    }

    auto end = members_m.cbegin() + index + 1;
    text_m = join(members_m.cbegin(), end, ",");
}

std::vector< std::string > roles_c::members() const
{
    char * p = startswith(roles_pm, "roles=");
    if (p != nullptr)
    {
        return split_exact(p, ";");
    }

    std::vector< std::string > members;
    return members;
}

std::string roles_as_string(const std::vector< std::string > & roles)
{
    return "roles=" + join(roles.cbegin(), roles.cend(), ";");
}
