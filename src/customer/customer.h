#ifndef _CUSTOMER_H__
#define _CUSTOMER_H__


//=============================================================================
//  customer switch
//=============================================================================
//#define CUSTOMER	// customer version
#undef CUSTOMER		// standard version


#ifdef CUSTOMER
//=============================================================================
//  customer version : one of the following should be selected (#define)
//=============================================================================
//#define SOM6867_EXFO_VERSION
//#define MIOE260_EC_VERSION
//#define EBCKF08_EC_VERSION
//#define EPCC301_EC_VERSION

//=============================================================================
//  SOM6867_EXFO_VERSION
//=============================================================================
#ifdef SOM6867_EXFO_VERSION
#include "../customer/exfo_ec_update/som6867_exfo.h"
#endif

//=============================================================================
//  MIOE260_EC_VERSION
//=============================================================================
#ifdef MIOE260_EC_VERSION
#include "../customer/mioe_260/mioe_260.h"
#endif

//=============================================================================
//  EBCKF08_EC_VERSION
//=============================================================================
#ifdef EBCKF08_EC_VERSION
#include "../customer/ebc_kf08/ebc_kf08.h"
#endif

//=============================================================================
//  EPCC301_EC_VERSION
//=============================================================================
#ifdef EPCC301_EC_VERSION
#include "../customer/epc_c301/epc_c301.h"
#endif

#else
//=============================================================================
//  standard version
//=============================================================================
#undef SOM6867_EXFO_VERSION
#undef MIOE260_EC_VERSION
#undef EBCKF08_EC_VERSION
#undef EPCC301_EC_VERSION

#endif	// CUSTOMER

#endif	// _CUSTIMER_H__
