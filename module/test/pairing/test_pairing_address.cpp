/** @file  test_pairing_address.cpp
 *  @brief Unit tests for the Pairing Address functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_address.h"

/* CONSTANTS ******************************************************************/
#define PAN_ID_DEFAULT              0xABC
#define PAN_ID_NEW                  0x666

#define UNIQUE_ID_1                 1
#define UNIQUE_ID_2                 2
#define UNIQUE_ID_3                 3
#define UNIQUE_ID_4                 4
#define UNIQUE_ID_NEW               5

#define COORDINATOR_ADDRESS_DEFAULT 0x12
#define COORDINATOR_ADDRESS_NEW     0x21
#define NODE_ADDRESS_DEFAULT        0x23
#define NODE_ADDRESS_NEW            0x32

#define NODE_ADDRESS_1              0x11
#define NODE_ADDRESS_2              0x12
#define NODE_ADDRESS_3              0xFE
#define NODE_ADDRESS_4              0x01

#define RESERVED_ADDRESS_1          0x00
#define RESERVED_ADDRESS_2          0xFF

#define COMPLETE_ADDRESS_DEFAULT    0xABCDE

#define DISCOVERY_LIST_SIZE_DEFAULT 4

#define DEVICE_ROLE                 1
#define COORDINATOR_ROLE            0

/* PRIVATE FUNCTIONS **********************************************************/
static void pairing_address_default(pairing_assigned_address_t *pairing_assigned_address)
{
    pairing_assigned_address->coordinator_address = COORDINATOR_ADDRESS_DEFAULT;
    pairing_assigned_address->node_address        = NODE_ADDRESS_DEFAULT;
    pairing_assigned_address->pan_id              = PAN_ID_DEFAULT;
}

/* UNIT TESTS *****************************************************************/
/* Since pairing_api validate parameters, they should be valid inputs */
SCENARIO( "pairing_address_init() : validates initialisation of the structure", "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    pairing_address_default(&pairing_assigned_address);

    GIVEN( "valid input" ) {
        WHEN( "calling the function" ) {
            pairing_address_init(&pairing_assigned_address);

            THEN( "the pairing address structure is initialize properly" ) {
                REQUIRE( pairing_assigned_address.coordinator_address == 0 );
                REQUIRE( pairing_assigned_address.node_address == 0 );
                REQUIRE( pairing_assigned_address.pan_id == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_set_pan_id() : validates the configuration of the pan_id", "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);

    GIVEN( "new pan id to set" ) {
        WHEN( "calling the function" ) {
            pairing_address_set_pan_id(PAN_ID_NEW);
            THEN( "pan id is set in the pairing address and no other fields are changed" ) {
                REQUIRE( pairing_assigned_address.pan_id == PAN_ID_NEW );
                REQUIRE( pairing_assigned_address.coordinator_address == 0 );
                REQUIRE( pairing_assigned_address.node_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_get_pan_id() : validates the recuperation of the pan_id", "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    uint16_t pan_id_to_validate = 0x000;
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);
    pairing_address_set_pan_id(PAN_ID_NEW);

    GIVEN( "nothing : try to get the pan id" ) {
        WHEN( "calling the function" ) {
            pan_id_to_validate = pairing_address_get_pan_id();
            THEN( "the returned pan id is the same as the one that was set before" ) {
                REQUIRE( pan_id_to_validate == pairing_assigned_address.pan_id );
                REQUIRE( pairing_assigned_address.coordinator_address == 0 );
                REQUIRE( pairing_assigned_address.node_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_set_coordinator_address() : validates the configuration of the coordinator address",
         "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);

    GIVEN( "new coordinator address to set" ) {
        WHEN( "calling the function" ) {
            pairing_address_set_coordinator_address(COORDINATOR_ADDRESS_NEW);
            THEN( "coordinator address is set in the pairing address and no other fields are changed" ) {
                REQUIRE( pairing_assigned_address.coordinator_address == COORDINATOR_ADDRESS_NEW );
                REQUIRE( pairing_assigned_address.pan_id == 0 );
                REQUIRE( pairing_assigned_address.node_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_get_coordinator_address() : validates the recuperation of the coordinator address",
         "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    uint8_t address_to_validate = 0x55;
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);
    pairing_address_set_coordinator_address(COORDINATOR_ADDRESS_NEW);

    GIVEN( "nothing : try to get the coordinator address" ) {
        WHEN( "calling the function" ) {
            address_to_validate = pairing_address_get_coordinator_address();
            THEN( "the returned coordinator address is the same as the one that was set before" ) {
                REQUIRE( address_to_validate == pairing_assigned_address.coordinator_address );
                REQUIRE( pairing_assigned_address.pan_id == 0 );
                REQUIRE( pairing_assigned_address.node_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_set_node_address() : validates the configuration of the node address", "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);

    GIVEN( "new node address to set" ) {
        WHEN( "calling the function" ) {
            pairing_address_set_node_address(NODE_ADDRESS_NEW);
            THEN( "node address is set in the pairing address and no other fields are changed" ) {
                REQUIRE( pairing_assigned_address.node_address == NODE_ADDRESS_NEW );
                REQUIRE( pairing_assigned_address.pan_id == 0 );
                REQUIRE( pairing_assigned_address.coordinator_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_get_node_address() : validates the recuperation of the node address", "[pairing]" ) {
    pairing_assigned_address_t pairing_assigned_address = {0};
    uint8_t address_to_validate = 0x55;
    pairing_address_default(&pairing_assigned_address);
    pairing_address_init(&pairing_assigned_address);
    pairing_address_set_node_address(NODE_ADDRESS_NEW);

    GIVEN( "nothing : try to get the node address" ) {
        WHEN( "calling the function" ) {
            address_to_validate = pairing_address_get_node_address();
            THEN( "the returned node address is the same as the one that was set before" ) {
                REQUIRE( address_to_validate == pairing_assigned_address.node_address );
                REQUIRE( pairing_assigned_address.pan_id == 0 );
                REQUIRE( pairing_assigned_address.coordinator_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_discovery_list_init() : validates the initialisation of the discovery list", "[pairing]" ) {
    pairing_discovery_list_t pairing_discovery_list[DISCOVERY_LIST_SIZE_DEFAULT] = {
        {.unique_id = UNIQUE_ID_1, .node_address = NODE_ADDRESS_1},
        {.unique_id = UNIQUE_ID_2, .node_address = NODE_ADDRESS_2},
        {.unique_id = UNIQUE_ID_3, .node_address = NODE_ADDRESS_3}};
    pairing_discovery_list_t *pairing_discovery_list_to_validate;
    GIVEN( "valid input" ) {
        WHEN( "calling the function" ) {
            pairing_address_discovery_list_init(pairing_discovery_list, DISCOVERY_LIST_SIZE_DEFAULT);
            pairing_discovery_list_to_validate = pairing_address_get_discovery_list();
            THEN( "no error is returned" ) {
                REQUIRE( pairing_address_get_discovery_list_size() == DISCOVERY_LIST_SIZE_DEFAULT );

                REQUIRE( pairing_discovery_list_to_validate[0].unique_id == UNIQUE_ID_1 );
                REQUIRE( pairing_discovery_list_to_validate[0].node_address == NODE_ADDRESS_1 );

                REQUIRE( pairing_discovery_list_to_validate[1].unique_id == UNIQUE_ID_2 );
                REQUIRE( pairing_discovery_list_to_validate[1].node_address == NODE_ADDRESS_2 );

                REQUIRE( pairing_discovery_list_to_validate[2].unique_id == UNIQUE_ID_3 );
                REQUIRE( pairing_discovery_list_to_validate[2].node_address == NODE_ADDRESS_3 );

                REQUIRE( pairing_discovery_list_to_validate[3].unique_id == 0 );
                REQUIRE( pairing_discovery_list_to_validate[3].node_address == 0 );
            }
        }
    }
}

SCENARIO( "pairing_address_get_available_node_id() : validates that unavailable addresses are not returned", "[pairing]" ) {
    pairing_discovery_list_t pairing_discovery_list[DISCOVERY_LIST_SIZE_DEFAULT] = {
        {.unique_id = UNIQUE_ID_1, .node_address = NODE_ADDRESS_1},
        {.unique_id = UNIQUE_ID_2, .node_address = NODE_ADDRESS_2},
        {.unique_id = UNIQUE_ID_3, .node_address = NODE_ADDRESS_3},
        {.unique_id = UNIQUE_ID_4, .node_address = (NODE_ADDRESS_1 + 2)}};

    uint32_t address_to_validate;

    pairing_address_discovery_list_init(pairing_discovery_list, DISCOVERY_LIST_SIZE_DEFAULT);

    GIVEN( "unavailable node address" ) {
        WHEN( "calling the function" ) {
            address_to_validate = pairing_address_get_available_node_id(NODE_ADDRESS_1);
            THEN( "the returned address is not an unavailable or reserved address" ) {
                REQUIRE( address_to_validate != NODE_ADDRESS_1 );
                REQUIRE( address_to_validate != NODE_ADDRESS_2 );
                REQUIRE( address_to_validate != NODE_ADDRESS_3 );
                REQUIRE( address_to_validate != (NODE_ADDRESS_1 + 2) );
                REQUIRE( address_to_validate != RESERVED_ADDRESS_1 );
                REQUIRE( address_to_validate != (RESERVED_ADDRESS_2) );
            }
        }
    }

    GIVEN( "near a reserved address as node address" ) {
        WHEN( "calling the function" ) {
            address_to_validate = pairing_address_get_available_node_id((RESERVED_ADDRESS_2 - 1));
            THEN( "the returned address is not an unavailable or reserved address" ) {
                REQUIRE( address_to_validate != NODE_ADDRESS_1 );
                REQUIRE( address_to_validate != NODE_ADDRESS_2 );
                REQUIRE( address_to_validate != NODE_ADDRESS_3 );
                REQUIRE( address_to_validate != (NODE_ADDRESS_1 + 2) );
                REQUIRE( address_to_validate != RESERVED_ADDRESS_1 );
                REQUIRE( address_to_validate != (RESERVED_ADDRESS_2) );
            }
        }
    }
}

SCENARIO( "pairing_address_add_node_to_device_discovery_list() : validates that address have been added properly",
         "[pairing]" ) {
    pairing_discovery_list_t pairing_discovery_list[DISCOVERY_LIST_SIZE_DEFAULT] = {
        {.unique_id = UNIQUE_ID_1, .node_address = NODE_ADDRESS_1},
        {.unique_id = UNIQUE_ID_2, .node_address = NODE_ADDRESS_2},
        {.unique_id = UNIQUE_ID_3, .node_address = NODE_ADDRESS_3},
        {.unique_id = UNIQUE_ID_4, .node_address = NODE_ADDRESS_4}};

    pairing_address_discovery_list_init(pairing_discovery_list, DISCOVERY_LIST_SIZE_DEFAULT);

    GIVEN( "new address to add in the discovery list at an invalid index" ) {
        WHEN( "calling the function" ) {
            pairing_address_add_node_to_device_discovery_list(DISCOVERY_LIST_SIZE_DEFAULT,
                                                              NODE_ADDRESS_NEW,
                                                              UNIQUE_ID_NEW);
            THEN( "the list has'nt changed : this error is not handled" ) {
                REQUIRE( pairing_discovery_list[0].node_address == NODE_ADDRESS_1 );
                REQUIRE( pairing_discovery_list[0].unique_id == UNIQUE_ID_1 );

                REQUIRE( pairing_discovery_list[1].node_address == NODE_ADDRESS_2 );
                REQUIRE( pairing_discovery_list[1].unique_id == UNIQUE_ID_2 );

                REQUIRE( pairing_discovery_list[2].node_address == NODE_ADDRESS_3 );
                REQUIRE( pairing_discovery_list[2].unique_id == UNIQUE_ID_3 );

                REQUIRE( pairing_discovery_list[3].node_address == NODE_ADDRESS_4 );
                REQUIRE( pairing_discovery_list[3].unique_id == UNIQUE_ID_4) ;
            }
        }
    }

    GIVEN( "new address to add in the discovery list at valid index" ) {
        WHEN( "calling the function" ) {
            pairing_address_add_node_to_device_discovery_list(DEVICE_ROLE, NODE_ADDRESS_NEW, UNIQUE_ID_NEW);
            THEN( "The address and the unique id have been added properly" ) {
                REQUIRE( pairing_discovery_list[0].node_address == NODE_ADDRESS_1 );
                REQUIRE( pairing_discovery_list[0].unique_id == UNIQUE_ID_1 );

                REQUIRE( pairing_discovery_list[DEVICE_ROLE].node_address == NODE_ADDRESS_NEW );
                REQUIRE( pairing_discovery_list[DEVICE_ROLE].unique_id == UNIQUE_ID_NEW );

                REQUIRE( pairing_discovery_list[2].node_address == NODE_ADDRESS_3 );
                REQUIRE( pairing_discovery_list[2].unique_id == UNIQUE_ID_3 );

                REQUIRE( pairing_discovery_list[3].node_address == NODE_ADDRESS_4 );
                REQUIRE( pairing_discovery_list[3].unique_id == UNIQUE_ID_4 );
            }
        }
    }
}

SCENARIO( "pairing_address_set_device_role() : validates the configuration of the device role", "[pairing]" ) {
    uint8_t device_role_to_validate = DEVICE_ROLE + 2;

    GIVEN( "new device role to set" ) {
        WHEN( "calling the function" ) {
            pairing_address_set_device_role(DEVICE_ROLE);
            device_role_to_validate = pairing_address_get_device_role();
            THEN( "the returned device role is the same as the one that was set before" ) {
                REQUIRE( device_role_to_validate == DEVICE_ROLE );
            }
        }
    }
}

SCENARIO( "pairing_address_is_address_reserved() : validates if the returned value is correct", "[pairing]" ) {
    bool boolean_to_validate = false;

    GIVEN( "a reserved address" ) {
        WHEN( "calling the function" ) {
            boolean_to_validate = pairing_address_is_address_reserved(RESERVED_ADDRESS_1);
            THEN( "the returned value is false" ) {
                REQUIRE( boolean_to_validate == true );
            }
        }
    }

    GIVEN( "a reserved address" ) {
        WHEN( "calling the function" ) {
            boolean_to_validate = pairing_address_is_address_reserved(RESERVED_ADDRESS_2);
            THEN( "the returned value is false" ) {
                REQUIRE( boolean_to_validate == true );
            }
        }
    }

    GIVEN( "a valid address" ) {
        WHEN( "calling the function" ) {
            boolean_to_validate = pairing_address_is_address_reserved(COMPLETE_ADDRESS_DEFAULT);
            THEN( "the returned value is true" ) {
                REQUIRE( boolean_to_validate == false );
            }
        }
    }
}
