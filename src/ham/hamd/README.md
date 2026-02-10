# Host Account Management Daemon (hamd)

Hamd is the "single source of truth" for SONiC user accounts. Hamd does only one thing – manage user accounts. As such, hamd follows the UNIX philosophy, which stipulates "programs should do one thing and do it well." The UNIX philosophy provides several benefits, such as modularity and reusability, to name a few.  

Hamd provides a D-Bus interface used by programs (NSS, PAM, etc.) to create, modify, delete user accounts on the Host. The D-Bus interface is required to ensure synchronous operations, which is to say that hamd replies immediately to requests.  

Having a central process to handle user accounts allow consistent user account operations for all applications. Leaving it to applications to call `useradd` directly is not the right approach because various developers working on different applications may invoke `useradd` with inconsistent options and may not be aware of additional user account requirements. For example, users may need additional credentials such as certificates (TLS, REST, gRPC) or keys (SSH). Trusting that every developer will always create user accounts in a consistent way is simply not realistic.

