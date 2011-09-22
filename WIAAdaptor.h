
#pragma once

#include <Wia.h>

typedef
	lib::wr::unique_com_interface<IWiaDevMgr2>::type
unique_com_wiadevmgr2;

typedef
	lib::wr::unique_com_interface<IEnumWIA_DEV_INFO>::type
unique_com_wiaenumdevinfo;
 
typedef
	lib::wr::unique_com_interface<IWiaPropertyStorage>::type
unique_com_wiapropertystorage;


