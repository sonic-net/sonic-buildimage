#ifndef CREDO_RENAMED_H
#define CREDO_RENAMED_H

// config option to let users pick if only using latest API names
#ifndef CREDO_NO_RENAMED

// compatibility to shorten error type name
#define CredoErrorCodes_t CredoError_t

// add DEV prefix to all device types
#define CREDO_BALDEAGLE_400       CR_DEV_BALDEAGLE_400
#define CREDO_BALDEAGLE_800       CR_DEV_BALDEAGLE_800
#define CREDO_OWL_400             CR_DEV_OWL_400
#define CREDO_OWL_800             CR_DEV_OWL_800
#define CREDO_BLACKHAWK_400       CR_DEV_BLACKHAWK_400
#define CREDO_BLACKHAWK_800       CR_DEV_BLACKHAWK_800
#define CREDO_HERON_P1            CR_DEV_HERON_P1
#define CREDO_HERON_P3            CR_DEV_HERON_P3
#define CREDO_HERON_1P0           CR_DEV_HERON_1P0
#define CREDO_HERON_MR            CR_DEV_HERON_MR
#define CREDO_OSPREY_400          CR_DEV_OSPREY_400
#define CREDO_OSPREY_800          CR_DEV_OSPREY_800
#define CREDO_OSPREY_AEC          CR_DEV_OSPREY_AEC
#define CREDO_NUTCRACKER_32       CR_DEV_NUTCRACKER_32
#define CREDO_KITE_TEST           CR_DEV_KITE_TEST
#define CREDO_RAVEN_TEST          CR_DEV_RAVEN_TEST
#define CREDO_ADMIRAL_TEST        CR_DEV_ADMIRAL_TEST
#define CREDO_SEAHAWK             CR_DEV_SEAHAWK
#define CREDO_OSTRICH_1P1         CR_DEV_OSTRICH_1P1
#define CREDO_SCREAMING_EAGLE_AEC CR_DEV_SCREAMING_EAGLE_AEC
#define CREDO_SCREAMING_EAGLE     CR_DEV_SCREAMING_EAGLE
#define CREDO_NIGHTHAWK_1P0       CR_DEV_NIGHTHAWK_1P0
#define CREDO_NIGHTHAWK           CR_DEV_NIGHTHAWK
#define CREDO_SPARROW_800         CR_DEV_SPARROW_800
#define CREDO_BLUEJAY             CR_DEV_BLUEJAY
#define CREDO_CROW                CR_DEV_CROW
#define CREDO_FAKE_400            CR_DEV_FAKE_400
#define CREDO_FAKE_800            CR_DEV_FAKE_800

#define CREDO_BALDEAGLE           CR_SLC_BALDEAGLE
#define CREDO_OWL                 CR_SLC_OWL
#define CREDO_OWL_A0              CR_SLC_OWL_A0
#define CREDO_OWL_B0              CR_SLC_OWL_B0
#define CREDO_OWL_B0B             CR_SLC_OWL_B0B
#define CREDO_OWL_B1              CR_SLC_OWL_B1
#define CREDO_BLACKHAWK           CR_SLC_BLACKHAWK
#define CREDO_BLACKHAWK_DC        CR_SLC_BLACKHAWK_DC
#define CREDO_BLACKHAWK_AC        CR_SLC_BLACKHAWK_AC
#define CREDO_HERON               CR_SLC_HERON
#define CREDO_OSPREY              CR_SLC_OSPREY
#define CREDO_NUTCRACKER          CR_SLC_NUTCRACKER
#define CREDO_KITE                CR_SLC_KITE
#define CREDO_RAVEN               CR_SLC_RAVEN
#define CREDO_ADMIRAL             CR_SLC_ADMIRAL
#define CREDO_SLC_SEAHAWK         CR_SLC_SEAHAWK
#define CREDO_OSTRICH             CR_SLC_OSTRICH
#define CREDO_SLC_SCREAMING_EAGLE CR_SLC_SCREAMING_EAGLE
#define CREDO_SLC_NIGHTHAWK       CR_SLC_NIGHTHAWK
#define CREDO_SPARROW             CR_SLC_SPARROW
#define CREDO_SLC_BLUEJAY         CR_SLC_BLUEJAY
#define CREDO_SLC_CROW            CR_SLC_CROW
#define CREDO_FAKE                CR_SLC_FAKE

#endif

#endif
